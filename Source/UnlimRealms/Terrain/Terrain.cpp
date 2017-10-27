///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Terrain/Terrain.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/Canvas.h"
#include "Sys/Input.h"
#include "Resources/Resources.h"
#include "ImguiRender/ImguiRender.h"

namespace UnlimRealms
{
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base terrain sub system
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Terrain::SubSystem::SubSystem(Terrain &terrain) :
		terrain(terrain)
	{

	}

	Terrain::SubSystem::~SubSystem()
	{
	
	}

	Result Terrain::SubSystem::Create(InstanceHandle& instanceHandle, const InstanceDesc &instanceDesc)
	{
        instanceHandle = InvalidHandle;
		return NotImplemented;
	}

	Result Terrain::SubSystem::AddInstance(InstanceHandle& instanceHandle, std::unique_ptr<Instance> &instance)
	{
		instanceHandle = InvalidHandle;
		if (ur_null == instance.get())
			return InvalidArgs;

		static ur_size InstanceHandleIdx = 0;
		instanceHandle = InstanceHandleIdx++;
		this->instances.insert(std::pair<InstanceHandle, std::unique_ptr<Instance>>(instanceHandle, std::move(instance)));
		
		return Success;
	}

	Result Terrain::SubSystem::RemoveInstance(const InstanceHandle& instanceHandle)
	{
		auto record = this->instances.find(instanceHandle);
		if (this->instances.end() == record)
			return NotFound;

		this->instances.erase(record);

		return Success;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Procedural data generator
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	Terrain::ProceduralData::ProceduralData(Terrain &terrain) : 
		Terrain::Data(terrain)
	{
	}

	Terrain::ProceduralData::~ProceduralData()
	{
	}

	Result Terrain::ProceduralData::Create(InstanceHandle& instanceHandle, const ProceduralData::InstanceDesc &desc)
	{
        // TODO
        return SubSystem::Create(instanceHandle, desc);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base presentation
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Result Terrain::Presentation::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const Atmosphere *atmosphere)
	{
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SimpleGrid
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Terrain::SimpleGrid::SimpleGrid(Terrain &terrain) :
		Terrain::Presentation(terrain)
	{
		this->CreateGfxObjects();
	}

	Terrain::SimpleGrid::~SimpleGrid()
	{
	}

	Result Terrain::SimpleGrid::CreateGfxObjects()
	{
		Result res = Result(Success);

		this->gfxObjects = GfxObjects(); // reset gfx resources

		Realm &realm = this->terrain.GetRealm();
		GfxObjects& objects = this->gfxObjects;

		// VS
		res = CreateVertexShaderFromFile(realm, objects.patchCommonVS, "TerrainPatch_vs.cso");
		if (Failed(res))
			return LogResult(Failure, realm.GetLog(), Log::Error, "Terrain::SimpleGrid::CreateGfxObjects: failed to initialize VS");

		// PS
		res = CreatePixelShaderFromFile(realm, objects.patchColorPS, "TerrainPatch_ps.cso");
		if (Failed(res))
			return LogResult(Failure, realm.GetLog(), Log::Error, "Terrain::SimpleGrid::CreateGfxObjects: failed to initialize PS");

		// Input Layout
		res = realm.GetGfxSystem()->CreateInputLayout(objects.patchIL);
		if (Succeeded(res))
		{
			GfxInputElement elements[] = {
				{ GfxSemantic::Position, 0, 0, GfxFormat::R32G32, GfxFormatView::Float, 0 }
			};
			res = objects.patchIL->Initialize(*objects.patchCommonVS.get(), elements, ur_array_size(elements));
		}
		if (Failed(res))
			return LogResult(Failure, realm.GetLog(), Log::Error, "Terrain::SimpleGrid::CreateGfxObjects: failed to initialize input layout");

		// Pipeline State
		res = realm.GetGfxSystem()->CreatePipelineState(objects.colorPassPLS);
		if (Succeeded(res))
		{
			objects.colorPassPLS->InputLayout = objects.patchIL.get();
			objects.colorPassPLS->VertexShader = objects.patchCommonVS.get();
			objects.colorPassPLS->PixelShader = objects.patchColorPS.get();
			GfxRenderState gfxRS = GfxRenderState::Default;
			res = objects.colorPassPLS->SetRenderState(gfxRS);
		}
		if (Failed(res))
			return LogResult(Failure, realm.GetLog(), Log::Error, "Terrain::SimpleGrid::CreateGfxObjects: failed to initialize pipeline state");

		// Constant Buffer
		res = realm.GetGfxSystem()->CreateBuffer(objects.patchCB);
		if (Succeeded(res))
		{
			res = objects.patchCB->Initialize(sizeof(PatchCB), GfxUsage::Dynamic,
				(ur_uint)GfxBindFlag::ConstantBuffer, (ur_uint)GfxAccessFlag::Write);
		}
		if (Failed(res))
			return LogResult(Failure, realm.GetLog(), Log::Error, "Terrain::SimpleGrid::CreateGfxObjects: failed to initialize constant buffer");

		return res;
	}

	Result Terrain::SimpleGrid::Create(InstanceHandle& instanceHandle, const SimpleGrid::InstanceDesc &desc)
	{
		std::unique_ptr<SubSystem::Instance> instance( new SimpleGrid::Instance());
		SimpleGrid::Instance *myInstance = static_cast<SimpleGrid::Instance*>(instance.get());
		myInstance->desc = desc;

		return this->AddInstance(instanceHandle, instance);
	}

	Result Terrain::SimpleGrid::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const Atmosphere *atmosphere)
	{
		// TODO
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Terrain
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Terrain::Terrain(Realm &realm) : 
		RealmEntity(realm)
	{

	}

	Terrain::~Terrain()
	{

	}

	Result Terrain::Init()
	{
		// nothing to do
		return Success;
	}

	Result Terrain::Update()
	{
		return Success;
	}

	Result Terrain::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const Atmosphere *atmosphere)
	{
		Result res = Success;

		for (auto &subSystem : this->presentationSubSystems)
		{
			Presentation *presentation = static_cast<Presentation*>(subSystem.second.get());
			res &= presentation->Render(gfxContext, viewProj, cameraPos, atmosphere);
		}

		return res;
	}

} // end namespace UnlimRealms