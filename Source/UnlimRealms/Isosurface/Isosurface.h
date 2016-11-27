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
		// Isosurface volume data block
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct UR_DECL Block
		{
			typedef ur_float ValueType;
			std::vector<ValueType> field;
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
		// Abstract isosurface presentation
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL Presentation : public SubSystem
		{
		public:

			Presentation(Isosurface &isosurface);

			~Presentation();

			virtual Result Construct(AdaptiveVolume &volume);

			virtual Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj);
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// "Surface Net" presentation implementation
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL SurfaceNet : public Presentation
		{
		public:

			SurfaceNet(Isosurface &isosurface);

			~SurfaceNet();

			virtual Result Construct(AdaptiveVolume &volume);

			virtual Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj);

		private:

			struct UR_DECL MeshBlock
			{
				bool initialized;
				std::unique_ptr<GfxBuffer> gfxVB;
				std::unique_ptr<GfxBuffer> gfxIB;

				MeshBlock() : initialized(false) {}
			};

			typedef Octree<MeshBlock> MeshTree;

			Result Construct(AdaptiveVolume &volume, AdaptiveVolume::Node *volumeNode, MeshTree::Node *meshNode);

			Result Construct(AdaptiveVolume &volume, const BoundingBox &bbox, Block &block, MeshBlock &mesh);

			Result Render(GfxContext &gfxContext, MeshTree::Node *meshNode);


			std::unique_ptr<MeshTree> tree;


			struct Stat
			{
				ur_uint nodes;
				ur_uint verticesCount;
				ur_uint primitivesCount;
				ur_uint videoMemory;
			} stats;

			void DrawTreeBounds(const MeshTree::Node *node);

			void DrawStats();

			void GatherStats(const MeshTree::Node *node);
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Hybrid triangulation approach
		// Subdivides volume box in to hierarchy of tetrahedra
		// Each tetrahedron is then divided into 4 hexahedra
		// Each hexahedron bounds the lattice used for surface mesh extraction
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL HybridTetrahedra : public Presentation
		{
		public:

			HybridTetrahedra(Isosurface &isosurface);

			~HybridTetrahedra();

			virtual Result Construct(AdaptiveVolume &volume);

			virtual Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj);

		private:

			// TODO

			struct UR_DECL Tetrahedron
			{
				struct UR_DECL Edge
				{
					ur_byte vid[2];
				};
				static const ur_uint EdgesCount = 6;
				static const Edge Edges[EdgesCount];

				struct UR_DECL Face
				{
					ur_byte vid[3];
					ur_byte eid[3];
				};
				static const ur_uint FacesCount = 4;
				static const Face Faces[FacesCount];

				typedef ur_float3 Vertex;
				static const ur_uint VerticesCount = 4;

				Vertex vertices[VerticesCount];
				ur_byte longestEdgeIdx;
				Tetrahedron *faceNeighbors[FacesCount];

				static const ur_uint ChildrenCount = 2;
				std::unique_ptr<Tetrahedron> children[ChildrenCount];

				struct UR_DECL SplitInfo
				{
					ur_byte subTetrahedra[ChildrenCount][VerticesCount];
				};
				static const SplitInfo EdgeSplitInfo[EdgesCount];

				Tetrahedron();

				void Init(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3);

				int LinkNeighbor(Tetrahedron *th);

				void Split();

				void Merge();

				inline bool HasChildren() const { return (this->children[0] != ur_null); }
			};

			float SmallestNodeSize(const BoundingBox &bbox, AdaptiveVolume::Node *volumeNode);

			Result Construct(AdaptiveVolume &volume, Tetrahedron *tetrahedron);
			
			void DrawTetrahedra(GfxContext &gfxContext, GenericRender &genericRender, Tetrahedron *tetrahedron);

			static const ur_uint RootsCount = 6;
			std::unique_ptr<Tetrahedron> root[RootsCount];
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
		};


		Isosurface(Realm &realm);

		virtual ~Isosurface();

		Result Init(const AdaptiveVolume::Desc &desc, std::unique_ptr<Loader> loader, std::unique_ptr<Presentation> presentation);

		Result Update();

		Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj);

		inline AdaptiveVolume* GetVolume() const { return this->volume.get(); }

		inline Loader* GetLoader() const { return this->loader.get(); }

		inline Presentation* GetPresentation() const { return this->presentation.get(); }

		inline Builder* GetBuilder() const { return this->builder.get(); }
	
	protected:

		Result CreateGfxObjects();

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
		std::unique_ptr<Presentation> presentation;
		std::unique_ptr<Builder> builder;
		std::unique_ptr<GfxObjects> gfxObjects;

		/*struct Stat
		{
			ur_uint volumeNodes;
			ur_uint surfaceNodes;
			ur_uint verticesCount;
			ur_uint primitivesCount;
			ur_uint videoMemory;
			ur_uint sysMemory;
		} stats;*/
	};

} // end namespace UnlimRealms