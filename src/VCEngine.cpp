#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <optional>
#include <set>
#include <map>
#include <memory>

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

#include "Vertex.hpp"

#include "UniformBufferObject.hpp"

#include "VCEngine.hpp"

typedef std::vector<Vertex> Mesh;

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

using namespace vcc;

void VCEngine::run(){};

VCEngine::VCEngine(
  int width,
  int height,
  const char* name):
  WIDTH(width),
  HEIGHT(height),
  APPNAME(name)
  {
  initGLFW();
  initInstance();

  if (enableValidationLayers){
    vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
  }
  auto surf = VkSurfaceKHR(surface);
  if (glfwCreateWindowSurface(VkInstance(instance), window, nullptr, &surf) != VK_SUCCESS) {
      throw std::runtime_error("failed to create window surface!");
  }
  pickPhysicalDevice();
  createLogicalDevice();
}

VCEngine::~VCEngine(){
  vkDestroyDevice(device, nullptr);

  if (enableValidationLayers) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
      func(instance, debugMessenger, nullptr);
    }
  }

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

  glfwDestroyWindow(window);

  glfwTerminate();
}

void VCEngine::initGLFW(){
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  window = glfwCreateWindow(WIDTH, HEIGHT, APPNAME, nullptr, nullptr);
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void VCEngine::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    vk::DeviceCreateInfo createInfo{};
    createInfo.sType =     createInfo.sType = vk::StructureType::eDeviceCreateInfo;
;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    physicalDevice.createDevice(&createInfo, nullptr, &device);

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

QueueFamilyIndices VCEngine::findQueueFamilies(vk::PhysicalDevice device){
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
          indices.graphicsFamily = i;
      }

      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

      if (presentSupport) {
          indices.presentFamily = i;
      }

      if (indices.isComplete()) {
          break;
      }

      i++;
  }

  return indices;
}

void VCEngine::pickPhysicalDevice() {
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

int VCEngine::deviceSuitability(vk::PhysicalDevice device) {
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


void VCEngine::initInstance(){
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
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();

      populateDebugMessengerCreateInfo(debugCreateInfo);
      createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
  } else {
      createInfo.enabledLayerCount = 0;

      createInfo.pNext = nullptr;
  }
  vk::createInstance(&createInfo, nullptr, &instance);
}

void VCEngine::populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo.sType = vk::StructureType::eDebugUtilsMessengerCreateInfoEXT;
    createInfo.messageSeverity =  vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose 
    | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
    | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | 
    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    createInfo.pfnUserCallback = debugCallback;
}

vk::Result VCEngine::CreateDebugUtilsMessengerEXT(vk::Instance instance, 
const vk::DebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
const vk::AllocationCallbacks* pAllocator, 
vk::DebugUtilsMessengerEXT* pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    auto create = VkDebugUtilsMessengerCreateInfoEXT(pCreateInfo);
    return func(instance, create*, pAllocator, pDebugMessenger);
  } else {
    return vk::Result::eErrorExtensionNotPresent;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

bool VCEngine::checkValidationLayerSupport() {
    uint32_t layerCount;
    vk::enumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<vk::LayerProperties> availableLayers(layerCount);
    vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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
