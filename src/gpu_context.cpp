#include "gpu_context.h"

#include <iostream>
#include <vulkan/vulkan_core.h>

#include "utils.h"

GpuContext::GpuContext() {
  vkb::InstanceBuilder instance_builder;
  auto instance_ret = instance_builder.set_app_name("bug repro")
                          .set_engine_name("repro")
                          .require_api_version(1, 3, 0)
                          .enable_validation_layers()
                          .use_default_debug_messenger()
                          .set_headless()
                          .build();
  if (!instance_ret) {
    std::cerr << "Failed to create instance: " << instance_ret.error().message()
              << std::endl;
    exit(1);
  }

  instance_ = instance_ret.value();
  volkLoadInstance(instance_);

  const VkPhysicalDeviceVulkan13Features vk_13_features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .synchronization2 = true,
      .dynamicRendering = true,
  };
  vkb::PhysicalDeviceSelector selector{instance_};

  auto phys_ret = selector.set_minimum_version(1, 3)
                      .set_required_features_13(vk_13_features)
                      .select();
  if (!phys_ret) {
    std::cerr << "Failed to find physical device: "
              << phys_ret.error().message() << std::endl;
    exit(1);
  }

  phys_device_ = phys_ret.value();
  std::cout << "Selected device: " << phys_device_.name << std::endl;

  vkb::DeviceBuilder device_builder{phys_device_};
  auto device_ret = device_builder.build();
  if (!device_ret) {
    std::cerr << "Failed to create device: " << device_ret.error().message()
              << std::endl;
    exit(1);
  }

  device_ = device_ret.value();
  volkLoadDevice(device_);

  VmaVulkanFunctions vk_functions{
      .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
      .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
      .vkCreateBuffer = vkCreateBuffer,
      .vkCreateImage = vkCreateImage,
  };
  VmaAllocatorCreateInfo alloc_create_info{
      .physicalDevice = phys_device_,
      .device = device_,
      .pVulkanFunctions = &vk_functions,
      .instance = instance_,
  };
  checkResult(vmaCreateAllocator(&alloc_create_info, &allocator_));

  auto graphics_queue_res = device_.get_queue(vkb::QueueType::graphics);
  if (!graphics_queue_res) {
    std::cerr << "Failed to get graphics queue: "
              << graphics_queue_res.error().message() << std::endl;
    exit(1);
  }

  graphics_queue_ = graphics_queue_res.value();
  uint32_t graphics_queue_idx =
      device_.get_queue_index(vkb::QueueType::graphics).value();

  VkCommandPoolCreateInfo command_pool_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = graphics_queue_idx,
  };
  checkResult(vkCreateCommandPool(device_, &command_pool_info, nullptr,
                                  &command_pool_));

  VkCommandBufferAllocateInfo command_buffer_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = command_pool_,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  checkResult(vkAllocateCommandBuffers(device_, &command_buffer_info,
                                       &command_buffer_));

  VkExtent3D frame_extent = {
      .width = WIDTH,
      .height = HEIGHT,
      .depth = 1,
  };
  VkImageCreateInfo nv12_image_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT |
               VK_IMAGE_CREATE_EXTENDED_USAGE_BIT,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
      .extent = frame_extent,
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &graphics_queue_idx,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  VmaAllocationCreateInfo nv12_image_alloc_info{
      .usage = VMA_MEMORY_USAGE_AUTO,
  };
  checkResult(vmaCreateImage(allocator_, &nv12_image_info,
                             &nv12_image_alloc_info, &nv12_image_,
                             &nv12_image_alloc_, nullptr));

  VkImageCreateInfo rgba_image_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .extent = frame_extent,
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_LINEAR,
      .usage =
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &graphics_queue_idx,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  VmaAllocationCreateInfo rgba_image_alloc_info{
      .usage = VMA_MEMORY_USAGE_AUTO,
  };
  checkResult(vmaCreateImage(allocator_, &rgba_image_info,
                             &rgba_image_alloc_info, &rgba_image_,
                             &rgba_image_alloc_, nullptr));

  VkCommandBufferBeginInfo begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  checkResult(vkBeginCommandBuffer(command_buffer_, &begin_info));
  transitionImage(command_buffer_, nv12_image_, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_NONE,
                  VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                  VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
  transitionImage(command_buffer_, rgba_image_, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_NONE,
                  VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                  VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
  checkResult(vkEndCommandBuffer(command_buffer_));
  VkSubmitInfo submit_info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &command_buffer_,
  };
  checkResult(vkQueueSubmit(graphics_queue_, 1, &submit_info, VK_NULL_HANDLE));
  checkResult(vkQueueWaitIdle(graphics_queue_));

  VkImageViewCreateInfo y_image_view_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = nv12_image_,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8_UNORM,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };
  VkImageViewCreateInfo uv_image_view_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = nv12_image_,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8_UNORM,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };
  VkImageViewCreateInfo rgba_image_view_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = rgba_image_,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };
  checkResult(
      vkCreateImageView(device_, &y_image_view_info, nullptr, &y_plane_view_));
  checkResult(vkCreateImageView(device_, &uv_image_view_info, nullptr,
                                &uv_plane_view_));
  checkResult(vkCreateImageView(device_, &rgba_image_view_info, nullptr,
                                &rgba_image_view_));

  VkSamplerCreateInfo sampler_info{
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
  };
  checkResult(vkCreateSampler(device_, &sampler_info, nullptr, &sampler_));

  VkShaderModule triangle_vert_shader;
  VkShaderModule y_plane_frag_shader;
  VkShaderModule uv_plane_frag_shader;
  VkShaderModule nv12_to_rgba_frag_shader;
  VkShaderModule fullscreen_quad_vert_shader;
  loadShader("./triangle.vert.spv", device_, &triangle_vert_shader);
  loadShader("./y_plane.frag.spv", device_, &y_plane_frag_shader);
  loadShader("./uv_plane.frag.spv", device_, &uv_plane_frag_shader);
  loadShader("./nv12_to_rgba.frag.spv", device_, &nv12_to_rgba_frag_shader);
  loadShader("./fullscreen_quad.vert.spv", device_,
             &fullscreen_quad_vert_shader);

  VkPushConstantRange push_constant_range{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .offset = 0,
      .size = 4,
  };
  VkPipelineLayoutCreateInfo nv12_pipeline_layout_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &push_constant_range,
  };
  checkResult(vkCreatePipelineLayout(device_, &nv12_pipeline_layout_info,
                                     nullptr, &nv12_pipeline_layout_));

  VkDescriptorSetLayoutBinding y_sampler_binding{
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
  };
  VkDescriptorSetLayoutBinding uv_sampler_binding{
      .binding = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
  };
  std::vector<VkDescriptorSetLayoutBinding> nv12_bindings = {
      y_sampler_binding, uv_sampler_binding};
  VkDescriptorSetLayoutCreateInfo nv12_descriptor_set_layout_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<uint32_t>(nv12_bindings.size()),
      .pBindings = nv12_bindings.data(),
  };
  checkResult(vkCreateDescriptorSetLayout(device_,
                                          &nv12_descriptor_set_layout_info,
                                          nullptr, &nv12_desc_set_layout_));

  VkPipelineLayoutCreateInfo rgba_pipeline_layout_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &nv12_desc_set_layout_,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr,
  };
  checkResult(vkCreatePipelineLayout(device_, &rgba_pipeline_layout_info,
                                     nullptr, &rgba_pipeline_layout_));

  createPipeline(device_, nv12_pipeline_layout_, triangle_vert_shader,
                 y_plane_frag_shader, &y_pipeline_,
                 {.width = WIDTH, .height = HEIGHT}, VK_FORMAT_R8_UNORM);
  createPipeline(device_, nv12_pipeline_layout_, triangle_vert_shader,
                 uv_plane_frag_shader, &uv_pipeline_,
                 {.width = WIDTH / 2, .height = HEIGHT / 2},
                 VK_FORMAT_R8G8_UNORM);
  createPipeline(device_, rgba_pipeline_layout_, fullscreen_quad_vert_shader,
                 nv12_to_rgba_frag_shader, &rgba_pipeline_,
                 {.width = WIDTH, .height = HEIGHT}, VK_FORMAT_R8G8B8A8_UNORM);

  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2} // For Y and UV
  };
  VkDescriptorPoolCreateInfo descriptor_pool_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1,
      .poolSizeCount = std::size(pool_sizes),
      .pPoolSizes = pool_sizes,
  };
  checkResult(vkCreateDescriptorPool(device_, &descriptor_pool_info, nullptr,
                                     &nv12_desc_set_pool_));

  VkDescriptorSetAllocateInfo descriptor_set_alloc_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = nv12_desc_set_pool_,
      .descriptorSetCount = 1,
      .pSetLayouts = &nv12_desc_set_layout_,
  };
  checkResult(vkAllocateDescriptorSets(device_, &descriptor_set_alloc_info,
                                       &nv12_desc_set_));

  VkDescriptorImageInfo y_image_info{
      .sampler = sampler_,
      .imageView = y_plane_view_,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
  VkWriteDescriptorSet y_descriptor_write{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = nv12_desc_set_,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &y_image_info,
  };
  VkDescriptorImageInfo uv_image_info{
      .sampler = sampler_,
      .imageView = uv_plane_view_,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
  VkWriteDescriptorSet uv_descriptor_write{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = nv12_desc_set_,
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &uv_image_info,
  };

  std::vector<VkWriteDescriptorSet> descriptor_writes = {y_descriptor_write,
                                                         uv_descriptor_write};
  vkUpdateDescriptorSets(device_,
                         static_cast<uint32_t>(descriptor_writes.size()),
                         descriptor_writes.data(), 0, nullptr);

  VmaAllocationCreateInfo staging_buffer_alloc_info{
      .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
               VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO,
  };
  VkBufferCreateInfo staging_buffer_info{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = WIDTH * HEIGHT * 4,
      .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
  };
  checkResult(vmaCreateBuffer(allocator_, &staging_buffer_info,
                              &staging_buffer_alloc_info, &staging_buffer_,
                              &staging_buffer_alloc_,
                              &staging_buffer_allocation_info_));

  vkDestroyShaderModule(device_, triangle_vert_shader, nullptr);
  vkDestroyShaderModule(device_, y_plane_frag_shader, nullptr);
  vkDestroyShaderModule(device_, uv_plane_frag_shader, nullptr);
  vkDestroyShaderModule(device_, nv12_to_rgba_frag_shader, nullptr);
  vkDestroyShaderModule(device_, fullscreen_quad_vert_shader, nullptr);
}

GpuContext::~GpuContext() {
  vkDeviceWaitIdle(device_);

  vmaDestroyBuffer(allocator_, staging_buffer_, staging_buffer_alloc_);

  vkDestroyPipeline(device_, y_pipeline_, nullptr);
  vkDestroyPipeline(device_, uv_pipeline_, nullptr);
  vkDestroyPipeline(device_, rgba_pipeline_, nullptr);
  vkDestroyDescriptorPool(device_, nv12_desc_set_pool_, nullptr);
  vkDestroyDescriptorSetLayout(device_, nv12_desc_set_layout_, nullptr);

  vkDestroyImageView(device_, y_plane_view_, nullptr);
  vkDestroyImageView(device_, uv_plane_view_, nullptr);
  vkDestroyImageView(device_, rgba_image_view_, nullptr);
  vkDestroyPipelineLayout(device_, nv12_pipeline_layout_, nullptr);
  vkDestroyPipelineLayout(device_, rgba_pipeline_layout_, nullptr);
  vkDestroySampler(device_, sampler_, nullptr);
  vkDestroyCommandPool(device_, command_pool_, nullptr);
  vmaDestroyImage(allocator_, nv12_image_, nv12_image_alloc_);
  vmaDestroyImage(allocator_, rgba_image_, rgba_image_alloc_);
  vmaDestroyAllocator(allocator_);
  vkb::destroy_device(device_);
  vkb::destroy_instance(instance_);
}
