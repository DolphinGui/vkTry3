#pragma once

#include "vk_mem_alloc.h"
#include <vulkan/vulkan.hpp>

namespace vcc {
class VCEngine;
class BufferBundle
{
public:
  vk::Buffer buffer;
  VmaAllocation mem;
  BufferBundle(const vk::BufferCreateInfo&,
               const VmaAllocationCreateInfo&,
               const VmaAllocator&);
  ~BufferBundle();

private:
  const VmaAllocator& alloc;
};

}
