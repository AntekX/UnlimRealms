///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Camera/Camera.h"

namespace UnlimRealms
{

	Camera::Camera(Realm &realm) :
		RealmEntity(realm)
	{
		this->minViewDist = 1.0e-2f;
		this->maxViewDist = 1.0e+6f;
		this->fieldOfView = MathConst<float>::Pi * 0.3f;
		this->aspectRatio = 1.0f;
		this->frame.SetIdentity();
		this->viewChanged = true;
		this->projChanged = true;
	}

	Camera::~Camera()
	{

	}

	void Camera::SetProjection(ur_float minViewDist, ur_float maxViewDist, ur_float fieldOfView, ur_float aspectRatio)
	{
		this->minViewDist = minViewDist;
		this->maxViewDist = maxViewDist;
		this->fieldOfView = fieldOfView;
		this->aspectRatio = aspectRatio;
		this->projChanged = true;
	}

	void Camera::SetAspectRatio(ur_float aspectRatio)
	{
		if (this->aspectRatio == aspectRatio)
			return;
		this->aspectRatio = aspectRatio;
		this->projChanged = true;
	}

	void Camera::SetFrame(const ur_float4x4 &frame)
	{
		this->frame = frame;
		this->viewChanged = true;
	}

	void Camera::SetFrame(const ur_float3 &right, const ur_float3 &up, const ur_float3 &ahead)
	{
		this->frame.r[0] = right;
		this->frame.r[1] = up;
		this->frame.r[2] = ahead;
		this->viewChanged = true;
	}

	void Camera::SetPosition(const ur_float3 &pos)
	{
		this->frame.r[3] = pos;
		this->viewChanged = true;
	}

	void Camera::Move(const ur_float3 &deltaPos)
	{
		(ur_float3&)this->frame.r[3] += deltaPos;
		this->viewChanged = true;
	}

	void Camera::MoveRight(const ur_float &delta)
	{
		(ur_float3&)this->frame.r[3] += (ur_float3&)this->frame.r[0] * delta;
		this->viewChanged = true;
	}

	void Camera::MoveUp(const ur_float &delta)
	{
		(ur_float3&)this->frame.r[3] += (ur_float3&)this->frame.r[1] * delta;
		this->viewChanged = true;
	}

	void Camera::MoveAhead(const ur_float &delta)
	{
		(ur_float3&)this->frame.r[3] += (ur_float3&)this->frame.r[2] * delta;
		this->viewChanged = true;
	}

	void Camera::SetRotation(const ur_float3 &axis, const ur_float &angle)
	{
		ur_float4 q = ur_float4::Quaternion(axis, angle);
		ur_float4x4 m = ur_float4x4::RotationQuaternion(q);
		this->frame.r[0] = m.r[0];
		this->frame.r[1] = m.r[1];
		this->frame.r[2] = m.r[2];
		this->viewChanged = true;
	}

	void Camera::Rotate(const ur_float3 &axis, const ur_float &delta)
	{
		ur_float4 q = ur_float4::Quaternion(axis, delta);
		ur_float4x4 m = ur_float4x4::RotationQuaternion(q);
		m = ur_float4x4::Multiply(this->frame, m);
		this->frame.r[0] = m.r[0];
		this->frame.r[1] = m.r[1];
		this->frame.r[2] = m.r[2];
		this->viewChanged = true;
	}

	void Camera::Pitch(const ur_float &delta)
	{
		this->Rotate((ur_float3&)this->frame.r[0], delta);
	}

	void Camera::Yaw(const ur_float &delta)
	{
		this->Rotate((ur_float3&)this->frame.r[1], delta);
	}

	void Camera::Roll(const ur_float &delta)
	{
		this->Rotate((ur_float3&)this->frame.r[2], delta);
	}

	const ur_float4x4& Camera::GetView()
	{
		if (this->viewChanged)
		{
			this->view = ur_float4x4::View(this->frame.r[3], this->frame.r[0], this->frame.r[1], this->frame.r[2]);
			this->viewChanged = false;
		}
		return this->view;
	}

	const ur_float4x4& Camera::GetProj()
	{
		if (this->projChanged)
		{
			this->proj = ur_float4x4::PerspectiveFov(this->fieldOfView, this->aspectRatio, this->minViewDist, this->maxViewDist);
			this->projChanged = false;
		}
		return this->proj;
	}

	const ur_float4x4& Camera::GetViewProj()
	{
		if (this->viewChanged || this->projChanged)
		{
			this->GetView();
			this->GetProj();
			this->viewProj = ur_float4x4::Multiply(this->view, this->proj);
		}
		return this->viewProj;
	}
	
} // end namespace UnlimRealms