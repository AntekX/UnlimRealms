///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline ur_uint GrafSystem::GetPhysicalDeviceCount()
	{
		return (ur_uint)grafPhysicalDeviceDesc.size();
	}

	inline const GrafPhysicalDeviceDesc* GrafSystem::GetPhysicalDeviceDesc(ur_uint deviceId)
	{
		return (deviceId < GetPhysicalDeviceCount() ? &grafPhysicalDeviceDesc[deviceId] : ur_null);
	}

	inline GrafSystem& GrafEntity::GetGrafSystem()
	{
		return this->grafSystem;
	}

	inline ur_uint GrafDevice::GetDeviceId()
	{
		return deviceId;
	}

	inline const GrafPhysicalDeviceDesc* GrafDevice::GetPhysicalDeviceDesc()
	{
		return this->GetGrafSystem().GetPhysicalDeviceDesc(this->deviceId);
	}

	inline GrafDevice* GrafDeviceEntity::GetGrafDevice()
	{
		return this->grafDevice;
	}

	inline ur_uint GrafCanvas::GetSwapChainImageCount() const
	{
		return this->swapChainImageCount;
	}

	inline ur_uint GrafCanvas::GetCurrentImageId() const
	{
		return this->swapChainCurrentImageId;
	}

	inline const GrafImageDesc& GrafImage::GetDesc() const
	{
		return this->imageDesc;
	}

	inline const GrafImageState& GrafImage::GetState() const
	{
		return this->imageState;
	}

	inline const GrafBufferDesc& GrafBuffer::GetDesc() const
	{
		return this->bufferDesc;
	}

	inline ur_uint64 GrafBuffer::GetDeviceAddress() const
	{
		return this->bufferDeviceAddress;
	}

	inline const GrafSamplerDesc& GrafSampler::GetDesc() const
	{
		return this->samplerDesc;
	}

	inline const GrafShaderType& GrafShader::GetShaderType() const
	{
		return this->shaderType;
	}

	inline const std::string& GrafShader::GetEntryPoint() const
	{
		return this->entryPoint;
	}

	inline ur_size GrafRenderPass::GetImageCount() const
	{
		return (ur_size)this->renderPassImageDescs.size();
	}

	inline const GrafRenderPassImageDesc& GrafRenderPass::GetImageDesc(ur_size idx) const
	{
		return this->renderPassImageDescs[idx];
	}

	GrafRenderPass* GrafRenderTarget::GetRenderPass() const
	{
		return this->renderPass;
	}

	GrafImage* GrafRenderTarget::GetImage(ur_uint imageId) const
	{
		return (imageId < this->images.size() ? this->images[imageId] : ur_null);
	}

	ur_uint GrafRenderTarget::GetImageCount() const
	{
		return (ur_uint)this->images.size();
	}

	inline const GrafDescriptorTableLayoutDesc& GrafDescriptorTableLayout::GetLayoutDesc() const
	{
		return this->layoutDesc;
	}

	inline GrafDescriptorTableLayout* GrafDescriptorTable::GetLayout() const
	{
		return this->layout;
	}

	inline GrafAccelerationStructureType GrafAccelerationStructure::GetStructureType() const
	{
		return this->structureType;
	}

	inline GrafAccelerationStructureBuildFlags GrafAccelerationStructure::GetStructureBuildFlags() const
	{
		return this->structureBuildFlags;
	}

	ur_bool GrafUtils::IsDepthStencilFormat(GrafFormat grafFormat)
	{
		return (ur_uint(grafFormat) >= ur_uint(GrafFormat::D16_UNORM) && ur_uint(grafFormat) <= ur_uint(GrafFormat::D32_SFLOAT_S8_UINT));
	}

} // end namespace UnlimRealmscs