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

private:
  friend class Renderer<3>;

  const vk::SwapchainKHR swapChain;
  const std::vector<vk::Image> swapChainImages;
  const vk::SurfaceFormatKHR swapChainImageFormat;
  const vk::Extent2D swapChainExtent;
  const vk::SurfaceCapabilitiesKHR capabilities;
  const std::vector<vk::ImageView> swapChainImageViews;
  const std::vector<vk::Framebuffer> swapChainFramebuffers;

  const vk::RenderPass renderPass;
  const vk::DescriptorSetLayout descriptorSetLayout;
  const std::pair<vk::PipelineLayout, vk::Pipeline> pipeline;

  const ImageBundle color;
  const ImageBundle depth;

  const vk::Queue graphicsQueue;
  const std::mutex graphicsLock;
  const vk::Queue presentQueue;
  const vk::Queue transferQueue;

  vk::SwapchainKHR createSwap();
  vk::SurfaceFormatKHR getSwapFormat();
  vk::Extent2D getSurfaceExtent();

  vk::RenderPass createRenderPass();
  vk::DescriptorSetLayout createDescriptorSetLayout();
  std::pair<vk::PipelineLayout, vk::Pipeline> createGraphicsPipeline();
  void createDepthResources();
  std::vector<vk::Framebuffer> createFramebuffers();

  vk::ShaderModule createShaderModule(const std::vector<char>& code);
  std::vector<vk::ImageView> createImageView(std::vector<vk::Image>,
                                             vk::Format,
                                             vk::ImageAspectFlags,
                                             uint32_t mipLevels);
  vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates,
                                 vk::ImageTiling tiling,
                                 vk::FormatFeatureFlags features);

  // TODO: change this to std::byte at some point
  static std::vector<char> readFile(const std::string_view& filename)
  {
    std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
      throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
  }
};
}
#endif
