#pragma once

#include <volk.h>

#include <VkBootstrap.h>

#include <vk_mem_alloc.h>

class GpuContext {
public:
  GpuContext();
  ~GpuContext();

  vkb::Instance instance_;
  vkb::PhysicalDevice phys_device_;
  vkb::Device device_;

  VkQueue graphics_queue_;

  VkCommandPool command_pool_;
  VkCommandBuffer command_buffer_;

  VkDescriptorPool nv12_desc_set_pool_;
  VkDescriptorSetLayout nv12_desc_set_layout_;
  VkDescriptorSet nv12_desc_set_;

  VkPipelineLayout nv12_pipeline_layout_;
  VkPipelineLayout rgba_pipeline_layout_;

  VkPipeline y_pipeline_;
  VkPipeline uv_pipeline_;
  VkPipeline rgba_pipeline_;

  VmaAllocator allocator_;

  VkImage nv12_image_;
  VmaAllocation nv12_image_alloc_;
  VkImageView y_plane_view_;
  VkImageView uv_plane_view_;

  VkImage rgba_image_;
  VmaAllocation rgba_image_alloc_;
  VkImageView rgba_image_view_;

  VkBuffer staging_buffer_;
  VmaAllocation staging_buffer_alloc_;
  VmaAllocationInfo staging_buffer_allocation_info_;

  VkSampler sampler_;
};
