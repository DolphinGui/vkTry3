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
    init(width, height, mipLevels, numSamples, format, tiling, usage, properties, &env->device, &env->physicalDevice, viewAspectFlags);
}

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
  void* data,
  size_t size,
  vk::ImageAspectFlags viewAspectFlags){
    init(width, height, mipLevels, numSamples, format, tiling, usage, properties, &env->device, &env->physicalDevice, viewAspectFlags);
    Buffer staging = Buffer(
        env,
        size,
        {},
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    void* stuff;
    stuff = env->device.mapMemory(staging.mem.get(), 0, size);
    memcpy(stuff, data, size);
    env->device.unmapMemory(staging.mem.get()); //TODO: fix this to transition layout
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

void ImageBundle::init(
  uint32_t width,
  uint32_t height,
  uint32_t mipLevels,
  vk::SampleCountFlagBits numSamples,
  vk::Format format,
  vk::ImageTiling tiling,
  vk::ImageUsageFlags usage,
  vk::MemoryPropertyFlags properties,
  vk::Device* device,
  vk::PhysicalDevice* physicalDevice,
  vk::ImageAspectFlags viewAspectFlags)
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
    image = device->createImageUnique(imageInfo);

    vk::MemoryRequirements memRequirements;
    device->getImageMemoryRequirements(image.get(),&memRequirements);
    const vk::MemoryAllocateInfo allocInfo(
        memRequirements.size,
        findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice));

    mem = device->allocateMemoryUnique(allocInfo);
    device->bindImageMemory(image.get(), mem.get(), 0);

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
        view = device->createImageViewUnique(viewInfo);
        }

  }