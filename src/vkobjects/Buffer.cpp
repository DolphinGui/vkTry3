#include <array>
#include <bits/stdint-uintn.h>
#include <vulkan/vulkan.hpp>

#include "Buffer.hpp"
#include "../VCEngine.hpp"

using namespace vcc;

Buffer::Buffer(
    VCEngine* env, 
    size_t size,
    vk::BufferCreateFlags create,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags memProps,
    vk::SharingMode share,
    uint32_t queueFamilyIndexCount,
    const uint32_t* queueFamilyIndicies
    ){

    vk::BufferCreateInfo bufferInfo(
        {},
        size,
        usage,
        share,
        queueFamilyIndexCount,
        queueFamilyIndicies
    );
    buffer = env->getDevPtr()->createBufferUnique(bufferInfo);
    vk::MemoryRequirements memReqs = env->getDevPtr()->getBufferMemoryRequirements(buffer.get());
    vk::PhysicalDeviceMemoryProperties physmem = env->getMemProps();
    uint32_t memory = 0;
    while(memory < physmem.memoryTypeCount) {
        if ((memReqs.memoryTypeBits & (1 << memory)) && 
        (physmem.memoryTypes[memory].propertyFlags & memProps) == memProps) {
            break;
        }
        memory++;
    }
    
    vk::MemoryAllocateInfo memInfo(memReqs.size, memory);
    mem = env->getDevPtr()->allocateMemoryUnique(memInfo);
}

Buffer Buffer::load(
    vcc::VCEngine* env,
    size_t size, 
    void* data,
    vk::BufferCreateFlags create,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags memProps,
    vk::SharingMode share,
    uint32_t queueFamilyIndexCount,
    const uint32_t* queueFamilyIndicies)
    {
    Buffer result = Buffer(env,size,create,usage,memProps,share,queueFamilyIndexCount,queueFamilyIndicies);
    Buffer staging = Buffer(
        env,
        size,
        create,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        share,
        queueFamilyIndexCount,
        queueFamilyIndicies
    );
    void* stuff;
    stuff = env->getDevPtr()->mapMemory(staging.mem.get(), 0, size);

    memcpy(stuff, data, size);

    env->getDevPtr()->unmapMemory(staging.mem.get());

    return result;
}