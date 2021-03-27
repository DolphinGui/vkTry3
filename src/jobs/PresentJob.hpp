#ifndef PRESENTJOB_H_INCLUDE
#define PRESENTJOB_H_INCLUDE

#include <memory>
#include <vulkan/vulkan.hpp>

#include "vkobjects/CmdBuffer.hpp"

namespace vcc {
struct PresentJob
{
public:
  
  PresentJob* dependent;//change this to support multiple dependencies later
  vcc::CmdBuffer* commands;
  PresentJob(vcc::CmdBuffer& buffer)
    : dependent(nullptr)
    , commands(&buffer)
  {}
  PresentJob(vcc::CmdBuffer& buffer, PresentJob& depend)
    : dependent(&depend)
    , commands(&buffer)
  {}
  PresentJob(){}
};
}
#endif