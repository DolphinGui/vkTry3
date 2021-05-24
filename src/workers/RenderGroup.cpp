#include <algorithm>
#include <array>
#include <mutex>
#include <vulkan/vulkan.hpp>

#include "workers/RenderGroup.hpp"
#include "workers/Renderer.hpp"

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
  , jobquery()
{
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
                       device.createSemaphore(vk::SemaphoreCreateInfo()),
                       device.createFramebuffer(info) });
  }
  workers.fill(Worker(device,
                      engine.graphicsQueue,
                      engine.queueIndices.graphicsFamily.value(),
                      jobquery,
                      4,
                      images.begin()));
}

template<int N, int W>
void
RenderGroup<N, W>::render(std::initializer_list<RenderJob> jobs)
{
  jobquery.enqueue_bulk(jobs.begin(), jobs.size());
}
template<int N, int W>
void
RenderGroup<N, W>::Worker::doStuff()
{
  std::lock_guard lock(living);
  std::array<RenderJob, maxJobGrab> jobs;
  while (alive) {
    Frame& frame = frames[frameNumber];
    int result = jobquery.wait_dequeue_bulk(jobs.begin(), maxJobGrab);
    allocBuffers(result, frame);
    auto buffer = frame.buffers.begin();
    for (auto job = jobs.begin(); job != jobs.begin() + result; job++) {
      job.execute(*buffer++);
    }

    graphics.submit(vk::SubmitInfo(
      1,
      &frame.start,
      vk::PipelineStageFlagBits::eColorAttachmentOutput, // This might change
      result,
      frame.buffers.data(),
      1,
      &frame.complete));
  }
}

}