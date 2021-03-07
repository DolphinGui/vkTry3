#ifndef TASK_H_INCLUDE
#define TASK_H_INCLUDE
#include <vulkan/vulkan.hpp>
#include <vector>
#include <utility>
#include <stb/stb_image.h>

#include "vkobjects/Buffer.hpp"
#include "vkobjects/ImageBundle.hpp"
#include "data/UniformBufferObject.hpp"
#include "data/Vertex.hpp"

namespace vcc{
class Setup;

class Task{
public:
Task(
    Setup* s,
    VCEngine* e, 
    stbi_uc* image, 
    vk::Extent2D imageSize, 
    size_t bSize,
    std::vector<std::pair<Vertex, uint32_t>>* verts
    );
~Task();
void run();
private:
VCEngine* engine;
Setup* setup;

ImageBundle texture;
vk::Sampler textureSampler;

std::vector<std::pair<Vertex, uint32_t>>* vertices;
Buffer vertexB;
Buffer indexB;

std::vector<Buffer> uniformB;

vk::DescriptorPool descriptorPool;
std::vector<vk::DescriptorSet> descriptorSets;

std::vector<vk::Semaphore> imageAvailableS;
std::vector<vk::Semaphore> renderFinishedS;
std::vector<vk::Fence> inFlightF;
std::vector<vk::Fence> imagesInFlightF;
size_t currFrame = 0;

bool resized = false;


vk::ImageMemoryBarrier transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout old, vk::ImageLayout neo, uint32_t mip);
ImageBundle loadImage(
    void* data, vk::Extent2D size, vk::Format format);
};
}
#endif