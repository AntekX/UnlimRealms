///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline Input& Input::Device::GetInput()
	{
		return this->input;
	}

	inline Input::Keyboard* Input::GetKeyboard() const
	{
		return this->keyboard.get();
	}

	inline Input::Mouse* Input::GetMouse() const
	{
		return this->mouse.get();
	}

	inline Input::Gamepad* Input::GetGamepad(ur_uint idx) const
	{
		return this->gamepad[std::min(idx, Input::Gamepad::MaxConnected - 1)].get();
	}

	inline ur_uint Input::Keyboard::GetInputQueueSize()
	{
		return this->inputQueueSize;
	}

	inline Input::KeyCode Input::Keyboard::GetInputQueueChar(ur_uint idx)
	{
		return (idx < this->inputQueueSize ? this->inputQueue[idx] : Input::InvalidCode);
	}

	inline Input::KeyCode Input::Keyboard::GetKeyCode(VKey vkey)
	{
		return ((ur_uint)vkey < (ur_uint)VKey::Count ? this->vkeyToCode[(ur_uint)vkey] : Input::InvalidCode);
	}

	inline Input::KeyState Input::Keyboard::GetKeyState(VKey vkey)
	{
		return this->vkeyState[(ur_uint)vkey];
	}

	inline bool Input::Keyboard::IsKeyUp(VKey vkey)
	{
		return (KeyState::Up == this->GetKeyState(vkey));
	}

	inline bool Input::Keyboard::IsKeyDown(VKey vkey)
	{
		return (KeyState::Down == this->GetKeyState(vkey));
	}

	inline bool Input::Keyboard::IsKeyReleased(VKey vkey)
	{
		return (KeyState::Released == this->GetKeyState(vkey));
	}

	inline const ur_int2& Input::Mouse::GetPos() const
	{
		return this->pos;
	}

	inline Input::KeyState Input::Mouse::GetLBState() const
	{
		return this->lbState;
	}

	inline Input::KeyState Input::Mouse::GetMBState() const
	{
		return this->mbState;
	}

	inline Input::KeyState Input::Mouse::GetRBState() const
	{
		return this->rbState;
	}

	inline ur_int Input::Mouse::GetWheelDelta() const
	{
		return this->wheelDelta;
	}

} // end namespace UnlimRealms