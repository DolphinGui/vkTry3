#include <array>
#include <bits/stdint-uintn.h>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Setup.hpp"
#include "VCEngine.hpp"
#include "data/Vertex.hpp"
#include "vkobjects/ImageBundle.hpp"
#include "workers/Renderer.hpp"

namespace {
// TODO: change this to std::byte at some point
std::vector<char>
readFile(const std::string_view& filename)
{
  std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}
vk::DescriptorSetLayout
createDescriptorSetLayout(const vk::Device device)
{
  vk::DescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
  uboLayoutBinding.pImmutableSamplers = nullptr;
  uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

  vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType =
    vk::DescriptorType::eCombinedImageSampler;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

  std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
    uboLayoutBinding, samplerLayoutBinding
  };
  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();
  vk::DescriptorSetLayout set;
  if (device.createDescriptorSetLayout(&layoutInfo, nullptr, &set) !=
      vk::Result::eSuccess)
    throw std::runtime_error("failed to create render pass");
  return set;
}

vk::ShaderModule
createShaderModule(const vk::Device device, const std::vector<char>& code)
{
  vk::ShaderModuleCreateInfo createInfo{};
  createInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  vk::ShaderModule shaderModule;
  if (device.createShaderModule(&createInfo, nullptr, &shaderModule) !=
      vk::Result::eSuccess)
    throw std::runtime_error("failed to create render pass");
  ;

  return shaderModule;
}

std::vector<vk::ImageView>
createImageView(const vk::Device device,
                const std::vector<vk::Image>& images,
                const vk::Format& format,
                vk::ImageAspectFlags aspectFlags,
                uint32_t mipLevels)
{
  std::vector<vk::ImageView> results(images.size());
  for (int i = 0; i < images.size(); i++) {
    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
    viewInfo.image = images[i];
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vk::ImageView imageView;
    if (device.createImageView(&viewInfo, nullptr, &imageView) !=
        vk::Result::eSuccess)
      throw std::runtime_error("failed to create render pass");
    results[i] = imageView;
  }

  return results;
}

std::pair<vk::PipelineLayout, vk::Pipeline>
createGraphicsPipeline(vk::Device device,
                       vk::SampleCountFlagBits msaaSamples,
                       vk::Extent2D swapChainExtent,
                       const vk::DescriptorSetLayout* descriptorSetLayout,
                       const vk::RenderPass& renderpass)
{
  auto vertShaderCode = readFile("../shaders/vert.spv");
  auto fragShaderCode = readFile("../shaders/frag.spv");

  vk::ShaderModule vertShaderModule =
    createShaderModule(device, vertShaderCode);
  vk::ShaderModule fragShaderModule =
    createShaderModule(device, fragShaderCode);

  vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo,
                                                       fragShaderStageInfo };

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();
  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount =
    static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  vk::Viewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swapChainExtent.width);
  viewport.height = static_cast<float>(swapChainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vk::Rect2D scissor{};
  vk::Offset2D offset = { 0, 0 };
  scissor.offset = offset;
  scissor.extent = swapChainExtent;

  vk::PipelineViewportStateCreateInfo viewportState{};
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  vk::PipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = vk::PolygonMode::eFill;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = vk::CullModeFlagBits::eBack;
  rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
  rasterizer.depthBiasEnable = VK_FALSE;

  vk::PipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = msaaSamples;

  vk::PipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = vk::CompareOp::eLess;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
  colorBlendAttachment.blendEnable = VK_FALSE;

  vk::PipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = vk::LogicOp::eCopy;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayout;
  vk::PipelineLayout layout;
  if (device.createPipelineLayout(&pipelineLayoutInfo, nullptr, &layout) !=
      vk::Result::eSuccess)
    throw std::runtime_error("failed to create render pass");

  vk::GraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = layout;
  pipelineInfo.renderPass = renderpass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = vk::Pipeline(nullptr);
  vk::Pipeline pipes;
  if (device.createGraphicsPipelines(
        vk::PipelineCache(nullptr), 1, &pipelineInfo, nullptr, &pipes) !=
      vk::Result::eSuccess)
    throw std::runtime_error("failed to create render pass");
  device.destroyShaderModule(fragShaderModule);
  device.destroyShaderModule(vertShaderModule);
  return { layout, pipes };
}

std::vector<vk::Framebuffer>
createFramebuffers(vk::Device device,
                   vk::Extent2D swapChainExtent,
                   const vcc::ImageBundle& color,
                   const vcc::ImageBundle& depth,
                   const vk::RenderPass& renderpass,
                   const std::vector<vk::ImageView>& swapChainImageViews)
{
  std::vector<vk::Framebuffer> frames(swapChainImageViews.size());
  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    std::array<vk::ImageView, 3> attachments = { color.view,
                                                 depth.view,
                                                 swapChainImageViews[i] };
    const vk::FramebufferCreateInfo framebufferInfo({},
                                                    renderpass,
                                                    static_cast<uint32_t>(3),
                                                    attachments.data(),
                                                    swapChainExtent.width,
                                                    swapChainExtent.height,
                                                    1);
    if (device.createFramebuffer(&framebufferInfo, nullptr, &frames[i]) !=
        vk::Result::eSuccess) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
  return frames;
}

vk::SwapchainKHR
createSwap(const vcc::VCEngine& env,
           const vk::SurfaceCapabilitiesKHR& capabilities,
           const vk::SurfaceFormatKHR& swapChainImageFormat,
           vk::Extent2D swapChainExtent)
{
  std::vector<vk::PresentModeKHR> presentModes =
    env.physicalDevice.getSurfacePresentModesKHR(env.surface, env.dload);
  vk::PresentModeKHR presentMode;
  for (const auto& availablePresentMode : presentModes) {
    if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
      presentMode = availablePresentMode;
    }
  }
  presentMode = vk::PresentModeKHR::eFifo;

  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 &&
      imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }
  uint32_t queueFamilyIndices[] = { env.queueIndices.graphicsFamily.value(),
                                    env.queueIndices.presentFamily.value() };
  vk::SharingMode share;
  uint32_t queueFamilyIndexCount;
  uint32_t* queueFamilyIndixes;
  if (env.queueIndices.graphicsFamily != env.queueIndices.presentFamily) {
    share = vk::SharingMode::eConcurrent;
    queueFamilyIndexCount = 2;
    queueFamilyIndixes = queueFamilyIndices;
  } else {
    share = vk::SharingMode::eExclusive;
  }
  vk::SwapchainCreateInfoKHR createInfo(
    {},
    env.surface,
    imageCount,
    swapChainImageFormat.format,
    swapChainImageFormat.colorSpace,
    swapChainExtent,
    1,
    vk::ImageUsageFlagBits::eColorAttachment,
    share,
    queueFamilyIndexCount,
    queueFamilyIndixes,
    capabilities.currentTransform,
    vk::CompositeAlphaFlagBitsKHR::eOpaque,
    presentMode,
    VK_TRUE);
  vk::SwapchainKHR swap;
  if (env.device.createSwapchainKHR(&createInfo, nullptr, &swap) !=
      vk::Result::eSuccess) {
    throw std::runtime_error("failed to create swap chain!");
  }
  return swap;
}
vk::SurfaceFormatKHR
getSwapFormat(const vcc::VCEngine& env)
{
  std::vector<vk::SurfaceFormatKHR> formats =
    env.physicalDevice.getSurfaceFormatsKHR(env.surface, env.dload);
  vk::SurfaceFormatKHR surfaceFormat;
  for (const auto& availableFormat : formats) {
    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
        availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      surfaceFormat = availableFormat;
    }
  }
  return formats[0];
}
vk::Extent2D
getSurfaceExtent(const vcc::VCEngine& env,
                 const vk::SurfaceCapabilitiesKHR& capabilities)
{
  vk::Extent2D extent;
  if (capabilities.currentExtent.width != UINT32_MAX) {
    extent = capabilities.currentExtent;
  } else {
    vk::Extent2D actualExtent(env.framebufferSize());

    actualExtent.width =
      std::max(capabilities.minImageExtent.width,
               std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(
      capabilities.minImageExtent.height,
      std::min(capabilities.maxImageExtent.height, actualExtent.height));

    extent = actualExtent;
  }
  return extent;
};
vk::Format
findSupportedFormat(vk::PhysicalDevice physicalDevice,
                    const std::vector<vk::Format>& candidates,
                    vk::ImageTiling tiling,
                    vk::FormatFeatureFlags features)
{
  for (vk::Format format : candidates) {
    vk::FormatProperties props;
    physicalDevice.getFormatProperties(format, &props);

    if (tiling == vk::ImageTiling::eLinear &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == vk::ImageTiling::eOptimal &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  throw std::runtime_error("failed to find supported format!");
}
vk::RenderPass
createRenderPass(vk::Device device,
                 vk::PhysicalDevice physicalDevice,
                 vk::Format format,
                 vk::SampleCountFlagBits msaaSamples)
{
  vk::AttachmentDescription colorAttachment(
    {},
    format,
    msaaSamples,
    vk::AttachmentLoadOp::eClear,
    vk::AttachmentStoreOp::eStore,
    vk::AttachmentLoadOp::eDontCare,
    vk::AttachmentStoreOp::eDontCare,
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eColorAttachmentOptimal);

  vk::AttachmentDescription depthAttachment(
    {},
    findSupportedFormat(physicalDevice,
                        { vk::Format::eD32Sfloat,
                          vk::Format::eD32SfloatS8Uint,
                          vk::Format::eD24UnormS8Uint },
                        vk::ImageTiling::eOptimal,
                        vk::FormatFeatureFlagBits::eDepthStencilAttachment),
    msaaSamples,
    vk::AttachmentLoadOp::eClear,
    vk::AttachmentStoreOp::eDontCare,
    vk::AttachmentLoadOp::eDontCare,
    vk::AttachmentStoreOp::eDontCare,
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eDepthStencilAttachmentOptimal);

  vk::AttachmentDescription colorAttachmentResolve(
    {},
    format,
    vk::SampleCountFlagBits::e1,
    vk::AttachmentLoadOp::eDontCare,
    vk::AttachmentStoreOp::eStore,
    vk::AttachmentLoadOp::eDontCare,
    vk::AttachmentStoreOp::eDontCare,
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::ePresentSrcKHR);

  vk::AttachmentReference colorAttachmentRef(
    0, vk::ImageLayout::eColorAttachmentOptimal);

  vk::AttachmentReference depthAttachmentRef(
    1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

  vk::AttachmentReference colorAttachmentResolveRef(
    2, vk::ImageLayout::eColorAttachmentOptimal);

  vk::SubpassDescription subpass({},
                                 vk::PipelineBindPoint::eGraphics,
                                 {},
                                 {},
                                 1,
                                 &colorAttachmentRef,
                                 &colorAttachmentResolveRef,
                                 &depthAttachmentRef);

  vk::SubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput |
                            vk::PipelineStageFlagBits::eEarlyFragmentTests;
  dependency.srcAccessMask = {};
  dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput |
                            vk::PipelineStageFlagBits::eEarlyFragmentTests;
  dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite |
                             vk::AccessFlagBits::eDepthStencilAttachmentWrite;

  std::array<vk::AttachmentDescription, 3> attachments = {
    colorAttachment, depthAttachment, colorAttachmentResolve
  };
  vk::RenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;
  vk::RenderPass pass;
  if (device.createRenderPass(&renderPassInfo, nullptr, &pass) !=
      vk::Result::eSuccess)
    throw std::runtime_error("failed to create render pass");
  return pass;
}
}

namespace vcc {
Setup::Setup(VCEngine* engine)
  : env(engine)
  , capabilities(
      env->physicalDevice.getSurfaceCapabilitiesKHR(env->surface, env->dload))
  , swapChainExtent(getSurfaceExtent(*env, capabilities))
  , swapChainImageFormat(getSwapFormat(*env))
  , swapChain( // Test if this actually is unititialized
      createSwap(*env, capabilities, swapChainImageFormat, swapChainExtent))
  , swapChainImages(env->device.getSwapchainImagesKHR(swapChain, env->dload))
  , swapChainImageViews(createImageView(env->device,
                                        swapChainImages,
                                        swapChainImageFormat.format,
                                        vk::ImageAspectFlagBits::eColor,
                                        1))
  , renderPass(createRenderPass(env->device,
                                env->physicalDevice,
                                swapChainImageFormat.format,
                                env->msaaSamples))
  , descriptorSetLayout(createDescriptorSetLayout(env->device))
  , pipeline(createGraphicsPipeline(env->device,
                                    env->msaaSamples,
                                    swapChainExtent,
                                    &descriptorSetLayout,
                                    renderPass))
  , allocationInfo([]{
    VmaAllocationCreateInfo create{};
    create.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    return create;
  }())
  , graphicsQueue(
      engine->device.getQueue(0, env->queueIndices.graphicsFamily.value()))
  , presentQueue(
      engine->device.getQueue(0, env->queueIndices.presentFamily.value()))
  , transferQueue(
      engine->device.getQueue(0, env->queueIndices.transferFamily.value()))
  , color(ImageBundle::create(
      vk::ImageCreateInfo({},
                          vk::ImageType::e2D,
                          swapChainImageFormat.format,
                          { swapChainExtent.width, swapChainExtent.height, 1 },
                          1,
                          {},
                          env->msaaSamples,
                          vk::ImageTiling::eOptimal,
                          vk::ImageUsageFlagBits::eTransientAttachment |
                            vk::ImageUsageFlagBits::eColorAttachment,
                          vk::SharingMode::eExclusive),
      allocationInfo,
      env,
      vk::ImageAspectFlagBits::eColor))
  , depth(ImageBundle::create(
      vk::ImageCreateInfo(
        {},
        vk::ImageType::e2D,
        findSupportedFormat(env->physicalDevice,
                            { vk::Format::eD32Sfloat,
                              vk::Format::eD32SfloatS8Uint,
                              vk::Format::eD24UnormS8Uint },
                            vk::ImageTiling::eOptimal,
                            vk::FormatFeatureFlagBits::eDepthStencilAttachment),
        { swapChainExtent.width, swapChainExtent.height, 1 },
        1,
        {},
        env->msaaSamples,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::SharingMode::eExclusive),
      allocationInfo,
      env,
      vk::ImageAspectFlagBits::eDepth))
  , swapChainFramebuffers(createFramebuffers(env->device,
                                             swapChainExtent,
                                             color,
                                             depth,
                                             renderPass,
                                             swapChainImageViews))
{}
Setup::~Setup()
{
  for (auto fbuffer : swapChainFramebuffers) {
    env->device.destroyFramebuffer(fbuffer);
  }
  env->device.destroyPipeline(pipeline.second);
  env->device.destroyPipelineLayout(pipeline.first);
  env->device.destroyRenderPass(renderPass);
  for (auto view : swapChainImageViews) {
    env->device.destroyImageView(view);
  }
  env->device.destroySwapchainKHR(swapChain);
  env->device.destroyDescriptorSetLayout(descriptorSetLayout);
}

}
