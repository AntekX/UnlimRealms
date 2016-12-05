///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Isosurface/Isosurface.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/Canvas.h"
#include "Sys/Input.h"
#include "ImguiRender/ImguiRender.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface::AdaptiveVolume
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::AdaptiveVolume::AdaptiveVolume(Isosurface &isosurface, ur_uint depth, Handler handler) :
		SubSystem(isosurface),
		Octree<Isosurface::Block>(depth, handler)
	{
	}

	Isosurface::AdaptiveVolume::~AdaptiveVolume()
	{

	}

	Result Isosurface::AdaptiveVolume::Init(const Desc &desc)
	{
		this->levelInfo.clear();
		this->desc = desc;
		this->desc.BlockResolution.SetMax(ur_uint3(1));
		this->desc.BlockSize.SetMax(ur_float3(0.0f));

		if (this->desc.Bound.IsInsideOut() ||
			this->desc.BlockSize.All() == false ||
			this->desc.DetailLevelDistance <= 0 ||
			this->desc.RefinementProgression < 1.0f)
			return Result(InvalidArgs);

		ur_float blockSizeMax = std::max(std::max(this->desc.BlockSize.x, this->desc.BlockSize.y), this->desc.BlockSize.z);
		ur_float blockSizeMin = std::min(std::min(this->desc.BlockSize.x, this->desc.BlockSize.y), this->desc.BlockSize.z);
		ur_float boundSizeMax = std::max(std::max(this->desc.Bound.SizeX(), this->desc.Bound.SizeY()), this->desc.Bound.SizeZ());
		ur_uint blocksAlongSide = ur_uint(ceil(boundSizeMax / blockSizeMin));
		this->alignedBound.Min = this->desc.Bound.Min;
		this->alignedBound.Max = this->alignedBound.Min + blockSizeMin * blocksAlongSide;

		ur_uint levelsCount = ur_uint(log2(blocksAlongSide)) + 1;
		this->levelInfo.resize(levelsCount);
		ur_float levelDistance = this->desc.DetailLevelDistance;
		for (ur_uint i = 0; i < levelsCount; ++i)
		{
			LevelInfo &level = this->levelInfo[levelsCount - i - 1];
			ur_uint blocksInRange = std::max(ur_uint(levelDistance / blockSizeMax + 0.5f), ur_uint(2));
			levelDistance = std::max(blockSizeMax * blocksInRange, levelDistance);
			level.Distance = levelDistance;
			levelDistance *= this->desc.RefinementProgression;
		}

		return Octree<Isosurface::Block>::Init(this->alignedBound);
	}

	void Isosurface::AdaptiveVolume::OnNodeChanged(Node* node, Event e)
	{
		if (ur_null == node)
			return;

		AdaptiveVolume& volume = static_cast<AdaptiveVolume&>(node->GetTree());
		Builder *builder = volume.GetIsosurface().GetBuilder();
		if (builder != ur_null)
		{
			switch (e)
			{
			case Event::NodeAdded: builder->AddNode(node); break;
			case Event::NodeRemoved: builder->RemoveNode(node); break;
			}
		}
	}

	void Isosurface::AdaptiveVolume::RefinementByDistance(const ur_float3 &pos)
	{
		if (this->levelInfo.empty())
			return;

		this->refinementPoint = pos;

		this->RefinementByDistance(pos, this->GetRoot());
	}

	void Isosurface::AdaptiveVolume::RefinementByDistance(const ur_float3 &pos, AdaptiveVolume::Node *node)
	{
		if (ur_null == node || (node->GetLevel() + 1) >= this->GetLevelsCount())
			return;

		if (node->GetBBox().Distance(pos) <= this->levelInfo[node->GetLevel()].Distance)
		{
			node->Split();
			if (node->HasSubNodes())
			{
				for (ur_uint i = 0; i < Node::SubNodesCount; ++i)
				{
					this->RefinementByDistance(pos, node->GetSubNode(i));
				}
			}
		}
		else
		{
			node->Merge();
		}
	}

	ur_uint Isosurface::AdaptiveVolume::MaxRefinementLevel(const BoundingBox &bbox)
	{
		if (ur_null == this->GetRoot() || !bbox.Intersects(this->GetRoot()->GetBBox()))
			return 0;

		ur_uint maxLevel = this->GetLevelsCount();
		
		this->MaxRefinementLevel(maxLevel, bbox, this->GetRoot());
		
		return maxLevel;
	}

	void Isosurface::AdaptiveVolume::MaxRefinementLevel(ur_uint &maxLevel, const BoundingBox &bbox, AdaptiveVolume::Node *node)
	{
		if (node == ur_null ||
			node->GetLevel() > maxLevel ||
			node->GetBBox().Intersects(bbox) == false)
			return;

		if (node->HasSubNodes())
		{
			for (ur_uint i = 0; i < Node::SubNodesCount; ++i)
			{
				this->MaxRefinementLevel(maxLevel, bbox, node->GetSubNode(i));
			}
		}
		else
		{
			maxLevel = std::min(maxLevel, node->GetLevel());
		}
	}

	void Isosurface::AdaptiveVolume::GatherBlocks(const ur_uint level, const BoundingBox &bbox,
		std::vector<Block*> &blocks, ur_uint3 &gridSize, ur_float3 &gridBlockSize, BoundingBox &gridBBox)
	{		
		if (0 == this->GetLevelsCount() || ur_null == this->GetRoot())
			return;

		BoundingBox ibbox = this->GetRoot()->GetBBox();
		ibbox.Min.SetMax(bbox.Min);
		ibbox.Max.SetMin(bbox.Max);
		if (ibbox.IsInsideOut())
			return;

		gridBlockSize = this->desc.BlockSize * float(1 << (this->GetLevelsCount() - level));
		
		ur_int3 gridMin(
			(ur_int)floor(ibbox.Min.x / gridBlockSize.x),
			(ur_int)floor(ibbox.Min.y / gridBlockSize.y),
			(ur_int)floor(ibbox.Min.z / gridBlockSize.z));
		ur_int3 gridMax(
			(ur_int)ceil(ibbox.Max.x / gridBlockSize.x),
			(ur_int)ceil(ibbox.Max.y / gridBlockSize.y),
			(ur_int)ceil(ibbox.Max.z / gridBlockSize.z));
		gridSize.x = ur_uint(gridMax.x - gridMin.x + 1);
		gridSize.y = ur_uint(gridMax.y - gridMin.y + 1);
		gridSize.z = ur_uint(gridMax.z - gridMin.z + 1);

		gridBBox.Min.x = gridBlockSize.x * gridMin.x;
		gridBBox.Min.y = gridBlockSize.y * gridMin.y;
		gridBBox.Min.z = gridBlockSize.z * gridMin.z;
		gridBBox.Max.x = gridBlockSize.x * gridMax.x;
		gridBBox.Max.y = gridBlockSize.y * gridMax.y;
		gridBBox.Max.z = gridBlockSize.z * gridMax.z;

		blocks.resize(gridSize.x * gridSize.y * gridSize.z, ur_null);

		this->GatherBlocks(this->GetRoot(), level, blocks, gridBlockSize, gridSize, gridBBox);
	}

	void Isosurface::AdaptiveVolume::GatherBlocks(AdaptiveVolume::Node *node, const ur_uint level, std::vector<Block*> &blocks,
		const ur_float3 &blockSize, const ur_uint3 &gridSize, const BoundingBox &gridBBox)
	{
		if (node == ur_null ||
			node->GetBBox().Intersects(gridBBox) == false)
			return;

		if (node->GetLevel() == level)
		{
			ur_uint3 gridPos(
				(ur_uint)floor((node->GetBBox().Min.x - gridBBox.Min.x) / blockSize.x),
				(ur_uint)floor((node->GetBBox().Min.y - gridBBox.Min.y) / blockSize.y),
				(ur_uint)floor((node->GetBBox().Min.z - gridBBox.Min.z) / blockSize.z));
			blocks[gridPos.x + gridPos.y * gridSize.x + gridPos.z * gridSize.x * gridSize.y] = &node->GetData();
		}
		else if (node->HasSubNodes())
		{
			for (ur_uint i = 0; i < Node::SubNodesCount; ++i)
			{
				this->GatherBlocks(node->GetSubNode(i), level, blocks, blockSize, gridSize, gridBBox);
			}
		}
	}

	Isosurface::Block::ValueType Isosurface::AdaptiveVolume::SampleBlocks(const ur_float3 &point, const std::vector<Block*> &blocks,
		const ur_uint3 &gridSize, const ur_uint3 &gridBlockSize, const BoundingBox &gridBBox)
	{
		Block::ValueType val = (Block::ValueType)0;
		
		ur_float3 fpos(
			(point.x - gridBBox.Min.x) / gridBlockSize.x,
			(point.y - gridBBox.Min.y) / gridBlockSize.y,
			(point.z - gridBBox.Min.z) / gridBlockSize.z);
		ur_int3 bpos(
			(ur_int)floor(fpos.x),
			(ur_int)floor(fpos.y),
			(ur_int)floor(fpos.z));
		fpos.x -= bpos.x;
		fpos.y -= bpos.y;
		fpos.z -= bpos.z;
		
		Block *block = blocks[bpos.x + bpos.y * gridSize.x + bpos.z * gridSize.x * gridSize.y];
		if (block != ur_null && !block->field.empty())
		{
			// following sampling algorithm considers that cell center is on the block's corner
			// (cell size = block size / (resolution - 1))
			const ur_uint3 &bres = this->GetDesc().BlockResolution;
			ur_float3 cf(
				lerp(0.0f, ur_float(bres.x), fpos.x),
				lerp(0.0f, ur_float(bres.y), fpos.y),
				lerp(0.0f, ur_float(bres.z), fpos.z));
			ur_int3 cp(
				(ur_int)floor(cf.x),
				(ur_int)floor(cf.y),
				(ur_int)floor(cf.z));
			cf.x -= cp.x;
			cf.y -= cp.y;
			cf.z -= cp.z;
			static const ur_int3 cofs[8] = {
				{ 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 },
				{ 0, 0, 1 }, { 1, 0, 1 }, { 0, 1, 1 }, { 1, 1, 1 }
			};
			const ur_uint3 fres(bres.x + BorderSize * 2, bres.y + BorderSize * 2, bres.z + BorderSize * 2);
			const ur_uint skipBorderOfs = fres.x * fres.y + fres.x + BorderSize;
			Block::ValueType cv[8];
			for (ur_uint i = 0; i < 8; ++i)
			{
				cv[i] = block->field[skipBorderOfs +
					(cp.x + cofs[i].x) + (cp.y + cofs[i].y) * bres.x + (cp.z + cofs[i].z) * bres.x * bres.y];
			}
			ur_float v0 = lerp(cv[0], cv[1], cf.x);
			ur_float v1 = lerp(cv[2], cv[3], cf.x);
			ur_float v2 = lerp(cv[4], cv[5], cf.x);
			ur_float v3 = lerp(cv[6], cv[7], cf.x);
			val = lerp(lerp(v0, v1, cf.y), lerp(v2, v3, cf.y), cf.z);
		}

		return val;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface::Loader
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::Loader::Loader(Isosurface &isosurface) : 
		SubSystem(isosurface)
	{

	}

	Isosurface::Loader::~Loader()
	{

	}

	Result Isosurface::Loader::Load(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox)
	{
		return Result(NotImplemented);
	}

	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface::ProceduralGenerator
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::ProceduralGenerator::ProceduralGenerator(Isosurface &isosurface, const Algorithm &algorithm, const GenerateParams &generateParams) :
		Loader(isosurface)
	{
		this->algorithm = algorithm;
		switch (this->algorithm)
		{
		case Algorithm::Fill: this->generateParams.reset(new FillParams((const FillParams&)generateParams)); break;
		case Algorithm::SphericalDistanceField: this->generateParams.reset(new SphericalDistanceFieldParams((const SphericalDistanceFieldParams&)generateParams)); break;
		case Algorithm::SimplexNoise: this->generateParams.reset(new SimplexNoiseParams((const SimplexNoiseParams&)generateParams)); break;
		}
	}

	Isosurface::ProceduralGenerator::~ProceduralGenerator()
	{

	}

	Result Isosurface::ProceduralGenerator::Load(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox)
	{
		Result res = Result(Success);

		// allocate field
		const ur_uint3 fieldSize = volume.GetDesc().BlockResolution + AdaptiveVolume::BorderSize * 2;
		block.field.resize(fieldSize.x * fieldSize.y * fieldSize.z);

		// choose algorithm and generate data
		switch (this->algorithm)
		{
			case Algorithm::Fill:
			{
				FillParams *params = static_cast<FillParams*>(this->generateParams.get());
				ProceduralGenerator::Fill(volume, block, bbox, *params);
			} break;
			case Algorithm::SphericalDistanceField:
			{
				SphericalDistanceFieldParams *params = static_cast<SphericalDistanceFieldParams*>(this->generateParams.get());
				ProceduralGenerator::GenerateSphericalDistanceField(volume, block, bbox, *params);
			} break;
			case Algorithm::SimplexNoise:
			{
				SimplexNoiseParams *params = static_cast<SimplexNoiseParams*>(this->generateParams.get());
				ProceduralGenerator::GenerateSimplexNoise(volume, block, bbox, *params);
			} break;
			default:
			{
				res = Result(NotImplemented);
			}
		}

		return res;
	}

	Result Isosurface::ProceduralGenerator::Fill(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox, const FillParams &params)
	{
		const ur_uint3 &blockSize = volume.GetDesc().BlockResolution;
		const ur_uint3 fieldSize = blockSize + AdaptiveVolume::BorderSize * 2;
		Block::ValueType *dstValue = block.field.data();
		for (ur_uint iz = 0; iz < fieldSize.z; ++iz)
		{
			for (ur_uint iy = 0; iy < fieldSize.y; ++iy)
			{
				for (ur_uint ix = 0; ix < fieldSize.x; ++ix)
				{
					if (ix < AdaptiveVolume::BorderSize || ix >= blockSize.x + AdaptiveVolume::BorderSize ||
						iy < AdaptiveVolume::BorderSize || iy >= blockSize.y + AdaptiveVolume::BorderSize ||
						iz < AdaptiveVolume::BorderSize || iz >= blockSize.z + AdaptiveVolume::BorderSize)
					{
						*dstValue++ = params.externalValue;
					}
					else
					{
						*dstValue++ = params.internalValue;
					}
				}
			}
		}

		return Result(Success);
	}

	Result Isosurface::ProceduralGenerator::GenerateSphericalDistanceField(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox, const SphericalDistanceFieldParams &params)
	{
		const ur_uint3 &blockSize = volume.GetDesc().BlockResolution;
		const ur_uint3 fieldSize = blockSize + AdaptiveVolume::BorderSize * 2;
		const ur_float3 halfVoxel(
			bbox.SizeX() / blockSize.x * 0.5f,
			bbox.SizeY() / blockSize.y * 0.5f,
			bbox.SizeZ() / blockSize.z * 0.5f);
		const BoundingBox bboxEx(bbox.Min - halfVoxel, bbox.Max + halfVoxel);
		ur_float3 vpos;
		Block::ValueType *dstValue = block.field.data();
		for (ur_uint iz = 0; iz < fieldSize.z; ++iz)
		{
			for (ur_uint iy = 0; iy < fieldSize.y; ++iy)
			{
				for (ur_uint ix = 0; ix < fieldSize.x; ++ix)
				{
					vpos.x = ur_float(ix) / (fieldSize.x - 1);
					vpos.y = ur_float(iy) / (fieldSize.y - 1);
					vpos.z = ur_float(iz) / (fieldSize.z - 1);
					vpos = ur_float3::Lerp(bboxEx.Min, bboxEx.Max, vpos);
					*dstValue++ = params.radius - (vpos - params.center).Length();
				}
			}
		}

		return Result(Success);
	}

	Result Isosurface::ProceduralGenerator::GenerateSimplexNoise(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox, const SimplexNoiseParams &params)
	{
		// temp: hardcoded generation params
		PerlinNoise noise;
		ur_float radius = volume.GetDesc().Bound.SizeX() * 0.4f;
		ur_float3 center = volume.GetDesc().Bound.Center();
		struct Octave
		{
			ur_float scale;
			ur_float freq;
		};
		Octave octaves[] = {
			{ -1.00f, 0.50f },
			{ -0.5f, 1.00f },
			//{ 0.05f, 8.00f },
		};
		const ur_uint numOctaves = ur_array_size(octaves);

		const ur_uint3 &blockSize = volume.GetDesc().BlockResolution;
		const ur_uint3 fieldSize = blockSize + AdaptiveVolume::BorderSize * 2;
		const ur_float3 halfVoxel(
			bbox.SizeX() / blockSize.x * 0.5f,
			bbox.SizeY() / blockSize.y * 0.5f,
			bbox.SizeZ() / blockSize.z * 0.5f);
		const BoundingBox bboxEx(bbox.Min - halfVoxel, bbox.Max + halfVoxel);
		ur_float3 vpos;
		Block::ValueType *dstValue = block.field.data();
		for (ur_uint iz = 0; iz < fieldSize.z; ++iz)
		{
			for (ur_uint iy = 0; iy < fieldSize.y; ++iy)
			{
				for (ur_uint ix = 0; ix < fieldSize.x; ++ix)
				{
					vpos.x = ur_float(ix) / (fieldSize.x - 1);
					vpos.y = ur_float(iy) / (fieldSize.y - 1);
					vpos.z = ur_float(iz) / (fieldSize.z - 1);
					vpos = ur_float3::Lerp(bboxEx.Min, bboxEx.Max, vpos);
					*dstValue = radius - (vpos - center).Length();
					for (ur_uint io = 0; io < numOctaves; ++io)
					{
						*dstValue += (ur_float)noise.Noise(
							ur_double(vpos.x * octaves[io].freq),
							ur_double(vpos.y * octaves[io].freq),
							ur_double(vpos.z * octaves[io].freq)) * octaves[io].scale;
					}
					dstValue++;
				}
			}
		}

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface::Presentation
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::Presentation::Presentation(Isosurface &isosurface) :
		SubSystem(isosurface)
	{

	}

	Isosurface::Presentation::~Presentation()
	{

	}

	Result Isosurface::Presentation::Construct(AdaptiveVolume &volume)
	{
		return Result(NotImplemented);
	}

	Result Isosurface::Presentation::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj)
	{
		return Result(NotImplemented);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface::SurfaceNet
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::SurfaceNet::SurfaceNet(Isosurface &isosurface) :
		Presentation(isosurface)
	{
	}

	Isosurface::SurfaceNet::~SurfaceNet()
	{

	}

	Result Isosurface::SurfaceNet::Construct(AdaptiveVolume &volume)
	{
		// init tree
		if (ur_null == this->tree.get())
		{
			this->tree.reset(new MeshTree());
			this->tree->Init(volume.GetDesc().Bound);
		}

		// update mesh blocks from volume
		return this->Construct(volume, volume.GetRoot(), this->tree->GetRoot());
	}

	Result Isosurface::SurfaceNet::Construct(AdaptiveVolume &volume, AdaptiveVolume::Node *volumeNode, MeshTree::Node *meshNode)
	{
		Result res = Result(Success);

		if (ur_null == meshNode)
			return Result(InvalidArgs);

		if (volumeNode != ur_null)
		{
			// construct mesh for current data field
			if (!meshNode->GetData().initialized)
			{
				Result buildRes = this->Construct(volume, volumeNode->GetBBox(), volumeNode->GetData(), meshNode->GetData());
				res = CombinedResult(res, buildRes);
			}

			if (volumeNode->HasSubNodes())
			{
				// build higher detail sub meshes
				meshNode->Split();
				for (ur_uint i = 0; i < MeshTree::Node::SubNodesCount; ++i)
				{
					this->Construct(volume, volumeNode->GetSubNode(i), meshNode->GetSubNode(i));
				}
			}
			else
			{
				meshNode->Merge();
			}
		}
		else
		{
			meshNode->Merge();
		}

		return res;
	}

	Result Isosurface::SurfaceNet::Construct(AdaptiveVolume &volume, const BoundingBox &bbox, Block &block, MeshBlock &mesh)
	{
		mesh.initialized = true;

		// compute surface data

		const ur_uint &blockBorder = AdaptiveVolume::BorderSize;
		const ur_uint3 &blockSize = volume.GetDesc().BlockResolution;
		const ur_uint fieldRow = blockSize.x + AdaptiveVolume::BorderSize * 2;
		const ur_uint fieldSlice = fieldRow * (blockSize.y + AdaptiveVolume::BorderSize * 2);
		const ur_uint3 gridSize = blockSize + 1;
		const ur_uint gridRow = gridSize.x;
		const ur_uint gridSlice = gridRow * gridSize.y;
		const ur_float3 halfVoxel(
			bbox.SizeX() / blockSize.x * 0.5f,
			bbox.SizeY() / blockSize.y * 0.5f,
			bbox.SizeZ() / blockSize.z * 0.5f);

		static const Index NoIndex = Index(-1);
		struct SurfaceInfo
		{
			Index vidx;
			ur_byte side; // flags indicating which axes polygons intersect: x = 0x1, y = 0x2, z = 0x4, 0x80 = back face
		} NoInfo = { NoIndex, 0 };
		std::vector<SurfaceInfo> surface(gridSize.x * gridSize.y * gridSize.z, NoInfo);
		std::vector<Vertex> vertices;
		std::vector<Index> indices;

		const ur_uint CellStride[8] = {
			0,							// left, bottom, near
			1,							// right, bottom, near
			fieldRow,					// left, top, near
			fieldRow + 1,				// right, top, near
			fieldSlice,					// left, bottom, far
			fieldSlice + 1,				// right, bottom, far
			fieldSlice + fieldRow,		// left, top, far
			fieldSlice + fieldRow + 1	// right, top, far
		};

		static const ur_float3 CornerOfs[8] = {
			{ -1.0f, -1.0f, -1.0f },
			{ +1.0f, -1.0f, -1.0f },
			{ -1.0f, +1.0f, -1.0f },
			{ +1.0f, +1.0f, -1.0f },
			{ -1.0f, -1.0f, +1.0f },
			{ +1.0f, -1.0f, +1.0f },
			{ -1.0f, +1.0f, +1.0f },
			{ +1.0f, +1.0f, +1.0f }
		};

		static const ur_uint2 CellEdge[12] = {
			{ 0, 1 },{ 2, 3 },{ 4, 5 },{ 6, 7 },
			{ 0, 2 },{ 1, 3 },{ 4, 6 },{ 5, 7 },
			{ 0, 4 },{ 1, 5 },{ 2, 6 },{ 3, 7 }
		};

		Vertex vert;
		Index vidx = 0;
		ur_bool addVertex;
		ur_uint scount = 0;
		ur_float3 vpos, grad;
		Block::ValueType fcell[8];
		for (ur_uint iz = 0; iz < gridSize.z; ++iz)
		{
			for (ur_uint iy = 0; iy < gridSize.y; ++iy)
			{
				for (ur_uint ix = 0; ix < gridSize.x; ++ix)
				{
					// build morphological field surface info
					Block::ValueType *pf = block.field.data() + ix + iy * fieldRow + iz * fieldSlice;
					SurfaceInfo *ps = surface.data() + ix + iy * gridRow + iz * gridSlice;
					for (ur_uint i = 0; i < 8; ++i)
					{
						fcell[i] = pf[CellStride[i]];
					}
					addVertex = false;
					if (fcell[0] < 0)
					{
						if (0 <= fcell[1]) { addVertex = true; ps->side |= 0x1; ++scount; }
						if (0 <= fcell[2]) { addVertex = true; ps->side |= 0x2; ++scount; }
						if (0 <= fcell[3]) { addVertex = true; }
						if (0 <= fcell[4]) { addVertex = true; ps->side |= 0x4; ++scount; }
						if (0 <= fcell[5]) { addVertex = true; }
						if (0 <= fcell[6]) { addVertex = true; }
						if (0 <= fcell[7]) { addVertex = true; }
					}
					else
					{
						if (0 > fcell[1]) { addVertex = true; ps->side |= (0x1 | 0x80); ++scount; }
						if (0 > fcell[2]) { addVertex = true; ps->side |= (0x2 | 0x80); ++scount; }
						if (0 > fcell[3]) { addVertex = true; }
						if (0 > fcell[4]) { addVertex = true; ps->side |= (0x4 | 0x80); ++scount; }
						if (0 > fcell[5]) { addVertex = true; }
						if (0 > fcell[6]) { addVertex = true; }
						if (0 > fcell[7]) { addVertex = true; }
					}

					// add vertex
					if (addVertex)
					{
						ps->vidx = vidx++;
						vpos.x = ur_float(ix) / blockSize.x;
						vpos.y = ur_float(iy) / blockSize.y;
						vpos.z = ur_float(iz) / blockSize.z;
						vpos = ur_float3::Lerp(bbox.Min, bbox.Max, vpos);

						// compute gradient
						grad.x = ((fcell[1] - fcell[0]) + (fcell[3] - fcell[2]) + (fcell[5] - fcell[4]) + (fcell[7] - fcell[6]));
						grad.y = ((fcell[2] - fcell[0]) + (fcell[3] - fcell[1]) + (fcell[6] - fcell[4]) + (fcell[7] - fcell[5]));
						grad.z = ((fcell[4] - fcell[0]) + (fcell[5] - fcell[1]) + (fcell[6] - fcell[2]) + (fcell[7] - fcell[3]));
						grad = ur_float3::Normalize(grad);

						// compute approximate vertex position on the surface
						vert.pos = 0.0f;
						auto edgeIntersected = [&](const ur_uint2 &e) -> ur_bool {
							return ((fcell[e.x] < 0 && fcell[e.y] >= 0) || (fcell[e.x] >= 0 && fcell[e.y] < 0));
						};
						ur_uint vcount = 0;
						for (ur_uint i = 0; i < 12; ++i)
						{
							if (edgeIntersected(CellEdge[i]))
							{
								//vert.pos += (vpos + (halfVoxel * CornerOfs[CellEdge[i].x]) + grad * fcell[CellEdge[i].x] * -1.0f);
								ur_float w = fcell[CellEdge[i].x] / (fcell[CellEdge[i].x] - fcell[CellEdge[i].y]);
								vert.pos += (vpos + halfVoxel * CornerOfs[CellEdge[i].x]) * (1.0f - w) +
									(vpos + halfVoxel * CornerOfs[CellEdge[i].y]) * w;
								++vcount;
							}
						}
						vert.pos /= float(vcount);

						// temp: write surface normal as color
						vert.col = Vector4ToRGBA32(ur_float4((grad * -1.0f + 1.0) * 0.5f, 1.0f));

						vertices.emplace_back(vert);
					}
				}
			}
		}

		// triangulate surface
		auto addTriangle = [](std::vector<Index> &indices, Index *tri, ur_bool isBackFace) -> void {
			indices.push_back(tri[0]);
			indices.push_back(isBackFace ? tri[2] : tri[1]);
			indices.push_back(isBackFace ? tri[1] : tri[2]);
		};
		indices.reserve(scount * 6);
		Index tri[3];
		for (ur_uint iz = 0; iz < gridSize.z; ++iz)
		{
			for (ur_uint iy = 0; iy < gridSize.y; ++iy)
			{
				for (ur_uint ix = 0; ix < gridSize.x; ++ix)
				{
					SurfaceInfo *ps = surface.data() + ix + iy * gridRow + iz * gridSlice;
					ur_bool isBackFace = ((ps->side & 0x80) != 0);
					if (ps->side & 0x1)
					{
						if (iy > 0 && iz > 0)
						{
							tri[0] = (ps)->vidx;
							tri[1] = (ps - gridSlice)->vidx;
							tri[2] = (ps - gridSlice - gridRow)->vidx;
							addTriangle(indices, tri, isBackFace);
							tri[0] = (ps - gridSlice - gridRow)->vidx;
							tri[1] = (ps - gridRow)->vidx;
							tri[2] = (ps)->vidx;
							addTriangle(indices, tri, isBackFace);
						}
					}
					if (ps->side & 0x2)
					{
						if (ix > 0 && iz > 0)
						{
							tri[0] = (ps - 1)->vidx;
							tri[1] = (ps - gridSlice)->vidx;
							tri[2] = (ps)->vidx;
							addTriangle(indices, tri, isBackFace);
							tri[0] = (ps - gridSlice)->vidx;
							tri[1] = (ps - 1)->vidx;
							tri[2] = (ps - gridSlice - 1)->vidx;
							addTriangle(indices, tri, isBackFace);
						}
					}
					if (ps->side & 0x4)
					{
						if (ix > 0 && iy > 0)
						{
							tri[0] = (ps - 1)->vidx;
							tri[1] = (ps)->vidx;
							tri[2] = (ps - gridRow)->vidx;
							addTriangle(indices, tri, isBackFace);
							tri[0] = (ps - gridRow)->vidx;
							tri[1] = (ps - gridRow - 1)->vidx;
							tri[2] = (ps - 1)->vidx;
							addTriangle(indices, tri, isBackFace);
						}
					}
				}
			}
		}

		if (indices.size() < 3)
			return Result(Success); // do not create empty gfx resources

		// prepare gfx resources
		GfxSystem *gfxSystem = volume.GetIsosurface().GetRealm().GetGfxSystem();
		if (ur_null == gfxSystem)
			return Result(Failure);

		// create vertex Buffer
		auto &gfxVB = mesh.gfxVB;
		Result res = gfxSystem->CreateBuffer(gfxVB);
		if (Succeeded(res))
		{
			GfxResourceData gfxRes = { vertices.data(), (ur_uint)vertices.size() * sizeof(Vertex), 0 };
			res = gfxVB->Initialize(gfxRes.RowPitch, GfxUsage::Immutable, (ur_uint)GfxBindFlag::VertexBuffer, 0, &gfxRes);
		}
		if (Failed(res))
			return Result(Failure);

		// create index buffer
		auto &gfxIB = mesh.gfxIB;
		res = gfxSystem->CreateBuffer(gfxIB);
		if (Succeeded(res))
		{
			GfxResourceData gfxRes = { indices.data(), (ur_uint)indices.size() * sizeof(Index), 0 };
			res = gfxIB->Initialize(gfxRes.RowPitch, GfxUsage::Immutable, (ur_uint)GfxBindFlag::IndexBuffer, 0, &gfxRes);
		}
		if (Failed(res))
			return Result(Failure);

		return Result(Success);
	}

	Result Isosurface::SurfaceNet::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj)
	{
		Result res = Result(Success);

		if (this->tree.get() != ur_null)
		{
			res = this->Render(gfxContext, this->tree->GetRoot());
		}

		// temp: alwyas draw debug info
		DrawStats();
		if (this->tree.get() != ur_null)
		{
			DrawTreeBounds(this->tree->GetRoot());
		}
		GenericRender *genericRender = this->isosurface.GetRealm().GetComponent<GenericRender>();
		if (genericRender != ur_null)
		{
			genericRender->Render(gfxContext, viewProj);
		}

		return res;
	}

	Result Isosurface::SurfaceNet::Render(GfxContext &gfxContext, MeshTree::Node *meshNode)
	{
		if (ur_null == meshNode)
			return Result(InvalidArgs);

		if (meshNode->HasSubNodes())
		{
			for (ur_uint i = 0; i < AdaptiveVolume::Node::SubNodesCount; ++i)
			{
				this->Render(gfxContext, meshNode->GetSubNode(i));
			}
		}
		else
		{
			const MeshBlock &meshBlock = meshNode->GetData();
			const ur_uint indexCount = (meshBlock.gfxIB.get() ? meshBlock.gfxIB->GetDesc().Size / sizeof(Index) : 0);
			gfxContext.SetVertexBuffer(meshBlock.gfxVB.get(), 0, sizeof(Vertex), 0);
			gfxContext.SetIndexBuffer(meshBlock.gfxIB.get(), sizeof(Index) * 8, 0);
			gfxContext.DrawIndexed(indexCount, 0, 0, 0, 0);
		}

		return Result(Success);
	}

	void Isosurface::SurfaceNet::DrawTreeBounds(const MeshTree::Node *node)
	{
		if (ur_null == node)
			return;

		if (node->HasSubNodes())
		{
			for (ur_uint i = 0; i < MeshTree::Node::SubNodesCount; ++i)
			{
				this->DrawTreeBounds(node->GetSubNode(i));
			}
		}
		else
		{
			static ur_float4 DebugColor[2] = {
				{ 0.5f, 0.5f, 0.55f, 1.0f },
				{ 1.0f, 1.0f, 0.0f, 1.0f }
			};
			bool hasSurface = (node->GetData().gfxIB.get() != ur_null && node->GetData().gfxIB->GetDesc().Size > 0);
			if (hasSurface)
			{
				GenericRender *genericRender = this->isosurface.GetRealm().GetComponent<GenericRender>();
				if (genericRender != ur_null)
				{
					genericRender->DrawBox(node->GetBBox().Min, node->GetBBox().Max, DebugColor[1]);
				}
			}
		}
	}

	void Isosurface::SurfaceNet::DrawStats()
	{
		memset(&this->stats, 0, sizeof(this->stats));
		if (this->tree.get() != ur_null)
		{
			this->GatherStats(this->tree->GetRoot());
		}

		ImGui::SetNextWindowSize({ 250.0f, 250.0f });
		ImGui::SetNextWindowPos({ 0.0f, 0.0f });
		ImGui::Begin("Isosurface", ur_null, ImGuiWindowFlags_NoResize);
		ImGui::Separator();
		ImGui::Text("Options:");
		//ImGui::Checkbox("Draw bounds", &this->drawDebugBounds);
		//ImGui::Checkbox("Draw Wireframe", &this->drawWireframe);
		ImGui::Separator();
		ImGui::Text("Statistics:");
		ImGui::Text("Tree Depth: %i", this->tree.get() ? this->tree->GetDepth(): 0);
		ImGui::Text("Tree Nodes: %i", this->stats.nodes);
		ImGui::Text("Primitives: %i", this->stats.primitivesCount);
		ImGui::Text("Vertices: %i", this->stats.verticesCount);
		ImGui::Text("VideoMemory: %i", this->stats.videoMemory);
		ImGui::End();
	}

	void Isosurface::SurfaceNet::GatherStats(const MeshTree::Node *node)
	{
		if (ur_null == node)
			return;

		this->stats.nodes += 1;

		if (node->GetData().gfxVB.get() != ur_null)
		{
			this->stats.videoMemory += node->GetData().gfxVB->GetDesc().Size;
			this->stats.verticesCount += node->GetData().gfxVB->GetDesc().Size / sizeof(Vertex);
		}
		if (node->GetData().gfxIB.get() != ur_null)
		{
			this->stats.videoMemory += node->GetData().gfxIB->GetDesc().Size;
			this->stats.primitivesCount += (node->GetData().gfxIB->GetDesc().Size / sizeof(Index)) / 3;
		}

		if (node->HasSubNodes())
		{
			for (ur_uint i = 0; i < AdaptiveVolume::Node::SubNodesCount; ++i)
			{
				this->GatherStats(node->GetSubNode(i));
			}
		}
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface::HybridTetrahedra
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	const Isosurface::HybridTetrahedra::Tetrahedron::Edge Isosurface::HybridTetrahedra::Tetrahedron::Edges[EdgesCount] = {
		{ 0, 1 }, { 1, 2 }, { 2, 0 }, { 0, 3 }, { 1, 3 }, { 2, 3 }
	};

	const Isosurface::HybridTetrahedra::Tetrahedron::Face Isosurface::HybridTetrahedra::Tetrahedron::Faces[FacesCount] = {
		{ { 0, 1, 2 }, { 0, 1, 2 } },
		{ { 0, 3, 1 }, { 0, 3, 4 } },
		{ { 1, 3, 2 }, { 1, 4, 5 } },
		{ { 2, 3, 0 }, { 2, 5, 3 } }
	};

	const Isosurface::HybridTetrahedra::Tetrahedron::SplitInfo Isosurface::HybridTetrahedra::Tetrahedron::EdgeSplitInfo[EdgesCount] = {
		{ { { 0, 0xff, 2, 3 }, { 0xff, 1, 2, 3 } } },
		{ { { 0, 1, 0xff, 3 }, { 0, 0xff, 2, 3 } } },
		{ { { 0, 1, 0xff, 3 }, { 0xff, 1, 2, 3 } } },
		{ { { 0, 1, 2, 0xff }, { 0xff, 1, 2, 3 } } },
		{ { { 0, 1, 2, 0xff }, { 0, 0xff, 2, 3 } } },
		{ { { 0, 1, 2, 0xff }, { 0, 1, 0xff, 3 } } }
	};

	Isosurface::HybridTetrahedra::Tetrahedron::Tetrahedron()
	{
		memset(this->edgeAdjacency, 0, sizeof(this->edgeAdjacency));
		this->longestEdgeIdx = 0;
	}

	void Isosurface::HybridTetrahedra::Tetrahedron::Init(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3)
	{
		this->vertices[0] = v0;
		this->vertices[1] = v1;
		this->vertices[2] = v2;
		this->vertices[3] = v3;

		float maxLen = 0.0f;
		for (ur_byte eidx = 0; eidx < EdgesCount; ++eidx)
		{
			const Edge &e = Edges[eidx];
			float len = (this->vertices[e.vid[0]] - this->vertices[e.vid[1]]).Length();
			if (len > maxLen)
			{
				maxLen = len;
				this->longestEdgeIdx = eidx;
			}
		}
	}

	int Isosurface::HybridTetrahedra::Tetrahedron::UpdateAdjacency(Tetrahedron *th)
	{
		if (ur_null == th)
			return -1;

		// compare edges with each other
		// edges are equal if they have 2 equal vertices
		for (ur_uint e0 = 0; e0 < EdgesCount; ++e0)
		{
			for (ur_uint e1 = 0; e1 < EdgesCount; ++e1)
			{
				const ur_float3 &e0_v0 = this->vertices[Edges[e0].vid[0]];
				const ur_float3 &e0_v1 = this->vertices[Edges[e0].vid[1]];
				const ur_float3 &e1_v0 = th->vertices[Edges[e1].vid[0]];
				const ur_float3 &e1_v1 = th->vertices[Edges[e1].vid[1]];
				if ((e0_v0 == e1_v0 && e0_v1 == e1_v1) || (e0_v0 == e1_v1 && e0_v1 == e1_v0))
				{
					this->LinkAdjacentTetrahedra(th, e0, e1);
					return e1;
				}
			}
		}

		return -1;
	}

	void Isosurface::HybridTetrahedra::Tetrahedron::LinkAdjacentTetrahedra(Tetrahedron *th, ur_uint myEdgeIdx, ur_uint adjEdgeIdx)
	{
		auto insertToAdjList = [](Tetrahedron **list, Tetrahedron *item) -> void
		{
			int insertPos = -1;
			for (int i = 5; i >= 0; --i)
			{
				if (list[i] == ur_null) insertPos = i;
				else if (list[i] == item) return;
			}
			assert(insertPos >= 0);
			list[insertPos] = item;
		};

		Tetrahedron **myEdgeList = this->edgeAdjacency[myEdgeIdx];
		Tetrahedron **adjEdgeList = th->edgeAdjacency[adjEdgeIdx];
		insertToAdjList(myEdgeList, th);
		insertToAdjList(adjEdgeList, this);
	}

	void Isosurface::HybridTetrahedra::Tetrahedron::UnlinkAdjacentTetrahedra(Tetrahedron *th, ur_uint myEdgeIdx, ur_uint adjEdgeIdx)
	{
		auto removeFromAdjList = [](Tetrahedron **list, Tetrahedron *item) -> void
		{
			for (int i = 5; i >= 0; --i)
			{
				if (list[i] == item) list[i] = ur_null;
			}
		};

		Tetrahedron **myEdgeList = this->edgeAdjacency[myEdgeIdx];
		Tetrahedron **adjEdgeList = th->edgeAdjacency[adjEdgeIdx];
		removeFromAdjList(myEdgeList, th);
		removeFromAdjList(adjEdgeList, this);
	}

	void Isosurface::HybridTetrahedra::Tetrahedron::Split()
	{
		if (this->HasChildren())
			return;

		// create sub tetrahedra
		const ur_byte *ev = this->Edges[this->longestEdgeIdx].vid;
		Vector3 ecp = this->vertices[ev[0]] * 0.5f + this->vertices[ev[1]] * 0.5f;
		const SplitInfo &splitInfo = EdgeSplitInfo[this->longestEdgeIdx];
		for (ur_uint subIdx = 0; subIdx < ChildrenCount; ++subIdx)
		{
			std::unique_ptr<Tetrahedron> subTetrahedron(new Tetrahedron());
			const ur_byte *vid = splitInfo.subTetrahedra[subIdx];
			subTetrahedron->Init(
				(vid[0] != 0xff ? this->vertices[vid[0]] : ecp),
				(vid[1] != 0xff ? this->vertices[vid[1]] : ecp),
				(vid[2] != 0xff ? this->vertices[vid[2]] : ecp),
				(vid[3] != 0xff ? this->vertices[vid[3]] : ecp));
			this->children[subIdx] = std::move(subTetrahedron);
		}
	}

	void Isosurface::HybridTetrahedra::Tetrahedron::Merge()
	{
		if (!this->HasChildren())
			return;

		for (ur_size i = 0; i < ChildrenCount; ++i)
		{
			this->children[i].reset(ur_null);
		}
	}

	Isosurface::HybridTetrahedra::HybridTetrahedra(Isosurface &isosurface) :
		Presentation(isosurface)
	{

	}

	Isosurface::HybridTetrahedra::~HybridTetrahedra()
	{

	}

	Result Isosurface::HybridTetrahedra::Construct(AdaptiveVolume &volume)
	{
		// init roots
		const BoundingBox &bbox = volume.GetDesc().Bound;
		if (ur_null == this->root[0].get())
		{
			std::unique_ptr<Tetrahedron> th(new Tetrahedron());
			th->Init(
				{ bbox.Min.x, bbox.Min.y, bbox.Min.z },
				{ bbox.Min.x, bbox.Max.y, bbox.Min.z },
				{ bbox.Max.x, bbox.Max.y, bbox.Min.z },
				{ bbox.Max.x, bbox.Max.y, bbox.Max.z }
			);
			this->root[0] = std::move(th);
		}
		if (ur_null == this->root[1].get())
		{
			std::unique_ptr<Tetrahedron> th(new Tetrahedron());
			th->Init(
				{ bbox.Min.x, bbox.Min.y, bbox.Min.z },
				{ bbox.Max.x, bbox.Min.y, bbox.Min.z },
				{ bbox.Max.x, bbox.Max.y, bbox.Min.z },
				{ bbox.Max.x, bbox.Max.y, bbox.Max.z }
			);
			for (ur_uint i = 0; i < 1; ++i) th->UpdateAdjacency(this->root[i].get());
			this->root[1] = std::move(th);
		}
		if (ur_null == this->root[2].get())
		{
			std::unique_ptr<Tetrahedron> th(new Tetrahedron());
			th->Init(
				{ bbox.Min.x, bbox.Min.y, bbox.Min.z },
				{ bbox.Max.x, bbox.Min.y, bbox.Min.z },
				{ bbox.Max.x, bbox.Min.y, bbox.Max.z },
				{ bbox.Max.x, bbox.Max.y, bbox.Max.z }
			);
			for (ur_uint i = 0; i < 2; ++i) th->UpdateAdjacency(this->root[i].get());
			this->root[2] = std::move(th);
		}
		if (ur_null == this->root[3].get())
		{
			std::unique_ptr<Tetrahedron> th(new Tetrahedron());
			th->Init(
				{ bbox.Min.x, bbox.Min.y, bbox.Min.z },
				{ bbox.Min.x, bbox.Max.y, bbox.Min.z },
				{ bbox.Min.x, bbox.Max.y, bbox.Max.z },
				{ bbox.Max.x, bbox.Max.y, bbox.Max.z }
			);
			for (ur_uint i = 0; i < 3; ++i) th->UpdateAdjacency(this->root[i].get());
			this->root[3] = std::move(th);
		}
		if (ur_null == this->root[4].get())
		{
			std::unique_ptr<Tetrahedron> th(new Tetrahedron());
			th->Init(
				{ bbox.Min.x, bbox.Min.y, bbox.Min.z },
				{ bbox.Min.x, bbox.Min.y, bbox.Max.z },
				{ bbox.Min.x, bbox.Max.y, bbox.Max.z },
				{ bbox.Max.x, bbox.Max.y, bbox.Max.z }
			);
			for (ur_uint i = 0; i < 4; ++i) th->UpdateAdjacency(this->root[i].get());
			this->root[4] = std::move(th);
		}
		if (ur_null == this->root[5].get())
		{
			std::unique_ptr<Tetrahedron> th(new Tetrahedron());
			th->Init(
				{ bbox.Min.x, bbox.Min.y, bbox.Min.z },
				{ bbox.Min.x, bbox.Min.y, bbox.Max.z },
				{ bbox.Max.x, bbox.Min.y, bbox.Max.z },
				{ bbox.Max.x, bbox.Max.y, bbox.Max.z }
			);
			for (ur_uint i = 0; i < 5; ++i) th->UpdateAdjacency(this->root[i].get());
			this->root[5] = std::move(th);
		}
			
		// temp
		static bool freeze = false;
		Input *input = this->isosurface.GetRealm().GetInput();
		if (input && input->GetKeyboard() && input->GetKeyboard()->IsKeyReleased(Input::VKey::P))
			freeze = !freeze;
		if (freeze)
			return Success;

		// update hierarchy
		Result res(Success);
		for (auto &tetrahedron : this->root)
		{
			res = CombinedResult(res, this->Construct(volume, tetrahedron.get()));
		}

		return res;
	}

	float Isosurface::HybridTetrahedra::SmallestNodeSize(const BoundingBox &bbox, AdaptiveVolume::Node *volumeNode)
	{
		if (ur_null == volumeNode || !bbox.Intersects(volumeNode->GetBBox()))
			return bbox.SizeMax();

		float nodeSize = volumeNode->GetBBox().SizeMin();
		if (volumeNode->HasSubNodes())
		{
			for (ur_uint i = 0; i < AdaptiveVolume::Node::SubNodesCount; ++i)
			{
				nodeSize = std::min(nodeSize, this->SmallestNodeSize(bbox, volumeNode->GetSubNode(i)));
			}
		}

		return nodeSize;
	}

	Result Isosurface::HybridTetrahedra::Construct(AdaptiveVolume &volume, Tetrahedron *tetrahedron)
	{
		Result res(Success);
		if (ur_null == tetrahedron)
			return res;

		res = CombinedResult( this->UpdateLoD(volume, tetrahedron), res );

		res = CombinedResult( this->BuildMesh(volume, tetrahedron), res );

		if (tetrahedron->HasChildren())
		{
			for (auto &subTetrahedron : tetrahedron->children)
			{
				res = CombinedResult( this->Construct(volume, subTetrahedron.get()), res);
			}
		}

		return res;
	}

	Result Isosurface::HybridTetrahedra::UpdateLoD(AdaptiveVolume &volume, Tetrahedron *tetrahedron)
	{
		Result res(Success);
		if (ur_null == tetrahedron)
			return res;

		// check whether tetrahedron's longest edge is a termional edge
		bool isTerminalEdge = true;
		Tetrahedron **edgeAdj = tetrahedron->edgeAdjacency[tetrahedron->longestEdgeIdx];
		for (int i = 0; i < 6; ++i)
		{
			Tetrahedron *adjTetrahedron = edgeAdj[i];
			if (adjTetrahedron != ur_null)
			{
				const ur_float3 &e0_v0 = tetrahedron->vertices[HybridTetrahedra::Tetrahedron::Edges[tetrahedron->longestEdgeIdx].vid[0]];
				const ur_float3 &e0_v1 = tetrahedron->vertices[HybridTetrahedra::Tetrahedron::Edges[tetrahedron->longestEdgeIdx].vid[1]];
				const ur_float3 &e1_v0 = adjTetrahedron->vertices[HybridTetrahedra::Tetrahedron::Edges[adjTetrahedron->longestEdgeIdx].vid[0]];
				const ur_float3 &e1_v1 = adjTetrahedron->vertices[HybridTetrahedra::Tetrahedron::Edges[adjTetrahedron->longestEdgeIdx].vid[1]];
				bool equalEdges = ((e0_v0 == e1_v0 && e0_v1 == e1_v1) || (e0_v0 == e1_v1 && e0_v1 == e1_v0));
				isTerminalEdge &= equalEdges;
				if (!isTerminalEdge) break;
			}
		}
		bool doSplit = isTerminalEdge;

		if (doSplit)
		{
			// find smallest node size in the intersected volume
			// todo: precompute BBox during init step
			const ur_float3 &ev0 = tetrahedron->vertices[HybridTetrahedra::Tetrahedron::Edges[tetrahedron->longestEdgeIdx].vid[0]];
			const ur_float3 &ev1 = tetrahedron->vertices[HybridTetrahedra::Tetrahedron::Edges[tetrahedron->longestEdgeIdx].vid[1]];
			BoundingBox bbox;
			bbox.Min.SetMin(ev0); bbox.Min.SetMin(ev1);
			bbox.Max.SetMax(ev0); bbox.Max.SetMax(ev1);
			float nodeSize = this->SmallestNodeSize(bbox, volume.GetRoot());
			doSplit = (nodeSize < bbox.SizeMax() * 0.5f);
		}

		if (doSplit)
		{
			tetrahedron->Split();
		}
		else
		{
			tetrahedron->Merge();
		}

		return res;
	}

	Result Isosurface::HybridTetrahedra::BuildMesh(AdaptiveVolume &volume, Tetrahedron *tetrahedron)
	{
		Result res(Success);
		if (ur_null == tetrahedron)
			return res;

		// gather underlying data blocks into a regular grid for more optimal sampling

		BoundingBox bbox;
		for (auto &v : tetrahedron->vertices)
		{
			bbox.Min.SetMin(v);
			bbox.Max.SetMax(v);
		}

		ur_uint maxLevel = volume.MaxRefinementLevel(bbox);
		std::vector<Block*> gridBlocks;
		ur_uint3 gridSize;
		ur_float3 gridBlockSize;
		BoundingBox gridBBox;
		volume.GatherBlocks(maxLevel, bbox, gridBlocks, gridSize, gridBlockSize, gridBBox);
		if (gridSize.x == 0 || gridSize.y == 0 || gridSize.z == 0)
			return res; // no intersected data blocks

		// todo: create surface net

		return res;
	}

	Result Isosurface::HybridTetrahedra::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj)
	{
		// todo

		// debug render
		GenericRender *genericRender = this->isosurface.GetRealm().GetComponent<GenericRender>();
		if (genericRender != ur_null)
		{
			for (auto &tetrahedron : this->root)
			{
				DrawTetrahedra(gfxContext, *genericRender, tetrahedron.get());
			}
		}

		return Result(Success);
	}

	void Isosurface::HybridTetrahedra::DrawTetrahedra(GfxContext &gfxContext, GenericRender &genericRender, Tetrahedron *tetrahedron)
	{
		if (ur_null == tetrahedron)
			return;

		if (tetrahedron->HasChildren())
		{
			for (auto &child : tetrahedron->children)
			{
				DrawTetrahedra(gfxContext, genericRender, child.get());
			}
		}
		else
		{
			static const ur_float4 s_debugColor = { 1.0f, 1.0f, 0.0f, 1.0f };
			for (const auto &edge : Tetrahedron::Edges)
			{
				genericRender.DrawLine(
					tetrahedron->vertices[edge.vid[0]],
					tetrahedron->vertices[edge.vid[1]],
					s_debugColor);
			}
		}
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface::Builder
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::Builder::Builder(Isosurface &isosurface) :
		SubSystem(isosurface)
	{
	}

	Isosurface::Builder::~Builder()
	{

	}

	void Isosurface::Builder::AddNode(AdaptiveVolume::Node *node)
	{
		if (ur_null == node)
			return;

		this->newNodes.insert(node);
	}

	void Isosurface::Builder::RemoveNode(AdaptiveVolume::Node *node)
	{
		this->newNodes.erase(node);
	}

	Result Isosurface::Builder::Build()
	{
		// acquire sub systems
		AdaptiveVolume *volume = this->isosurface.GetVolume();
		Loader *loader = this->isosurface.GetLoader();
		Presentation *presentation = this->isosurface.GetPresentation();
		if (ur_null == volume ||
			ur_null == loader ||
			ur_null == presentation)
			return Result(NotInitialized);

		// temp: simple synchronous build process
		// support: sync/async construction

		// load new volume blocks
		for (AdaptiveVolume::Node *node : this->newNodes)
		{
			loader->Load(*volume, node->GetData(), node->GetBBox());
		}
		this->newNodes.clear();

		// update presentation
		presentation->Construct(*volume);

		return Result(Success);
	}

	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::Isosurface(Realm &realm) :
		RealmEntity(realm)
	{
	}

	Isosurface::~Isosurface()
	{
		// explicit deinitialization, order matters:
		// volume calls handler function, which expects builder to be valid or at least null
		this->volume.reset(ur_null);
		this->builder.reset(ur_null);
		this->loader.reset(ur_null);
		this->presentation.reset(ur_null);
	}

	Result Isosurface::Init(const AdaptiveVolume::Desc &desc, std::unique_ptr<Loader> loader, std::unique_ptr<Presentation> presentation)
	{
		Result res(Success);

		this->volume.reset(new AdaptiveVolume(*this));
		this->builder.reset(new Builder(*this));
		this->loader = std::move(loader);
		this->presentation = std::move(presentation);
		
		res = this->volume->Init(desc);
		if (Failed(res))
			return ResultError(Failure, "Isosurface::Init: failed to initialize volume");
		
		res = this->CreateGfxObjects();

		return res;
	}

	Result Isosurface::Update()
	{
		// build volume nodes
		if (this->builder.get() != ur_null)
		{
			this->builder->Build();
		}

		return Result(Success);
	}

	Result Isosurface::CreateGfxObjects()
	{
		Result res = Result(Success);

		this->gfxObjects.reset(new GfxObjects());

		// VS
		{
			std::unique_ptr<File> file;
			res = this->GetRealm().GetStorage().Open("Isosurface_vs.cso", ur_uint(StorageAccess::Read) | ur_uint(StorageAccess::Binary), file);
			if (Succeeded(res))
			{
				ur_size sizeInBytes = file->GetSize();
				std::unique_ptr<ur_byte[]> bytecode(new ur_byte[sizeInBytes]);
				file->Read(sizeInBytes, bytecode.get());
				res = this->GetRealm().GetGfxSystem()->CreateVertexShader(this->gfxObjects->VS);
				if (Succeeded(res))
				{
					res = this->gfxObjects->VS->Initialize(std::move(bytecode), sizeInBytes);
				}
			}
		}
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGfxObjects: failed to initialize VS");

		// PS
		{
			std::unique_ptr<File> file;
			res = this->GetRealm().GetStorage().Open("Isosurface_ps.cso", ur_uint(StorageAccess::Read) | ur_uint(StorageAccess::Binary), file);
			if (Succeeded(res))
			{
				ur_size sizeInBytes = file->GetSize();
				std::unique_ptr<ur_byte[]> bytecode(new ur_byte[sizeInBytes]);
				file->Read(sizeInBytes, bytecode.get());
				res = this->GetRealm().GetGfxSystem()->CreatePixelShader(this->gfxObjects->PS);
				if (Succeeded(res))
				{
					res = this->gfxObjects->PS->Initialize(std::move(bytecode), sizeInBytes);
				}
			}
		}
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGfxObjects: failed to initialize PS");

		// Input Layout
		res = this->GetRealm().GetGfxSystem()->CreateInputLayout(this->gfxObjects->inputLayout);
		if (Succeeded(res))
		{
			GfxInputElement elements[] = {
				{ GfxSemantic::Position, 0, 0, GfxFormat::R32G32B32, GfxFormatView::Float, 0 },
				{ GfxSemantic::Color, 0, 0, GfxFormat::R8G8B8A8, GfxFormatView::Unorm, 0 }
			};
			res = this->gfxObjects->inputLayout->Initialize(*this->gfxObjects->VS.get(), elements, ur_array_size(elements));
		}
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGfxObjects: failed to initialize input layout");

		// Pipeline State
		res = this->GetRealm().GetGfxSystem()->CreatePipelineState(this->gfxObjects->pipelineState);
		if (Succeeded(res))
		{
			this->gfxObjects->pipelineState->InputLayout = this->gfxObjects->inputLayout.get();
			this->gfxObjects->pipelineState->VertexShader = this->gfxObjects->VS.get();
			this->gfxObjects->pipelineState->PixelShader = this->gfxObjects->PS.get();
			GfxRenderState gfxRS = GfxRenderState::Default;
			gfxRS.RasterizerState.CullMode = GfxCullMode::CCW;
			gfxRS.RasterizerState.FillMode = GfxFillMode::Solid;
			res = this->gfxObjects->pipelineState->SetRenderState(gfxRS);
		}
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGfxObjects: failed to initialize pipeline state");

		// Constant Buffer
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxObjects->CB);
		if (Succeeded(res))
		{
			res = this->gfxObjects->CB->Initialize(sizeof(CommonCB), GfxUsage::Dynamic,
				(ur_uint)GfxBindFlag::ConstantBuffer, (ur_uint)GfxAccessFlag::Write);
		}
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGfxObjects: failed to initialize constant buffer");

		return res;
	}

	Result Isosurface::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj)
	{
		Result res(Success);
		if (ur_null == this->volume)
			return res;

		// render isosurface

		if (this->presentation.get() != ur_null)
		{
			CommonCB cb;
			cb.viewProj = viewProj;
			GfxResourceData cbResData = { &cb, sizeof(CommonCB), 0 };
			gfxContext.UpdateBuffer(this->gfxObjects->CB.get(), GfxGPUAccess::WriteDiscard, false, &cbResData, 0, cbResData.RowPitch);
			const RectI &canvasBound = this->GetRealm().GetCanvas()->GetBound();
			GfxViewPort viewPort = { 0.0f, 0.0f, (float)canvasBound.Width(), (float)canvasBound.Height(), 0.0f, 1.0f };
			gfxContext.SetViewPort(&viewPort);
			gfxContext.SetConstantBuffer(this->gfxObjects->CB.get(), 0);
			gfxContext.SetPipelineState(this->gfxObjects->pipelineState.get());

			res = this->presentation->Render(gfxContext, viewProj);
		}

		// render debug info
		/*
		if (this->GetRealm().GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::B)) this->drawDebugBounds = !this->drawDebugBounds;
		if (this->GetRealm().GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::M)) this->drawWireframe = !this->drawWireframe;
		
		if (this->drawDebugBounds)
		{
			this->DrawDebugTreeBounds(this->volume->GetRoot());
		}
		this->debugRender.Render(gfxContext, viewProj);

		this->GatherStats();

		ImGui::SetNextWindowSize({ 250.0f, 250.0f });
		ImGui::SetNextWindowPos({ 0.0f, 0.0f });
		ImGui::Begin("Isosurface", ur_null, ImGuiWindowFlags_NoResize);
		ImGui::Separator();
		ImGui::Text("Options:");
		ImGui::Checkbox("Draw bounds", &this->drawDebugBounds);
		ImGui::Checkbox("Draw Wireframe", &this->drawWireframe);
		ImGui::Separator();
		ImGui::Text("Statistics:");
		ImGui::Text("Volume Depth: %i", this->volume->GetLevelsCount());
		ImGui::Text("Volume Nodes: %i", this->stats.volumeNodes);
		ImGui::Text("Surface Nodes: %i", this->stats.surfaceNodes);
		ImGui::Text("Primitives: %i", this->stats.primitivesCount);
		ImGui::Text("Vertices: %i", this->stats.verticesCount);
		ImGui::Text("VideoMemory: %i", this->stats.videoMemory);
		ImGui::Text("SysMemory: %i", this->stats.sysMemory);
		ImGui::End();

		if (this->gfxObjects->pipelineState != ur_null)
		{
			const GfxFillMode &gfxFillMode = this->gfxObjects->pipelineState->GetRenderState().RasterizerState.FillMode;
			if ((this->drawWireframe == true && GfxFillMode::Solid == gfxFillMode) ||
				(this->drawWireframe == false && GfxFillMode::Wireframe == gfxFillMode))
			{
				GfxRenderState gfxRS = this->gfxObjects->pipelineState->GetRenderState();
				gfxRS.RasterizerState.FillMode = (this->drawWireframe ? GfxFillMode::Wireframe : GfxFillMode::Solid);
				this->gfxObjects->pipelineState->SetRenderState(gfxRS);
			}
		}
		*/
		return res;
	}

} // end namespace UnlimRealms