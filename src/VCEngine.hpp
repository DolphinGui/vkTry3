#ifndef VCENNGINE_H_INCLUDE
#define VCENNGINE_H_INCLUDE
#include <GLFW/glfw3.h>
#include <string>
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
  std::string info()
  {
    if (!isComplete())
      return "Incomplete";
    return "Graphics: " + std::to_string(graphicsFamily.value()) + "\n" +
           "Present: " + std::to_string(presentFamily.value()) + "\n" +
           "Transfer: " + std::to_string(transferFamily.value()) + "\n";
  }
};
class Setup;
class VCEngine
{
public:
  const uint32_t WIDTH;
  const uint32_t HEIGHT;
  const char* APPNAME;

  void run(Setup* setup);
  VCEngine(int width, int height, const char* name);
  ~VCEngine();
  vk::SampleCountFlagBits getMSAAsamples() { return msaaSamples; }

  const vk::Device* const getDevPtr() { return &device; }

  const vk::PhysicalDevice* const getPhyDevPtr() { return &physicalDevice; }

  const vk::Queue* const getGpxPtr() { return &graphicsQueue; }

  const vk::SurfaceKHR* const getSurfacePtr() { return &surface; }

  const vk::DispatchLoaderDynamic* const getDload() { return &dload; }

  vk::PhysicalDeviceMemoryProperties getMemProps() { return physProps; }

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
  const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };

  const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  vk::Device device;
  vk::PhysicalDevice physicalDevice;
  vk::PhysicalDeviceMemoryProperties physProps;
  vk::Queue graphicsQueue;

  GLFWwindow* window;
  bool framebufferResized = false;

  vk::Instance instance;
  vk::DebugUtilsMessengerEXT debugMessenger;
  vk::SurfaceKHR surface;
  vk::DispatchLoaderDynamic dload;

  vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e2;

  vk::Queue presentQueue;

  bool checkValidationLayerSupport();
  void initInstance();
  vk::Result CreateDebugUtilsMessengerEXT(
    vk::Instance instance,
    const vk::DebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const vk::AllocationCallbacks* pAllocator,
    vk::DebugUtilsMessengerEXT* pDebugMessenger);
  void pickPhysicalDevice();
  int deviceSuitability(vk::PhysicalDevice device);
  void createLogicalDevice();
  void initGLFW();

  void populateDebugMessengerCreateInfo(
    vk::DebugUtilsMessengerCreateInfoEXT& createInfo);

  static void framebufferResizeCallback(GLFWwindow* window,
                                        int width,
                                        int height)
  {
    auto app = reinterpret_cast<VCEngine*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
  }

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData)
  {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
  }
  std::vector<const char*> getRequiredExtensions()
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
