#include <bits/stdint-uintn.h>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Doer.hpp"
#include "VCEngine.hpp"
#include "Setup.hpp"

namespace vcc {
Doer::Doer(vcc::Setup* s, vk::Device* d, uint32_t graphicsIndex, uint32_t poolCount, uint32_t b): dev(d), set(s){
    
    for(int i = 0; i < poolCount; i++){
        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
        poolInfo.queueFamilyIndex = graphicsIndex;
        vk::CommandPool p;
        if(d->createCommandPool(&poolInfo, nullptr, &p)!=vk::Result::eSuccess)
            throw std::runtime_error("failed to create command pool");
        commands.push_back(
            std::pair(
                p, 
                dev->allocateCommandBuffers(
                    vk::CommandBufferAllocateInfo(
                        p, 
                        vk::CommandBufferLevel::ePrimary,
                        b
                    )
                )
            )
        );
    }
}

Doer::~Doer(){
    for(auto p : commands){
        dev->freeCommandBuffers(p.first, p.second);
    }
    for(auto p : commands){
        dev->destroyCommandPool(p.first);
    }
}
void Doer::record(uint32_t p, uint32_t b, void (*funct)(vk::CommandBuffer c)){
    commands[p].second[b].begin(
        vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
    );
    funct(commands[p].second[b]);
    commands[p].second[b].end();
};
void Doer::submit(uint32_t p, vk::SubmitInfo info){
    set->graphicsQueue.submit(info);
}
void Doer::start(){
    
}
}
