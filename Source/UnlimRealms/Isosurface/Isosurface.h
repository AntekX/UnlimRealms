///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"
#include "Gfx/GfxSystem.h"
#include "GenericRender/GenericRender.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL Isosurface : public RealmEntity
	{
	protected:


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Isosurface inner data block
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct UR_DECL Block
		{
			typedef ur_float ValueType;
			std::vector<ValueType> field;
			std::unique_ptr<GfxBuffer> gfxVB;
			std::unique_ptr<GfxBuffer> gfxIB;
		};
		
	public:


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Base isosurface sub system
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL SubSystem
		{
		private:

			// non copyable
			SubSystem(const SubSystem &v) : isosurface(v.isosurface) {};

		public:

			SubSystem(Isosurface &isosurface) : isosurface(isosurface) {};

			virtual ~SubSystem() {};

		protected:

			Isosurface &isosurface;
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Voxel data tree
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL AdaptiveVolume : public SubSystem, public Octree<Block>
		{
		public:

			struct Desc
			{
				BoundingBox Bound; // full volume bound
				ur_uint3 BlockResolution; // data block field dimensions (min 3x3x3)
				ur_float3 BlockSize; // most detailed octree node world size
				ur_float DetailLevelDistance; // distance at which most detailed blocks are expected
				ur_float PartitionProgression; // per level partition distance increase factor
			};

			struct LevelInfo
			{
				ur_float Distance;
			};

			static const ur_uint BorderSize = 1;

			AdaptiveVolume(Isosurface &isosurface, ur_uint depth = DefaultDepth, Handler handeler = OnNodeChanged);
			
			~AdaptiveVolume();

			Result Init(const Desc &desc);

			void PartitionByDistance(const ur_float3 &point);

			inline Isosurface& GetIsosurface() { return this->isosurface; }

			inline const Desc& GetDesc() const { return this->desc; }

			inline ur_uint GetLevelsCount() const { return (ur_uint)this->levelInfo.size(); }

			inline const LevelInfo& GetLevelInfo(ur_uint i) const { return this->levelInfo[i]; }

			inline const Vector3& GetPartitionPoint() const { return this->partitionPoint; }

		private:

			static void OnNodeChanged(Node* node, Event e);

			void PartitionByDistance(const ur_float3 &pos, AdaptiveVolume::Node *node);

			Desc desc;
			BoundingBox alignedBound;
			std::vector<LevelInfo> levelInfo;
			Vector3 partitionPoint;
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Abstract volume data loader
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL Loader : public SubSystem
		{
		public:

			Loader(Isosurface &isosurface);

			~Loader();

			virtual Result Load(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox);
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Procedural volume data generator
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL ProceduralGenerator : public Loader
		{
		public:

			enum class UR_DECL Algorithm
			{
				Fill,
				SphericalDistanceField,
				SimplexNoise
			};

			struct UR_DECL GenerateParams
			{
			};

			struct UR_DECL FillParams : GenerateParams
			{
				Block::ValueType internalValue;
				Block::ValueType externalValue;
			};

			struct UR_DECL SphericalDistanceFieldParams : GenerateParams
			{
				ur_float3 center;
				ur_float radius;
			};

			struct UR_DECL SimplexNoiseParams : GenerateParams
			{
			};

			
			ProceduralGenerator(Isosurface &isosurface, const Algorithm &algorithm, const GenerateParams &generateParams);

			~ProceduralGenerator();

			virtual Result Load(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox);

		private:

			static Result Fill(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox, const FillParams &params);

			static Result GenerateSphericalDistanceField(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox, const SphericalDistanceFieldParams &params);

			static Result GenerateSimplexNoise(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox, const SimplexNoiseParams &params);


			Algorithm algorithm;
			std::unique_ptr<GenerateParams> generateParams;
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Abstract isosurface triangulator
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL Triangulator : public SubSystem
		{
		public:

			Triangulator(Isosurface &isosurface);

			~Triangulator();

			virtual Result Construct(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox);
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// "Surface Net" triangulator implementation
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL SurfaceNet : public Triangulator
		{
		public:

			SurfaceNet(Isosurface &isosurface);

			~SurfaceNet();

			virtual Result Construct(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox);
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Hybrid triangulation approach
		// Subdivides volume box in to hierarchy of tetrahedra
		// Each tetrahedron is then divided into 4 hexahedra
		// Each hexahedron bounds the lattice used for surface mesh extraction
		//
		// TODO: tetrahedra based approach doesn't fit "blocky" build logic, where per block data can be independent and stores gfx resources.
		// Triangulator should be refactored into some abstract Mesh constructor, which manages gfx buffers and rendering
		// (in this new approach crnt version of SurfaceNet triangulator will have to duplicate blocks hierachy)
		//
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL HybridTetrahedra : public Triangulator
		{
		public:

			struct UR_DECL Tetrahedron
			{
				typedef ur_float3 Vertex;

				struct UR_DECL Face
				{
					ur_byte v0;
					ur_byte v1;
					ur_byte v2;
				};

				struct UR_DECL Edge
				{
					ur_byte v0;
					ur_byte v1;
				};

				Vertex vertices[4];
				Face faces[4];
				Edge edges[6];

				Tetrahedron *children[2];
				Tetrahedron *neighbors[4];
			};

			HybridTetrahedra(Isosurface &isosurface);

			~HybridTetrahedra();

			virtual Result Construct(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox);

		private:

			std::unique_ptr<Tetrahedron> root[6];
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Isosurface build process managing class
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL Builder : public SubSystem
		{
		public:

			Builder(Isosurface &isosurface);

			~Builder();

			void AddNode(AdaptiveVolume::Node *node);

			void RemoveNode(AdaptiveVolume::Node *node);

			virtual Result Build();

		private:

			std::set<AdaptiveVolume::Node*> newNodes;
			std::multimap<ur_float, AdaptiveVolume::Node*> priorityQueue;

			// temp: async build test implementation
			// todo: rewrite
			std::thread thread;
			struct BuildCacheEntry
			{
				enum class State
				{
					Idle,
					Working,
					Finished,
				};
				Block block;
				BoundingBox bbox;
				std::atomic<State> state;
			};
			BuildCacheEntry buildCache;

			static void BuildBlock(AdaptiveVolume *volume, BuildCacheEntry *cacheEntry);
		};


		Isosurface(Realm &realm);

		virtual ~Isosurface();

		Result Init(const AdaptiveVolume::Desc &desc, std::unique_ptr<Loader> loader, std::unique_ptr<Triangulator> triangulator);

		Result Update();

		Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj);

		inline AdaptiveVolume* GetVolume() const { return this->volume.get(); }

		inline Loader* GetLoader() const { return this->loader.get(); }

		inline Triangulator* GetTriangulator() const { return this->triangulator.get(); }

		inline Builder* GetBuilder() const { return this->builder.get(); }
	
	protected:

		Result RenderSurface(GfxContext &gfxContext, AdaptiveVolume::Node *volumeNode);

		Result CreateGfxObjects();

		void DrawDebugTreeBounds(const AdaptiveVolume::Node *node);

		void GatherStats();

		void GatherStats(const AdaptiveVolume::Node *node);

		struct GfxObjects
		{
			std::unique_ptr<GfxVertexShader> VS;
			std::unique_ptr<GfxPixelShader> PS;
			std::unique_ptr<GfxInputLayout> inputLayout;
			std::unique_ptr<GfxPipelineState> pipelineState;
			std::unique_ptr<GfxBuffer> CB;
		};

		struct CommonCB
		{
			ur_float4x4 viewProj;
		};

		struct Vertex
		{
			ur_float3 pos;
			ur_uint32 col;
		};

		typedef ur_uint32 Index;

		std::unique_ptr<AdaptiveVolume> volume;
		std::unique_ptr<Loader> loader;
		std::unique_ptr<Triangulator> triangulator;
		std::unique_ptr<Builder> builder;
		std::unique_ptr<GfxObjects> gfxObjects;
		GenericRender debugRender;
		ur_bool drawDebugBounds;
		ur_bool drawWireframe;

		struct Stat
		{
			ur_uint volumeNodes;
			ur_uint surfaceNodes;
			ur_uint verticesCount;
			ur_uint primitivesCount;
			ur_uint videoMemory;
			ur_uint sysMemory;
		} stats;
	};

} // end namespace UnlimRealms