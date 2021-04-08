#ifndef VCENNGINE_H_INCLUDE
#define VCENNGINE_H_INCLUDE
#include "VulkanMemoryAllocator/src/VmaUsage.h"
#include <GLFW/glfw3.h>
#include <array>
#include <string>
#include <string_view>
#include <vulkan/vulkan.hpp>

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
  std::string info()
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
  VCEngine(int width, int height, const char* name, GLFWwindow*);
  ~VCEngine();
  friend class Setup;

  const uint32_t WIDTH;
  const uint32_t HEIGHT;
  const char* APPNAME;

  const uint32_t vkVersion = VK_API_VERSION_1_2;
  const vk::Instance instance;
  GLFWwindow* window;
  const vk::SurfaceKHR surface;
  const vk::PhysicalDevice physicalDevice;
  const vk::Device device;
  const vk::PhysicalDeviceMemoryProperties physProps;
  const vk::Queue graphicsQueue;
  const vk::Queue presentQueue;

  bool framebufferResized = false;

  const vk::DebugUtilsMessengerEXT debugMessenger;

  const vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e2;
  const vk::DispatchLoaderDynamic dload;

  const VmaAllocator vmaAlloc;

  void run(Setup* setup);

  const vk::SampleCountFlagBits getMSAAsamples() { return msaaSamples; }

  const vk::Device* const getDevPtr() { return &device; }

  const vk::PhysicalDevice* const getPhyDevPtr() { return &physicalDevice; }

  const vk::SurfaceKHR* const getSurfacePtr() { return &surface; }

  const vk::PhysicalDeviceMemoryProperties getMemProps() { return physProps; }

  vk::Extent2D framebufferSize()
  {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return vk::Extent2D(static_cast<uint32_t>(width),
                        static_cast<uint32_t>(height));
  }

  void memProps(vk::PhysicalDeviceMemoryProperties* out);

  QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device) const;

private:
  static constexpr std::array<const char*, 1> validationLayers{
    "VK_LAYER_KHRONOS_validation"
  };

  static constexpr std::array<const char*, 1> deviceExtensions = {
    "VK_KHR_swapchain"
  };

  bool checkValidationLayerSupport();
  vk::Instance initInstance();
  vk::PhysicalDevice pickPhysicalDevice();
  int deviceSuitability(vk::PhysicalDevice device);
  vk::Device createLogicalDevice();
  GLFWwindow* initGLFW(GLFWwindow* w);

  static void framebufferResizeCallback(GLFWwindow* window,
                                        int width,
                                        int height)
  {
    auto app = reinterpret_cast<VCEngine*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
  }

  static std::vector<const char*> getRequiredExtensions()
  {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions,
                                        glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
  }
};
}
#endif
