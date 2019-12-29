///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ImguiRender/ImguiRender.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/Input.h"
#include "Sys/Canvas.h"
#include "Resources/Resources.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ImguiRender
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct VertexTransformCB
	{
		ur_float4x4 viewProj;
	};

	ImguiRender::ImguiRender(Realm &realm) :
		RealmEntity(realm)
	{
		#if defined(UR_GRAF)
		this->grafRenderer = ur_null;
		#endif
	}

	ImguiRender::~ImguiRender()
	{
		ImGui::Shutdown();
	}

	void ImguiRender::ReleaseGfxObjects()
	{
		#if defined(UR_GRAF)
		
		this->grafBindingTables.clear();
		this->grafCBs.clear();
		this->grafVS.reset();
		this->grafPS.reset();
		this->grafVB.reset();
		this->grafIB.reset();
		this->grafSampler.reset();
		this->grafFontImage.reset();
		this->grafBindingLayout.reset();
		this->grafPipeline.reset();
		this->grafRenderer = ur_null;

		#else
		
		gfxVS.reset(ur_null);
		gfxPS.reset(ur_null);
		gfxInputLayout.reset(ur_null);
		gfxPipelineState.reset(ur_null);
		gfxFontsTexture.reset(ur_null);
		gfxCB.reset(ur_null);
		gfxVB.reset(ur_null);
		
		#endif
	}

	void ImguiRender::InitKeyMapping()
	{
		ImGuiIO &io = ImGui::GetIO();
		Input::Keyboard *kb = this->GetRealm().GetInput()->GetKeyboard();
		if (kb != ur_null)
		{
			io.KeyMap[ImGuiKey_Tab] = (int)kb->GetKeyCode(Input::VKey::Tab);
			io.KeyMap[ImGuiKey_LeftArrow] = (int)kb->GetKeyCode(Input::VKey::Left);
			io.KeyMap[ImGuiKey_RightArrow] = (int)kb->GetKeyCode(Input::VKey::Right);
			io.KeyMap[ImGuiKey_UpArrow] = (int)kb->GetKeyCode(Input::VKey::Up);
			io.KeyMap[ImGuiKey_DownArrow] = (int)kb->GetKeyCode(Input::VKey::Down);
			io.KeyMap[ImGuiKey_PageUp] = (int)kb->GetKeyCode(Input::VKey::Prior);
			io.KeyMap[ImGuiKey_PageDown] = (int)kb->GetKeyCode(Input::VKey::Next);
			io.KeyMap[ImGuiKey_Home] = (int)kb->GetKeyCode(Input::VKey::Home);
			io.KeyMap[ImGuiKey_End] = (int)kb->GetKeyCode(Input::VKey::End);
			io.KeyMap[ImGuiKey_Delete] = (int)kb->GetKeyCode(Input::VKey::Delete);
			io.KeyMap[ImGuiKey_Backspace] = (int)kb->GetKeyCode(Input::VKey::Back);
			io.KeyMap[ImGuiKey_Enter] = (int)kb->GetKeyCode(Input::VKey::Return);
			io.KeyMap[ImGuiKey_Escape] = (int)kb->GetKeyCode(Input::VKey::Escape);
		}
	}

	#if !defined(UR_GRAF)
	Result ImguiRender::Init()
	{
		Result res = Result(Success);

		// release previous objects

		ReleaseGfxObjects();

		// VS
		res = CreateVertexShaderFromFile(this->GetRealm(), this->gfxVS, "Imgui_vs.cso");
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to initialize VS");

		// PS
		res = CreatePixelShaderFromFile(this->GetRealm(), this->gfxPS, "Imgui_ps.cso");
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to initialize PS");

		// Input Layout
		res = this->GetRealm().GetGfxSystem()->CreateInputLayout(this->gfxInputLayout);
		if (Succeeded(res))
		{
			GfxInputElement elements[] = {
				{ GfxSemantic::Position, 0, 0, GfxFormat::R32G32, GfxFormatView::Float, 0 },
				{ GfxSemantic::TexCoord, 0, 0, GfxFormat::R32G32, GfxFormatView::Float, 0 },
				{ GfxSemantic::Color, 0, 0, GfxFormat::R8G8B8A8, GfxFormatView::Unorm, 0 }
			};
			res = this->gfxInputLayout->Initialize(*this->gfxVS.get(), elements, ur_array_size(elements));
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to initialize input layout");

		// Fonts Texture
		ImGui::GetIO().Fonts->TexID = ur_null;
		res = this->GetRealm().GetGfxSystem()->CreateTexture(this->gfxFontsTexture);
		if (Succeeded(res))
		{
			unsigned char* pixels;
			int width, height;
			ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
			ImGui::GetIO().Fonts->TexID = (void*)this->gfxFontsTexture.get();

			GfxResourceData gfxTexData;
			gfxTexData.Ptr = pixels;
			gfxTexData.RowPitch = (ur_uint)width * 4;
			gfxTexData.SlicePitch = 0;

			GfxTextureDesc gfxTexDesc;
			gfxTexDesc.Width = (ur_uint)width;
			gfxTexDesc.Height = (ur_uint)height;
			gfxTexDesc.Levels = 1;
			gfxTexDesc.Format = GfxFormat::R8G8B8A8;
			gfxTexDesc.FormatView = GfxFormatView::Unorm;
			gfxTexDesc.Usage = GfxUsage::Default;
			gfxTexDesc.BindFlags = (ur_uint)GfxBindFlag::ShaderResource;
			gfxTexDesc.AccessFlags = 0;

			res = this->gfxFontsTexture->Initialize(gfxTexDesc, &gfxTexData);
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to initialize fonts texture");
		ImGui::GetIO().Fonts->TexID = (void*)this->gfxFontsTexture.get();

		// Constant Buffer
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxCB);
		if (Succeeded(res))
		{
			res = this->gfxCB->Initialize(sizeof(VertexTransformCB), 0, GfxUsage::Dynamic,
				(ur_uint)GfxBindFlag::ConstantBuffer, (ur_uint)GfxAccessFlag::Write);
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to initialize constant buffer");

		// Vertex Buffer
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxVB);
		if (Succeeded(res))
		{
			res = this->gfxVB->Initialize(5000 * sizeof(ImDrawVert), sizeof(ImDrawVert), GfxUsage::Dynamic,
				(ur_uint)GfxBindFlag::VertexBuffer, (ur_uint)GfxAccessFlag::Write);
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to initialize vertex buffer");

		// Index Buffer
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxIB);
		if (Succeeded(res))
		{
			res = this->gfxIB->Initialize(10000 * sizeof(ImDrawIdx), sizeof(ImDrawIdx), GfxUsage::Dynamic,
				(ur_uint)GfxBindFlag::IndexBuffer, (ur_uint)GfxAccessFlag::Write);
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to initialize index buffer");

		// Pipeline State
		res = this->GetRealm().GetGfxSystem()->CreatePipelineState(this->gfxPipelineState);
		if (Succeeded(res))
		{
			this->gfxPipelineState->PrimitiveTopology = GfxPrimitiveTopology::TriangleList;
			this->gfxPipelineState->InputLayout = this->gfxInputLayout.get();
			this->gfxPipelineState->VertexShader = this->gfxVS.get();
			this->gfxPipelineState->PixelShader = this->gfxPS.get();

			GfxRenderState gfxState = GfxRenderState::Default;

			gfxState.BlendState[0].BlendEnable = true;
			gfxState.BlendState[0].SrcBlend = GfxBlendFactor::SrcAlpha;
			gfxState.BlendState[0].DstBlend = GfxBlendFactor::InvSrcAlpha;
			gfxState.BlendState[0].BlendOp = GfxBlendOp::Add;
			gfxState.BlendState[0].SrcBlendAlpha = GfxBlendFactor::InvSrcAlpha;
			gfxState.BlendState[0].DstBlendAlpha = GfxBlendFactor::Zero;
			gfxState.BlendState[0].BlendOpAlpha = GfxBlendOp::Add;

			gfxState.RasterizerState.CullMode = GfxCullMode::None;
			gfxState.RasterizerState.ScissorEnable = true;

			gfxState.DepthStencilState.DepthEnable = false;
			gfxState.DepthStencilState.DepthFunc = GfxCmpFunc::Always;
			gfxState.DepthStencilState.StencilEnable = false;

			gfxState.SamplerState[0].MinFilter = GfxFilter::Linear;
			gfxState.SamplerState[0].MagFilter = GfxFilter::Linear;
			gfxState.SamplerState[0].MipFilter = GfxFilter::Linear;
			gfxState.SamplerState[0].AddressU = GfxTextureAddressMode::Wrap;
			gfxState.SamplerState[0].AddressV = GfxTextureAddressMode::Wrap;
			gfxState.SamplerState[0].AddressW = GfxTextureAddressMode::Wrap;
			gfxState.SamplerState[0].MipLodMin = 0.0f;
			gfxState.SamplerState[0].MipLodMax = 0.0f;
			gfxState.SamplerState[0].MipLodBias = 0.0f;
			gfxState.SamplerState[0].CmpFunc = GfxCmpFunc::Never;

			res = this->gfxPipelineState->SetRenderState(gfxState);
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to initialize pipeline state");

		// Keyboard mapping
		this->InitKeyMapping();

		return res;
	}
	#endif

	Result ImguiRender::NewFrame()
	{
		ImGuiIO &imguiIO = ImGui::GetIO();

		imguiIO.MouseDrawCursor = true;

		Input::Mouse *mouse = this->GetRealm().GetInput()->GetMouse();
		if (mouse != ur_null)
		{
			imguiIO.MouseDown[0] = (mouse->GetLBState() == Input::KeyState::Down);
			imguiIO.MouseDown[1] = (mouse->GetRBState() == Input::KeyState::Down);
			imguiIO.MouseDown[2] = (mouse->GetMBState() == Input::KeyState::Down);
			imguiIO.MousePos.x = (float)mouse->GetPos().x;
			imguiIO.MousePos.y = (float)mouse->GetPos().y;
			if (mouse->GetWheelDelta() != 0) imguiIO.MouseWheel += (mouse->GetWheelDelta() > 0 ? +1.0f : -1.0f);
		}

		Input::Keyboard *keyboard = this->GetRealm().GetInput()->GetKeyboard();
		if (keyboard != ur_null)
		{
			for (ur_uint i = 0; i < (ur_uint)Input::VKey::Count; ++i)
			{
				Input::KeyCode kcode = keyboard->GetKeyCode(Input::VKey(i));
				if (kcode != Input::InvalidCode)
				{
					imguiIO.KeysDown[kcode] = keyboard->IsKeyDown(Input::VKey(i));
				}
			}
			imguiIO.KeyCtrl = keyboard->IsKeyDown(Input::VKey::Control);
			imguiIO.KeyShift = keyboard->IsKeyDown(Input::VKey::Shift);
			imguiIO.KeyAlt = keyboard->IsKeyDown(Input::VKey::Alt);
			imguiIO.KeySuper = false;
			for (ur_uint i = 0; i < keyboard->GetInputQueueSize(); ++i)
			{
				imguiIO.AddInputCharacter((ImWchar)keyboard->GetInputQueueChar(i));
			}
		}

		const RectI &canvasBound = this->GetRealm().GetCanvas()->GetClientBound();
		imguiIO.DisplaySize.x = (float)canvasBound.Width();
		imguiIO.DisplaySize.y = (float)canvasBound.Height();

		ClockTime timePointNow = Clock::now();
		if (this->timePoint == ClockTime(std::chrono::microseconds(0)))
			this->timePoint = timePointNow;

		auto deltaTime = ClockDeltaAs<std::chrono::microseconds>(timePointNow - this->timePoint);
		this->timePoint = timePointNow;
		imguiIO.DeltaTime = (float)deltaTime.count() * 1.0e-6f; // to seconds

		ImGui::NewFrame();

		return Result(Success);
	}

	Result ImguiRender::Render(GfxContext &gfxContext)
	{
	#if defined(UR_GRAF)
		return Result(NotImplemented);
	#else
		ImGui::Render();

		ImDrawData *drawData = ImGui::GetDrawData();

		if (ur_null == this->gfxVB || ur_null == this->gfxIB)
			return Result(NotInitialized);

		// prepare VB
		static const ur_uint VBReserveSize = 5000 * sizeof(ImDrawVert);
		ur_uint vbSizeRequired = (ur_uint)drawData->TotalVtxCount * sizeof(ImDrawVert);
		if (this->gfxVB->GetDesc().Size < vbSizeRequired)
		{
			GfxBufferDesc desc = this->gfxVB->GetDesc();
			desc.Size = vbSizeRequired + VBReserveSize;
			Result res = this->gfxVB->Initialize(desc, ur_null);
			if (Failed(res))
				return res;
		}

		// prepare IB
		static const ur_uint IBReserveSize = 10000 * sizeof(ImDrawIdx);
		ur_uint ibSizeRequired = (ur_uint)drawData->TotalIdxCount * sizeof(ImDrawIdx);
		if (this->gfxIB->GetDesc().Size < ibSizeRequired)
		{
			GfxBufferDesc desc = this->gfxIB->GetDesc();
			desc.Size = ibSizeRequired + IBReserveSize;
			Result res = this->gfxIB->Initialize(desc, ur_null);
			if (Failed(res))
				return res;
		}

		// fill VB
		auto updateVB = [&](GfxResourceData *dstData) -> Result
		{
			if (ur_null == dstData)
				return Result(InvalidArgs);
			ImDrawVert *imVertDst = (ImDrawVert*)dstData->Ptr;
			for (ur_int i = 0; i < (ur_int)drawData->CmdListsCount; i++)
			{
				const ImDrawList* cmdList = drawData->CmdLists[i];
				memcpy(imVertDst, &cmdList->VtxBuffer[0], cmdList->VtxBuffer.size() * sizeof(ImDrawVert));
				imVertDst += cmdList->VtxBuffer.size();
			}
			return Result(Success);
		};
		gfxContext.UpdateBuffer(this->gfxVB.get(), GfxGPUAccess::WriteDiscard, updateVB);

		// fill IB
		auto updateIB = [&](GfxResourceData *dstData) -> Result
		{
			if (ur_null == dstData)
				return Result(InvalidArgs);
			ImDrawIdx *imIdxDst = (ImDrawIdx*)dstData->Ptr;
			for (ur_int i = 0; i < (ur_int)drawData->CmdListsCount; i++)
			{
				const ImDrawList* cmdList = drawData->CmdLists[i];
				memcpy(imIdxDst, &cmdList->IdxBuffer[0], cmdList->IdxBuffer.size() * sizeof(ImDrawIdx));
				imIdxDst += cmdList->IdxBuffer.size();
			}
			return Result(Success);
		};
		gfxContext.UpdateBuffer(this->gfxIB.get(), GfxGPUAccess::WriteDiscard, updateIB);

		// fill CB
		VertexTransformCB cb;
		float L = 0.0f;
		float R = ImGui::GetIO().DisplaySize.x;
		float B = ImGui::GetIO().DisplaySize.y;
		float T = 0.0f;
		cb.viewProj =
		{
			{ 2.0f / (R - L),		0.0f,				0.0f,       0.0f },
			{ 0.0f,					2.0f / (T - B),     0.0f,       0.0f },
			{ 0.0f,					0.0f,				0.5f,       0.0f },
			{ (R + L) / (L - R),	(T + B) / (B - T),	0.5f,       1.0f },
		};
		GfxResourceData cbResData = { &cb, sizeof(VertexTransformCB), 0 };
		gfxContext.UpdateBuffer(this->gfxCB.get(), GfxGPUAccess::WriteDiscard, &cbResData, 0, sizeof(VertexTransformCB));

		// setup pipeline
		gfxContext.SetPipelineState(this->gfxPipelineState.get());
		gfxContext.SetConstantBuffer(this->gfxCB.get(), 0);
		gfxContext.SetVertexBuffer(this->gfxVB.get(), 0);
		gfxContext.SetIndexBuffer(this->gfxIB.get());

		// draw command lists
		ur_uint vbOfs = 0;
		ur_uint ibOfs = 0;
		for (ur_uint icl = 0; icl < (ur_uint)drawData->CmdListsCount; icl++)
		{
			const ImDrawList *cmdList = drawData->CmdLists[icl];
			for (ur_uint icb = 0; icb < (ur_uint)cmdList->CmdBuffer.size(); icb++)
			{
				const ImDrawCmd* pCmd = &cmdList->CmdBuffer[icb];
				if (pCmd->UserCallback)
				{
					pCmd->UserCallback(cmdList, pCmd);
				}
				else
				{
					const RectI r = { (ur_int)pCmd->ClipRect.x, (ur_int)pCmd->ClipRect.y, (ur_int)pCmd->ClipRect.z, (ur_int)pCmd->ClipRect.w };
					if (r.Area() > 0)
					{
						gfxContext.SetScissorRect(&r);
						gfxContext.SetTexture((GfxTexture*)pCmd->TextureId, 0);
						gfxContext.DrawIndexed(pCmd->ElemCount, ibOfs, vbOfs, 0, 0);
					}
				}
				ibOfs += (ur_uint)pCmd->ElemCount;
			}
			vbOfs += (ur_uint)cmdList->VtxBuffer.size();
		}

		return Result(Success);
	#endif
	}

	#if defined(UR_GRAF)
	Result ImguiRender::Init()
	{
		// release previous objects

		ReleaseGfxObjects();

		// validate GRAF objects
		Result res = Result(Success);
		this->grafRenderer = this->GetRealm().GetComponent<GrafRenderer>();
		if (ur_null == this->grafRenderer)
			return ResultError(Failure, "ImguiRender::Init: failed to initialize, invalid GrafRenderer");
		GrafSystem* grafSystem = this->grafRenderer->GetGrafSystem();
		GrafDevice* grafDevice = this->grafRenderer->GetGrafDevice();
		GrafRenderPass* grafRenderPass = this->grafRenderer->GetCanvasRenderPass();
		ur_uint frameCount = this->grafRenderer->GetRecordedFrameCount();

		// VS
		res = GrafUtils::CreateShaderFromFile(*grafDevice, "Imgui_vs.spv", GrafShaderType::Vertex, this->grafVS);
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to initialize VS");

		// PS
		res = GrafUtils::CreateShaderFromFile(*grafDevice, "Imgui_ps.spv", GrafShaderType::Pixel, this->grafPS);
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to initialize PS");

		// shader bindings layout
		res = grafSystem->CreateDescriptorTableLayout(this->grafBindingLayout);
		if (Succeeded(res))
		{
			GrafDescriptorRangeDesc grafBindingLayoutRanges[] = {
				{ GrafDescriptorType::ConstantBuffer, 0, 1 },
				{ GrafDescriptorType::Sampler, 0, 1 },
				{ GrafDescriptorType::Texture, 0, 1 },
			};
			GrafDescriptorTableLayoutDesc grafBindingLayoutDesc = {
				GrafShaderStageFlags((ur_uint)GrafShaderStageFlag::Vertex | (ur_uint)GrafShaderStageFlag::Pixel),
				grafBindingLayoutRanges, ur_array_size(grafBindingLayoutRanges)
			};
			res = this->grafBindingLayout->Initialize(grafDevice, { grafBindingLayoutDesc });
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to create descriptor table layout");

		// per frame binding tables
		this->grafBindingTables.resize(frameCount);
		for (ur_uint iframe = 0; iframe < frameCount; ++iframe)
		{
			res = grafSystem->CreateDescriptorTable(this->grafBindingTables[iframe]);
			if (Failed(res)) break;
			res = this->grafBindingTables[iframe]->Initialize(grafDevice, { this->grafBindingLayout.get() });
			if (Failed(res)) break;
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to create descriptor table(s)");

		// per frame constant buffers
		this->grafCBs.resize(frameCount);
		for (ur_uint iframe = 0; iframe < frameCount; ++iframe)
		{
			res = grafSystem->CreateBuffer(this->grafCBs[iframe]);
			if (Failed(res)) break;
			GrafBufferDesc bufferDesc;
			bufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::ConstantBuffer;
			bufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			bufferDesc.SizeInBytes = sizeof(VertexTransformCB);
			res = this->grafCBs[iframe]->Initialize(grafDevice, { bufferDesc });
			if (Failed(res)) break;
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to create CB(s)");

		// graphics pipeline configuration
		res = grafSystem->CreatePipeline(this->grafPipeline);
		if (Succeeded(res))
		{
			GrafShader* shaderStages[] = {
				this->grafVS.get(),
				this->grafPS.get()
			};
			GrafDescriptorTableLayout* bindingLayouts[] = {
				this->grafBindingLayout.get(),
			};
			GrafVertexElementDesc sampleVertexElements[] = {
				{ GrafFormat::R32G32_SFLOAT, 0 },
				{ GrafFormat::R32G32_SFLOAT, 8 },
				{ GrafFormat::R8G8B8A8_UNORM, 16 }
			};
			GrafVertexInputDesc vertexInputs[] = { {
				GrafVertexInputType::PerVertex, 0, sizeof(ImDrawVert),
				sampleVertexElements, ur_array_size(sampleVertexElements)
			} };
			GrafPipeline::InitParams pipelineParams = GrafPipeline::InitParams::Default;
			pipelineParams.RenderPass = grafRenderPass;
			pipelineParams.ShaderStages = shaderStages;
			pipelineParams.ShaderStageCount = ur_array_size(shaderStages);
			pipelineParams.DescriptorTableLayouts = bindingLayouts;
			pipelineParams.DescriptorTableLayoutCount = ur_array_size(bindingLayouts);
			pipelineParams.VertexInputDesc = vertexInputs;
			pipelineParams.VertexInputCount = ur_array_size(vertexInputs);
			res = this->grafPipeline->Initialize(grafDevice, pipelineParams);
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to create pipeline object");

		// reserve dynamic VB
		res = grafSystem->CreateBuffer(this->grafVB);
		if (Succeeded(res))
		{
			GrafBufferDesc bufferDesc;
			bufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::VertexBuffer;
			bufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			bufferDesc.SizeInBytes = 2 * (1 << 20);
			res = this->grafVB->Initialize(grafDevice, { bufferDesc });
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to create VB");

		// reserve dynamic IB
		res = grafSystem->CreateBuffer(this->grafIB);
		if (Succeeded(res))
		{
			GrafBufferDesc bufferDesc;
			bufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::IndexBuffer;
			bufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			bufferDesc.SizeInBytes = 2 * (1 << 20);
			res = this->grafIB->Initialize(grafDevice, { bufferDesc });
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to create IB");

		// default sampler
		res = grafSystem->CreateSampler(this->grafSampler);
		if (Succeeded(res))
		{
			res = this->grafSampler->Initialize(grafDevice, { GrafSamplerDesc::Default });
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to create sampler");

		// get font image data
		unsigned char* pixels;
		int width, height;
		ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		GrafImageDesc fontImageDesc = {};
		fontImageDesc.Type = GrafImageType::Tex2D;
		fontImageDesc.Format = GrafFormat::R8G8B8A8_UNORM;
		fontImageDesc.Size.x = (ur_uint)width;
		fontImageDesc.Size.y = (ur_uint)height;
		fontImageDesc.Size.z = 1;
		fontImageDesc.MipLevels = 1;
		fontImageDesc.Usage = (ur_uint)GrafImageUsageFlag::TransferDst | (ur_uint)GrafImageUsageFlag::ShaderInput;
		fontImageDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::GpuLocal;

		// create font image in gpu local memory
		res = grafSystem->CreateImage(this->grafFontImage);
		if (Succeeded(res))
		{
			res = this->grafFontImage->Initialize(grafDevice, { fontImageDesc });
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to create font image");
		ImGui::GetIO().Fonts->TexID = (void*)this->grafFontImage.get();

		// upload font data to gpu texture
		this->grafRenderer->Upload(pixels, width * height * 4, this->grafFontImage.get(), GrafImageState::ShaderRead);

		// Keyboard mapping
		this->InitKeyMapping();

		return Result(Success);
	}
	#endif

	Result ImguiRender::Render(GrafCommandList& grafCmdList)
	{
	#if !defined(UR_GRAF)
		return Result(NotImplemented);
	#else
		
		ImGui::Render();

		ImDrawData *drawData = ImGui::GetDrawData();

		ur_uint frameIdx = this->grafRenderer->GetCurrentFrameId();

		// prepare VB
		ur_uint vbSizeRequired = (ur_uint)drawData->TotalVtxCount * sizeof(ImDrawVert);
		if (this->grafVB->GetDesc().SizeInBytes < vbSizeRequired)
			return Result(OutOfMemory);

		// prepare IB
		ur_uint ibSizeRequired = (ur_uint)drawData->TotalIdxCount * sizeof(ImDrawIdx);
		if (this->grafIB->GetDesc().SizeInBytes < ibSizeRequired)
			return Result(OutOfMemory);

		// fill VB
		GrafWriteCallback updateVB = [&drawData](ur_byte* mappedDataPtr) -> Result
		{
			ImDrawVert *imVertDst = (ImDrawVert*)mappedDataPtr;
			for (ur_int i = 0; i < (ur_int)drawData->CmdListsCount; i++)
			{
				const ImDrawList* cmdList = drawData->CmdLists[i];
				memcpy(imVertDst, &cmdList->VtxBuffer[0], cmdList->VtxBuffer.size() * sizeof(ImDrawVert));
				imVertDst += cmdList->VtxBuffer.size();
			}
			return Result(Success);
		};
		this->grafVB->Write(updateVB);

		// fill IB
		GrafWriteCallback updateIB = [&drawData](ur_byte* mappedDataPtr) -> Result
		{
			ImDrawIdx *imIdxDst = (ImDrawIdx*)mappedDataPtr;
			for (ur_int i = 0; i < (ur_int)drawData->CmdListsCount; i++)
			{
				const ImDrawList* cmdList = drawData->CmdLists[i];
				memcpy(imIdxDst, &cmdList->IdxBuffer[0], cmdList->IdxBuffer.size() * sizeof(ImDrawIdx));
				imIdxDst += cmdList->IdxBuffer.size();
			}
			return Result(Success);
		};
		this->grafIB->Write(updateIB);

		// fill CB
		VertexTransformCB cb;
		float L = 0.0f;
		float R = ImGui::GetIO().DisplaySize.x;
		float B = ImGui::GetIO().DisplaySize.y;
		float T = 0.0f;
		cb.viewProj =
		{
			{ 2.0f / (R - L),		0.0f,				0.0f,		0.0f },
			{ 0.0f,					2.0f / (T - B),		0.0f,		0.0f },
			{ 0.0f,					0.0f,				0.5f,		0.0f },
			{ (R + L) / (L - R),	(T + B) / (B - T),	0.5f,		1.0f },
		};
		this->grafCBs[frameIdx]->Write((ur_byte*)&cb);
		
		// fill shader inputs
		this->grafBindingTables[frameIdx]->SetConstantBuffer(0, this->grafCBs[frameIdx].get());
		this->grafBindingTables[frameIdx]->SetSampledImage(0, this->grafFontImage.get(), this->grafSampler.get());

		// setup pipeline
		grafCmdList.BindPipeline(this->grafPipeline.get());
		grafCmdList.BindDescriptorTable(this->grafBindingTables[frameIdx].get(), this->grafPipeline.get());
		grafCmdList.BindVertexBuffer(this->grafVB.get(), 0);
		grafCmdList.BindIndexBuffer(this->grafIB.get(), GrafIndexType::UINT16);

		// draw command lists
		ur_uint vbOfs = 0;
		ur_uint ibOfs = 0;
		for (ur_uint icl = 0; icl < (ur_uint)drawData->CmdListsCount; icl++)
		{
			const ImDrawList *cmdList = drawData->CmdLists[icl];
			for (ur_uint icb = 0; icb < (ur_uint)cmdList->CmdBuffer.size(); icb++)
			{
				const ImDrawCmd* pCmd = &cmdList->CmdBuffer[icb];
				if (pCmd->UserCallback)
				{
					pCmd->UserCallback(cmdList, pCmd);
				}
				else
				{
					const RectI r = {
						std::max((ur_int)0, (ur_int)pCmd->ClipRect.x),
						std::max((ur_int)0, (ur_int)pCmd->ClipRect.y),
						(ur_int)pCmd->ClipRect.z,
						(ur_int)pCmd->ClipRect.w
					};
					if (r.Area() > 0)
					{
						grafCmdList.SetScissorsRect(r);
						grafCmdList.DrawIndexed(pCmd->ElemCount, 1, ibOfs, vbOfs, 0);
					}
				}
				ibOfs += (ur_uint)pCmd->ElemCount;
			}
			vbOfs += (ur_uint)cmdList->VtxBuffer.size();
		}

		return Result(Success);
	#endif
	}

} // end namespace UnlimRealms