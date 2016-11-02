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
	// Camera
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL Camera : public RealmEntity
	{
	public:

		Camera(Realm &realm);

		virtual ~Camera();

		void SetProjection(ur_float minViewDist, ur_float maxViewDist, ur_float fieldOfView, ur_float aspectRatio);

		void SetAspectRatio(ur_float aspectRatio);

		void SetFrame(const ur_float4x4 &frame);

		void SetFrame(const ur_float3 &right, const ur_float3 &up, const ur_float3 &ahead);

		void SetPosition(const ur_float3 &pos);

		void Move(const ur_float3 &deltaPos);

		void MoveRight(const ur_float &delta);

		void MoveUp(const ur_float &delta);

		void MoveAhead(const ur_float &delta);

		void SetRotation(const ur_float3 &axis, const ur_float &angle);

		void Rotate(const ur_float3 &axis, const ur_float &delta);

		void Pitch(const ur_float &delta);

		void Yaw(const ur_float &delta);

		void Roll(const ur_float &delta);

		const ur_float4x4& GetView();

		const ur_float4x4& GetProj();

		const ur_float4x4& GetViewProj();

		inline ur_float GetMinViewDist() const;

		inline ur_float GetMaxViewDist() const;

		inline ur_float GetFieldOFView() const;

		inline ur_float GetAspectRatio() const;

		inline const ur_float4x4& GetFrame() const;

		inline const ur_float3& GetPosition() const;

		inline const ur_float3& GetDirection() const;

	private:

		ur_float minViewDist;
		ur_float maxViewDist;
		ur_float fieldOfView;
		ur_float aspectRatio;
		ur_float4x4 frame;
		ur_float4x4 view;
		ur_float4x4 proj;
		ur_float4x4 viewProj;
		bool viewChanged;
		bool projChanged;
	};

} // end namespace UnlimRealms

#include "Camera/Camera.inline.h"