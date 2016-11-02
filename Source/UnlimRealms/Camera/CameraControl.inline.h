///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline Camera* CameraControl::GetCamera() const
	{
		return this->camera;
	}

	inline void CameraControl::SetWorldUp(const ur_float3 v)
	{
		this->worldUp = v;
	}

	inline const ur_float3& CameraControl::GetWorldUp() const
	{
		return this->worldUp;
	}

	inline void CameraControl::SetTargetPoint(const ur_float3 p)
	{
		this->targetPoint = p;
	}
	
	inline const ur_float3& CameraControl::GetTargetPoint() const
	{
		return this->targetPoint;
	}

	inline void CameraControl::SetSpeed(ur_float speed)
	{
		this->speed = speed;
	}

	inline ur_float CameraControl::GetSpeed() const
	{
		return this->speed;
	}

	inline void CameraControl::SetAngularSpeed(ur_float angularSpeed)
	{
		this->angularSpeed = angularSpeed;
	}

	inline ur_float CameraControl::GetAngularSpeed() const
	{
		return this->angularSpeed;
	}

	inline void CameraControl::SetAcceleration(ur_float accel)
	{
		this->accel = accel;
	}

	inline ur_float CameraControl::GetAcceleration() const
	{
		return this->accel;
	}

} // end namespace UnlimRealms