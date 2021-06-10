#ifndef SETUP_H_INCLUDE
#define SETUP_H_INCLUDE
#include <bits/stdint-uintn.h>
#include <fstream>
#include <mutex>
#include <string_view>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "vkobjects/ImageBundle.hpp"
#include "workers/RenderGroup.hpp"

using namespace vcc;
namespace vcc {
class Setup
{
public:
  VCEngine* env;

  Setup(VCEngine* engine);
  ~Setup();

private:
  const vk::SurfaceFormatKHR swapChainImageFormat;
  const vk::Extent2D swapChainExtent;

public:
  const vk::RenderPass renderPass;
  const ImageBundle color;
  const ImageBundle depth;

private:
  const vk::SurfaceCapabilitiesKHR capabilities;
  const vk::SwapchainKHR swapChain;
  const std::vector<vk::Image> swapChainImages;

  const std::vector<vk::ImageView> swapChainImageViews;
  const std::vector<vk::Framebuffer> swapChainFramebuffers;

  const vk::DescriptorSetLayout descriptorSetLayout;
  const std::pair<vk::PipelineLayout, vk::Pipeline>
    pipeline; // figure out dynamic pipeline recreation

  const vk::Queue graphicsQueue;
  const std::mutex graphicsLock;
  const vk::Queue presentQueue;
  const vk::Queue transferQueue;

  VmaAllocationCreateInfo allocationInfo;
};
}
#endif
