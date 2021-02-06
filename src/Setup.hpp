#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <fstream>

#ifndef SETUP_H_INCLUDE
#define SETUP_H_INCLUDE

#include "VCEngine.hpp"
#include "Image.hpp"

namespace vcc{
class Setup{
public:
  Setup(VCEngine* engine);
  ~Setup();
private:
  vcc::VCEngine* env;

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

  vk::CommandPool commandPool;

  vcc::Image color;

  void createSwapChain();
  void createRenderPass();
  void createDescriptorSetLayout();
  void createGraphicsPipeline();
  void createCommandPool();
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
  static std::vector<char> readFile(const std::string& filename) {
      std::ifstream file(filename, std::ios::ate | std::ios::binary);

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
