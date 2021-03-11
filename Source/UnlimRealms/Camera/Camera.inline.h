///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline ur_float Camera::GetMinViewDist() const
	{
		return this->minViewDist;
	}

	inline ur_float Camera::GetMaxViewDist() const
	{
		return this->maxViewDist;
	}

	inline ur_float Camera::GetFieldOFView() const
	{
		return this->fieldOfView;
	}

	inline ur_float Camera::GetAspectRatio() const
	{
		return this->aspectRatio;
	}

	inline const ur_float4x4& Camera::GetFrame() const
	{
		return this->frame;
	}

	inline const ur_float3& Camera::GetPosition() const
	{
		return (const ur_float3&)this->frame.r[3];
	}

	inline const ur_float3& Camera::GetDirection() const
	{
		return (const ur_float3&)this->frame.r[2];
	}

} // end namespace UnlimRealms