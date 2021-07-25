#pragma once

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#include "VCEngine.hpp"
#include "vk_mem_alloc.h"

namespace vcc {
/*Change to use VMA later. Maybe figure out a smarter allocation system.*/
class ImageBundle
{
public:
  vk::Image image;
  VmaAllocation mem;
  vk::ImageView view;
  ~ImageBundle();

  static ImageBundle create(const vk::ImageCreateInfo&,
                            const VmaAllocationCreateInfo&,
                            const VCEngine& env,
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

