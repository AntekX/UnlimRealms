///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"
#include "Camera/Camera.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CameraControl
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL CameraControl : public RealmEntity
	{
	public:

		enum class Mode
		{
			Free,
			FixedUp,
			AroundPoint
		};

		CameraControl(Realm &realm, Camera *camera, Mode mode = Mode::Free);

		virtual ~CameraControl();

		void SetCamera(Camera *camera);

		void SetMode(Mode mode);

		void Update();

		void ShowImgui();

		inline Camera* GetCamera() const;

		inline void SetWorldUp(const ur_float3 v);
		inline const ur_float3& GetWorldUp() const;

		inline void SetTargetPoint(const ur_float3 p);
		inline const ur_float3& GetTargetPoint() const;

		inline void SetSpeed(ur_float speed);
		inline ur_float GetSpeed() const;
		
		inline void SetAngularSpeed(ur_float angularSpeed);
		inline ur_float GetAngularSpeed() const;

		inline void SetAcceleration(ur_float accel);
		inline ur_float GetAcceleration() const;

	private:

		Camera *camera;
		Mode mode;
		ur_float3 worldUp;
		ur_float3 targetPoint;
		ur_float speed;
		ur_float accel;
		ur_float angularSpeed;
		ur_float pointerSpeed;
		ur_int2 pointerPos;
		ClockTime time;
	};

} // end namespace UnlimRealms

#include "Camera/CameraControl.inline.h"