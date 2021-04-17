#ifndef TASK_H_INCLUDE
#define TASK_H_INCLUDE
#include <atomic>
#include <cstddef>
#include <stb/stb_image.h>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "data/UniformBufferObject.hpp"
#include "data/Vertex.hpp"
#include "vkobjects/Buffer.hpp"
#include "vkobjects/ImageBundle.hpp"
#include "workers/Renderer.hpp"

namespace vcc {
class Setup;

class Task
{
public:
  Task(Setup* s,
       VCEngine* e);
  ~Task();
  void run(
       stbi_uc* image,
       vk::Extent2D imageSize,
       size_t bSize,
       std::vector<std::pair<Vertex, uint32_t>>* verts);

private:
  VCEngine* engine;
  Setup* setup;

  Renderer<3> render;
  Renderer<3> mover;

  std::atomic_flag resized;

  void loadContent(void* data, size_t, vk::Buffer in);
};
}
#endif