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
		// Abstract voxel data accessor/mutator
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class UR_DECL DataVolume : public SubSystem
		{
		public:

			typedef ur_float ValueType;

			DataVolume(Isosurface &isosurface);

			~DataVolume();

			virtual Result Read(ValueType &value, const ur_float3 &point);

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

			virtual Result Read(ValueType &value, const ur_float3 &point);

		private:

			Result GenerateSphericalDistanceField(ValueType &value, const ur_float3 &point);

			Result GenerateSimplexNoise(ValueType &value, const ur_float3 &point);


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

				bool UpdateAdjacency(Tetrahedron *th);

				void LinkAdjacentTetrahedra(Tetrahedron *th, ur_uint myEdgeIdx, ur_uint adjEdgeIdx);
				
				void UnlinkAdjacentTetrahedra(Tetrahedron *th, ur_uint myEdgeIdx, ur_uint adjEdgeIdx);

				void Split();

				void Merge();

				inline bool HasChildren() const { return (this->children[0] != ur_null); }
			};


			Result Update(const ur_float3 &refinementPoint, Tetrahedron *tetrahedron);

			Result UpdateLoD(const ur_float3 &refinementPoint, Tetrahedron *tetrahedron);

			Result BuildMesh(Tetrahedron *tetrahedron);

			Result MarchCubes(Hexahedron &hexahedron);

			Result Render(GfxContext &gfxContext, GenericRender *genericRender, const ur_float4x4 &viewProj, Tetrahedron *tetrahedron);
			
			Result RenderDebug(GfxContext &gfxContext, GenericRender *genericRender, Tetrahedron *tetrahedron);


			Desc desc;
			static const ur_uint RootsCount = 6;
			std::unique_ptr<Tetrahedron> root[RootsCount];
			
			bool drawTetrahedra;
			bool drawHexahedra;
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
			ur_float3 norm;
			ur_uint32 col;
		};

		typedef ur_uint32 Index;

		std::unique_ptr<DataVolume> data;
		std::unique_ptr<Presentation> presentation;
		std::unique_ptr<GfxObjects> gfxObjects;
	};

} // end namespace UnlimRealms