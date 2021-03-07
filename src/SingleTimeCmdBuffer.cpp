#include "SingleTimeCmdBuffer.hpp"

namespace vcc {

SingleTimeCmdBuffer::SingleTimeCmdBuffer(
    const vk::Device* const dev, const vk::Queue* const graphics, const vk::CommandPool* const pool):
    device(dev), 
    graphicsQueue(graphics), 
    cmdPool(pool),
    cmd(dev->allocateCommandBuffers(
        vk::CommandBufferAllocateInfo(
            *pool,
            vk::CommandBufferLevel::ePrimary,
            1
        )
    )[0])
    {}

SingleTimeCmdBuffer::~SingleTimeCmdBuffer(){
    graphicsQueue->waitIdle();
    device->freeCommandBuffers(*cmdPool, 1, &cmd);
}

void SingleTimeCmdBuffer::submit(){
    cmd.end();
    graphicsQueue->submit(vk::SubmitInfo({},{},{},1, &cmd));
}
}