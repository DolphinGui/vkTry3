
#include "EASTL/fixed_vector.h"
#include <EASTL/vector.h>
#include <algorithm>
#include <array>
#include <mutex>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "workers/RenderGroup.hpp"

namespace vcc {

void
RenderGroup::render(std::initializer_list<RenderJob> jobs)
{
  jobquery.enqueue_bulk(jobs.begin(), jobs.size());
  workload += jobs.size();
  wakeup.notify_all();
  device.waitForFences(images[currentImage].finishFence, VK_TRUE, 1'000'000);

  device.acquireNextImageKHR(
    swapchain, 1'000'000, images[currentImage].finished, nullptr);
  // deal with resizing later
  // figure out something smarter with this later
  eastl::fixed_vector<vk::SubmitInfo, 10> results;
  submitquery.wait_dequeue_bulk(results.begin(), 10);
  graphics.submit({ static_cast<uint32_t>(results.size()), results.data() });
}

RenderGroup::RenderGroup(const vcc::VCEngine& engine,
                         const vk::ImageView& color,
                         const vk::ImageView& depth,
                         const vk::RenderPass& renderpass,
                         const vk::SwapchainKHR& swapchain,
                         gsl::span<vk::ImageView> swapChainImageViews,
                         vk::Extent2D size,
                         int layers)
  : device(engine.device)
  , graphics(engine.graphicsQueue)
  , swapchain(swapchain)
  , images([=]() -> std::vector<PresentImage> {
    std::vector<PresentImage> images(F);
    for (int n = F; n--;) {
      vk::ImageView data[3] = { color, depth, swapChainImageViews[n] };
      vk::FramebufferCreateInfo info(
        {}, renderpass, 3, data, size.width, size.height, layers);
      images.push_back({ device.createSemaphore(vk::SemaphoreCreateInfo()),
                         device.createSemaphore(vk::SemaphoreCreateInfo()),
                         device.createFramebuffer(info) });
    }

    return images;
  }())
  // maybe have a better way to do this later.
  // probably with template metaprogramming jank
  , workers{ Worker(device,
                    graphics,
                    0,
                    engine.queueIndices.graphicsFamily.value(),
                    jobquery,
                    submitquery,
                    wakeup,
                    workload,
                    images.begin()),
             Worker(device,
                    graphics,
                    1,
                    engine.queueIndices.graphicsFamily.value(),
                    jobquery,
                    submitquery,
                    wakeup,
                    workload,
                    images.begin()) }
{}

RenderGroup::~RenderGroup(){
  for(auto& image : images){
    device.destroySemaphore(image.available);
    device.destroySemaphore(image.finished);
    device.destroyFramebuffer(image.itself);
    //destroy fences later
  }
}

void
RenderGroup::Worker::doStuff()
{
  std::lock_guard lock(living);
  eastl::fixed_vector<RenderJob, maxJobGrab> jobs;
  while (alive) {
    Frame& frame = frames[frameNumber];
    int result = jobquery.try_dequeue_bulk(jobs.begin(), maxJobGrab);
    if (workload == 0) {
      std::unique_lock<std::mutex> lock{};
      alarmclock.wait(lock);
      continue;
    }
    auto buffer = frame.buffers.begin();
    for (auto job = jobs.begin(); job != jobs.begin() + result; job++) {
      job->record(*buffer++);
    }
    const vk::PipelineStageFlags banana =
      vk::PipelineStageFlagBits::eColorAttachmentOutput; // This might change,
                                                         // should probably
                                                         // have a compiletime
                                                         // thing to figure
                                                         // this out
    submitquery.enqueue(vk::SubmitInfo(1,
                                       &frame.start,
                                       &banana,
                                       result,
                                       frame.buffers.data(),
                                       1,
                                       &frame.complete));
  }
}

}