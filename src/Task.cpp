#include "Task.hpp"

#include <functional>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "Setup.hpp"
#include "vk_mem_alloc.h"
#include "jobs/RecordJob.hpp"
#include "vkobjects/BufferBundle.hpp"
#include "vkobjects/ImageBundle.hpp"

using namespace vcc;

namespace {
std::pair<vk::Buffer, VmaAllocation>
stageAndLoad(VmaAllocator vmaAlloc, const void* const data, size_t size)
{
  VkBuffer staging;
  VmaAllocation alloc;
  VkBufferCreateInfo bufferCreate(
    vk::BufferCreateInfo({}, size, vk::BufferUsageFlagBits::eTransferSrc));
  VmaAllocationCreateInfo allocCreate{};
  allocCreate.usage = VMA_MEMORY_USAGE_CPU_ONLY;
  vmaCreateBuffer(
    vmaAlloc, &bufferCreate, &allocCreate, &staging, &alloc, nullptr);
  void* buffer;
  vmaMapMemory(vmaAlloc, alloc, &buffer);
  memcpy(buffer, data, size);
  vmaUnmapMemory(vmaAlloc, alloc);
  return { staging, alloc };
}
void
transitionImageLayout(vk::Image image,
                      vk::Format format,
                      vk::ImageLayout oldLayout,
                      vk::ImageLayout newLayout,
                      uint32_t mipLevels,
                      vk::CommandBuffer commandBuffer)
{
  vk::ImageMemoryBarrier barrier(
    {},
    {},
    oldLayout,
    newLayout,
    VK_QUEUE_FAMILY_IGNORED,
    VK_QUEUE_FAMILY_IGNORED,
    image,
    vk::ImageSubresourceRange(
      vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1));

  vk::PipelineStageFlags sourceStage;
  vk::PipelineStageFlags destinationStage;

  if (oldLayout == vk::ImageLayout::eUndefined &&
      newLayout == vk::ImageLayout::eTransferDstOptimal) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    destinationStage = vk::PipelineStageFlagBits::eTransfer;

  } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
             newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {

    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    sourceStage = vk::PipelineStageFlagBits::eTransfer;
    destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  commandBuffer.pipelineBarrier(
    sourceStage, destinationStage, {}, 0, nullptr, 0, nullptr, 1, &barrier);
}
}

Task::Task(Setup& s, VCEngine& e)
  : setup(s)
  , engine(e)
  , render(e,
           s.color.view,
           s.depth.view,
           s.renderPass,
           s.swapChain,
           s.swapChainImageViews,
           s.swapChainExtent,
           1)
  , mover(e.graphicsQueue, // change this later
          e.device,
          e.queueIndices.graphicsFamily.value())
{}

Task::~Task() {}

void
Task::run(const stbi_uc* const textureData,
          vk::Extent2D imageSize,
          size_t bSize,
          const std::vector<Vertex>& verticies,
          const std::vector<uint32_t>& indicies)
{

  VmaAllocationCreateInfo textureCreate{};
  textureCreate.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  textureCreate.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  uint32_t mipLevels =
    static_cast<uint32_t>(
      std::floor(std::log2(std::max(imageSize.width, imageSize.height)))) +
    1;

  vk::ImageCreateInfo imageInfo({},
                                vk::ImageType::e2D,
                                vk::Format::eR8G8B8A8Srgb,
                                {imageSize.width, imageSize.height, 1},
                                mipLevels,
                                1,
                                engine.msaaSamples,
                                vk::ImageTiling::eOptimal,
                                vk::ImageUsageFlagBits::eTransferDst |
                                  vk::ImageUsageFlagBits::eTransferSrc |
                                  vk::ImageUsageFlagBits::eSampled,
                                vk::SharingMode::eExclusive);

  ImageBundle tex(ImageBundle::create(imageInfo, textureCreate, engine));
  loadImage(textureData,
            bSize,
            imageSize,
            mipLevels,
            vk::Format::eR8G8B8A8Srgb,
            nullptr,
            tex.image);

  vk::BufferCreateInfo meshInfo{};
  meshInfo.size = sizeof(verticies[0]) * verticies.size();
  meshInfo.usage = vk::BufferUsageFlagBits::eTransferDst |
                   vk::BufferUsageFlagBits::eVertexBuffer;
  meshInfo.sharingMode = vk::SharingMode::eExclusive;
  VmaAllocationCreateInfo meshCreate{};
  meshCreate.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  textureCreate.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  BufferBundle vertex(meshInfo, meshCreate, engine.vmaAlloc);
  loadBuffer(&verticies, meshInfo.size, engine.device, vertex.buffer);
  meshInfo.size = sizeof(indicies[0]) * indicies.size();
  meshInfo.usage = vk::BufferUsageFlagBits::eTransferDst |
                   vk::BufferUsageFlagBits::eIndexBuffer;
  BufferBundle index(meshInfo, meshCreate, engine.vmaAlloc);
}
vk::Fence
Task::loadBuffer(const void* const data,
                 size_t size,
                 vk::Device fence,
                 vk::Buffer& out)
{
  std::pair<vk::Buffer, VmaAllocation> stage(
    stageAndLoad(engine.vmaAlloc, data, size));
  mover.submit(Mover::MoveJob{
    vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    [out, size, stage](vk::CommandBuffer cmd) {
      cmd.copyBuffer(stage.first, out, vk::BufferCopy(0, 0, size));
    } });
  vmaDestroyBuffer(engine.vmaAlloc, stage.first, stage.second);
  return fence.createFence(vk::FenceCreateInfo());
}

// Formats the image using a memory barrier.
vk::Fence
Task::loadImage(const void* const data,
                size_t size,
                vk::Extent2D imageDimensions,
                uint32_t mipLevels,
                vk::Format format,
                vk::Device fence,
                vk::Image& out)
{
  std::pair<vk::Buffer, VmaAllocation> stage(
    stageAndLoad(engine.vmaAlloc, data, size));

  vmaDestroyBuffer(engine.vmaAlloc, stage.first, stage.second);
  return fence.createFence(vk::FenceCreateInfo());
}
