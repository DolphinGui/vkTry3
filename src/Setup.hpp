#pragma once
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
  const VCEngine& env;

  Setup(VCEngine& engine);
  ~Setup();

  const vk::SurfaceCapabilitiesKHR capabilities;
  const vk::Extent2D swapChainExtent;
  const vk::SurfaceFormatKHR swapChainImageFormat;
  const vk::SwapchainKHR swapChain;
  const std::vector<vk::Image> swapChainImages;
  std::vector<vk::ImageView> swapChainImageViews;

  const vk::RenderPass renderPass;
  const vk::DescriptorSetLayout descriptorSetLayout;
  const std::pair<vk::PipelineLayout, vk::Pipeline>
    pipeline; // figure out dynamic pipeline recreation
  VmaAllocationCreateInfo allocationInfo;
  const vk::Queue graphicsQueue;
  const std::mutex graphicsLock;
  const vk::Queue presentQueue;
  const vk::Queue transferQueue;
  const ImageBundle color;
  const ImageBundle depth;
  const std::vector<vk::Framebuffer> swapChainFramebuffers;

};
}

