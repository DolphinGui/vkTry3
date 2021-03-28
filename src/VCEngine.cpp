#include <bits/stdint-uintn.h>
#include <string>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

#include "Setup.hpp"
#include "VCEngine.hpp"

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

using namespace vcc;
void
VCEngine::run(Setup* s){};

VCEngine::VCEngine(int width, int height, const char* name)
  : WIDTH(width)
  , HEIGHT(height)
  , APPNAME(name)
{
  initGLFW();
  initInstance();
  dload = vk::DispatchLoaderDynamic(
    instance,
    vk::DynamicLoader().getProcAddress<PFN_vkGetInstanceProcAddr>(
      "vkGetInstanceProcAddr"));
  if (enableValidationLayers) {
    vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(
          instance, &createInfo, nullptr, &debugMessenger) !=
        vk::Result::eSuccess) {
      throw std::runtime_error("failed to set up debug messenger!");
    }
  }
  VkSurfaceKHR surf;
  if (glfwCreateWindowSurface(VkInstance(instance), window, nullptr, &surf) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }

  surface = vk::SurfaceKHR(surf);

  pickPhysicalDevice();
  createLogicalDevice();

  physProps = physicalDevice.getMemoryProperties();

  dload = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr, device);
}

VCEngine::~VCEngine()
{
  vkDestroyDevice(device, nullptr);

  if (enableValidationLayers) {

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)instance.getProcAddr(
      "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
      func(instance, debugMessenger, nullptr);
    }
  }

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

  glfwDestroyWindow(window);

  glfwTerminate();
}

void
VCEngine::initGLFW()
{
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  window = glfwCreateWindow(WIDTH, HEIGHT, APPNAME, nullptr, nullptr);
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void
VCEngine::createLogicalDevice()
{
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(),
                                             indices.presentFamily.value() };

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo queueCreateInfo(
      {}, queueFamily, 2, &queuePriority);
    queueCreateInfos.push_back(queueCreateInfo);
  }

  vk::PhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  vk::DeviceCreateInfo createInfo{};
  createInfo.sType = createInfo.sType = vk::StructureType::eDeviceCreateInfo;

  createInfo.queueCreateInfoCount =
    static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount =
    static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
      static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }
  if (physicalDevice.createDevice(&createInfo, nullptr, &device) !=
      vk::Result::eSuccess) {
    throw std::runtime_error("failed to create device");
  }

  device.getQueue(indices.graphicsFamily.value(), 0, &graphicsQueue);
  device.getQueue(indices.presentFamily.value(), 0, &presentQueue);
}

#ifndef NDEBUG
std::string
getFamilyFlags(const vk::QueueFamilyProperties& props)
{
  std::string results("Flags: ");
  if (props.queueFlags & vk::QueueFlagBits::eCompute) {
    results += "eCompute, ";
  }
  if (props.queueFlags & vk::QueueFlagBits::eGraphics) {
    results += "eGraphics, ";
  }
  if (props.queueFlags & vk::QueueFlagBits::eTransfer) {
    results += "eTransfer, ";
  }
  results += std::to_string(props.queueCount);
  return results;
}
#endif

QueueFamilyIndices
VCEngine::findQueueFamilies(vk::PhysicalDevice device) const
{
  QueueFamilyIndices indices;

  std::vector<vk::QueueFamilyProperties> queueFamilies(
    device.getQueueFamilyProperties());
  int i = 0;
  for (const auto& queueFamily : queueFamilies) {

    if (!(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) &&
        !(queueFamily.queueFlags & vk::QueueFlagBits::eCompute)) {
      indices.transferFamily = i;
      continue;
    }

    VkBool32 presentSupport = false;
    if (device.getSurfaceSupportKHR(i, surface, &presentSupport) !=
        vk::Result::eSuccess)
      throw std::runtime_error("failed to get surface support details");
    if (presentSupport) {
      indices.presentFamily = i;
    }

    if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
      indices.graphicsFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }

    i++;
  }
#ifndef NDEBUG
  std::cout << "VCEngine::findQueueFamilies:  "<< indices.info() << std::endl;
#endif
  return indices;
}

void
VCEngine::pickPhysicalDevice()
{
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
  std::multimap<int, VkPhysicalDevice> candidates;

  for (const auto& device : devices) {
    int score = deviceSuitability(device);
    candidates.insert(std::make_pair(score, device));
  }

  if (candidates.rbegin()->first > 0) {
    physicalDevice = candidates.rbegin()->second;
  } else {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

int
VCEngine::deviceSuitability(vk::PhysicalDevice device)
{
  vk::PhysicalDeviceProperties deviceProperties;
  vk::PhysicalDeviceFeatures deviceFeatures;
  device.getProperties(&deviceProperties);
  device.getFeatures(&deviceFeatures);

  int score = 0;

  if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
    score += 1000;
  }

  score += deviceProperties.limits.maxImageDimension2D;

  if (!deviceFeatures.geometryShader) {
    return 0;
  }

  return score;
}

void
VCEngine::initInstance()
{
  if (enableValidationLayers && !checkValidationLayerSupport()) {
    throw std::runtime_error("validation layers requested, but not available!");
  }

  vk::ApplicationInfo appInfo{};
  appInfo.sType = vk::StructureType::eApplicationInfo;
  appInfo.pApplicationName = APPNAME;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  vk::InstanceCreateInfo createInfo{};
  createInfo.sType = vk::StructureType::eInstanceCreateInfo;
  createInfo.pApplicationInfo = &appInfo;

  auto extensions = getRequiredExtensions();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
      static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;

    createInfo.pNext = nullptr;
  }
  if (vk::createInstance(&createInfo, nullptr, &instance) !=
      vk::Result::eSuccess)
    throw std::runtime_error("failed to create image!");
}

void
VCEngine::populateDebugMessengerCreateInfo(
  vk::DebugUtilsMessengerCreateInfoEXT& createInfo)
{
  createInfo.sType = vk::StructureType::eDebugUtilsMessengerCreateInfoEXT;
  createInfo.messageSeverity =
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
  createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
  createInfo.pfnUserCallback = debugCallback;
}

vk::Result
VCEngine::CreateDebugUtilsMessengerEXT(
  vk::Instance instance,
  const vk::DebugUtilsMessengerCreateInfoEXT* pCreateInfo,
  const vk::AllocationCallbacks* pAllocator,
  vk::DebugUtilsMessengerEXT* pDebugMessenger)
{

  return instance.createDebugUtilsMessengerEXT(
    pCreateInfo, pAllocator, pDebugMessenger, dload);
}

void
DestroyDebugUtilsMessengerEXT(VkInstance instance,
                              VkDebugUtilsMessengerEXT debugMessenger,
                              const VkAllocationCallbacks* pAllocator)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

bool
VCEngine::checkValidationLayerSupport()
{
  uint32_t layerCount;
  if (vk::enumerateInstanceLayerProperties(&layerCount, nullptr) !=
      vk::Result::eSuccess)
    throw std::runtime_error("failed to count layer properties");

  std::vector<vk::LayerProperties> availableLayers(layerCount);
  if (vk::enumerateInstanceLayerProperties(
        &layerCount, availableLayers.data()) != vk::Result::eSuccess)
    throw std::runtime_error("failed to count layer properties");

  for (const char* layerName : validationLayers) {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      return false;
    }
  }

  return true;
}
