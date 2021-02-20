#ifndef IMAGE_H_INCLUDE
#define IMAGE_H_INCLUDE

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

namespace vcc{
class ImageBundle{
public:
vk::UniqueImage image;
vk::UniqueDeviceMemory mem;
vk::UniqueImageView view;
vk::Device* dev;
ImageBundle(){};
ImageBundle(
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
  vk::ImageAspectFlags viewAspectFlags = {});

uint32_t findMemoryType(
  uint32_t typeFilter,
  vk::MemoryPropertyFlags properties,
  vk::PhysicalDevice* physicalDevice);
};
}
#endif
