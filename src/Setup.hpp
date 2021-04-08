#ifndef SETUP_H_INCLUDE
#define SETUP_H_INCLUDE
#include <bits/stdint-uintn.h>
#include <string_view>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <fstream>
#include <mutex>

#include "vkobjects/ImageBundle.hpp"
#include "Renderer.hpp"

using namespace vcc;
namespace vcc{
class Setup{
public:
  VCEngine* env;

  Setup(VCEngine* engine);
  ~Setup();

  

private:
friend class Renderer<3>;

  vk::SwapchainKHR swapChain;
  std::vector<vk::Image> swapChainImages;
  vk::Format swapChainImageFormat;
  vk::Extent2D swapChainExtent;
  std::vector<vk::ImageView> swapChainImageViews;
  std::vector<vk::Framebuffer> swapChainFramebuffers;

  vk::RenderPass renderPass;
  vk::DescriptorSetLayout descriptorSetLayout;
  vk::PipelineLayout pipelineLayout;
  vk::Pipeline graphicsPipeline;

  ImageBundle color;
  ImageBundle depth;

  vk::Queue graphicsQueue;
  std::mutex graphicsLock;
  vk::Queue presentQueue;
  vk::Queue transferQueue;

  void createSwapChain();
  void createRenderPass();
  void createDescriptorSetLayout();
  void createGraphicsPipeline();
  void createDepthResources();
  void createFramebuffers();
  
  vk::ShaderModule createShaderModule(const std::vector<char>& code);
  vk::ImageView createImageView(vk::Image image,
    vk::Format format, vk::ImageAspectFlags aspectFlags,
    uint32_t mipLevels);
  vk::Format findSupportedFormat(
    const std::vector<vk::Format>& candidates,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features);
    
  //TODO: change this to std::byte at some point
  static std::vector<char> readFile(const std::string_view& filename) {
      std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);

      if (!file.is_open()) {
          throw std::runtime_error("failed to open file!");
      }

      size_t fileSize = (size_t) file.tellg();
      std::vector<char> buffer(fileSize);

      file.seekg(0);
      file.read(buffer.data(), fileSize);

      file.close();

      return buffer;
  }

};
}
#endif
