///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"
#include "Sys/JobSystem.h"
#include "Sys/Log.h"
#include "Gfx/GfxSystem.h"
#include "GenericRender/GenericRender.h"
#include "Atmosphere/Atmosphere.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Terrain
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL Terrain : public RealmEntity, public NonCopyable
	{
	public:

		typedef ur_size InstanceHandle; // data access handle type
		static const InstanceHandle InvalidHandle = ~InstanceHandle(0);


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Base terrain sub system
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL SubSystem : public NonCopyable
		{
		public:

			SubSystem(Terrain &terrain);

			virtual ~SubSystem();

			class UR_DECL InstanceDesc {};

			Result Create(InstanceHandle& instanceHandle, const InstanceDesc &instanceDesc);

		protected:

			// SubSystem's per instance data
			class UR_DECL Instance {};

			Result AddInstance(InstanceHandle& instanceHandle, std::unique_ptr<Instance> &instance);
			
			Result RemoveInstance(const InstanceHandle& instanceHandle);


			Terrain &terrain;
			std::unordered_map<InstanceHandle, std::unique_ptr<Instance>> instances;
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Base data accessor/mutator
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL Data : public SubSystem
		{
		public:

			Data(Terrain &terrain) : SubSystem(terrain) {};

			virtual ~Data() {}
		};

		
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Procedural data generator
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL ProceduralData : public Data
		{
		public:

			ProceduralData(Terrain &terrain);

			virtual ~ProceduralData();

			class UR_DECL InstanceDesc : public SubSystem::InstanceDesc
			{
			public:

				ur_uint Width;
				ur_uint Height;
				ur_uint Seed;
			};

			Result Create(InstanceHandle& instanceHandle, const ProceduralData::InstanceDesc &desc);
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Base presentation
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL Presentation : public SubSystem
		{
		public:
			
			Presentation(Terrain &terrain) : SubSystem(terrain) {}

			virtual ~Presentation() {}

			virtual Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const Atmosphere *atmosphere);
		};

		
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Base presentation
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL SimpleGrid : public Presentation
		{
		public:

			SimpleGrid(Terrain &terrain);

			virtual ~SimpleGrid();

			class UR_DECL InstanceDesc : public SubSystem::InstanceDesc
			{
			public:

				BoundingBox Bound;
				ur_uint2 Resolution;
			};

			Result Create(InstanceHandle& instanceHandle, const SimpleGrid::InstanceDesc &desc);

			virtual Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const Atmosphere *atmosphere);

		protected:

			class UR_DECL Instance : public SubSystem::Instance
			{
			};

			struct GfxObjects
			{
				std::unique_ptr<GfxInputLayout> inputLayout;
				std::unique_ptr<GfxVertexShader> VS;
				std::unique_ptr<GfxPixelShader> PS;
				std::unique_ptr<GfxBuffer> CB;
				std::unique_ptr<GfxPipelineState> PLSColorPass;
			} gfxObjects;
		};


		Terrain(Realm &realm);
		
		virtual ~Terrain();

		Result Init();

		template <typename TSubSystem>
		Result RegisterSubSystem();

		template <typename TDataType, typename TPresentationType>
		Result Create(InstanceHandle &instanceHandle, const typename TDataType::InstanceDesc &dataDesc, const typename TPresentationType::InstanceDesc &presentationDesc);

		Result Update();

		Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const Atmosphere *atmosphere);

	protected:

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Per instance data
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL Instance
		{
		public:

			Instance() {}
			~Instance() {}

		private:
			InstanceHandle dataHandle;
			InstanceHandle presentationHandle;
			Data *data;
			Presentation *presentation;
		};

	private:

		// non copyable arrays
		std::unordered_map<std::type_index, std::unique_ptr<SubSystem>> dataSubSystems;
		std::unordered_map<std::type_index, std::unique_ptr<SubSystem>> presentationSubSystems;
		std::unordered_map<InstanceHandle, std::unique_ptr<Instance>> instances;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// template functions implementation
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename TSubSystem>
	Result Terrain::RegisterSubSystem()
	{
		static const bool isData = std::is_base_of<Terrain::Data, TSubSystem>();
		static const bool isPresentation = std::is_base_of<Terrain::Presentation, TSubSystem>();
		static const bool isSupportedType = isData || isPresentation;
		static_assert(isSupportedType, "Terrain::RegisterSubSystem: unsupported sub system type");

		static std::type_index subSystemTypeIdx = std::type_index(typeid(TSubSystem));
		if (isData)
		{
			this->dataSubSystems[subSystemTypeIdx].reset( static_cast<Terrain::SubSystem*>(new TSubSystem(*this)) );
		}
		else if (isPresentation)
		{
			this->presentationSubSystems[subSystemTypeIdx].reset( static_cast<Terrain::SubSystem*>(new TSubSystem(*this)) );
		}
		
		return Success;
	}

	template <typename TDataType, typename TPresentationType>
	Result Terrain::Create(InstanceHandle &instanceHandle, const typename TDataType::InstanceDesc &dataDesc, const typename TPresentationType::InstanceDesc &presentationDesc)
	{
		static_assert(std::is_base_of<Terrain::Data, TDataType>(), "Terrain::Init: TDataType is not a Terrain::Data");
		static_assert(std::is_base_of<Terrain::Presentation, TPresentationType>(), "Terrain::Init: TPresentationType is not a Terrain::Presentation");
		static std::type_index dataTypeIdx = std::type_index(typeid(TDataType));
		static std::type_index presentationTypeIdx = std::type_index(typeid(TPresentationType));

		auto dataEntry = this->dataSubSystems.find(dataTypeIdx);
		if (this->dataSubSystems.end() == dataEntry)
			return ResultError(InvalidArgs, "Terrain::Create: trying to create an instance of unregistered data type");
		auto presentationEntry = this->presentationSubSystems.find(presentationTypeIdx);
		if (this->presentationSubSystems.end() == presentationEntry)
			return ResultError(InvalidArgs, "Terrain::Create: trying to create an instance of unregistered presentation type");
				
		InstanceHandle dataHandle = InvalidHandle;
		TDataType &data = static_cast<TDataType&>(*dataEntry->second.get());
		data.Create(dataHandle, dataDesc);
		
		InstanceHandle presentationHandle = InvalidHandle;
		TPresentationType &presentation = static_cast<TPresentationType&>(*presentationEntry->second.get());
		presentation.Create(presentationHandle, presentationDesc);
		
		return Success;
	}

} // end namespace UnlimRealms