///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GenericRender/GenericRender.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/Canvas.h"
#include "Resources/Resources.h"

namespace UnlimRealms
{

	GenericRender::GenericRender(Realm &realm) :
		RealmEntity(realm)
	{
		for (ur_uint ip = 0; ip < (ur_uint)PrimitiveType::Count; ++ip)
		{
			this->batches[ip].primitiveType = PrimitiveType(ip);
			this->batches[ip].verticesCount = 0;
			this->batches[ip].indicesCount = 0;
		}
	}

	GenericRender::~GenericRender()
	{

	}

	Result GenericRender::Init()
	{
		return CreateGfxObjects();
	}

	Result GenericRender::CreateGfxObjects()
	{
		Result res = Result(Success);

		this->gfxObjects.reset(new GfxObjects());

		// VS
		res = CreateVertexShaderFromFile(this->GetRealm(), this->gfxObjects->VS, "Generic_vs.cso");
		if (Failed(res))
			return ResultError(Failure, "GenericRender::CreateGfxObjects: failed to initialize VS");

		// PS
		res = CreatePixelShaderFromFile(this->GetRealm(), this->gfxObjects->PS, "Generic_ps.cso");
		if (Failed(res))
			return ResultError(Failure, "GenericRender::CreateGfxObjects: failed to initialize PS");

		// Input Layout
		res = this->GetRealm().GetGfxSystem()->CreateInputLayout(this->gfxObjects->inputLayout);
		if (Succeeded(res))
		{
			GfxInputElement elements[] = {
				{ GfxSemantic::Position, 0, 0, GfxFormat::R32G32B32, GfxFormatView::Float, 0 },
				{ GfxSemantic::Color, 0, 0, GfxFormat::R8G8B8A8, GfxFormatView::Unorm, 0 },
				{ GfxSemantic::TexCoord, 0, 0, GfxFormat::R32G32, GfxFormatView::Float, 0 },
			};
			res = this->gfxObjects->inputLayout->Initialize(*this->gfxObjects->VS.get(), elements, ur_array_size(elements));
		}
		if (Failed(res))
			return ResultError(Failure, "GenericRender::CreateGfxObjects: failed to initialize input layout");

		// Pipeline State
		for (ur_uint i = 0; i < (ur_size)PrimitiveType::Count; ++i)
		{
			std::unique_ptr<GfxPipelineState> gfxPipelineState;
			res = this->GetRealm().GetGfxSystem()->CreatePipelineState(gfxPipelineState);
			if (Succeeded(res))
			{
				gfxPipelineState->InputLayout = this->gfxObjects->inputLayout.get();
				gfxPipelineState->VertexShader = this->gfxObjects->VS.get();
				gfxPipelineState->PixelShader = this->gfxObjects->PS.get();

				GfxRenderState gfxState = GfxRenderState::Default;
				switch (PrimitiveType(i))
				{
				case PrimitiveType::Point: gfxState.PrimitiveTopology = GfxPrimitiveTopology::PointList; break;
				case PrimitiveType::Line: gfxState.PrimitiveTopology = GfxPrimitiveTopology::LineList; break;
				case PrimitiveType::Triangle: gfxState.PrimitiveTopology = GfxPrimitiveTopology::TriangleList; break;
				}
				gfxState.RasterizerState.CullMode = GfxCullMode::None;
				gfxState.DepthStencilState.DepthEnable = true;

				res = gfxPipelineState->SetRenderState(gfxState);

				this->gfxObjects->pipelineState[i] = std::move(gfxPipelineState);
			}
			if (Failed(res))
				return ResultError(Failure, "GenericRender::CreateGfxObjects: failed to initialize pipeline state");
		}

		// Atlas Texture
		res = this->GetRealm().GetGfxSystem()->CreateTexture(this->gfxObjects->atlas);
		if (Succeeded(res))
		{
			GfxTextureDesc gfxTexDesc;
			gfxTexDesc.Width = 1;
			gfxTexDesc.Height = 1;
			gfxTexDesc.Levels = 1;
			gfxTexDesc.Format = GfxFormat::R8G8B8A8;
			gfxTexDesc.FormatView = GfxFormatView::Unorm;
			gfxTexDesc.Usage = GfxUsage::Default;
			gfxTexDesc.BindFlags = (ur_uint)GfxBindFlag::ShaderResource;
			gfxTexDesc.AccessFlags = 0;

			GfxResourceData gfxTexData;
			ur_uint32 pixels[] = { 0xffffffff };
			gfxTexData.Ptr = pixels;
			gfxTexData.RowPitch = gfxTexDesc.Width * sizeof(*pixels);
			gfxTexData.SlicePitch = 0;

			res = this->gfxObjects->atlas->Initialize(gfxTexDesc, &gfxTexData);
		}
		if (Failed(res))
			return ResultError(Failure, "GenericRender::CreateGfxObjects: failed to initialize texture atlas");

		// Constant Buffer
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxObjects->CB);
		if (Succeeded(res))
		{
			res = this->gfxObjects->CB->Initialize(sizeof(CommonCB), GfxUsage::Dynamic,
				(ur_uint)GfxBindFlag::ConstantBuffer, (ur_uint)GfxAccessFlag::Write);
		}
		if (Failed(res))
			return ResultError(Failure, "GenericRender::CreateGfxObjects: failed to initialize constant buffer");

		// Vertex Buffer
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxObjects->VB);
		if (Succeeded(res))
		{
			res = this->gfxObjects->VB->Initialize(sizeof(Vertex), GfxUsage::Dynamic,
				(ur_uint)GfxBindFlag::VertexBuffer, (ur_uint)GfxAccessFlag::Write);
		}
		if (Failed(res))
			return ResultError(Failure, "GenericRender::CreateGfxObjects: failed to initialize vertex buffer");

		// Index Buffer
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxObjects->IB);
		if (Succeeded(res))
		{
			res = this->gfxObjects->IB->Initialize(sizeof(Index), GfxUsage::Dynamic,
				(ur_uint)GfxBindFlag::IndexBuffer, (ur_uint)GfxAccessFlag::Write);
		}
		if (Failed(res))
			return ResultError(Failure, "GenericRender::CreateGfxObjects: failed to initialize index buffer");

		// Quad Vertex Buffer
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxObjects->quadVB);
		if (Succeeded(res))
		{
			Vertex quadVertices[] = {
				{ { -1.0f, +1.0f, 0.0f }, 0xffffffff, { 0.0f, 0.0f } },
				{ { +1.0f, +1.0f, 0.0f }, 0xffffffff, { 1.0f, 0.0f } },
				{ { -1.0f, -1.0f, 0.0f }, 0xffffffff, { 0.0f, 1.0f } },
				{ { +1.0f, -1.0f, 0.0f }, 0xffffffff, { 1.0f, 1.0f } }
			};
			GfxResourceData gfxResData = { quadVertices, sizeof(quadVertices), 0 };
			res = this->gfxObjects->quadVB->Initialize(gfxResData.RowPitch, GfxUsage::Immutable,
				(ur_uint)GfxBindFlag::VertexBuffer, 0, &gfxResData);
		}
		if (Failed(res))
			return ResultError(Failure, "GenericRender::CreateGfxObjects: failed to initialize quad vertex buffer");

		// Quad Pipeline State
		{
			std::unique_ptr<GfxPipelineState> gfxPipelineState;
			res = this->GetRealm().GetGfxSystem()->CreatePipelineState(gfxPipelineState);
			if (Succeeded(res))
			{
				gfxPipelineState->InputLayout = this->gfxObjects->inputLayout.get();
				gfxPipelineState->VertexShader = this->gfxObjects->VS.get();
				gfxPipelineState->PixelShader = this->gfxObjects->PS.get();

				GfxRenderState gfxState = GfxRenderState::Default;
				gfxState.PrimitiveTopology = GfxPrimitiveTopology::TriangleStrip;
				gfxState.RasterizerState.CullMode = GfxCullMode::None;
				gfxState.DepthStencilState.DepthEnable = false;

				res = gfxPipelineState->SetRenderState(gfxState);

				this->gfxObjects->quadState = std::move(gfxPipelineState);
			}
		}
		if (Failed(res))
			return ResultError(Failure, "GenericRender::CreateGfxObjects: failed to initialize quad pipeline state");

		return res;
	}

	GenericRender::Batch* GenericRender::FindBatch(PrimitiveType primType)
	{
		Batch *batch = ((ur_size)primType < (ur_size)PrimitiveType::Count ? &this->batches[(ur_size)primType] : ur_null);

		return batch;
	}

	void GenericRender::DrawPrimitives(const PrimitiveType primType, const ur_uint primCount, const ur_uint *indices,
		const ur_uint pointsCount, const ur_float3 *points, const ur_float4 *colors)
	{
		ur_uint icount = 0;
		switch (primType)
		{
		case PrimitiveType::Point: icount = primCount;  break;
		case PrimitiveType::Line: icount = primCount * 2;  break;
		case PrimitiveType::Triangle: icount = primCount * 3;  break;
		default: return;
		}
		if (0 == icount || (ur_null == indices && primType != PrimitiveType::Point) ||
			0 == pointsCount || ur_null == points)
			return;

		Batch *batch = FindBatch(primType);
		if (ur_null == batch)
			return;

		ur_uint vofs = batch->verticesCount;
		batch->reserveVertices(pointsCount);
		Vertex *pv = batch->vertices.data() + vofs;
		static const ur_uint32 defaultColor = 0xffffffff;
		for (ur_uint i = 0; i < pointsCount; ++i, ++pv)
		{
			pv->pos = points[i];
			pv->col = (colors ? Vector4ToRGBA32(colors[i]) : defaultColor);
			pv->tex = 0;
		}

		ur_size iofs = batch->indicesCount;
		batch->reserveIndices(icount);
		Index *pi = batch->indices.data() + iofs;
		for (ur_uint i = 0; i < icount; ++i, ++pi)
		{
			*pi = Index((indices ? indices[i] : i) + vofs);
		}
	}

	void GenericRender::DrawLine(const ur_float3 &from, const ur_float3 &to, const ur_float4 &color)
	{
		Batch *batch = FindBatch(PrimitiveType::Line);
		if (ur_null == batch)
			return;

		ur_size vofs = batch->verticesCount;
		batch->reserveVertices(2);
		Vertex *pv = batch->vertices.data() + vofs;
		ur_uint32 colorUint32 = Vector4ToRGBA32(color);
		pv->pos = from;
		pv->col = colorUint32;
		pv->tex = 0;
		pv++;
		pv->pos = to;
		pv->col = colorUint32;
		pv->tex = 0;
		pv++;

		ur_size iofs = batch->indicesCount;
		batch->reserveIndices(2);
		Index *pi = batch->indices.data() + iofs;
		*pi++ = Index(vofs);
		*pi++ = Index(vofs + 1);
	}

	void GenericRender::DrawPolyline(const ur_uint pointsCount, const ur_float3 *points, const ur_float4 &color)
	{
		Batch *batch = FindBatch(PrimitiveType::Line);
		if (ur_null == batch)
			return;

		ur_uint primCount = (pointsCount > 0 ? pointsCount - 1 : 0);
		if (0 == primCount)
			return;

		ur_size vofs = batch->verticesCount;
		batch->reserveVertices(pointsCount);
		Vertex *pv = batch->vertices.data() + vofs;
		ur_uint32 colorUint32 = Vector4ToRGBA32(color);
		for (ur_uint iv = 0; iv < pointsCount; ++iv, ++pv)
		{
			pv->pos = points[iv];
			pv->col = colorUint32;
			pv->tex = 0;
		}

		ur_size iofs = batch->indicesCount;
		batch->reserveIndices(primCount * 2);
		Index *pi = batch->indices.data() + iofs;
		for (ur_uint ip = 0; ip < primCount; ++ip)
		{
			*pi++ = Index(vofs + ip);
			*pi++ = Index(vofs + ip + 1);
		}
	}

	void GenericRender::DrawConvexPolygon(const ur_uint pointsCount, const ur_float3 *points, const ur_float4 &color)
	{
		Batch *batch = FindBatch(PrimitiveType::Triangle);
		if (ur_null == batch)
			return;

		ur_uint primCount = (pointsCount > 2 ? pointsCount - 2 : 0);
		if (0 == primCount)
			return;

		ur_size vofs = batch->verticesCount;
		batch->reserveVertices(pointsCount);
		Vertex *pv = batch->vertices.data() + vofs;
		ur_uint32 colorUint32 = Vector4ToRGBA32(color);
		for (ur_uint iv = 0; iv < pointsCount; ++iv, ++pv)
		{
			pv->pos = points[iv];
			pv->col = colorUint32;
			pv->tex = 0;
		}

		ur_size iofs = batch->indicesCount;
		batch->reserveIndices(primCount * 3);
		Index *pi = batch->indices.data() + iofs;
		for (ur_uint ip = 0; ip < primCount; ++ip)
		{
			*pi++ = Index(vofs);
			*pi++ = Index(vofs + ip + 1);
			*pi++ = Index(vofs + ip + 2);
		}
	}

	void GenericRender::DrawBox(const ur_float3 &bmin, const ur_float3 &bmax, const ur_float4 &color)
	{
		Batch *batch = FindBatch(PrimitiveType::Line);
		if (ur_null == batch)
			return;

		static const ur_uint pointsCount = 8;
		ur_float3 points[pointsCount] = {
			{ bmin.x, bmin.y, bmin.z },
			{ bmax.x, bmin.y, bmin.z },
			{ bmin.x, bmax.y, bmin.z },
			{ bmax.x, bmax.y, bmin.z },
			{ bmin.x, bmin.y, bmax.z },
			{ bmax.x, bmin.y, bmax.z },
			{ bmin.x, bmax.y, bmax.z },
			{ bmax.x, bmax.y, bmax.z }
		};

		static const ur_uint primCount = 12;
		static const ur_uint indicesCount = primCount * 2;
		static const Index indices[indicesCount] = {
			0,1, 1,3, 3,2, 2,0,
			4,5, 5,7, 7,6, 6,4,
			0,4, 1,5, 2,6, 3,7
		};

		ur_size vofs = batch->verticesCount;
		batch->reserveVertices(pointsCount);
		Vertex *pv = batch->vertices.data() + vofs;
		ur_uint32 colorUint32 = Vector4ToRGBA32(color);
		for (ur_uint iv = 0; iv < pointsCount; ++iv, ++pv)
		{
			pv->pos = points[iv];
			pv->col = colorUint32;
			pv->tex = 0;
		}
		
		ur_size iofs = batch->indicesCount;
		batch->reserveIndices(indicesCount);
		Index *pi = batch->indices.data() + iofs;
		for (ur_uint i = 0; i < indicesCount; ++i, ++pi)
		{
			*pi = Index(indices[i] + vofs);
		}
	}

	Result GenericRender::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj)
	{
		if (ur_null == this->gfxObjects ||
			ur_null == this->gfxObjects->VB ||
			ur_null == this->gfxObjects->IB ||
			ur_null == this->gfxObjects->CB)
			return Result(NotInitialized);

		ur_uint totalVertices = 0;
		ur_uint totalIndices = 0;
		for (ur_uint i = 0; i < (ur_uint)PrimitiveType::Count; ++i)
		{
			totalVertices += this->batches[i].verticesCount;
			totalIndices += this->batches[i].indicesCount;
		}

		// prepare VB
		ur_uint vbSizeRequired = totalVertices * sizeof(Vertex);
		if (this->gfxObjects->VB->GetDesc().Size < vbSizeRequired)
		{
			GfxBufferDesc desc = this->gfxObjects->VB->GetDesc();
			desc.Size = vbSizeRequired;
			Result res = this->gfxObjects->VB->Initialize(desc, ur_null);
			if (Failed(res))
				return res;
		}
		auto updateVB = [&](GfxResourceData *dstData) -> Result
		{
			if (ur_null == dstData)
				return Result(InvalidArgs);
			ur_byte *pdst = (ur_byte*)dstData->Ptr;
			for (ur_uint i = 0; i < (ur_uint)PrimitiveType::Count; ++i)
			{
				ur_size batchSize = this->batches[i].verticesCount * sizeof(Vertex);
				memcpy(pdst, this->batches[i].vertices.data(), batchSize);
				pdst += batchSize;
			}
			return Result(Success);
		};
		gfxContext.UpdateBuffer(this->gfxObjects->VB.get(), GfxGPUAccess::WriteDiscard, false, updateVB);

		// prepare IB
		ur_uint ibSizeRequired = totalIndices * sizeof(Index);
		if (this->gfxObjects->IB->GetDesc().Size < ibSizeRequired)
		{
			GfxBufferDesc desc = this->gfxObjects->IB->GetDesc();
			desc.Size = ibSizeRequired;
			Result res = this->gfxObjects->IB->Initialize(desc, ur_null);
			if (Failed(res))
				return res;
		}
		auto updateIB = [&](GfxResourceData *dstData) -> Result
		{
			if (ur_null == dstData)
				return Result(InvalidArgs);
			ur_byte *pdst = (ur_byte*)dstData->Ptr;
			for (ur_uint i = 0; i < (ur_uint)PrimitiveType::Count; ++i)
			{
				ur_size batchSize = this->batches[i].indicesCount * sizeof(Index);
				memcpy(pdst, this->batches[i].indices.data(), batchSize);
				pdst += batchSize;
			}
			return Result(Success);
		};
		gfxContext.UpdateBuffer(this->gfxObjects->IB.get(), GfxGPUAccess::WriteDiscard, false, updateIB);

		// prepare CB
		CommonCB cb;
		cb.viewProj = viewProj;
		GfxResourceData cbResData = { &cb, sizeof(CommonCB), 0 };
		gfxContext.UpdateBuffer(this->gfxObjects->CB.get(), GfxGPUAccess::WriteDiscard, false, &cbResData, 0, cbResData.RowPitch);

		// draw batches
		gfxContext.SetConstantBuffer(this->gfxObjects->CB.get(), 0);
		gfxContext.SetVertexBuffer(this->gfxObjects->VB.get(), 0, sizeof(Vertex), 0);
		gfxContext.SetIndexBuffer(this->gfxObjects->IB.get(), sizeof(Index) * 8, 0);
		gfxContext.SetTexture(this->gfxObjects->atlas.get(), 0);
		ur_uint vbOfs = 0, ibOfs = 0;
		for (ur_uint i = 0; i < (ur_uint)PrimitiveType::Count; ++i)
		{
			gfxContext.SetPipelineState(this->gfxObjects->pipelineState[i].get());
			gfxContext.DrawIndexed((ur_uint)this->batches[i].indicesCount, ibOfs, vbOfs, 0, 0);
			vbOfs += (ur_uint)this->batches[i].verticesCount;
			ibOfs += (ur_uint)this->batches[i].indicesCount;
		}

		// reset batches
		for (ur_uint ip = 0; ip < (ur_uint)PrimitiveType::Count; ++ip)
		{
			this->batches[ip].verticesCount = 0;
			this->batches[ip].indicesCount = 0;
		}

		return Result(Success);
	}

	Result GenericRender::RenderScreenQuad(GfxContext &gfxContext, GfxTexture *texture,
		const ur_float4x4 *transform, GfxPixelShader *customPS, GfxBuffer *customCB1)
	{
		if (ur_null == this->gfxObjects ||
			ur_null == this->gfxObjects->quadVB ||
			ur_null == this->gfxObjects->quadState ||
			ur_null == this->gfxObjects->CB)
			return Result(NotInitialized);

		this->gfxObjects->quadState->PixelShader = (customPS != ur_null ? customPS : this->gfxObjects->PS.get());

		CommonCB cb;
		cb.viewProj = (transform != ur_null ? *transform : ur_float4x4::Identity);
		GfxResourceData cbResData = { &cb, sizeof(CommonCB), 0 };
		gfxContext.UpdateBuffer(this->gfxObjects->CB.get(), GfxGPUAccess::WriteDiscard, false, &cbResData, 0, cbResData.RowPitch);

		gfxContext.SetConstantBuffer(this->gfxObjects->CB.get(), 0);
		gfxContext.SetConstantBuffer(customCB1, 1);
		gfxContext.SetVertexBuffer(this->gfxObjects->quadVB.get(), 0, sizeof(Vertex), 0);
		gfxContext.SetTexture(texture != ur_null ? texture : this->gfxObjects->atlas.get(), 0);
		gfxContext.SetPipelineState(this->gfxObjects->quadState.get());
		gfxContext.Draw(4, 0, 0, 0);

		return Result(Success);
	}

	Result GenericRender::RenderScreenQuad(GfxContext &gfxContext, GfxTexture *texture, const RectF &rect,
		GfxPixelShader *customPS, GfxBuffer *customCB1)
	{
		GfxViewPort viewPort;
		Result res = gfxContext.GetViewPort(viewPort);
		if (Failed(res))
			return res;

		ur_float4x4 mx = ur_float4x4::Identity;
		mx.r[0][0] = rect.Width() / viewPort.Width;
		mx.r[1][1] = rect.Height() / viewPort.Height;
		mx.r[3][0] = rect.Min.x / viewPort.Width * 2.0f - 1.0f + mx.r[0][0];
		mx.r[3][1] = (1.0f - rect.Min.y / viewPort.Height) * 2.0f - 1.0f - mx.r[1][1];
		
		return this->RenderScreenQuad(gfxContext, texture, &mx, customPS, customCB1);
	}

} // end namespace UnlimRealms