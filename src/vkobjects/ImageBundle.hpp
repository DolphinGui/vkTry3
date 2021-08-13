#pragma once

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <tuple>
#include <vulkan/vulkan.hpp>

#include "VCEngine.hpp"
#include "vk_mem_alloc.h"

namespace vcc {
/*Change to use VMA later. Maybe figure out a smarter allocation system.*/
class ImageBundle
{
public:
  ~ImageBundle();
  ImageBundle(const ImageBundle&) = delete;
  ImageBundle(ImageBundle&&) noexcept;

  ImageBundle(const vk::ImageCreateInfo&,
              const VmaAllocationCreateInfo&,
              const VCEngine& env,
              vk::ImageAspectFlags viewAspectFlags = {});
  ImageBundle(vk::Image,
              VmaAllocation,
              vk::ImageView,
              vk::Device,
              const VmaAllocator);
  auto release() noexcept
  {
    auto result = std::make_tuple(image, mem, view, dev, alloc);
    image = nullptr;
    mem = nullptr;
    view = nullptr;
    return result;
  };
  static uint32_t findMemoryType(uint32_t typeFilter,
                                 vk::MemoryPropertyFlags properties,
                                 const vk::PhysicalDevice& physicalDevice);

  vk::Image image;
  VmaAllocation mem;
  vk::ImageView view;

private:
  vk::Device dev;     // non-owning
  VmaAllocator alloc; // non-owning
};
}
