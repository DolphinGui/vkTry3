#include <algorithm>
#include <array>
#include <mutex>
#include <vulkan/vulkan.hpp>

#include "workers/RenderGroup.hpp"

namespace vcc {

/* It must be the iterator ImageView of the swapchain vector.
    There must be at least N ImageViews in it */
template<int N, int W>
template<typename It>
RenderGroup<N, W>::RenderGroup(const vcc::VCEngine& engine,
                               const vk::ImageView& color,
                               const vk::ImageView& depth,
                               const vk::RenderPass& renderpass,
                               const vk::SwapchainKHR& swapchain,
                               It swapChainImageViews,
                               vk::Extent2D size,
                               int layers)
  : device(device)
  , graphics(engine.graphicsQueue)
  , swapchain(swapchain)
{
  images.reserve(N);
  for (int n = N; n--;) {
    std::array<vk::ImageView, 3> attatchments = { color,
                                                  depth,
                                                  *swapChainImageViews++ };
    vk::FramebufferCreateInfo info({},
                                   renderpass,
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
                      graphics,
                      engine.queueIndices.graphicsFamily.value(),
                      jobquery,
                      submitquery,
                      wakeup,
                      workload,
                      4,
                      images.begin()));
}

template<int N, int W>
void
RenderGroup<N, W>::render(std::initializer_list<RenderJob> jobs)
{
  jobquery.enqueue_bulk(jobs.begin(), jobs.size());
  workload += jobs.size();
  wakeup.notify_all();
  device.waitForFences(images[currentImage].finishFence, VK_TRUE, 1'000'000);

  device.acquireNextImageKHR(
    swapchain, 1'000'000, images[currentImage].begin, nullptr);
  // deal with resizing later

  // std::array<vk::SubmitInfo, 5> submitResults
}
template<int N, int W>
void
RenderGroup<N, W>::Worker::doStuff()
{
  std::lock_guard lock(living);
  std::array<RenderJob, maxJobGrab> jobs;
  while (alive) {
    Frame& frame = frames[frameNumber];
    int result = jobquery.try_dequeue_bulk(jobs.begin(), maxJobGrab);
    if(workload == 0){
      std::unique_lock<std::mutex> lock{};
      alarmclock.wait(lock);
      continue;
    }
    auto buffer = frame.buffers.begin();
    for (auto job = jobs.begin(); job != jobs.begin() + result; job++) {
      job.record(*buffer++);
    }
    submitquery.enqueue(vk::SubmitInfo(
      1,
      &frame.start,
      vk::PipelineStageFlagBits::eColorAttachmentOutput, // This might change,
                                                         // should probably
                                                         // have a compiletime
                                                         // thing to figure
                                                         // this out
      result,
      frame.buffers.data(),
      1,
      &frame.complete));
  }
}

}