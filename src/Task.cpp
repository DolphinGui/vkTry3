#include "Task.hpp"
#include "vkobjects/Buffer.hpp"
#include "Setup.hpp"
#include "SingleTimeCmdBuffer.hpp"
#include <vulkan/vulkan.hpp>

using namespace vcc;

Task::Task(
    Setup* s,
    VCEngine* e, 
    stbi_uc* image, 
    vk::Extent2D imageSize, 
    size_t bSize,
    std::vector<std::pair<Vertex, uint32_t>>* verts
    ):
    setup(s),
    engine(e),
    texture(loadImage(image, imageSize, vk::Format::eR8G8B8A8Srgb)),
    textureSampler(e->device.createSampler(
        vk::SamplerCreateInfo(
            {},
            vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eLinear,
            vk::SamplerAddressMode::eRepeat,
            vk::SamplerAddressMode::eRepeat,
            vk::SamplerAddressMode::eRepeat,
            0.0f,
            VK_TRUE,
            e->physicalDevice.getProperties().limits.maxSamplerAnisotropy,
            VK_FALSE,
            vk::CompareOp::eAlways,
            0.0f,
            static_cast<float>(static_cast<uint32_t>(
            std::floor(
                std::log2(
                    std::max(imageSize.width, imageSize.height))
            )) + 1),
            vk::BorderColor::eIntOpaqueBlack,
            VK_FALSE
        )
    )),
    vertices(verts)
    {
    size_t vertSize = sizeof(verts[0])*verts->size();
    Buffer staging(Buffer::load(
        e, 
        vertSize, 
        verts->data(),
        {}, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent));
    vertexB = Buffer(
        e,
        vertSize,
        {},
        vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
}


ImageBundle Task::loadImage(void *data, vk::Extent2D size, vk::Format format){
    uint32_t imgSize = size.height*size.width*4;
    uint32_t mip = static_cast<uint32_t>(
            std::floor(
                std::log2(
                    std::max(size.width, size.height))
        )) + 1;
    
    ImageBundle result(
        size.width,
        size.height,
        mip,
        vk::SampleCountFlagBits::e1,
        format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc|
            vk::ImageUsageFlagBits::eTransferDst|
            vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        engine,
        vk::ImageAspectFlagBits::eColor);

    vcc::Buffer staging(vcc::Buffer::load(
        engine, 
        imgSize, 
        data, 
        {}, 
        vk::BufferUsageFlagBits::eTransferSrc, 
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        )
    );

//figure out what to do about this
    SingleTimeCmdBuffer cmd(engine->device, engine->graphicsQueue, &commandPool);
    cmd.cmd.copyBufferToImage(
        staging.buffer.get(), 
        result.image.get(), 
        vk::ImageLayout::eTransferDstOptimal, 
        vk::BufferImageCopy(
            0,
            0,
            0,
            vk::ImageSubresourceLayers(
                vk::ImageAspectFlagBits::eColor,
                0,
                0,
                1
            ),
            vk::Offset3D(0,0,0),
            vk::Extent3D(size.width, size.height, 1)
        )
    );

    vk::FormatProperties fprop = engine->physicalDevice.getFormatProperties(format);
    if(!(fprop.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        throw std::runtime_error("physicalDevice does not support linear blitting");
    vk::ImageMemoryBarrier mipmapper{};
    mipmapper.image = result.image.get();
    mipmapper.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mipmapper.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mipmapper.subresourceRange = vk::ImageSubresourceRange(
        vk::ImageAspectFlagBits::eColor,
        {},
        1,
        0,
        1
    );

    for (uint32_t i = 1; i < mip; i++) {
        mipmapper.subresourceRange.baseMipLevel = i - 1;
        mipmapper.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        mipmapper.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        mipmapper.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        mipmapper.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cmd.cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {},
            nullptr,
            nullptr,
            mipmapper
        );

        vk::ImageBlit blit;
        blit.srcOffsets[0] = vk::Offset3D(0,0,0);
        blit.srcOffsets[1] = vk::Offset3D(size.width, size.height, 1);
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = vk::Offset3D(0,0,0);
        blit.dstOffsets[1] = vk::Offset3D( size.width > 1 ? size.width / 2 : 1, size.height > 1 ? size.height / 2 : 1, 1 );
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        cmd.cmd.blitImage(
            result.image.get(),
            vk::ImageLayout::eTransferSrcOptimal,
            result.image.get(),
            vk::ImageLayout::eTransferDstOptimal,
            blit,
            vk::Filter::eLinear//research more on texel filtering later
        );

        mipmapper.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        mipmapper.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        mipmapper.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        mipmapper.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cmd.cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {},
            nullptr,
            nullptr,
            {
                mipmapper,
                transitionImageLayout(
                    result.image.get(), 
                    format, 
                    vk::ImageLayout::eUndefined, 
                    vk::ImageLayout::eUndefined, 
                    mip
                )
            }
        );

        if (size.width > 1) size.width /= 2;
        if (size.height > 1) size.height /= 2;
    }
    cmd.submit();
    return result;
}

vk::ImageMemoryBarrier Task::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout old, vk::ImageLayout neo, uint32_t mip){
    vk::ImageMemoryBarrier barrier(
        {},{}, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor,
            0,
            mip,
            0,
            1
        )
    );
    vk::PipelineStageFlags src;
    vk::PipelineStageFlags dst;

    if (old == vk::ImageLayout::eUndefined && neo == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        src = vk::PipelineStageFlagBits::eTopOfPipe;
        dst = vk::PipelineStageFlagBits::eTransfer;
    } else if (old == vk::ImageLayout::eTransferDstOptimal && neo == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        src = vk::PipelineStageFlagBits::eTransfer;
        dst = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }
    return barrier;
}

Task::~Task(){
    engine->device.destroyCommandPool(commandPool);
}
