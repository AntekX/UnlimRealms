///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"
#include "Gfx/GfxSystem.h"
#include "Graf/GrafRenderer.h"

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
			Line,
			Triangle,
			Count
		};

		typedef GfxPipelineState State;

		GenericRender(Realm &realm);

		~GenericRender();

		Result Init();

		void DrawPrimitives(const PrimitiveType primType, const ur_uint primCount, const ur_uint *indices,
			const ur_uint pointsCount, const ur_float3 *points, const ur_float4 *colors);

		void DrawLine(const ur_float3 &from, const ur_float3 &to, const ur_float4 &color);

		void DrawPolyline(const ur_uint pointsCount, const ur_float3 *points, const ur_float4 &color);

		void DrawConvexPolygon(const ur_uint pointsCount, const ur_float3 *points, const ur_float4 &color);

		void DrawWireBox(const ur_float3 &bmin, const ur_float3 &bmax, const ur_float4 &color);

		// Gfx API rendering functions

		Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj);

		Result RenderScreenQuad(GfxContext &gfxContext, GfxTexture *texture, const ur_float4x4 *transform = ur_null,
			State *customState = ur_null);

		Result RenderScreenQuad(GfxContext &gfxContext, GfxTexture *texture, const RectF &rect,
			State *customState = ur_null);

		Result CreateScreenQuadState(std::unique_ptr<State> &pipelineState,
			GfxPixelShader *customPS = ur_null, GfxRenderState *customRS = ur_null, ur_uint stencilRef = 0);

		const GfxRenderState& GetDefaultQuadRenderState() const { return DefaultQuadRenderState; }

		// GRAF rendering functions

		Result Init(GrafRenderPass* grafRenderPass);

		Result Render(GrafCommandList& grafCmdList, const ur_float4x4 &viewProj);

	protected:

		#if defined(UR_GRAF)
		struct GrafObjects
		{
			std::unique_ptr<GrafShader> VS;
			std::unique_ptr<GrafShader> PS;
			std::unique_ptr<GrafPipeline> pipeline[(ur_size)PrimitiveType::Count];
			std::unique_ptr<GrafDescriptorTableLayout> shaderDescriptorLayout;
			std::vector<std::unique_ptr<GrafDescriptorTable>> shaderDescriptorTable;
			std::unique_ptr<GrafBuffer> VB;
			std::unique_ptr<GrafBuffer> IB;
			std::unique_ptr<GrafSampler> sampler;
			std::unique_ptr<GrafImage> texture;
		};
		#else
		struct GfxObjects
		{
			std::unique_ptr<GfxVertexShader> VS;
			std::unique_ptr<GfxPixelShader> PS;
			std::unique_ptr<GfxInputLayout> inputLayout;
			std::unique_ptr<GfxBuffer> CB;
			std::unique_ptr<GfxBuffer> VB;
			std::unique_ptr<GfxBuffer> IB;
			std::unique_ptr<GfxTexture> atlas;
			std::unique_ptr<GfxBuffer> quadVB;
			std::unique_ptr<GfxPipelineState> pipelineState[(ur_size)PrimitiveType::Count];
			std::unique_ptr<GfxPipelineState> quadState;
		};
		#endif

		struct CommonCB
		{
			ur_float4x4 viewProj;
		};

		struct Vertex
		{
			ur_float3 pos;
			ur_uint32 col;
			ur_float2 tex;
			ur_float  psize;
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

		#if defined(UR_GRAF)
		Result CreateGrafObjects(GrafRenderPass* grafRenderPass);
		#else
		Result CreateGfxObjects();
		#endif
		static GfxRenderState DefaultQuadRenderState;

		Batch* FindBatch(PrimitiveType primType);

		Batch batches[(ur_size)PrimitiveType::Count];
		#if defined(UR_GRAF)
		GrafRenderer* grafRenderer;
		std::unique_ptr<GrafObjects> grafObjects;
		#else
		std::unique_ptr<GfxObjects> gfxObjects;
		#endif
	};

} // end namespace UnlimRealms