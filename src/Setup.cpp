#include <bits/stdint-uintn.h>
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <array>
#include <cstdint>
#include <iostream>
#include <ostream>

#include "Buffer.hpp"
#include "Setup.hpp"
#include "data/Vertex.hpp"
#include "ImageBundle.hpp"
#include "VCEngine.hpp"
#include "SingleTimeCmdBuffer.hpp"

using namespace vcc;
Setup::Setup(VCEngine* engine):
 env(engine)
 {
  createSwapChain();
  swapChainImageViews.resize(swapChainImages.size());

  for (uint32_t i = 0; i < swapChainImages.size(); i++) {
      swapChainImageViews[i] = createImageView(
        swapChainImages[i],
        swapChainImageFormat,
        vk::ImageAspectFlagBits::eColor,
        1);
  }
  createRenderPass();
  createDescriptorSetLayout();
  createGraphicsPipeline();
  QueueFamilyIndices queueFamilyIndices = env->findQueueFamilies(env->physicalDevice);

  vk::CommandPoolCreateInfo poolInfo{};
  poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
  if(env->device.createCommandPool(&poolInfo, nullptr, &commandPool)!=vk::Result::eSuccess)
    throw std::runtime_error("failed to create command pool");

  vk::Format colorFormat = swapChainImageFormat;

  color = ImageBundle(
      swapChainExtent.width,
      swapChainExtent.height,
      1,
      env->msaaSamples,
      colorFormat,
      vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      env,
      vk::ImageAspectFlagBits::eColor
  );

  depth = ImageBundle(
      swapChainExtent.width,
      swapChainExtent.height,
      1,
      env->msaaSamples,
      findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment),
      vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eDepthStencilAttachment,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      env,
      vk::ImageAspectFlagBits::eDepth
  );
  createFramebuffers();
}

Setup::~Setup(){
    for(auto fbuffer : swapChainFramebuffers){
        env->device.destroyFramebuffer(fbuffer);
    }
    env->device.destroyPipeline(graphicsPipeline);
    env->device.destroyPipelineLayout(pipelineLayout);
    env->device.destroyRenderPass(renderPass);
    for(auto view : swapChainImageViews){
        env->device.destroyImageView(view);
    }
    env->device.destroySwapchainKHR(swapChain);
    env->device.destroyDescriptorSetLayout(descriptorSetLayout);
    env->device.destroyCommandPool(commandPool);
}

void Setup::createRenderPass() {
    vk::AttachmentDescription colorAttachment(
        {},
        swapChainImageFormat,
        env->msaaSamples,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal
    );

    vk::AttachmentDescription depthAttachment(
        {},
        findSupportedFormat(
            {
                vk::Format::eD32Sfloat,
                vk::Format::eD32SfloatS8Uint,
                vk::Format::eD24UnormS8Uint
            },
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment
        ),
        env->msaaSamples,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );

    vk::AttachmentDescription colorAttachmentResolve(
        {},
        swapChainImageFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR
    );

    vk::AttachmentReference colorAttachmentRef(
        0,
        vk::ImageLayout::eColorAttachmentOptimal
    );

    vk::AttachmentReference depthAttachmentRef(
        1,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );

    vk::AttachmentReference colorAttachmentResolveRef(
        2,
        vk::ImageLayout::eColorAttachmentOptimal
    );

    vk::SubpassDescription subpass(
        {},
        vk::PipelineBindPoint::eGraphics,
        {},{},
        1,
        &colorAttachmentRef,
        &colorAttachmentResolveRef,
        &depthAttachmentRef
    );

    vk::SubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
    vk::PipelineStageFlagBits::eColorAttachmentOutput |
    vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcAccessMask = {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput |
    vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask =
    vk::AccessFlagBits::eColorAttachmentWrite |
    vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    std::array<vk::AttachmentDescription, 3> attachments = {
        colorAttachment, depthAttachment, colorAttachmentResolve };
    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    if(env->device.createRenderPass(&renderPassInfo, nullptr, &renderPass)!=vk::Result::eSuccess)
        throw std::runtime_error("failed to create render pass");
}

vk::Format Setup::findSupportedFormat(
    const std::vector<vk::Format>& candidates,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features) {
    for (vk::Format format : candidates) {
        vk::FormatProperties props;
        env->physicalDevice.getFormatProperties(format, &props);

        if (tiling == vk::ImageTiling::eLinear &&
        (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal
        && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

void Setup::createDescriptorSetLayout() {
    vk::DescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

    vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    if(env->device.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout)!=vk::Result::eSuccess)
        throw std::runtime_error("failed to create render pass");
}

vk::ImageView Setup::createImageView(
  vk::Image image,
  vk::Format format,
  vk::ImageAspectFlags aspectFlags,
  uint32_t mipLevels)
  {
    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
    viewInfo.image = image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vk::ImageView imageView;
    if(env->device.createImageView(&viewInfo, nullptr, &imageView)!=vk::Result::eSuccess)
        throw std::runtime_error("failed to create render pass");

    return imageView;
}

void Setup::createGraphicsPipeline() {
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor{};
    vk::Offset2D offset = {0, 0};
    scissor.offset = offset;
    scissor.extent = swapChainExtent;

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = env->msaaSamples;

    vk::PipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = vk::StructureType::ePipelineDepthStencilStateCreateInfo;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
    vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    if(env->device.createPipelineLayout(
        &pipelineLayoutInfo,
        nullptr,
        &pipelineLayout)!=vk::Result::eSuccess)
        throw std::runtime_error("failed to create render pass");

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = vk::Pipeline(nullptr);
    if(env->device.createGraphicsPipelines(
        vk::PipelineCache(nullptr),
        1,
        &pipelineInfo, nullptr,
        &graphicsPipeline)!=vk::Result::eSuccess)
        throw std::runtime_error("failed to create render pass");


    vkDestroyShaderModule(env->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(env->device, vertShaderModule, nullptr);
}

vk::ShaderModule Setup::createShaderModule(const std::vector<char>& code) {
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    vk::ShaderModule shaderModule;
    if(env->device.createShaderModule(&createInfo,
    nullptr,
    &shaderModule)!=vk::Result::eSuccess)
        throw std::runtime_error("failed to create render pass");;

    return shaderModule;
}

void Setup::createFramebuffers(){
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<vk::ImageView,3> attachments = {
            color.view.get(),
            depth.view.get(),
            swapChainImageViews[i]
        };
        const vk::FramebufferCreateInfo framebufferInfo(
            {},
            renderPass,
            static_cast<uint32_t>(3),
            attachments.data(),
            swapChainExtent.width,
            swapChainExtent.height,
            1
        );
        if (env->device.createFramebuffer(&framebufferInfo, nullptr, &swapChainFramebuffers[i]) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void Setup::createSwapChain() {
    std::vector<vk::SurfaceFormatKHR> formats =
    env->physicalDevice.getSurfaceFormatsKHR(env->surface, env->dload);
    vk::SurfaceFormatKHR surfaceFormat;
    for (const auto& availableFormat : formats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
        availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            surfaceFormat = availableFormat;
        }
    }
    surfaceFormat = formats[0];

    std::vector<vk::PresentModeKHR> presentModes =
    env->physicalDevice.getSurfacePresentModesKHR(env->surface, env->dload);
    vk::PresentModeKHR presentMode;
    for (const auto& availablePresentMode : presentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            presentMode= availablePresentMode;
        }
    }
    presentMode = vk::PresentModeKHR::eFifo;

    vk::SurfaceCapabilitiesKHR capabilities =
    env->physicalDevice.getSurfaceCapabilitiesKHR(env->surface, env->dload);

    vk::Extent2D extent;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(env->window, &width, &height);

        vk::Extent2D actualExtent(
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        );

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        extent = actualExtent;
    }

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    QueueFamilyIndices indices = env->findQueueFamilies(env->physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    vk::SharingMode share;
    uint32_t queueFamilyIndexCount;
    uint32_t* queueFamilyIndixes;
    if (indices.graphicsFamily != indices.presentFamily) {
        share = vk::SharingMode::eConcurrent;
        queueFamilyIndexCount = 2;
        queueFamilyIndixes = queueFamilyIndices;
    } else {
        share = vk::SharingMode::eExclusive;
    }
    vk::SwapchainCreateInfoKHR createInfo(
        {},
        env->surface,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        share,
        queueFamilyIndexCount,
        queueFamilyIndixes,
        capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentMode,
        VK_TRUE
    );

    if (env->device.createSwapchainKHR(&createInfo, nullptr, &swapChain) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create swap chain!");
    }
    swapChainImages = env->device.getSwapchainImagesKHR(swapChain, env->dload);
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

ImageBundle Setup::loadImage(void *data, vk::Extent2D size, vk::Format format){
    uint32_t imgSize = size.height*size.width*4;
    uint32_t mip = static_cast<uint32_t>(
            std::floor(
                std::log2(
                    std::max(size.width, size.height))
        )) + 1;
    
    ImageBundle result(
        size.width,
        size.height,
        mip,
        vk::SampleCountFlagBits::e1,
        format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc|
            vk::ImageUsageFlagBits::eTransferDst|
            vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        env,
        vk::ImageAspectFlagBits::eColor);

    vcc::Buffer staging(
        env,
        imgSize,
        {},
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    void* stuff = env->device.mapMemory(staging.mem.get(),0,imgSize);
    memcpy(stuff, data, imgSize);
    env->device.unmapMemory(staging.mem.get());

    SingleTimeCmdBuffer cmd(&env->device, &env->graphicsQueue, &commandPool);
    transitionImageLayout(
        cmd.cmd,
        result.image.get(), 
        format, 
        vk::ImageLayout::eUndefined, 
        vk::ImageLayout::eUndefined, mip);
    
    cmd.cmd.copyBufferToImage(
        staging.buffer.get(), 
        result.image.get(), 
        vk::ImageLayout::eTransferDstOptimal, 
        vk::BufferImageCopy(
            0,
            0,
            0,
            vk::ImageSubresourceLayers(
                vk::ImageAspectFlagBits::eColor,
                0,
                0,
                1
            ),
            vk::Offset3D(0,0,0),
            vk::Extent3D(size.width, size.height, 1)
        )
    );

    vk::FormatProperties fprop = env->physicalDevice.getFormatProperties(format);
    if(!(fprop.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        throw std::runtime_error("physicalDevice does not support linear blitting");
    //Finish mipmap generation, and determine syncronization.
    vk::ImageMemoryBarrier mipmapper{};
    mipmapper.image = result.image.get();
    mipmapper.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mipmapper.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mipmapper.subresourceRange = vk::ImageSubresourceRange(
        vk::ImageAspectFlagBits::eColor,
        {},
        1,
        0,
        1
    );

    for (uint32_t i = 1; i < mip; i++) {
        mipmapper.subresourceRange.baseMipLevel = i - 1;
        mipmapper.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        mipmapper.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        mipmapper.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        mipmapper.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cmd.cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {},
            nullptr,
            nullptr,
            mipmapper
        );

        vk::ImageBlit blit;
        blit.srcOffsets[0] = vk::Offset3D(0,0,0);
        blit.srcOffsets[1] = vk::Offset3D(size.width, size.height, 1);
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = vk::Offset3D(0,0,0);
        blit.dstOffsets[1] = vk::Offset3D( size.width > 1 ? size.width / 2 : 1, size.height > 1 ? size.height / 2 : 1, 1 );
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        cmd.cmd.blitImage(
            result.image.get(),
            vk::ImageLayout::eTransferSrcOptimal,
            result.image.get(),
            vk::ImageLayout::eTransferDstOptimal,
            blit,
            vk::Filter::eLinear//research more on texel filtering later
        );

        mipmapper.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        mipmapper.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        mipmapper.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        mipmapper.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cmd.cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {},
            nullptr,
            nullptr,
            mipmapper
        );

        if (size.width > 1) size.width /= 2;
        if (size.height > 1) size.height /= 2;
    }
    cmd.submit();
    return result;
}

void Setup::transitionImageLayout(vk::CommandBuffer cmd, vk::Image image, vk::Format format, vk::ImageLayout old, vk::ImageLayout neo, uint32_t mip){
    vk::ImageMemoryBarrier barrier(
        {},{}, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor,
            0,
            mip,
            0,
            1
        )
    );
    vk::PipelineStageFlags src;
    vk::PipelineStageFlags dst;

    if (old == vk::ImageLayout::eUndefined && neo == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        src = vk::PipelineStageFlagBits::eTopOfPipe;
        dst = vk::PipelineStageFlagBits::eTransfer;
    } else if (old == vk::ImageLayout::eTransferDstOptimal && neo == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        src = vk::PipelineStageFlagBits::eTransfer;
        dst = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }
    cmd.pipelineBarrier(
        src, dst,
        {},
        nullptr,
        nullptr,
        barrier
    );
}