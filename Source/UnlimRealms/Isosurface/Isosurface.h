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
			};

			
			ProceduralGenerator(Isosurface &isosurface, const Algorithm &algorithm, const GenerateParams &generateParams);

			~ProceduralGenerator();

			virtual Result Read(ValueType *values, const ur_float3 *points, const ur_uint count, const BoundingBox &bbox);

		private:

			Result GenerateSphericalDistanceField(ValueType *values, const ur_float3 *points, const ur_uint count, const BoundingBox &bbox);

			Result GenerateSimplexNoise(ValueType *values, const ur_float3 *points, const ur_uint count, const BoundingBox &bbox);


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

			struct Desc
			{
				ur_float CellSize; // expected lattice cell size at the highest LoD
				ur_uint3 LatticeResolution; // hexahedron lattice dimensions (min 2x2x2)
				ur_float DetailLevelDistance; // distance at which most detailed lattice is expected
			};

			HybridCubes(Isosurface &isosurface, const Desc &desc);

			~HybridCubes();

			virtual Result Update(const ur_float3 &refinementPoint, const ur_float4x4 &viewProj);

			virtual Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj);

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

			struct UR_DECL GfxMesh
			{
				std::unique_ptr<GfxBuffer> VB;
				std::unique_ptr<GfxBuffer> IB;
			};

			struct UR_DECL Hexahedron
			{
				static const ur_uint VerticesCount = 8;
				Vertex vertices[VerticesCount];
				GfxMesh gfxMesh;
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
				bool initialized;

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
				ur_uint buildQueue;
			};

			void UpdateRefinementTree(const ur_float3 &refinementPoint, EmptyOctree::Node *node);

			bool CheckRefinementTree(const BoundingBox &bbox, EmptyOctree::Node *node);

			Result Update(const ur_float3 &refinementPoint, Node *node, Node *cachedNode = ur_null, Stats *stats = ur_null);

			Result UpdateLoD(const ur_float3 &refinementPoint, Node *node, Node *cachedNode = ur_null);

			Result BuildMesh(Tetrahedron *tetrahedron, Stats *stats = ur_null);

			Result MarchCubes(Hexahedron &hexahedron);

			Result Render(GfxContext &gfxContext, GenericRender *genericRender, const ur_float4x4 &viewProj, Node *node);
			
			Result RenderDebug(GfxContext &gfxContext, GenericRender *genericRender, Node *node);

			Result RenderOctree(GfxContext &gfxContext, GenericRender *genericRender, EmptyOctree::Node *node);

			static void UpdateAsync(HybridCubes *presentation);


			Desc desc;
			static const ur_uint RootsCount = 6;
			std::unique_ptr<Node> root[RootsCount];
			EmptyOctree refinementTree;
			std::multimap<ur_uint, Tetrahedron*> buildQueue;
			
			// job(s) data
			std::shared_ptr<Job> jobUpdate;
			ur_float3 updatePoint;
			std::unique_ptr<Node> rootBack[RootsCount];
			Stats statsBack;
			
			// debug info
			bool freezeUpdate;
			bool drawTetrahedra;
			bool hideEmptyTetrahedra;
			bool drawHexahedra;
			bool drawRefinementTree;
			Stats stats;
		};


		Isosurface(Realm &realm);

		virtual ~Isosurface();

		Result Init(std::unique_ptr<DataVolume> data, std::unique_ptr<Presentation> presentation);

		Result Update();

		Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj);

		void ShowImgui();

		inline DataVolume* GetData() const { return this->data.get(); }

		inline Presentation* GetPresentation() const { return this->presentation.get(); }
	
	protected:

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
		};

		struct CommonCB
		{
			ur_float4x4 viewProj;
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
		std::unique_ptr<GfxObjects> gfxObjects;
		bool drawWireframe;
	};

} // end namespace UnlimRealms