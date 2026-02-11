#include <cstdint>

#define VOLK_IMPLEMENTATION
#include "volk.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <VkBootstrap.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>

#include "utils.h"

int main() {
  volkInitialize();

  GpuContext ctx;

  VkFence fence;
  VkFenceCreateInfo fence_info{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  checkResult(vkCreateFence(ctx.device_, &fence_info, nullptr, &fence));

  for (int i = 0; i < 10; i++) {
    std::cout << "Rendering frame " << i + 1 << std::endl;

    checkResult(vkResetFences(ctx.device_, 1, &fence));
    checkResult(vkResetCommandBuffer(ctx.command_buffer_, 0));

    VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    checkResult(vkBeginCommandBuffer(ctx.command_buffer_, &begin_info));

    float time = static_cast<float>(i) * 0.125;

    VkRenderingAttachmentInfo y_attachment_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = ctx.y_plane_view_,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {.color = {.float32 = {0, 0, 0, 0}}},
    };
    VkRenderingInfo y_rendering_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {.extent = {.width = WIDTH, .height = HEIGHT}},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &y_attachment_info,
    };

    VkRenderingAttachmentInfo uv_attachment_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = ctx.uv_plane_view_,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {.color = {.float32 = {0.5, 0.5, 0, 0}}},
    };
    VkRenderingInfo uv_rendering_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {.extent = {.width = WIDTH / 2, .height = HEIGHT / 2}},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &uv_attachment_info,
    };

    renderPlane(ctx, &y_rendering_info, ctx.y_pipeline_, time);
    renderPlane(ctx, &uv_rendering_info, ctx.uv_pipeline_, time);
    convertNV12ToRgba(ctx);

    checkResult(vkEndCommandBuffer(ctx.command_buffer_));

    VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &ctx.command_buffer_,
    };
    checkResult(vkQueueSubmit(ctx.graphics_queue_, 1, &submit_info, fence));
    checkResult(vkWaitForFences(ctx.device_, 1, &fence, VK_TRUE, UINT64_MAX));

    std::cout << "Saving frame " << i + 1 << " to file" << std::endl;
    stbi_write_jpg(("output_" + std::to_string(i + 1) + ".jpg").c_str(), WIDTH,
                   HEIGHT, 4, ctx.staging_buffer_allocation_info_.pMappedData,
                   WIDTH * 4);
  }

  vkDestroyFence(ctx.device_, fence, nullptr);

  return 0;
}
