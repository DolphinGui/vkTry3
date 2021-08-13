#pragma once
#include <atomic>
#include <bits/stdint-uintn.h>
#include <cstddef>
#include <future>
#include <stb/stb_image.h>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "data/UniformBufferObject.hpp"
#include "data/Vertex.hpp"
#include "vkobjects/BufferBundle.hpp"
#include "vkobjects/ImageBundle.hpp"
#include "workers/Mover.hpp"
#include "workers/RenderGroup.hpp"

namespace vcc {
class Setup;

class Task
{
public:
  Task(Setup& s, VCEngine& e);
  ~Task();
  void run(const stbi_uc* const image,
           vk::Extent2D imageSize,
           size_t bSize,
           const std::vector<Vertex>& verticies,
           const std::vector<uint32_t>& indicies);

private:
  VCEngine& engine;
  Setup& setup;

  RenderGroup render;
  Mover mover;

  std::atomic_flag resized;

  /* technically the idiomatic way to do this is something with std::future, but
  I couldn't figure out how to do it efficiently.*/
  std::future<BufferBundle> loadBuffer(const void* const data, size_t, BufferBundle&& out);
    std::future<ImageBundle> loadImage(const void* const data,
                 size_t,
                 vk::Extent2D,
                 uint32_t mipLevels,
                 vk::Format,
                 ImageBundle&& out);
};
}
