#ifndef MOVER_H_INCLUDE
#define MOVER_H_INCLUDE
#include <bits/stdint-uintn.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "VCEngine.hpp"
#include "jobs/RecordJob.hpp"
#include "jobs/SubmitJob.hpp"
#include "vkobjects/CmdBuffer.hpp"

namespace vcc {
/* T is the amount of frames in flight
 */
template<int T>
class Mover
{

public:
  Mover(vk::Queue& graphics, vk::Device& dev, uint32_t graphicsIndex);
  ~Mover();
  void start();
  void submit(RecordJob record);

private:
  vk::Device* dev;
  vk::Queue* transferQueue;
  std::thread thread;
  vk::CommandPool pool;
  std::vector<vcc::CmdBuffer> buffers;
  std::queue<RecordJob> recordJobs;
  std::vector<SubmitJob> submitJobs;
  bool alive;
  std::condition_variable deathtoll;

  vcc::SubmitJob record(const RecordJob& job);
  void present();
};
}
#endif