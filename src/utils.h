#pragma once

#include "volk.h"

#include "gpu_context.h"

#define WIDTH 1280
#define HEIGHT 720

void checkResult(VkResult result);

void transitionImage(VkCommandBuffer command_buffer, VkImage image,
                     VkImageLayout old_layout, VkImageLayout new_layout,
                     VkPipelineStageFlags2 src_stage,
                     VkPipelineStageFlags2 dst_stage, VkAccessFlags2 src_access,
                     VkAccessFlags2 dst_access);

void loadShader(const char *path, VkDevice device, VkShaderModule *module);

void createPipeline(VkDevice device, VkPipelineLayout layout,
                    VkShaderModule vertex_shader,
                    VkShaderModule fragment_shader, VkPipeline *pipeline,
                    VkExtent2D extent, VkFormat format);

void renderPlane(const GpuContext &ctx, VkRenderingInfo *rendering_info,
                 VkPipeline pipeline, float time);

void convertNV12ToRgba(const GpuContext& ctx);
