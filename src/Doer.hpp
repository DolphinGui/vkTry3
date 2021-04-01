#ifndef DOER_H_INCLUDE
#define DOER_H_INCLUDE
#include "jobs/RecordJob.hpp"
#include "jobs/SubmitJob.hpp"
#include <vulkan/vulkan.hpp>
namespace vcc {
class Doer
{
public:
  Doer(vk::Queue& graphics, vk::Device& dev, uint32_t graphicsIndex)
    : dev(&dev)
    , graphics(&graphics){};
  virtual ~Doer();
  virtual void start() = 0;
  virtual void record(vcc::RecordJob record) = 0;
private:
  vk::Device* dev;
  vk::Queue* graphics;
};
}

#endif