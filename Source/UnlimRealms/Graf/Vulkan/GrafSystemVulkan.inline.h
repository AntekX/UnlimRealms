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

	inline VkCommandPool GrafDeviceVulkan::GetVkGraphicsCommandPool() const
	{
		return this->vkGraphicsCommandPool;
	}

	inline VkCommandPool GrafDeviceVulkan::GetVkComputeCommandPool() const
	{
		return this->vkComputeCommandPool;
	}

	inline VkCommandPool GrafDeviceVulkan::GetVkTransferCommandPool() const
	{
		return this->vkTransferCommandPool;
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

	inline void GrafImageVulkan::SetState(GrafImageState& state)
	{
		this->imageState = state;
	}

	inline VkBuffer GrafBufferVulkan::GetVkBuffer() const
	{
		return this->vkBuffer;
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

} // end namespace UnlimRealmscs