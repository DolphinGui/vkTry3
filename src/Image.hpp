#ifndef IMAGE_H_INCLUDE
#define IMAGE_H_INCLUDE

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

namespace vcc{
class Image{
public:
  vk::UniqueImage image;
  vk::UniqueDeviceMemory mem;
  vk::UniqueImageView view;
  vk::Device* dev;
  Image(){};
  Image(
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
    vk::ImageAspectFlags viewAspectFlags = {}):
    dev(device)
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
      vk::ImageLayout::eUndefined);

    vk::Image im;

    if(device->createImage(&imageInfo, nullptr, &im)!=vk::Result::eSuccess)
      throw std::runtime_error("Failed to create Image");
    image = device->createImageUnique(imageInfo);

    vk::MemoryRequirements memRequirements;
    device->getImageMemoryRequirements(image.get(),&memRequirements);
    const vk::MemoryAllocateInfo allocInfo(
      memRequirements.size, 
      findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice));
    
    mem = device->allocateMemoryUnique(allocInfo);
    device->bindImageMemory(image.get(), mem.get(), 0);

    if(!viewAspectFlags){
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
    
    
  };

  uint32_t findMemoryType(
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

};
}
#endif
