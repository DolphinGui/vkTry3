#include <GLFW/glfw3.h>
#include <iterator>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#include "ImageBundle.hpp"
#include "Buffer.hpp"
#include "VCEngine.hpp"
using namespace vcc;
ImageBundle::ImageBundle(
    uint32_t width,
    uint32_t height,
    uint32_t mipLevels,
    vk::SampleCountFlagBits numSamples,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    VCEngine* env,
    vk::ImageAspectFlags viewAspectFlags
    ):
dev(&env->device)
{
    const vk::ImageCreateInfo imageInfo(
        {},
        vk::ImageType::e2D,
        format,
        vk::Extent3D(width, height, 1),
        mipLevels,
        1,
        numSamples,
        tiling,
        usage,
        vk::SharingMode::eExclusive,
        {}, {},
        vk::ImageLayout::eUndefined
    );
    image = env->device.createImageUnique(imageInfo);

    vk::MemoryRequirements memRequirements;
    env->device.getImageMemoryRequirements(image.get(),&memRequirements);
    const vk::MemoryAllocateInfo allocInfo(
        memRequirements.size,
        findMemoryType(memRequirements.memoryTypeBits, properties, &env->physicalDevice));

    mem = env->device.allocateMemoryUnique(allocInfo);
    env->device.bindImageMemory(image.get(), mem.get(), 0);

    if(viewAspectFlags){
        const vk::ImageViewCreateInfo viewInfo(
        {},
        image.get(),
        vk::ImageViewType::e2D,
        format,
        {},
        vk::ImageSubresourceRange(
            viewAspectFlags,
            0,
            mipLevels,
            0,
            1
        ));
        view = env->device.createImageViewUnique(viewInfo);
    }
}

uint32_t ImageBundle::findMemoryType(
    uint32_t typeFilter,
    vk::MemoryPropertyFlags properties,
    vk::PhysicalDevice* physicalDevice)
    {
    vk::PhysicalDeviceMemoryProperties memProperties;
    physicalDevice->getMemoryProperties(&memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

