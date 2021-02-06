#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "VCEngine.hpp"

#ifndef IMAGE_H_INCLUDE
#define IMAGE_H_INCLUDE

namespace vcc{
class Image{
public:
  vk::Image image;
  vk::DeviceMemory mem;
  vk::ImageView view = nullptr;
  vk::Device dev;
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
  vk::Device device,
  vk::PhysicalDevice physicalDevice,
  vk::ImageAspectFlags viewAspectFlags = {}):
  dev(device)
  {
    vk::ImageCreateInfo imageInfo{};
    imageInfo.sType = vk::StructureType::eImageCreateInfo;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;

    device.createImage(&imageInfo, nullptr, &image);
    
    //vkGetImageMemoryRequirements
    vk::MemoryRequirements memRequirements;
    device.getImageMemoryRequirements(image, &memRequirements);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);
    device.allocateMemory(&allocInfo, nullptr, &mem);

    vkBindImageMemory(device, image, mem, 0);
    if(!viewAspectFlags){
      vk::ImageViewCreateInfo viewInfo{};
      viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
      viewInfo.image = image;
      viewInfo.viewType = vk::ImageViewType::e2D;
      viewInfo.format = format;
      viewInfo.subresourceRange.aspectMask = viewAspectFlags;
      viewInfo.subresourceRange.baseMipLevel = 0;
      viewInfo.subresourceRange.levelCount = mipLevels;
      viewInfo.subresourceRange.baseArrayLayer = 0;
      viewInfo.subresourceRange.layerCount = 1;
      device.createImageView(&viewInfo, nullptr, &view);
    }
  };
  ~Image(){
    if(!view){
      vkDestroyImageView(dev, view, nullptr);
    }
    vkDestroyImage(dev, image, nullptr);
    vkFreeMemory(dev, mem, nullptr);
  };

  uint32_t findMemoryType(
    uint32_t typeFilter,
    vk::MemoryPropertyFlags properties,
    vk::PhysicalDevice physicalDevice)
    {
      vk::PhysicalDeviceMemoryProperties memProperties;
      physicalDevice.getMemoryProperties(&memProperties);

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
