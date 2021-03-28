#ifndef PRESENTJOB_H_INCLUDE
#define PRESENTJOB_H_INCLUDE

#include <memory>
#include <vulkan/vulkan.hpp>

#include "vkobjects/CmdBuffer.hpp"

namespace vcc {
struct SubmitJob
{
public:
  
  SubmitJob* dependent;//change this to support multiple dependencies later
  vcc::CmdBuffer* commands;
  SubmitJob(vcc::CmdBuffer& buffer)
    : dependent(nullptr)
    , commands(&buffer)
  {}
  SubmitJob(vcc::CmdBuffer& buffer, SubmitJob& depend)
    : dependent(&depend)
    , commands(&buffer)
  {}
  SubmitJob(){}
};
}
#endif