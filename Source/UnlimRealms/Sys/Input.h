///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base input system
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL Input : public RealmEntity
	{
	public:

		typedef ur_size KeyCode;
		static const KeyCode InvalidCode = ~KeyCode(0);

		enum class UR_DECL KeyState : ur_byte
		{
			Up,
			Down,
			Released,
		};

		enum class UR_DECL VKey : ur_byte
		{
			Num0,
			Num1,
			Num2,
			Num3,
			Num4,
			Num5,
			Num6,
			Num7,
			Num8,
			Num9,
			A,
			B,
			C,
			D,
			E,
			F,
			G,
			H,
			I,
			J,
			K,
			L,
			M,
			N,
			O,
			P,
			Q,
			R,
			S,
			T,
			U,
			V,
			W,
			X,
			Y,
			Z,
			Oem1,		// ';:'
			Oem2,		// '/?'
			Oem3,		// '`~'
			Oem4,		// '[{'
			Oem5,		// '\|'
			Oem6,		// ']}'
			Oem7,		// ''"'
			Oem8,
			OemPlus,	// '+'
			OemComma,	// ','
			OemMinus,	// '-'
			OemPeriod,	// '.'
			Back,
			Tab,
			Return,
			Pause,
			Escape,
			Space,
			Prior,
			Next,
			End,
			Home,
			Left,
			Up,
			Right,
			Down,
			Insert,
			Delete,
			NumLock,
			NumPad0,
			NumPad1,
			NumPad2,
			NumPad3,
			NumPad4,
			NumPad5,
			NumPad6,
			NumPad7,
			NumPad8,
			NumPad9,
			Multiply,
			Add,
			Separator,
			Subtract,
			Decimal,
			Divide,
			F1,
			F2,
			F3,
			F4,
			F5,
			F6,
			F7,
			F8,
			F9,
			F10,
			F11,
			F12,
			Shift,
			Control,
			Alt,
			LButton,
			RButton,
			MButton,
			Count,
			Undefined = 0xff
		};

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Basic input device
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
		class UR_DECL Device
		{
		public:

			Device(Input &input);

			~Device();

			virtual Result Initialize();

			virtual void Update();

			inline Input& GetInput();
		
		private:

			Input &input;
		};

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Keyboard state class
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
		class UR_DECL Keyboard : public Device
		{
		public:

			Keyboard(Input &input);
			
			virtual ~Keyboard();

			virtual Result Initialize();

			inline ur_uint GetInputQueueSize();

			inline KeyCode GetInputQueueChar(ur_uint idx);

			inline KeyCode GetKeyCode(VKey vkey);

			inline KeyState GetKeyState(VKey vkey);

			inline bool IsKeyUp(VKey vkey);

			inline bool IsKeyDown(VKey vkey);

			inline bool IsKeyReleased(VKey vkey);

		protected:

			static const ur_uint InputQueueMaxSize = 256;

			KeyState vkeyState[0xff];
			KeyCode vkeyToCode[0xff];
			VKey codeToVKey[0xff];
			KeyCode inputQueue[InputQueueMaxSize];
			ur_uint inputQueueSize;
		};

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Mouse state class
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
		class UR_DECL Mouse : public Device
		{
		public:

			Mouse(Input &input);

			virtual ~Mouse();

			virtual Result Initialize();

			inline const ur_int2& GetPos() const;

			inline KeyState GetLBState() const;
			
			inline KeyState GetMBState() const;
			
			inline KeyState GetRBState() const;
			
			inline ur_int GetWheelDelta() const;

		protected:

			ur_int2 pos;
			KeyState lbState;
			KeyState mbState;
			KeyState rbState;
			ur_int wheelDelta;
		};

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Gamepad state class
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
		class UR_DECL Gamepad : public Device
		{
		public:

			static const ur_uint MaxConnected = 4;

			Gamepad(Input &input);

			virtual ~Gamepad();

			virtual Result Initialize();

			// todo: not implemented yet
		};


		Input(Realm &realm);

		virtual ~Input();

		virtual Result Initialize();

		virtual void Update();

		inline Keyboard* GetKeyboard() const;

		inline Mouse* GetMouse() const;

		inline Gamepad* GetGamepad(ur_uint idx) const;

	protected:

		std::unique_ptr<Keyboard> keyboard;
		std::unique_ptr<Mouse> mouse;
		std::unique_ptr<Gamepad> gamepad[Gamepad::MaxConnected];
	};

} // end namespace UnlimRealms

#include "Sys/Input.inline.h"