#include "utils.h"
#include "gpu_context.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <vk_mem_alloc.h>

void checkResult(VkResult result) {
  if (result != VK_SUCCESS) {
    std::cerr << "Vulkan error: " << result << std::endl;
    exit(result);
  }
}

void transitionImage(VkCommandBuffer command_buffer, VkImage image,
                     VkImageLayout old_layout, VkImageLayout new_layout,
                     VkPipelineStageFlags2 src_stage,
                     VkPipelineStageFlags2 dst_stage, VkAccessFlags2 src_access,
                     VkAccessFlags2 dst_access) {
  VkImageMemoryBarrier2 image_barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = src_stage,
      .srcAccessMask = src_access,
      .dstStageMask = dst_stage,
      .dstAccessMask = dst_access,
      .oldLayout = old_layout,
      .newLayout = new_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };
  VkDependencyInfo barrier_dependency{
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &image_barrier,
  };
  vkCmdPipelineBarrier2(command_buffer, &barrier_dependency);
}

void loadShader(const char *path, VkDevice device, VkShaderModule *module) {
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Couldn't open shader file at: " << path << std::endl;
    exit(1);
  }

  size_t size = file.tellg();
  file.seekg(0);

  std::vector<uint32_t> buffer(size / sizeof(uint32_t));
  file.read(reinterpret_cast<char *>(buffer.data()), size);
  file.close();

  VkShaderModuleCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = buffer.size() * sizeof(uint32_t),
      .pCode = buffer.data(),
  };

  checkResult(vkCreateShaderModule(device, &info, nullptr, module));
}

void createPipeline(VkDevice device, VkPipelineLayout layout,
                    VkShaderModule vertex_shader,
                    VkShaderModule fragment_shader, VkPipeline *pipeline,
                    VkExtent2D extent, VkFormat format) {
  VkPipelineVertexInputStateCreateInfo vertex_input_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  };
  VkPipelineInputAssemblyStateCreateInfo input_assembly_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  };
  std::vector<VkPipelineShaderStageCreateInfo> shader_stages{
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vertex_shader,
          .pName = "main",
      },
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = fragment_shader,
          .pName = "main",
      },
  };
  VkViewport viewport{
      .width = static_cast<float>(extent.width),
      .height = static_cast<float>(extent.height),
      .minDepth = 0.0,
      .maxDepth = 1.0,
  };
  VkRect2D scissor{
      .extent = extent,
  };
  VkPipelineViewportStateCreateInfo viewport_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor,
  };
  VkPipelineDepthStencilStateCreateInfo depth_stencil_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
  };

  VkPipelineRasterizationStateCreateInfo rasterization_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f,
  };
  VkPipelineMultisampleStateCreateInfo multisample_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
  };
  VkPipelineColorBlendAttachmentState color_blend_attachment{
      .blendEnable = VK_FALSE,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };
  VkPipelineColorBlendStateCreateInfo color_blend_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &color_blend_attachment,
  };

  VkPipelineRenderingCreateInfo rendering_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &format,
  };
  VkGraphicsPipelineCreateInfo pipeline_info{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &rendering_info,
      .stageCount = static_cast<uint32_t>(shader_stages.size()),
      .pStages = shader_stages.data(),
      .pVertexInputState = &vertex_input_state,
      .pInputAssemblyState = &input_assembly_state,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterization_state,
      .pMultisampleState = &multisample_state,
      .pDepthStencilState = &depth_stencil_state,
      .pColorBlendState = &color_blend_state,
      .layout = layout,
      .subpass = 0,
  };
  checkResult(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
                                        &pipeline_info, nullptr, pipeline));
}

void renderPlane(const GpuContext &ctx, VkRenderingInfo *rendering_info,
                 VkPipeline pipeline, float time) {
  vkCmdBeginRendering(ctx.command_buffer_, rendering_info);
  vkCmdBindPipeline(ctx.command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline);
  vkCmdPushConstants(ctx.command_buffer_, ctx.nv12_pipeline_layout_,
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                     0, sizeof(float), &time);
  vkCmdDraw(ctx.command_buffer_, 3, 1, 0, 0);
  vkCmdEndRendering(ctx.command_buffer_);
}

void convertNV12ToRgba(const GpuContext &ctx) {
  transitionImage(
      ctx.command_buffer_, ctx.nv12_image_, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
      VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT);

  VkRenderingAttachmentInfo rgba_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = ctx.rgba_image_view_,
      .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
  };
  VkRenderingInfo rgba_rendering_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {.extent = {.width = WIDTH, .height = HEIGHT}},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &rgba_attachment_info,
  };
  vkCmdBeginRendering(ctx.command_buffer_, &rgba_rendering_info);
  vkCmdBindPipeline(ctx.command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    ctx.rgba_pipeline_);
  vkCmdBindDescriptorSets(ctx.command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          ctx.rgba_pipeline_layout_, 0, 1, &ctx.nv12_desc_set_,
                          0, nullptr);
  vkCmdDraw(ctx.command_buffer_, 3, 1, 0, 0);
  vkCmdEndRendering(ctx.command_buffer_);

  transitionImage(ctx.command_buffer_, ctx.nv12_image_,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                  VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                  VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                  VK_ACCESS_2_SHADER_READ_BIT,
                  VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

  transitionImage(
      ctx.command_buffer_, ctx.rgba_image_, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
      VK_ACCESS_2_TRANSFER_READ_BIT);

  VkBufferImageCopy region{
      .imageSubresource =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .layerCount = 1,
          },
      .imageExtent =
          {
              .width = WIDTH,
              .height = HEIGHT,
              .depth = 1,
          },
  };

  vkCmdCopyImageToBuffer(ctx.command_buffer_, ctx.rgba_image_,
                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         ctx.staging_buffer_, 1, &region);

  transitionImage(
      ctx.command_buffer_, ctx.rgba_image_,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      VK_PIPELINE_STAGE_2_TRANSFER_BIT,
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_2_TRANSFER_READ_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
}
