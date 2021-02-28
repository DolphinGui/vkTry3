#include "SingleTimeCmdBuffer.hpp"

namespace vcc {

SingleTimeCmdBuffer::SingleTimeCmdBuffer(
    vk::Device* dev, vk::Queue* graphics, vk::CommandPool* pool):
    device(dev), graphicsQueue(graphics), cmdPool(pool){
    
    vk::CommandBufferAllocateInfo alloc(
        *pool,
        vk::CommandBufferLevel::ePrimary,
        1
    );
    cmd = device->allocateCommandBuffers(alloc)[0];
    
    cmd.begin(
        vk::CommandBufferBeginInfo(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        )
    );
}

SingleTimeCmdBuffer::~SingleTimeCmdBuffer(){
    graphicsQueue->waitIdle();
    device->freeCommandBuffers(*cmdPool, 1, &cmd);
}

void SingleTimeCmdBuffer::submit(){
    cmd.end();
    graphicsQueue->submit(vk::SubmitInfo({},{},{},1, &cmd));
}
}