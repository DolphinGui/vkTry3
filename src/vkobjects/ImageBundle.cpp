#include <GLFW/glfw3.h>
#include <iterator>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#include "Buffer.hpp"
#include "ImageBundle.hpp"
#include "VCEngine.hpp"
using namespace vcc;

ImageBundle
ImageBundle::create(uint32_t width,
                    uint32_t height,
                    uint32_t mipLevels,
                    vk::SampleCountFlagBits numSamples,
                    vk::Format format,
                    vk::ImageTiling tiling,
                    vk::ImageUsageFlags usage,
                    vk::MemoryPropertyFlags properties,
                    VCEngine* env,
                    vk::ImageAspectFlags viewAspectFlags)
{
  VkImageCreateInfo imageInfo(
    vk::ImageCreateInfo({},
                        vk::ImageType::e2D,
                        format,
                        vk::Extent3D(width, height, 1),
                        mipLevels,
                        1,
                        numSamples,
                        tiling,
                        usage,
                        vk::SharingMode::eExclusive,
                        {},
                        {},
                        vk::ImageLayout::eUndefined));

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  VkImage image;
  VmaAllocation alloc;
  vk::ImageView view{};
  vmaCreateImage(
    env->vmaAlloc, &imageInfo, &allocInfo, &image, &alloc, nullptr);
  if (viewAspectFlags) {
    const vk::ImageViewCreateInfo viewInfo(
      {},
      image,
      vk::ImageViewType::e2D,
      format,
      {},
      vk::ImageSubresourceRange(viewAspectFlags, 0, mipLevels, 0, 1));
    view = env->device.createImageView(viewInfo);
  }
  return ImageBundle(image, alloc, view, env->device, env->vmaAlloc);
}

ImageBundle::ImageBundle(vk::Image image,
                         VmaAllocation alloc,
                         vk::ImageView view,
                         vk::Device device,
                         const VmaAllocator& allocator)
  : image(image)
  , mem(alloc)
  , view(view)
  , dev(device)
  , alloc(allocator)
{}

ImageBundle::~ImageBundle()
{
  vmaDestroyImage(alloc, image, mem);
  if(view)
    dev.destroyImageView(view);
}

uint32_t
ImageBundle::findMemoryType(uint32_t typeFilter,
                            vk::MemoryPropertyFlags properties,
                            const vk::PhysicalDevice* physicalDevice)
{
  vk::PhysicalDeviceMemoryProperties memProperties;
  physicalDevice->getMemoryProperties(&memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}
