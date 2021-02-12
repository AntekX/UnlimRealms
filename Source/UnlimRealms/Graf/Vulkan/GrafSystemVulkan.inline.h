///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline VkInstance GrafSystemVulkan::GetVkInstance() const
	{
		return this->vkInstance;
	}

	inline VkPhysicalDevice GrafSystemVulkan::GetVkPhysicalDevice(ur_uint deviceId) const
	{
		return (deviceId < (ur_uint)vkPhysicalDevices.size() ? vkPhysicalDevices[deviceId] : VK_NULL_HANDLE);
	}

	inline VkDevice GrafDeviceVulkan::GetVkDevice() const
	{
		return this->vkDevice;
	}

	inline VmaAllocator GrafDeviceVulkan::GetVmaAllocator() const
	{
		return this->vmaAllocator;
	}

	inline VkDescriptorPool GrafDeviceVulkan::GetVkDescriptorPool() const
	{
		return this->vkDescriptorPool;
	}

	inline ur_uint GrafDeviceVulkan::GetVkDeviceGraphicsQueueId() const
	{
		return this->deviceGraphicsQueueId;
	}

	inline ur_uint GrafDeviceVulkan::GetVkDeviceComputeQueueId() const
	{
		return this->deviceComputeQueueId;
	}

	inline ur_uint GrafDeviceVulkan::GetVkDeviceTransferQueueId() const
	{
		return this->deviceTransferQueueId;
	}

	inline VkCommandBuffer GrafCommandListVulkan::GetVkCommandBuffer() const
	{
		return this->vkCommandBuffer;
	}

	inline VkFence GrafCommandListVulkan::GetVkSubmitFence() const
	{
		return this->vkSubmitFence;
	}

	inline VkEvent GrafFenceVulkan::GetVkEvent() const
	{
		return this->vkEvent;
	}

	inline VkImage GrafImageVulkan::GetVkImage() const
	{
		return this->vkImage;
	}

	inline VkImageView GrafImageVulkan::GetVkImageView() const
	{
		return this->vkImageView;
	}

	inline GrafImageSubresource* GrafImageVulkan::GetDefaultSubresource() const
	{
		return this->grafDefaultSubresource.get();
	}

	inline void GrafImageVulkan::SetState(GrafImageState& state)
	{
		this->imageState = state;
	}

	inline void GrafImageSubresourceVulkan::SetState(GrafImageState& state)
	{
		this->subresourceState = state;
	}

	inline VkImageView GrafImageSubresourceVulkan::GetVkImageView() const
	{
		return this->vkImageView;
	}

	inline VkBuffer GrafBufferVulkan::GetVkBuffer() const
	{
		return this->vkBuffer;
	}

	inline void GrafBufferVulkan::SetState(GrafBufferState& state)
	{
		this->bufferState = state;
	}

	inline VkSampler GrafSamplerVulkan::GetVkSampler() const
	{
		return this->vkSampler;
	}

	inline VkShaderModule GrafShaderVulkan::GetVkShaderModule() const
	{
		return this->vkShaderModule;
	}

	inline VkRenderPass GrafRenderPassVulkan::GetVkRenderPass() const
	{
		return this->vkRenderPass;
	}

	inline VkFramebuffer GrafRenderTargetVulkan::GetVkFramebuffer() const
	{
		return this->vkFramebuffer;
	}

	inline VkDescriptorSetLayout GrafDescriptorTableLayoutVulkan::GetVkDescriptorSetLayout() const
	{
		return this->vkDescriptorSetLayout;
	}

	inline VkDescriptorSet GrafDescriptorTableVulkan::GetVkDescriptorSet() const
	{
		return this->vkDescriptorSet;
	}

	inline VkPipeline GrafPipelineVulkan::GetVkPipeline() const
	{
		return this->vkPipeline;
	}

	inline VkPipelineLayout GrafPipelineVulkan::GetVkPipelineLayout() const
	{
		return this->vkPipelineLayout;
	}

	inline VkPipeline GrafComputePipelineVulkan::GetVkPipeline() const
	{
		return this->vkPipeline;
	}

	inline VkPipelineLayout GrafComputePipelineVulkan::GetVkPipelineLayout() const
	{
		return this->vkPipelineLayout;
	}

	inline VkPipeline GrafRayTracingPipelineVulkan::GetVkPipeline() const
	{
		return this->vkPipeline;
	}

	inline VkPipelineLayout GrafRayTracingPipelineVulkan::GetVkPipelineLayout() const
	{
		return this->vkPipelineLayout;
	}

	#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
	inline VkAccelerationStructureKHR GrafAccelerationStructureVulkan::GetVkAccelerationStructure() const
	{
		return this->vkAccelerationStructure;
	}
	#endif

	inline GrafBuffer* GrafAccelerationStructureVulkan::GetScratchBuffer() const
	{
		return this->grafScratchBuffer.get();
	}

} // end namespace UnlimRealms