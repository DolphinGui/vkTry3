#include "Task.hpp"
#include "Setup.hpp"
#include "jobs/RecordJob.hpp"
#include "vkobjects/Buffer.hpp"
#include <functional>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

using namespace vcc;

Task::Task(Setup* s, VCEngine* e)
  : setup(s)
  , engine(e)
  , render(e->graphicsQueue,
           e->device,
           e->findQueueFamilies(e->physicalDevice).graphicsFamily.value())
  , mover(e->graphicsQueue, // change this later
          e->device,
          e->findQueueFamilies(e->physicalDevice).graphicsFamily.value())
{}

Task::~Task() {}

void
Task::run(stbi_uc* image,
          vk::Extent2D imageSize,
          size_t bSize,
          std::vector<std::pair<Vertex, uint32_t>>* verts)
{
  VkImage texture;
  VmaAllocation textureMem;

  VmaAllocationCreateInfo textureCreate{};
  textureCreate.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  textureCreate.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  VkImageCreateInfo imageinfo(
    static_cast<VkImageCreateInfo>(vk::ImageCreateInfo(
      {},
      vk::ImageType::e2D,
      vk::Format::eR8G8B8A8Srgb,
      vk::Extent3D(imageSize.width, imageSize.height),
      static_cast<uint32_t>(
        std::floor(std::log2(std::max(imageSize.width, imageSize.height)))) +
        1,
      1,
      engine->msaaSamples,
      vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eTransferDst |
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled,
      vk::SharingMode::eExclusive)));

  vmaCreateImage(engine->vmaAlloc,
                 &imageinfo,
                 &textureCreate,
                 &texture,
                 &textureMem,
                 nullptr);
}
// stages and loads data into buffer. no formatting is done.
void
Task::loadContent(void* data, size_t size, vk::Buffer out)
{
  VkBuffer staging;
  VmaAllocation alloc;
  VkBufferCreateInfo bufferCreate(
    vk::BufferCreateInfo({}, size, vk::BufferUsageFlagBits::eTransferSrc));
  VmaAllocationCreateInfo allocCreate{};
  allocCreate.usage = VMA_MEMORY_USAGE_CPU_ONLY;
  vmaCreateBuffer(
    engine->vmaAlloc, &bufferCreate, &allocCreate, &staging, &alloc, nullptr);
  void* buffer;
  vmaMapMemory(engine->vmaAlloc, alloc, &buffer);
  memcpy(buffer, data, size);
  vmaUnmapMemory(engine->vmaAlloc, alloc);
  mover.submit({ vcc::RecordJob(
    [out, size, staging](vk::CommandBuffer cmd) {
      cmd.copyBuffer(staging, out, vk::BufferCopy(0, 0, size));
    },
    nullptr) });
  vmaDestroyBuffer(engine->vmaAlloc, staging, alloc);
}
