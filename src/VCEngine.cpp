#include <string_view>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "VulkanMemoryAllocator/src/VmaUsage.h"

#include <bits/stdint-uintn.h>
#include <map>
#include <set>
#include <vector>
#include <vulkan/vulkan.hpp>

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
VCEngine::run(Setup* s){}; // TODO: add stuff to this, maybe

namespace {

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* pUserData)
{
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

vk::DebugUtilsMessengerEXT
createDebugMessenger(vk::Instance instance)
{
  VkDebugUtilsMessengerEXT debugMessenger;
  if (enableValidationLayers) {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    auto funct = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    if (funct) {
      funct(instance, &createInfo, nullptr, &debugMessenger);
    } else {
      throw std::runtime_error("failed to set up debug messenger!");
    }
    return debugMessenger;
  }
}
vk::SurfaceKHR
createSurfaceGLFW(vk::Instance instance, GLFWwindow* window)
{
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(
        VkInstance(instance), window, nullptr, &surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
  return vk::SurfaceKHR(surface);
}
VmaAllocator
vmaInfo(uint32_t vkVersion,
        vk::PhysicalDevice physicalDevice,
        vk::Device device,
        vk::Instance instance)
{
  VmaAllocatorCreateInfo allocInfo{};
  allocInfo.vulkanApiVersion = vkVersion;
  allocInfo.physicalDevice = physicalDevice;
  allocInfo.device = device;
  allocInfo.instance = instance;
  VmaAllocator result;
  vmaCreateAllocator(&allocInfo, &result);
  return result;
}
}

VCEngine::VCEngine(int width, int height, const char* name, GLFWwindow* w)
  : WIDTH(width)
  , HEIGHT(height)
  , APPNAME(name)
  , instance(initInstance())
  , window(initGLFW(w))
  , surface(createSurfaceGLFW(instance, window))
  , physicalDevice(pickPhysicalDevice())
  , physProps(physicalDevice.getMemoryProperties())
  , dload(vk::DispatchLoaderDynamic(
      instance,
      vk::DynamicLoader().getProcAddress<PFN_vkGetInstanceProcAddr>(
        "vkGetInstanceProcAddr")))
  , device(createLogicalDevice())
  , debugMessenger(createDebugMessenger(instance))
  , vmaAlloc(vmaInfo(vkVersion, physicalDevice, device, instance))
{}

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

#ifndef NDEBUG
namespace {
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
}
#endif

GLFWwindow*
VCEngine::initGLFW(GLFWwindow* w)
{
  glfwSetWindowUserPointer(w, this);
  glfwSetFramebufferSizeCallback(w, framebufferResizeCallback);
  return w;
}

vk::Device
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
  vk::Device d;
  vk::Result result = physicalDevice.createDevice(&createInfo, nullptr, &d);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error("failed to create device");
  }
  return d;
}
/*change this later*/
QueueFamilyIndices
VCEngine::findQueueFamilies(vk::PhysicalDevice device) const
{
  QueueFamilyIndices indices;

  std::vector<vk::QueueFamilyProperties> queueFamilies =
    device.getQueueFamilyProperties();

  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
      indices.graphicsFamily = i;
    }

    if (device.getSurfaceSupportKHR(i, surface)) {
      indices.presentFamily = i;
    }

    if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) {
      indices.transferFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }

    i++;
  }

#ifndef NDEBUG
  std::cout << "VCEngine::findQueueFamilies:  " << indices.info() << std::endl;
#endif
  return indices;
}

vk::PhysicalDevice
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
  vk::PhysicalDevice result;
  if (candidates.rbegin()->first > 0) {
    result = candidates.rbegin()->second;
  } else {
    throw std::runtime_error("failed to find a suitable GPU!");
  }

  return result;
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

vk::Instance
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
  appInfo.apiVersion = vkVersion;
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
    debugCreateInfo.messageSeverity =
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debugCreateInfo.messageType =
      vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    debugCreateInfo.pfnUserCallback = debugCallback;
    createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;

    createInfo.pNext = nullptr;
  }
  vk::Instance result;
  if (vk::createInstance(&createInfo, nullptr, &result) != vk::Result::eSuccess)
    throw std::runtime_error("failed to create image!");
  return result;
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

  for (const std::string_view& layerName : validationLayers) {
    bool layerFound = false;

    for (const vk::LayerProperties& layerProperties : availableLayers) {
      if (strcmp(layerName.data(), layerProperties.layerName) == 0) {
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
