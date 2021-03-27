#ifndef DOER_H_INCLUDE
#define DOER_H_IN
#include <bits/stdint-uintn.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "VCEngine.hpp"
#include "jobs/PresentJob.hpp"
#include "jobs/RecordJob.hpp"
#include "vkobjects/CmdBuffer.hpp"

namespace vcc {
/* T is the amount of frames in flight,
  U is the amount of command buffers
  V is the amount of semaphores allowed
*/
template<int T, int U, int V>
class Doer
{

public:
  Doer(vk::Queue& graphics,
       vk::Device& dev,
       uint32_t graphicsIndex,
       uint32_t poolCount);
  ~Doer();
  void start();

private:
  template<int A, int B>
  struct Frame
  {
    vk::CommandPool pool;
    std::vector<vcc::CmdBuffer> buffers;
    std::queue<vk::Semaphore> semaphores;
    Frame(vk::CommandPool pool,
          std::array<vcc::CmdBuffer, A> buffers,
          std::array<vk::Semaphore, B> semaphores)
    {
      this->pool = pool, this->buffers = buffers, this->semaphores = semaphores;
    }
  };
  std::array<Frame<U, V>, T> commands;
  vcc::PresentJob record(const RecordJob& job, Frame<U, V>& frame);
  void present(const std::vector<PresentJob>& jobs,
               const Frame<U, V>& frame,
               const vk::Semaphore& semaphore);
  vk::Device* dev;
  vk::Queue* graphics;
  std::thread thread;
  std::queue<RecordJob> records;
  bool alive;
  std::condition_variable deathtoll;
};
}
#endif