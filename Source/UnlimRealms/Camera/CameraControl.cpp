///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Camera/CameraControl.h"
#include "Sys/Input.h"
#include "ImguiRender/ImguiRender.h"

namespace UnlimRealms
{

	CameraControl::CameraControl(Realm &realm, Camera *camera, Mode mode) :
		RealmEntity(realm), camera(camera), mode(mode)
	{
		this->worldUp = ur_float3(0.0f, 1.0f, 0.0f);
		this->targetPoint = ur_float3(0.0f);
		this->speed = 1.0f;
		this->accel = 0.4f;
		this->angularSpeed = MathConst<float>::Pi * 0.25f;
		this->pointerSpeed = MathConst<float>::Pi * 0.75e-3f;
		this->pointerPos = ur_int2(-1);
		this->time = ClockTime(std::chrono::microseconds(0));
	}

	CameraControl::~CameraControl()
	{

	}

	void CameraControl::SetCamera(Camera *camera)
	{
		this->camera = camera;
	}

	void CameraControl::SetMode(Mode mode)
	{
		this->mode = mode;
	}

	void CameraControl::Update()
	{
		ClockTime timeNow = Clock::now();
		if (this->time == ClockTime(std::chrono::microseconds(0)))
			this->time = timeNow;

		auto deltaTime = ClockDeltaAs<std::chrono::microseconds>(timeNow - this->time);
		this->time = timeNow;
		float elapsedTime = (float)deltaTime.count() * 1.0e-6f; // to seconds

		if (ur_null == this->camera ||
			ur_null == this->GetRealm().GetInput())
			return;

		ur_float rs = this->speed * elapsedTime;
		ur_float ras = this->angularSpeed * elapsedTime;
		ur_float ra = this->accel * rs;

		Input::Keyboard *keyboard = this->GetRealm().GetInput()->GetKeyboard();
		if (keyboard != ur_null)
		{
			if (keyboard->IsKeyDown(Input::VKey::Shift)) { rs *= 10.0f; ra *= 10.0f; }
			if (keyboard->IsKeyDown(Input::VKey::OemMinus)) this->speed += ra;
			if (keyboard->IsKeyDown(Input::VKey::OemPlus)) this->speed += ra;
			if (keyboard->IsKeyDown(Input::VKey::A)) this->camera->MoveRight(-rs);
			if (keyboard->IsKeyDown(Input::VKey::D)) this->camera->MoveRight(+rs);
			if (keyboard->IsKeyDown(Input::VKey::S)) this->camera->MoveAhead(-rs);
			if (keyboard->IsKeyDown(Input::VKey::W)) this->camera->MoveAhead(+rs);
			if (keyboard->IsKeyDown(Input::VKey::Down)) this->camera->MoveUp(-rs);
			if (keyboard->IsKeyDown(Input::VKey::Up)) this->camera->MoveUp(+rs);
			if (Mode::Free == this->mode)
			{
				if (keyboard->IsKeyDown(Input::VKey::E)) this->camera->Roll(-ras);
				if (keyboard->IsKeyDown(Input::VKey::Q)) this->camera->Roll(+ras);
			}
		}

		Input::Mouse *mouse = this->GetRealm().GetInput()->GetMouse();
		if (mouse != ur_null)
		{
			if (this->pointerPos.x == -1) this->pointerPos = mouse->GetPos();
			if (mouse->GetRBState() == Input::KeyState::Down)
			{
				ur_int2 deltaPos = mouse->GetPos() - this->pointerPos;
				if (Mode::AroundPoint == this->mode)
				{
					ur_float4x4 cm = this->camera->GetFrame();
					ur_float4 qpitch = ur_float4::Quaternion((ur_float3&)cm.r[0], this->pointerSpeed * deltaPos.y);
					ur_float4 qyaw = ur_float4::Quaternion(this->worldUp, this->pointerSpeed * deltaPos.x);
					ur_float4x4 rm = ur_float4x4::Multiply(ur_float4x4::RotationQuaternion(qpitch), ur_float4x4::RotationQuaternion(qyaw));
					(ur_float3&)cm.r[3] -= this->targetPoint;
					cm = ur_float4x4::Multiply(cm, rm);
					(ur_float3&)cm.r[3] += this->targetPoint;
					this->camera->SetFrame(cm);
				}
				else
				{
					if (Mode::FixedUp == this->mode)
					{
						camera->Rotate(this->worldUp, this->pointerSpeed * deltaPos.x);
					}
					else
					{
						camera->Yaw(this->pointerSpeed * deltaPos.x);
					}
					camera->Pitch(this->pointerSpeed * deltaPos.y);
				}
			}
			this->pointerPos = mouse->GetPos();

			if (mouse->GetWheelDelta() != 0)
			{
				this->camera->MoveAhead(mouse->GetWheelDelta()*rs);
			}
		}
	}

	void CameraControl::ShowImgui()
	{
		if (ur_null == this->camera)
			return;

		ImGui::SetNextTreeNodeOpen(true);
		if (ImGui::CollapsingHeader("CameraControl"))
		{
			const char* ModeListBoxItems = "Free\0FixedUp\0AroundPoint";
			static int listbox_item_current = 1;
			ImGui::Combo("Mode", (int*)&this->mode, ModeListBoxItems);
			
			if (ImGui::Button("Reset Roll"))
			{
				ur_float4x4 m = this->camera->GetFrame();
				ur_float3 &m_right = m.r[0];
				ur_float d = ur_float3::Dot(m_right, this->worldUp);
				if (1.0f - fabs(d) > 1.0e-2f)
				{
					ur_float3 &m_up = m.r[1];
					ur_float3 &m_ahead = m.r[2];
					m_right = ur_float3::Normalize(m_right - m_up * d);
					m_up = ur_float3::Normalize(ur_float3::Cross(m_ahead, m_right));
				}
				else
				{
					m = ur_float4x4::Identity;
				}
				this->camera->SetFrame(m);
			}
		}
	}

} // end namespace UnlimRealms