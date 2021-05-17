#include <array>
#include <bits/stdint-uintn.h>
#include <vulkan/vulkan.hpp>

#include "BufferBundle.hpp"
#include "VCEngine.hpp"

using namespace vcc;

BufferBundle::BufferBundle(const vk::BufferCreateInfo& bufferInfo,
                           const VmaAllocationCreateInfo& memInfo,
                           const VmaAllocator& alloc)
  : alloc(alloc)
{
  vmaCreateBuffer(alloc,
                  reinterpret_cast<const VkBufferCreateInfo*>(&bufferInfo),
                  &memInfo,
                  reinterpret_cast<VkBuffer*>(&buffer),
                  &mem,
                  nullptr);
}
BufferBundle::~BufferBundle()
{
  vmaDestroyBuffer(alloc, static_cast<VkBuffer>(buffer), mem);
}