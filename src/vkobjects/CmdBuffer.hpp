#ifndef CMDBUFFERINFO_H_INCLUDE
#define CMDBUFFERINFO_H_INCLUDE
#include <vulkan/vulkan.hpp>

namespace vcc {

struct CmdBuffer
{
  enum struct states
  {
    kInitial,
    kRecording,
    kExecutable,
    kPending,
    kInvalid
  };

  vk::CommandBuffer cmd;
  states state;
  operator vk::CommandBuffer() { return cmd; }
  CmdBuffer(vk::CommandBuffer commands):
  cmd(commands),
  state(states::kInitial){}
};
}


#endif