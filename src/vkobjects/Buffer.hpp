#ifndef BUFFERMEMORY_H_INCLUDE
#define BUFFERMEMORY_H_INCLUDE

#include <vulkan/vulkan.hpp>

namespace vcc {
class VCEngine;
class Buffer
{
public:
  vk::UniqueBuffer buffer;
  vk::UniqueDeviceMemory mem;
  Buffer(vk::Device,
         const vk::PhysicalDeviceMemoryProperties&,
         size_t size,
         vk::BufferCreateFlags,
         vk::BufferUsageFlags,
         vk::MemoryPropertyFlags,
         vk::SharingMode = vk::SharingMode::eExclusive,
         uint32_t queueFamilyIndexCount = {},
         const uint32_t* queueFamilyIndicies = {});
  Buffer() = default;
  static Buffer load(vk::Device,
                     const vk::PhysicalDeviceMemoryProperties&,
                     size_t size,
                     void* data,
                     vk::BufferCreateFlags,
                     vk::BufferUsageFlags,
                     vk::MemoryPropertyFlags,
                     vk::SharingMode share = vk::SharingMode::eExclusive,
                     uint32_t queueFamilyIndexCount = {},
                     const uint32_t* queueFamilyIndicies = {});
};

}
#endif