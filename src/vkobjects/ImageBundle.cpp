#include <GLFW/glfw3.h>
#include <iterator>
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "ImageBundle.hpp"
#include "VCEngine.hpp"
using namespace vcc;

ImageBundle
ImageBundle::create(const vk::ImageCreateInfo& imageInfo,
                    const VmaAllocationCreateInfo& allocInfo,
                    const VCEngine& env,
                    vk::ImageAspectFlags viewAspectFlags)
{
  vk::Image image;
  VmaAllocation alloc;
  vmaCreateImage(env.vmaAlloc,
                 reinterpret_cast<const VkImageCreateInfo*>(&imageInfo),
                 reinterpret_cast<const VmaAllocationCreateInfo*>(&allocInfo),
                 reinterpret_cast<VkImage*>(&image),
                 &alloc,
                 nullptr);
    vk::ImageView view{};

  if (viewAspectFlags) {
    view = env.device.createImageView(vk::ImageViewCreateInfo(
      {},
      image,
      vk::ImageViewType::e2D,
      imageInfo.format,
      {},
      vk::ImageSubresourceRange(
        viewAspectFlags, 0, imageInfo.mipLevels, 0, 1)));
  }
  return {image, alloc, view, env.device, env.vmaAlloc};
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
