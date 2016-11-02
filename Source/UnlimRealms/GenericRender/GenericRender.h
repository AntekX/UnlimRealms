///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"
#include "Gfx/GfxSystem.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Generic Primitives Immediate Mode Renderer
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GenericRender : public RealmEntity
	{
	public:

		enum class UR_DECL PrimitiveType
		{
			Point,
			Line,
			Triangle,
			Count
		};

		GenericRender(Realm &realm);

		~GenericRender();

		Result Init();

		void DrawPrimitives(PrimitiveType primType, ur_uint primCount, const ur_uint *indices,
			ur_uint pointsCount, const ur_float3 *points, const ur_float4 *colors);

		void DrawLine(ur_float3 from, ur_float3 to, ur_float4 color);

		void DrawPolyline(ur_uint pointsCount, const ur_float3 *points, ur_float4 color);

		void DrawConvexPolygon(ur_uint pointsCount, ur_float3 *points, ur_float4 color);

		void DrawBox(ur_float3 bmin, ur_float3 bmax, ur_float4 color);

		Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj);

	protected:

		struct GfxObjects
		{
			std::unique_ptr<GfxVertexShader> VS;
			std::unique_ptr<GfxPixelShader> PS;
			std::unique_ptr<GfxInputLayout> inputLayout;
			std::unique_ptr<GfxPipelineState> pipelineState[(ur_size)PrimitiveType::Count];
			std::unique_ptr<GfxBuffer> CB;
			std::unique_ptr<GfxBuffer> VB;
			std::unique_ptr<GfxBuffer> IB;
			std::unique_ptr<GfxTexture> atlas;
		};

		struct CommonCB
		{
			ur_float4x4 viewProj;
		};

		struct Vertex
		{
			ur_float3 pos;
			ur_uint32 col;
			ur_float2 tex;
		};

		typedef ur_uint32 Index;

		struct Batch
		{
			PrimitiveType primitiveType;
			std::vector<Vertex> vertices;
			std::vector<Index> indices;
		};

	protected:

		Result CreateGfxObjects();

		Batch* FindBatch(PrimitiveType primType);

		std::unique_ptr<GfxObjects> gfxObjects;
		Batch batches[(ur_size)PrimitiveType::Count];
	};

} // end namespace UnlimRealms