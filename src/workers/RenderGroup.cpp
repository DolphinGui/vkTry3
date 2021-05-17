#include "workers/RenderGroup.hpp"
#include "workers/Renderer.hpp"
#include <algorithm>

namespace vcc {

/* It must be the iterator ImageView of the swapchain vector.
    There must be at least N ImageViews in it */
template<int N, int W>
template<typename It>
RenderGroup<N, W>::RenderGroup(const vcc::VCEngine engine,
                               const vcc::Setup& setup,
                               It swapChain,
                               vk::Extent2D size,
                               int layers)
  : device(device)
  , images()
{
  std::fill(workers.start(),
            workers.end(),
            Renderer<N>(engine.graphicsQueue,
                        engine.device,
                        engine.queueIndices.graphicsFamily));
  images.reserve(N);
  for (int n = N; n--;) {
    std::array<vk::ImageView, 3> attatchments = { setup.color,
                                                  setup.depth,
                                                  *swapChain++ };
    vk::FramebufferCreateInfo info({},
                                   setup.renderPass,
                                   attatchments.size(),
                                   attatchments.data(),
                                   size.width,
                                   size.height,
                                   layers);
    images.push_back({ device.createSemaphore(vk::SemaphoreCreateInfo()),
                       device.createFramebuffer(info) });
  }
}
template<int N, int W>
void
RenderGroup<N, W>::render(Frame jobs)
{
  int workerIndex = 0;
  for (auto job = jobs.first; job != jobs.second; job++) {
    if(workers[workerIndex].done()) workers[workerIndex].submit(*job);
  }
}

}