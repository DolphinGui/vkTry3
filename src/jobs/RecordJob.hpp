#ifndef RECORDJOB_H_INCLUDE
#define RECORDJOB_H_INCLUDE
#include "vkobjects/CmdBuffer.hpp"
#include <vulkan/vulkan.hpp>

namespace vcc {
class RecordJob
{
public:
  RecordJob* dependency;
  const vk::CommandBufferBeginInfo info;

  RecordJob(const void (*funct)(vk::CommandBuffer&),
            RecordJob* dep,
            const vk::CommandBufferBeginInfo info,
            vk::Fence* isDone = nullptr)
    : dependency(dep)
    , exec(funct)
    , info(info)
    , isDone(isDone)
  {}
  void record(vcc::CmdBuffer& b) const
  {
    b.state = bufferStates::kRecording;//can probably be removed
    b.cmd.begin(info);
    exec(b.cmd);
    b.cmd.end();
    b.state = bufferStates::kExecutable;
  }

private:
  const void (*exec)(vk::CommandBuffer&);
  vk::Fence* isDone;
};
}
#endif