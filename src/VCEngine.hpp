#pragma once
#include <GLFW/glfw3.h>
#include <array>
#include <string>
#include <string_view>
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace vcc {
struct QueueFamilyIndices
{ // should remove this later.
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  std::optional<uint32_t> transferFamily;
  bool isComplete()
  {
    return graphicsFamily.has_value() && presentFamily.has_value() &&
           transferFamily.has_value();
  }
#ifndef NDEBUG
  std::string const info() const
  {
    std::string results("Family Indicies: ");
    std::array<std::string, 3> familyNames(
      { "graphics", "present", "transfer" });
    auto inc = familyNames.begin();
    for (std::optional<uint32_t> o :
         { graphicsFamily, presentFamily, transferFamily }) {
      if (o.has_value())
        results += *inc++ + ": " + std::to_string(o.value()) + ", ";
      else
        inc++;
    }
    return results;
  }
#endif
};

class Setup;
class VCEngine
{
public:
  VCEngine(int width, int height, const std::string_view name, GLFWwindow*);
  ~VCEngine();
  friend class Setup;

  const uint32_t WIDTH;
  const uint32_t HEIGHT;
  const std::string_view APPNAME;
  constexpr static uint32_t vkVersion = VK_API_VERSION_1_2;

  GLFWwindow* window;
  const vk::Instance instance;

  const vk::SurfaceKHR surface;
  const vk::PhysicalDevice physicalDevice;
  const vk::PhysicalDeviceMemoryProperties physProps;
  const vk::DispatchLoaderDynamic dload;
  const QueueFamilyIndices queueIndices;
  const vk::Device device;
  const vk::DebugUtilsMessengerEXT debugMessenger;
  const VmaAllocator vmaAlloc;
  const vk::Queue graphicsQueue;
  const vk::Queue presentQueue;

  bool isFramebufferResized() { return framebufferResized; }

  

  const vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e2;

  

  vk::Extent2D framebufferSize() const
  {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return vk::Extent2D(static_cast<uint32_t>(width),
                        static_cast<uint32_t>(height));
  }

  void memProps(vk::PhysicalDeviceMemoryProperties* out);

private:
  static constexpr std::array<const char*, 1> validationLayers{
    "VK_LAYER_KHRONOS_validation"
  };

  static constexpr std::array<const char*, 1> deviceExtensions = {
    "VK_KHR_swapchain"
  };
  bool framebufferResized = false;

  static void framebufferResizeCallback(GLFWwindow* window,
                                        int width,
                                        int height)
  {
    auto app = reinterpret_cast<VCEngine*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
  }
};
}