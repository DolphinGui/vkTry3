#ifndef BUFFERMEMORY_H_INCLUDE
#define BUFFERMEMORY_H_INCLUDE

#include "VulkanMemoryAllocator/src/VmaUsage.h"
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
#endif