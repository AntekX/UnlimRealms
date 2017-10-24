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
	class UR_DECL Terrain : public RealmEntity
	{
	public:

		typedef ur_uint Handle; // data access handle type
		static const Handle InvalidHandle = ~Handle(0);


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Base terrain sub system
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL SubSystem
		{
		private:

			// non copyable
			SubSystem(const SubSystem &v) : terrain(v.terrain) {};

		public:

			SubSystem(Terrain &terrain) : terrain(terrain) {};

			virtual ~SubSystem() {};

			class UR_DECL InstanceDesc
			{
			};

			Result Create(Handle& instanceHandle, const InstanceDesc &instanceDesc) { return NotImplemented; }

		protected:

			// SubSystem's per instance data
			class UR_DECL Instance
			{
			};

			Terrain &terrain;
			std::unordered_map<Handle, std::unique_ptr<Instance>> instances;
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

			Result Create(Handle& instanceHandle, const ProceduralData::InstanceDesc &desc);
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Base presentation
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL Presentation : public SubSystem
		{
		public:
			
			Presentation(Terrain &terrain) : SubSystem(terrain) {}

			virtual ~Presentation() {}
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

			Result Create(Handle& instanceHandle, const SimpleGrid::InstanceDesc &desc);
		};


		Terrain(Realm &realm);
		
		virtual ~Terrain();

		Result Init();

		template <typename TSubSystem>
		Result RegisterSubSystem();

		template <typename TDataType, typename TPresentationType>
		Result Create(Handle &instanceHandle, const typename TDataType::InstanceDesc &dataDesc, const typename TPresentationType::InstanceDesc &presentationDesc);

	protected:

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Per instance data
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL Instance
		{
		public:

		private:
			Handle dataHandle;
			Handle presentationHandle;
			Data *data;
			Presentation *presentation;
		};

	private:

		std::unordered_map<std::type_index, std::unique_ptr<SubSystem>> subSystems;
		std::unordered_map<Handle, std::unique_ptr<Instance>> instances;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// template functions implementation
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename TSubSystem>
	Result Terrain::RegisterSubSystem()
	{
		static_assert(
			std::is_base_of<Terrain::Data, TSubSystem>() ||
			std::is_base_of<Terrain::Presentation, TSubSystem>(),
			"Terrain::RegisterSubSystem: unsupported sub system type");
		
		static std::type_index dataTypeIdx = std::type_index(typeid(TSubSystem));
		this->subSystems[dataTypeIdx] = static_cast<SubSystem*>(new TSubSystem(*this));

		return Success;
	}

	template <typename TDataType, typename TPresentationType>
	Result Terrain::Create(Handle &instanceHandle, const typename TDataType::InstanceDesc &dataDesc, const typename TPresentationType::InstanceDesc &presentationDesc)
	{
		static_assert(std::is_base_of<Terrain::Data, TDataType>(), "Terrain::Init: TDataType is not a Terrain::Data");
		static_assert(std::is_base_of<Terrain::Presentation, TPresentationType>(), "Terrain::Init: TPresentationType is not a Terrain::Presentation");
		static std::type_index dataTypeIdx = std::type_index(typeid(TDataType));
		static std::type_index presentationTypeIdx = std::type_index(typeid(TPresentationType));

		auto dataEntry = this->subSystems[dataTypeIdx];
		if (this->subSystems.end() == dataEntry)
			return ResultError(InvalidArgs, "Terrain::Create: trying to create an instance of unregistered data type");
		auto presentationEntry = this->subSystems[dataTypeIdx];
		if (this->subSystems.end() == presentationEntry)
			return ResultError(InvalidArgs, "Terrain::Create: trying to create an instance of unregistered presentation type");
				
		Handle dataHandle = InvalidHandle;
		TDataType &data = static_cast<TDataType&>(*dataEntry);
		data.Create(dataHandle, dataDesc);
		
		Handle presentationHandle = InvalidHandle;
		TPresentationType &presentation = static_cast<TPresentationType&>(*presentationEntry);
		presentation.Create(presentationHandle, presentationDesc);
		
		return Success;
	}

} // end namespace UnlimRealms