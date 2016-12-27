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
		// Blocks 3D array
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct UR_DECL BlockArray
		{
			BoundingBox bbox;
			ur_uint3 size;
			std::vector<Block*> blocks;
			ur_uint3 blockResolution;
			ur_uint blockBorder;
			ur_float3 blockSize;

			BlockArray() : size(0), blockResolution(0), blockBorder(0), blockSize(0.0f) {}

			Block::ValueType Sample(const ur_float3 &point) const;
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
				ur_float RefinementProgression; // per level refinement distance increase factor
			};

			struct LevelInfo
			{
				ur_float Distance;
			};

			static const ur_uint BorderSize = 1;

			AdaptiveVolume(Isosurface &isosurface, ur_uint depth = DefaultDepth, Handler handeler = OnNodeChanged);
			
			~AdaptiveVolume();

			Result Init(const Desc &desc);

			void RefinementByDistance(const ur_float3 &point);

			ur_uint MaxRefinementLevel(const BoundingBox &bbox);

			void GatherBlocks(const ur_uint level, const BoundingBox &bbox, BlockArray &blockArray);

			inline Isosurface& GetIsosurface() { return this->isosurface; }

			inline const Desc& GetDesc() const { return this->desc; }

			inline ur_uint GetLevelsCount() const { return (ur_uint)this->levelInfo.size(); }

			inline const LevelInfo& GetLevelInfo(ur_uint i) const { return this->levelInfo[i]; }

			inline const Vector3& GetRefinementPoint() const { return this->refinementPoint; }

		private:

			static void OnNodeChanged(Node* node, Event e);

			void RefinementByDistance(const ur_float3 &pos, AdaptiveVolume::Node *node);

			void MaxRefinementLevel(ur_uint &maxLevel, const BoundingBox &bbox, AdaptiveVolume::Node *node);

			void GatherBlocks(AdaptiveVolume::Node *node, const ur_uint level, BlockArray &blockArray);

			Desc desc;
			BoundingBox alignedBound;
			std::vector<LevelInfo> levelInfo;
			Vector3 refinementPoint;
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
		// Hybrid presentation approach
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

			typedef ur_float3 Vertex;

			struct UR_DECL Edge
			{
				ur_byte vid[2];
			};

			struct UR_DECL Face
			{
				ur_byte vid[3];
				ur_byte eid[3];
			};

			struct UR_DECL Hexahedron
			{
				static const ur_uint VerticesCount = 8;
				Vertex vertices[VerticesCount];
				std::unique_ptr<GfxBuffer> gfxVB;
				std::unique_ptr<GfxBuffer> gfxIB;

				// temp: debug stuff
				std::vector<ur_float3> lattice;
				std::vector<ur_float3> dbgVertices;
				std::vector<ur_uint> dbgIndices;
			};

			struct UR_DECL Tetrahedron
			{
				static const ur_uint VerticesCount = 4;

				static const ur_uint EdgesCount = 6;
				static const Edge Edges[EdgesCount];

				static const ur_uint FacesCount = 4;
				static const Face Faces[FacesCount];
				
				static const ur_uint ChildrenCount = 2;
				struct UR_DECL SplitInfo
				{
					ur_byte subTetrahedra[ChildrenCount][VerticesCount];
				};
				static const SplitInfo EdgeSplitInfo[EdgesCount];

				std::unique_ptr<Tetrahedron> children[ChildrenCount];
				Vertex vertices[VerticesCount];
				ur_byte longestEdgeIdx;
				Tetrahedron *edgeAdjacency[EdgesCount][6];
				Hexahedron hexahedra[4];
				

				Tetrahedron();

				void Init(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3);

				int UpdateAdjacency(Tetrahedron *th);

				void LinkAdjacentTetrahedra(Tetrahedron *th, ur_uint myEdgeIdx, ur_uint adjEdgeIdx);
				
				void UnlinkAdjacentTetrahedra(Tetrahedron *th, ur_uint myEdgeIdx, ur_uint adjEdgeIdx);

				void Split();

				void Merge();

				inline bool HasChildren() const { return (this->children[0] != ur_null); }
			};


			float SmallestNodeSize(const BoundingBox &bbox, AdaptiveVolume::Node *volumeNode);

			Result Construct(AdaptiveVolume &volume, Tetrahedron *tetrahedron);

			Result UpdateLoD(AdaptiveVolume &volume, Tetrahedron *tetrahedron);

			Result BuildMesh(AdaptiveVolume &volume, Tetrahedron *tetrahedron);

			Result MarchCubes(Hexahedron &hexahedron, const BlockArray &data);
			
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