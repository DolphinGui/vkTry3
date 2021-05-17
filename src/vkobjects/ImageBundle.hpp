#ifndef IMAGE_H_INCLUDE
#define IMAGE_H_INCLUDE

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#include "VCEngine.hpp"
#include "VulkanMemoryAllocator/src/VmaUsage.h"

namespace vcc {
class ImageBundle
{
public:
  vk::Image image;
  VmaAllocation mem;
  vk::ImageView view;
  ~ImageBundle();

  static ImageBundle create(const vk::ImageCreateInfo&,
                            const VmaAllocationCreateInfo&,
                            VCEngine* env,
                            vk::ImageAspectFlags viewAspectFlags = {});
  static uint32_t findMemoryType(uint32_t typeFilter,
                                 vk::MemoryPropertyFlags properties,
                                 const vk::PhysicalDevice& physicalDevice);

private:
  ImageBundle(vk::Image,
              VmaAllocation,
              vk::ImageView,
              vk::Device,
              const VmaAllocator&);
  const vk::Device dev;
  const VmaAllocator& alloc;
};
}
#endif
