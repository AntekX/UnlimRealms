///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"
#include "Sys/JobSystem.h"
#include "Gfx/GfxSystem.h"
#include "GenericRender/GenericRender.h"
#include "Atmosphere/Atmosphere.h"
// TEMP: referencing Atmosphere class here for atmosperic scattering rendering test
// TODO: isosurface should provide basic rendering only, all advanced lighting stuff should be a part of a "VoxelPlanet" class

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL Isosurface : public RealmEntity
	{
	public:

		
		// forward declaration
		class UR_DECL Instance;


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Base isosurface sub system
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL SubSystem
		{
		private:

			// non copyable
			SubSystem(const SubSystem &v) : isosurface(v.isosurface) {};

		public:

			// sub system's per instance data
			class UR_DECL InstanceData
			{
			public:

				explicit InstanceData(Isosurface::Instance &owner) : owner(owner) {};
				explicit InstanceData(const InstanceData &i) : owner(i.owner) {};
				~InstanceData() {}

			private:

				Isosurface::Instance &owner;
			};

			
			SubSystem(Isosurface &isosurface) : isosurface(isosurface) {};

			virtual ~SubSystem() {};

		protected:

			Isosurface &isosurface;
			std::list<std::unique_ptr<InstanceData>> instances;
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Abstract voxel data accessor/mutator
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL DataVolume : public SubSystem
		{
		public:

			typedef ur_float ValueType;

			DataVolume(Isosurface &isosurface);

			~DataVolume();

			virtual Result Read(ValueType *values, const ur_float3 *points, const ur_uint count, const BoundingBox &bbox);

			// todo: support data modification
			//virtual Result Write(const ValueType &value);

			virtual Result Save(const std::string &fileName);

			virtual Result Save(const std::string &fileName, ur_float cellSize, ur_uint3 blockResolution);

			inline const BoundingBox& GetBound() const { return this->bound; }

		protected:

			BoundingBox bound;
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Procedural volume data generator
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL ProceduralGenerator : public DataVolume
		{
		public:

			enum class UR_DECL Algorithm
			{
				SphericalDistanceField,
				SimplexNoise
			};

			struct UR_DECL GenerateParams
			{
				BoundingBox bound;
			};

			struct UR_DECL SphericalDistanceFieldParams : GenerateParams
			{
				ur_float3 center;
				ur_float radius;
			};

			struct UR_DECL SimplexNoiseParams : GenerateParams
			{
				struct Octave
				{
					ur_float scale;
					ur_float freq;
					ur_float clamp_min;
					ur_float clamp_max;
				};
				ur_float radiusMin;
				ur_float radiusMax;
				std::vector<Octave> octaves;
			};

			
			ProceduralGenerator(Isosurface &isosurface, const Algorithm &algorithm, const GenerateParams &generateParams);

			~ProceduralGenerator();

			virtual Result Read(ValueType *values, const ur_float3 *points, const ur_uint count, const BoundingBox &bbox);

			virtual Result Save(const std::string &fileName, ur_float cellSize, ur_uint3 blockResolution);

		private:

			Result GenerateSphericalDistanceField(ValueType *values, const ur_float3 *points, const ur_uint count, const BoundingBox &bbox);

			Result GenerateSimplexNoise(ValueType *values, const ur_float3 *points, const ur_uint count, const BoundingBox &bbox);


			// todo: per instance data
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

			virtual Result Update(const ur_float3 &refinementPoint, const ur_float4x4 &viewProj);

			virtual Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj);

			virtual Result Render(GrafCommandList &grafCmdList, const ur_float4x4 &viewProj);

			virtual void ShowImgui();
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Hybrid presentation approach
		// Subdivides volume box in to hierarchy of tetrahedra
		// Each tetrahedron is then divided into 4 hexahedra
		// Each hexahedron bounds the lattice used for surface mesh extraction
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL HybridCubes : public Presentation
		{
		public:

			struct UR_DECL Desc
			{
				ur_float CellSize; // expected lattice cell size at the highest LoD
				ur_uint3 LatticeResolution; // hexahedron lattice dimensions (min 2x2x2)
				ur_float DetailLevelDistance; // distance at which most detailed lattice is expected
			};

			HybridCubes(Isosurface &isosurface, const Desc &desc);

			~HybridCubes();

			virtual Result Update(const ur_float3 &refinementPoint, const ur_float4x4 &viewProj);

			virtual Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj);

			virtual Result Render(GrafCommandList &grafCmdList, const ur_float4x4 &viewProj);

			virtual void ShowImgui();

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

			#if defined(UR_GRAF)
			struct UR_DECL GrafMesh
			{
				std::unique_ptr<GrafBuffer> VB;
				std::unique_ptr<GrafBuffer> IB;
				
				GrafMesh();
				~GrafMesh();
			};
			#else
			struct UR_DECL GfxMesh
			{
				std::unique_ptr<GfxBuffer> VB;
				std::unique_ptr<GfxBuffer> IB;
			};
			#endif

			struct UR_DECL Hexahedron
			{
				static const ur_uint VerticesCount = 8;
				Vertex vertices[VerticesCount];
				#if defined(UR_GRAF)
				GrafMesh grafMesh;
				#else
				GfxMesh gfxMesh;
				#endif
			};

			struct UR_DECL Tetrahedron
			{
				static const ur_uint VerticesCount = 4;

				static const ur_uint EdgesCount = 6;
				static const Edge Edges[EdgesCount];
				static const ur_byte EdgeOppositeId[EdgesCount];

				static const ur_uint FacesCount = 4;
				static const Face Faces[FacesCount];
				
				static const ur_uint ChildrenCount = 2;
				struct UR_DECL SplitInfo
				{
					ur_byte subTetrahedra[ChildrenCount][VerticesCount];
				};
				static const SplitInfo EdgeSplitInfo[EdgesCount];

				ur_uint level;
				Vertex vertices[VerticesCount];
				ur_byte longestEdgeIdx;
				BoundingBox bbox;
				Hexahedron hexahedra[4];
				ur_bool initialized;
				ur_bool visible;

				Tetrahedron();

				~Tetrahedron();

				void Init(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3);

				void Split(std::array<std::unique_ptr<Tetrahedron>, ChildrenCount> &subTetrahedra);
			};

			struct UR_DECL Node
			{
				std::shared_ptr<Tetrahedron> tetrahedron;
				std::unique_ptr<Node> children[Tetrahedron::ChildrenCount];

				Node();

				Node(std::unique_ptr<Tetrahedron> tetrahedron);

				~Node();

				void Split(Node *cachedNode);

				void Merge();

				inline bool HasChildren() const { return (this->children[0] != ur_null); }
			};

			struct Stats
			{
				ur_uint tetrahedraCount;
				ur_uint treeMemory;
				ur_uint meshVideoMemory;
				ur_uint primitivesRendered;
				ur_uint buildQueue;
			};

			void UpdateRefinementTree(const ur_float3 &refinementPoint, EmptyOctree::Node *node);

			bool CheckRefinementTree(const BoundingBox &bbox, EmptyOctree::Node *node);

			Result Update(const ur_float3 &refinementPoint, Node *node, Node *cachedNode = ur_null, Stats *stats = ur_null);

			Result UpdateLoD(const ur_float3 &refinementPoint, Node *node, Node *cachedNode = ur_null);

			Result BuildMesh(Tetrahedron *tetrahedron, Stats *stats = ur_null);

			Result MarchCubes(Hexahedron &hexahedron);

			Result Render(GfxContext &gfxContext, GenericRender *genericRender, const ur_float4(&frustumPlanes)[6], Node *node);

			Result Render(GrafCommandList &grafCmdList, GenericRender *genericRender, const ur_float4(&frustumPlanes)[6], Node *node);
			
			Result RenderDebug(GenericRender *genericRender, Node *node);

			Result RenderOctree(GenericRender *genericRender, EmptyOctree::Node *node);

			// common data
			ur_float3 updatePoint;
			std::multimap<ur_uint, Tetrahedron*> buildQueue;
			std::shared_ptr<Job> jobUpdate;
			std::list<std::shared_ptr<Job>> jobBuild;
			std::list<std::pair<HybridCubes*, Tetrahedron*>> jobBuildCtx;
			std::atomic<ur_uint> jobBuildCounter;
			std::atomic<ur_uint> jobBuildRequested;

			// todo: per instance data
			Desc desc;
			EmptyOctree refinementTree;
			std::vector<ur_float> refinementDistance;
			static const ur_uint RootsCount = 6;
			std::unique_ptr<Node> root[RootsCount];
			std::unique_ptr<Node> rootBack[RootsCount];
			
			// debug info
			bool freezeUpdate;
			bool hideSurface;
			bool drawTetrahedra;
			bool hideEmptyTetrahedra;
			bool drawHexahedra;
			bool drawRefinementTree;
			Stats stats;
			Stats statsBack; // async update structure
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Isosurface instance
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL Instance
		{
		public:

			explicit Instance(Isosurface &isosurface) : isosurface(isosurface) {}
			~Instance() {}

		protected:

			Isosurface &isosurface;
			DataVolume::InstanceData *dataInstance;
			Presentation::InstanceData *presentationInstance;
		};


		Isosurface(Realm &realm);

		virtual ~Isosurface();

		Result Init(std::unique_ptr<DataVolume> data, std::unique_ptr<Presentation> presentation);

		Result Init(std::unique_ptr<DataVolume> data, std::unique_ptr<Presentation> presentation, GrafRenderPass* grafRenderPass);

		Result Update();

		Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const Atmosphere *atmosphere);

		Result Render(GrafCommandList &grafCmdList, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const Atmosphere *atmosphere);

		void ShowImgui();

		inline DataVolume* GetData() const { return this->data.get(); }

		inline Presentation* GetPresentation() const { return this->presentation.get(); }
	
	protected:

		#if defined(UR_GRAF)

		Result CreateGrafObjects(GrafRenderPass* grafRenderPass);

		struct GrafObjects
		{
			std::unique_ptr<GrafShader> VS;
			std::unique_ptr<GrafShader> PS;
			std::unique_ptr<GrafShader> PSDbg;
			std::unique_ptr<GrafDescriptorTableLayout> shaderDescriptorLayout;
			std::vector<std::unique_ptr<GrafDescriptorTable>> shaderDescriptorTable;
			std::unique_ptr<GrafPipeline> pipelineSolid;
			std::unique_ptr<GrafPipeline> pipelineDebug;
		} grafObjects;
		GrafRenderer *grafRenderer;

		#else

		Result CreateGfxObjects();

		struct GfxObjects
		{
			std::unique_ptr<GfxVertexShader> VS;
			std::unique_ptr<GfxPixelShader> PS;
			std::unique_ptr<GfxPixelShader> PSDbg;
			std::unique_ptr<GfxInputLayout> inputLayout;
			std::unique_ptr<GfxBuffer> CB;
			std::unique_ptr<GfxPipelineState> pipelineState;
			std::unique_ptr<GfxPipelineState> wireframeState;
			std::unique_ptr<GfxTexture> albedoMaps[3];
			std::unique_ptr<GfxTexture> normalMaps[3];
		} gfxObjects;

		#endif

		struct alignas(16) CommonCB
		{
			ur_float4x4 ViewProj;
			ur_float4 CameraPos;
			Atmosphere::Desc AtmoParams;
		};

		struct Vertex
		{
			ur_float3 pos;
			ur_float3 norm;
			ur_uint32 col;
		};

		typedef ur_uint32 Index;

		std::unique_ptr<DataVolume> data;
		std::unique_ptr<Presentation> presentation;
		std::list<std::unique_ptr<Instance>> instances;
		bool drawWireframe;
	};

} // end namespace UnlimRealms