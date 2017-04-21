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

		void DrawPrimitives(const PrimitiveType primType, const ur_uint primCount, const ur_uint *indices,
			const ur_uint pointsCount, const ur_float3 *points, const ur_float4 *colors);

		void DrawLine(const ur_float3 &from, const ur_float3 &to, const ur_float4 &color);

		void DrawPolyline(const ur_uint pointsCount, const ur_float3 *points, const ur_float4 &color);

		void DrawConvexPolygon(const ur_uint pointsCount, const ur_float3 *points, const ur_float4 &color);

		void DrawBox(const ur_float3 &bmin, const ur_float3 &bmax, const ur_float4 &color);

		Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj);

		Result RenderScreenQuad(GfxContext &gfxContext, GfxTexture *texture, const ur_float4x4 *transform = ur_null,
			GfxRenderState *customRenderState = ur_null,
			GfxPixelShader *customPixelShader = ur_null,
			GfxBuffer *customConstBufferSlot1 = ur_null);

		Result RenderScreenQuad(GfxContext &gfxContext, GfxTexture *texture, const RectF &rect,
			GfxRenderState *customRenderState = ur_null,
			GfxPixelShader *customPixelShader = ur_null,
			GfxBuffer *customConstBufferSlot1 = ur_null);

		const GfxRenderState& GetDefaultQuadRenderState() const { return DefaultQuadRenderState; }

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
			std::unique_ptr<GfxBuffer> quadVB;
			std::unique_ptr<GfxPipelineState> quadState;
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
			ur_uint verticesCount;
			ur_uint indicesCount;

			inline void reserveVertices(ur_uint size)
			{
				if ((ur_uint)this->vertices.size() < verticesCount + size) this->vertices.resize(verticesCount + size);
				verticesCount += size;
			}

			inline void reserveIndices(ur_uint size)
			{
				if ((ur_uint)this->indices.size() < indicesCount + size) this->indices.resize(indicesCount + size);
				indicesCount += size;
			}
		};

	protected:

		static GfxRenderState DefaultQuadRenderState;

		Result CreateGfxObjects();

		Batch* FindBatch(PrimitiveType primType);

		std::unique_ptr<GfxObjects> gfxObjects;
		Batch batches[(ur_size)PrimitiveType::Count];
	};

} // end namespace UnlimRealms