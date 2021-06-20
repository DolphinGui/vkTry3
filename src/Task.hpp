#ifndef TASK_H_INCLUDE
#define TASK_H_INCLUDE
#include <atomic>
#include <bits/stdint-uintn.h>
#include <cstddef>
#include <stb/stb_image.h>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "data/UniformBufferObject.hpp"
#include "data/Vertex.hpp"
#include "vkobjects/BufferBundle.hpp"
#include "vkobjects/ImageBundle.hpp"
#include "workers/RenderGroup.hpp"
#include "workers/Mover.hpp"

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

  RenderGroup<3, 1> render;
  Mover<2> mover;

  std::atomic_flag resized;

  vk::Fence loadBuffer(const void* const data,
                       size_t,
                       vk::Device fence,
                       vk::Buffer& out);
  vk::Fence loadImage(const void* const data,
                      size_t,
                      vk::Extent2D,
                      uint32_t mipLevels,
                      vk::Format,
                      vk::Device fence,
                      vk::Image& out);
};
}
#endif