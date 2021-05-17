#include <GLFW/glfw3.h>
#include <iterator>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#include "ImageBundle.hpp"
#include "VCEngine.hpp"
using namespace vcc;

ImageBundle
ImageBundle::create(const vk::ImageCreateInfo& imageInfo,
                    const VmaAllocationCreateInfo& allocInfo,
                    VCEngine* env,
                    vk::ImageAspectFlags viewAspectFlags)
{
  VkImage image;
  VmaAllocation alloc;
  vk::ImageView view{};
  vmaCreateImage(env->vmaAlloc,
                 reinterpret_cast<const VkImageCreateInfo*>(&imageInfo),
                 &allocInfo,
                 &image,
                 &alloc,
                 nullptr);
  if (viewAspectFlags) {
    const vk::ImageViewCreateInfo viewInfo(
      {},
      image,
      vk::ImageViewType::e2D,
      imageInfo.format,
      {},
      vk::ImageSubresourceRange(viewAspectFlags, 0, imageInfo.mipLevels, 0, 1));
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
  if (view)
    dev.destroyImageView(view);
}

uint32_t
ImageBundle::findMemoryType(uint32_t typeFilter,
                            vk::MemoryPropertyFlags properties,
                            const vk::PhysicalDevice& physicalDevice)
{
  vk::PhysicalDeviceMemoryProperties memProperties;
  physicalDevice.getMemoryProperties(&memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}
