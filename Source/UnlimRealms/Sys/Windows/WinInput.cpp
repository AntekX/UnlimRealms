///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Sys/Log.h"
#include "Sys/Windows/WinInput.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// WinInput
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	WinInput::WinInput(Realm &realm) : 
		Input(realm)
	{
		this->msgQueue.reserve(128);
	}

	WinInput::~WinInput()
	{

	}

	Result WinInput::Initialize()
	{
		// initialize keyboard

		std::unique_ptr<WinKeyboard> winKeyboard(new WinKeyboard(*this));
		if (Succeeded(winKeyboard->Initialize()))
		{
			this->keyboard = std::move(winKeyboard);
		}

		// initalize mouse

		std::unique_ptr<WinMouse> winMouse(new WinMouse(*this));
		if (Succeeded(winMouse->Initialize()))
		{
			this->mouse = std::move(winMouse);
		}

		return Result(this->keyboard != ur_null && this->mouse != ur_null ? Success : Failure);
	}

	void WinInput::Update()
	{
		Input::Update();

		// queue can now be cleared after all devices have processed the accumulated messages
		this->msgQueue.clear();
	}

	void WinInput::ProcessMsg(const MSG &msg)
	{
		this->msgQueue.push_back(msg);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// WinKeyboard
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	WinInput::WinKeyboard::WinKeyboard(WinInput &winInput) :
		Keyboard(winInput)
	{
		// virtual keys mapping
		
		this->vkeyToCode[(ur_uint)VKey::Num0] = 0x30;
		this->vkeyToCode[(ur_uint)VKey::Num1] = 0x31;
		this->vkeyToCode[(ur_uint)VKey::Num2] = 0x32;
		this->vkeyToCode[(ur_uint)VKey::Num3] = 0x33;
		this->vkeyToCode[(ur_uint)VKey::Num4] = 0x34;
		this->vkeyToCode[(ur_uint)VKey::Num5] = 0x35;
		this->vkeyToCode[(ur_uint)VKey::Num6] = 0x36;
		this->vkeyToCode[(ur_uint)VKey::Num7] = 0x37;
		this->vkeyToCode[(ur_uint)VKey::Num8] = 0x38;
		this->vkeyToCode[(ur_uint)VKey::Num9] = 0x39;
		this->vkeyToCode[(ur_uint)VKey::A] = 0x41;
		this->vkeyToCode[(ur_uint)VKey::B] = 0x42;
		this->vkeyToCode[(ur_uint)VKey::C] = 0x43;
		this->vkeyToCode[(ur_uint)VKey::D] = 0x44;
		this->vkeyToCode[(ur_uint)VKey::E] = 0x45;
		this->vkeyToCode[(ur_uint)VKey::F] = 0x46;
		this->vkeyToCode[(ur_uint)VKey::G] = 0x47;
		this->vkeyToCode[(ur_uint)VKey::H] = 0x48;
		this->vkeyToCode[(ur_uint)VKey::I] = 0x49;
		this->vkeyToCode[(ur_uint)VKey::J] = 0x4A;
		this->vkeyToCode[(ur_uint)VKey::K] = 0x4B;
		this->vkeyToCode[(ur_uint)VKey::L] = 0x4C;
		this->vkeyToCode[(ur_uint)VKey::M] = 0x4D;
		this->vkeyToCode[(ur_uint)VKey::N] = 0x4E;
		this->vkeyToCode[(ur_uint)VKey::O] = 0x4F;
		this->vkeyToCode[(ur_uint)VKey::P] = 0x50;
		this->vkeyToCode[(ur_uint)VKey::Q] = 0x51;
		this->vkeyToCode[(ur_uint)VKey::R] = 0x52;
		this->vkeyToCode[(ur_uint)VKey::S] = 0x53;
		this->vkeyToCode[(ur_uint)VKey::T] = 0x54;
		this->vkeyToCode[(ur_uint)VKey::U] = 0x55;
		this->vkeyToCode[(ur_uint)VKey::V] = 0x56;
		this->vkeyToCode[(ur_uint)VKey::W] = 0x57;
		this->vkeyToCode[(ur_uint)VKey::X] = 0x58;
		this->vkeyToCode[(ur_uint)VKey::Y] = 0x59;
		this->vkeyToCode[(ur_uint)VKey::Z] = 0x5A;
		this->vkeyToCode[(ur_uint)VKey::Oem1] = VK_OEM_1;
		this->vkeyToCode[(ur_uint)VKey::Oem2] = VK_OEM_2;
		this->vkeyToCode[(ur_uint)VKey::Oem3] = VK_OEM_3;
		this->vkeyToCode[(ur_uint)VKey::Oem4] = VK_OEM_4;
		this->vkeyToCode[(ur_uint)VKey::Oem5] = VK_OEM_5;
		this->vkeyToCode[(ur_uint)VKey::Oem6] = VK_OEM_6;
		this->vkeyToCode[(ur_uint)VKey::Oem7] = VK_OEM_7;
		this->vkeyToCode[(ur_uint)VKey::Oem8] = VK_OEM_8;
		this->vkeyToCode[(ur_uint)VKey::OemPlus] = VK_OEM_PLUS;
		this->vkeyToCode[(ur_uint)VKey::OemComma] = VK_OEM_COMMA;
		this->vkeyToCode[(ur_uint)VKey::OemMinus] = VK_OEM_MINUS;
		this->vkeyToCode[(ur_uint)VKey::OemPeriod] = VK_OEM_PERIOD;
		this->vkeyToCode[(ur_uint)VKey::Back] = VK_BACK;
		this->vkeyToCode[(ur_uint)VKey::Tab] = VK_TAB;
		this->vkeyToCode[(ur_uint)VKey::Return] = VK_RETURN;
		this->vkeyToCode[(ur_uint)VKey::Pause] = VK_PAUSE;
		this->vkeyToCode[(ur_uint)VKey::Escape] = VK_ESCAPE;
		this->vkeyToCode[(ur_uint)VKey::Space] = VK_SPACE;
		this->vkeyToCode[(ur_uint)VKey::Prior] = VK_PRIOR;
		this->vkeyToCode[(ur_uint)VKey::Next] = VK_NEXT;
		this->vkeyToCode[(ur_uint)VKey::End] = VK_END;
		this->vkeyToCode[(ur_uint)VKey::Home] = VK_HOME;
		this->vkeyToCode[(ur_uint)VKey::Left] = VK_LEFT;
		this->vkeyToCode[(ur_uint)VKey::Up] = VK_UP;
		this->vkeyToCode[(ur_uint)VKey::Right] = VK_RIGHT;
		this->vkeyToCode[(ur_uint)VKey::Down] = VK_DOWN;
		this->vkeyToCode[(ur_uint)VKey::Insert] = VK_INSERT;
		this->vkeyToCode[(ur_uint)VKey::Delete] = VK_DELETE;
		this->vkeyToCode[(ur_uint)VKey::NumLock] = VK_NUMLOCK;
		this->vkeyToCode[(ur_uint)VKey::NumPad0] = VK_NUMPAD0;
		this->vkeyToCode[(ur_uint)VKey::NumPad1] = VK_NUMPAD1;
		this->vkeyToCode[(ur_uint)VKey::NumPad2] = VK_NUMPAD2;
		this->vkeyToCode[(ur_uint)VKey::NumPad3] = VK_NUMPAD3;
		this->vkeyToCode[(ur_uint)VKey::NumPad4] = VK_NUMPAD4;
		this->vkeyToCode[(ur_uint)VKey::NumPad5] = VK_NUMPAD5;
		this->vkeyToCode[(ur_uint)VKey::NumPad6] = VK_NUMPAD6;
		this->vkeyToCode[(ur_uint)VKey::NumPad7] = VK_NUMPAD7;
		this->vkeyToCode[(ur_uint)VKey::NumPad8] = VK_NUMPAD8;
		this->vkeyToCode[(ur_uint)VKey::NumPad9] = VK_NUMPAD9;
		this->vkeyToCode[(ur_uint)VKey::Multiply] = VK_MULTIPLY;
		this->vkeyToCode[(ur_uint)VKey::Add] = VK_ADD;
		this->vkeyToCode[(ur_uint)VKey::Separator] = VK_SEPARATOR;
		this->vkeyToCode[(ur_uint)VKey::Subtract] = VK_SUBTRACT;
		this->vkeyToCode[(ur_uint)VKey::Decimal] = VK_DECIMAL;
		this->vkeyToCode[(ur_uint)VKey::Divide] = VK_DIVIDE;
		this->vkeyToCode[(ur_uint)VKey::F1] = VK_F1;
		this->vkeyToCode[(ur_uint)VKey::F2] = VK_F2;
		this->vkeyToCode[(ur_uint)VKey::F3] = VK_F3;
		this->vkeyToCode[(ur_uint)VKey::F4] = VK_F4;
		this->vkeyToCode[(ur_uint)VKey::F5] = VK_F5;
		this->vkeyToCode[(ur_uint)VKey::F6] = VK_F6;
		this->vkeyToCode[(ur_uint)VKey::F7] = VK_F7;
		this->vkeyToCode[(ur_uint)VKey::F8] = VK_F8;
		this->vkeyToCode[(ur_uint)VKey::F9] = VK_F9;
		this->vkeyToCode[(ur_uint)VKey::F10] = VK_F10;
		this->vkeyToCode[(ur_uint)VKey::F11] = VK_F11;
		this->vkeyToCode[(ur_uint)VKey::F12] = VK_F12;
		this->vkeyToCode[(ur_uint)VKey::Shift] = VK_SHIFT;
		this->vkeyToCode[(ur_uint)VKey::Control] = VK_CONTROL;
		this->vkeyToCode[(ur_uint)VKey::Alt] = VK_MENU;

		for (ur_uint i = 0; i < (ur_uint)VKey::Count; ++i)
		{
			this->codeToVKey[this->vkeyToCode[i]] = (VKey)i;
		}
	}

	WinInput::WinKeyboard::~WinKeyboard()
	{

	}

	Result WinInput::WinKeyboard::Initialize()
	{
		return Result(Success);
	}

	void WinInput::WinKeyboard::Update()
	{
		this->inputQueueSize = 0;
		for (ur_uint i = 0; i < (ur_uint)VKey::Count; ++i)
		{
			// reset released keys to "Up" state 
			if (this->vkeyState[i] == KeyState::Released)
				this->vkeyState[i] = KeyState::Up;
		}

		WinInput &winInput = static_cast<WinInput&>(this->GetInput());
		for (const MSG &msg : winInput.msgQueue)
		{
			switch (msg.message)
			{
			case WM_KEYDOWN:
			{
				ur_uint vkey = (ur_uint)this->codeToVKey[msg.wParam];
				if (vkey < (ur_uint)VKey::Count)
				{
					this->vkeyState[vkey] = KeyState::Down;
				}
			}
			break;
			case WM_KEYUP:
			{
				ur_uint vkey = (ur_uint)this->codeToVKey[msg.wParam];
				if (vkey < (ur_uint)VKey::Count)
				{
					this->vkeyState[vkey] = KeyState::Released;
				}
			}
			break;
			case WM_CHAR:
			{
				if (this->inputQueueSize < InputQueueMaxSize)
				{
					this->inputQueue[this->inputQueueSize++] = (KeyCode)msg.wParam;
				}
			}
			break;
			}
		}
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// WinMouse
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	WinInput::WinMouse::WinMouse(WinInput &winInput) :
		Mouse(winInput)
	{

	}

	WinInput::WinMouse::~WinMouse()
	{

	}

	Result WinInput::WinMouse::Initialize()
	{
		return Result(Success);
	}

	void WinInput::WinMouse::Update()
	{
		if (this->lbState == KeyState::Released) this->lbState = KeyState::Up;
		if (this->rbState == KeyState::Released) this->rbState = KeyState::Up;
		if (this->mbState == KeyState::Released) this->mbState = KeyState::Up;
		this->wheelDelta = 0;

		WinInput &winInput = static_cast<WinInput&>(this->GetInput());
		for (const MSG &msg : winInput.msgQueue)
		{
			switch (msg.message)
			{
			case WM_LBUTTONDOWN:
				this->lbState = KeyState::Down;
				break;
			case WM_LBUTTONUP:
				this->lbState = KeyState::Released;
				break;
			case WM_RBUTTONDOWN:
				this->rbState = KeyState::Down;
				break;
			case WM_RBUTTONUP:
				this->rbState = KeyState::Released;
				break;
			case WM_MBUTTONDOWN:
				this->mbState = KeyState::Down;
				break;
			case WM_MBUTTONUP:
				this->mbState = KeyState::Released;
				break;
			case WM_MOUSEWHEEL:
				this->wheelDelta += (ur_int)GET_WHEEL_DELTA_WPARAM(msg.wParam);
				break;
			case WM_MOUSEMOVE:
				this->pos.x = (ur_int)LOWORD(msg.lParam);
				this->pos.y = (ur_int)HIWORD(msg.lParam);
				break;
			}
		}
	}

} // end namespace UnlimRealms