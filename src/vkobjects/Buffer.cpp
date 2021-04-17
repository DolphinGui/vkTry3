#include <array>
#include <bits/stdint-uintn.h>
#include <vulkan/vulkan.hpp>

#include "Buffer.hpp"
#include "VCEngine.hpp"

using namespace vcc;

Buffer::Buffer(vk::Device device,
               const vk::PhysicalDeviceMemoryProperties& physProps,
               size_t size,
               vk::BufferCreateFlags create,
               vk::BufferUsageFlags usage,
               vk::MemoryPropertyFlags memProps,
               vk::SharingMode share,
               uint32_t queueFamilyIndexCount,
               const uint32_t* queueFamilyIndicies)
{

  vk::BufferCreateInfo bufferInfo(
    {}, size, usage, share, queueFamilyIndexCount, queueFamilyIndicies);
  buffer = device.createBufferUnique(bufferInfo);
  vk::MemoryRequirements memReqs =
    device.getBufferMemoryRequirements(buffer.get());
  vk::PhysicalDeviceMemoryProperties physmem = physProps;
  uint32_t memory = 0;
  while (memory < physmem.memoryTypeCount) {
    if ((memReqs.memoryTypeBits & (1 << memory)) &&
        (physmem.memoryTypes[memory].propertyFlags & memProps) == memProps) {
      break;
    }
    memory++;
  }

  vk::MemoryAllocateInfo memInfo(memReqs.size, memory);
  mem = device.allocateMemoryUnique(memInfo);
}

Buffer
Buffer::load(vk::Device device,
             const vk::PhysicalDeviceMemoryProperties& physProps,
             size_t size,
             void* data,
             vk::BufferCreateFlags create,
             vk::BufferUsageFlags usage,
             vk::MemoryPropertyFlags memProps,
             vk::SharingMode share,
             uint32_t queueFamilyIndexCount,
             const uint32_t* queueFamilyIndicies)
{
  Buffer result = Buffer(device,
                         physProps,
                         size,
                         create,
                         usage,
                         memProps,
                         share,
                         queueFamilyIndexCount,
                         queueFamilyIndicies);
  Buffer staging = Buffer(device,
                          physProps,
                          size,
                          create,
                          vk::BufferUsageFlagBits::eTransferSrc,
                          vk::MemoryPropertyFlagBits::eHostVisible |
                            vk::MemoryPropertyFlagBits::eHostCoherent,
                          share,
                          queueFamilyIndexCount,
                          queueFamilyIndicies);
  void* stuff;
  stuff = device.mapMemory(staging.mem.get(), 0, size);

  memcpy(stuff, data, size);

  device.unmapMemory(staging.mem.get());

  return result;
}