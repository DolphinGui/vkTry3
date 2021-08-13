#include "Task.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>

#include "Setup.hpp"
#include "jobs/RecordJob.hpp"
#include "vk_mem_alloc.h"
#include "vkobjects/BufferBundle.hpp"
#include "vkobjects/ImageBundle.hpp"

using namespace vcc;

namespace {
vcc::BufferBundle
stageAndLoad(VmaAllocator vmaAlloc,
             const void* const data,
             size_t size) noexcept
{
  vk::BufferCreateInfo bufferCreate(
    vk::BufferCreateInfo({}, size, vk::BufferUsageFlagBits::eTransferSrc));
  VmaAllocationCreateInfo allocCreate{};
  allocCreate.usage = VMA_MEMORY_USAGE_CPU_ONLY;
  vcc::BufferBundle stage(bufferCreate, allocCreate, vmaAlloc);
  void* buffer{};
  auto result = vmaMapMemory(vmaAlloc, stage.mem, &buffer);
  std::memcpy(buffer, data, size);
  vmaUnmapMemory(vmaAlloc, stage.mem);
  return stage;
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

Task::~Task() = default;

typedef uint32_t (*mipFunct)(vk::Extent2D extent);

constexpr mipFunct
mip(vk::SampleCountFlags msaa)
{
  if (msaa != vk::SampleCountFlagBits::e1)
    return [](vk::Extent2D extent) -> uint32_t { return 1; };
  return [](vk::Extent2D extent) -> uint32_t {
    return static_cast<uint32_t>(
             std::floor(std::log2(std::max(extent.width, extent.height)))) +
           1;
  };
}

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

  uint32_t mipLevels = mip(engine.msaaSamples)(imageSize);

  vk::ImageCreateInfo imageInfo({}, // imagecreateflags
                                vk::ImageType::e2D,
                                vk::Format::eR8G8B8A8Srgb,
                                { imageSize.width, imageSize.height, 1 },
                                mipLevels,
                                1,
                                engine.msaaSamples,
                                vk::ImageTiling::eOptimal,
                                vk::ImageUsageFlagBits::eTransferDst |
                                  vk::ImageUsageFlagBits::eTransferSrc |
                                  vk::ImageUsageFlagBits::eSampled,
                                vk::SharingMode::eExclusive);

  ImageBundle tex(ImageBundle(imageInfo, textureCreate, engine));
  loadImage(textureData,
            bSize,
            imageSize,
            mipLevels,
            vk::Format::eR8G8B8A8Srgb,
            tex.image);

  vk::BufferCreateInfo meshInfo{};
  meshInfo.size = sizeof(verticies[0]) * verticies.size();
  meshInfo.usage = vk::BufferUsageFlagBits::eTransferDst |
                   vk::BufferUsageFlagBits::eVertexBuffer;
  meshInfo.sharingMode = vk::SharingMode::eExclusive;
  VmaAllocationCreateInfo meshAlloc{};
  meshAlloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  textureCreate.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  BufferBundle vertex(meshInfo, meshAlloc, engine.vmaAlloc);
  loadBuffer(verticies.data(), meshInfo.size, vertex.buffer);

  meshInfo.size = sizeof(indicies[0]) * indicies.size();
  meshInfo.usage = vk::BufferUsageFlagBits::eTransferDst |
                   vk::BufferUsageFlagBits::eIndexBuffer;
  BufferBundle index(meshInfo, meshAlloc, engine.vmaAlloc);
  loadBuffer(indicies.data(), meshInfo.size, index.buffer);
  mover.wait();
}
std::future<BufferBundle>
Task::loadBuffer(const void* const data, size_t size, BufferBundle&& out)
{
  vcc::BufferBundle stage(stageAndLoad(engine.vmaAlloc, data, size));
  /*This lambda capture holds 4 pointers and 1 size_t.
    Becuase std::function cannot move for some reason.
    Should probably figure out a way to get around mallocing.
    the max size gcc doesn't malloc is 2 pointers.*/
  std::promise<BufferBundle> promise;
  auto r = mover.submit(Mover::MoveJob{
    vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    [auto 
      outBuffer = out.buffer,
     size,
     args = std::tuple_cat(stage.release(), std::make_tuple(stage.alloc))](
      vk::CommandBuffer cmd) mutable {
      BufferBundle buffer = std::make_from_tuple<BufferBundle>(args);
      cmd.copyBuffer(buffer.buffer, outBuffer, vk::BufferCopy(0, 0, size));
    } });

  return;
}

// Formats the image using a memory barrier.
std::future<ImageBundle>
Task::loadImage(const void* const data,
                size_t size,
                vk::Extent2D imageDimensions,
                uint32_t mipLevels,
                vk::Format format,
                ImageBundle&& out)
{
  vcc::BufferBundle stage(stageAndLoad(engine.vmaAlloc, data, size));
  return;
}
