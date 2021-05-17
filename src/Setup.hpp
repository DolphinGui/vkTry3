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
#include "workers/Renderer.hpp"

using namespace vcc;
namespace vcc {
class Setup
{
public:
  VCEngine* env;

  Setup(VCEngine* engine);
  ~Setup();

  const ImageBundle color;
  const ImageBundle depth;
  const vk::RenderPass renderPass;

private:

  const vk::SwapchainKHR swapChain;
  const std::vector<vk::Image> swapChainImages;
  const vk::SurfaceFormatKHR swapChainImageFormat;
  const vk::Extent2D swapChainExtent;
  const vk::SurfaceCapabilitiesKHR capabilities;
  const std::vector<vk::ImageView> swapChainImageViews;
  const std::vector<vk::Framebuffer> swapChainFramebuffers;

  
  const vk::DescriptorSetLayout descriptorSetLayout;
  const std::pair<vk::PipelineLayout, vk::Pipeline> pipeline;

  

  const vk::Queue graphicsQueue;
  const std::mutex graphicsLock;
  const vk::Queue presentQueue;
  const vk::Queue transferQueue;

  VmaAllocationCreateInfo allocationInfo;
};
}
#endif
