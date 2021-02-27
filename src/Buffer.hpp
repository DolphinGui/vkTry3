#ifndef BUFFERMEMORY_H_INCLUDE
#define BUFFERMEMORY_H_INCLUDE

#include <vulkan/vulkan.hpp>

namespace vcc{
class VCEngine;
class Buffer{
public:
vk::UniqueBuffer buffer;
vk::UniqueDeviceMemory mem;
vk::Device* dev;
Buffer(
    vcc::VCEngine* env,
    size_t size, 
    vk::BufferCreateFlags create,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags memProps,
    vk::SharingMode share = vk::SharingMode::eExclusive,
    uint32_t queueFamilyIndexCount = {},
    const uint32_t* queueFamilyIndicies = {}
);
Buffer(){};
static Buffer load(
    vcc::VCEngine* env,
    size_t size, 
    void* data,
    vk::BufferCreateFlags create,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags memProps,
    vk::SharingMode share = vk::SharingMode::eExclusive,
    uint32_t queueFamilyIndexCount = {},
    const uint32_t* queueFamilyIndicies = {}
);

};

}
#endif