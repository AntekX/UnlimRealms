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

	inline const GrafDescriptorTableLayoutDesc& GrafDescriptorTableLayout::GetLayoutDesc() const
	{
		return this->layoutDesc;
	}

	inline GrafDescriptorTableLayout* GrafDescriptorTable::GetLayout() const
	{
		return this->layout;
	}

} // end namespace UnlimRealmscs