#ifndef RENDERGROUP_H_INCLUDE
#define RENDERGROUP_H_INCLUDE

#include <array>
#include <functional>
#include <iterator>
#include <queue>
#include <tuple>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "VCEngine.hpp"
#include "workers/Renderer.hpp"
#include "Setup.hpp"

namespace vcc {
template<int N, int W>
class RenderGroup
{ 
public:
  // TODO: change this to std::range when c++20 support is better
  using Frame = // this is the start and end pointers. Sort of a hackneyed
                // std::range
    std::pair<std::function<void(vk::CommandBuffer, vk::Framebuffer)>*,
              std::function<void(vk::CommandBuffer, vk::Framebuffer)>*>;
  template<typename It>
  RenderGroup(const vcc::VCEngine,
              const vcc::Setup&, 
              It swapchain,
              vk::Extent2D,
              int layers);
  void render(Frame job);

private:
  std::array<vcc::Renderer<N>, W> workers;
  std::vector<std::tuple<vk::Semaphore, vk::Framebuffer>> images;
  vk::Device device;
};

}
#endif