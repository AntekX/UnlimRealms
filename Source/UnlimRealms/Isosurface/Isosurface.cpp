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
#include "Resources/Resources.h"
#include "ImguiRender/ImguiRender.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface::AdaptiveVolume
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Isosurface::AdaptiveVolume::AdaptiveVolume(Isosurface &isosurface, ur_uint depth, Handler handler) :
	//	SubSystem(isosurface),
	//	Octree<Isosurface::Block>(depth, handler)
	//{
	//}

	//Isosurface::AdaptiveVolume::~AdaptiveVolume()
	//{

	//}

	//Result Isosurface::AdaptiveVolume::Init(const Desc &desc)
	//{
	//	this->levelInfo.clear();
	//	this->desc = desc;
	//	this->desc.BlockResolution.SetMax(ur_uint3(1));
	//	this->desc.BlockSize.SetMax(ur_float3(0.0f));

	//	if (this->desc.Bound.IsInsideOut() ||
	//		this->desc.BlockSize.All() == false ||
	//		this->desc.DetailLevelDistance <= 0 ||
	//		this->desc.RefinementProgression < 1.0f)
	//		return Result(InvalidArgs);

	//	ur_float blockSizeMax = std::max(std::max(this->desc.BlockSize.x, this->desc.BlockSize.y), this->desc.BlockSize.z);
	//	ur_float blockSizeMin = std::min(std::min(this->desc.BlockSize.x, this->desc.BlockSize.y), this->desc.BlockSize.z);
	//	ur_float boundSizeMax = std::max(std::max(this->desc.Bound.SizeX(), this->desc.Bound.SizeY()), this->desc.Bound.SizeZ());
	//	ur_uint blocksAlongSide = ur_uint(ceil(boundSizeMax / blockSizeMin));
	//	this->alignedBound.Min = this->desc.Bound.Min;
	//	this->alignedBound.Max = this->alignedBound.Min + blockSizeMin * blocksAlongSide;

	//	ur_uint levelsCount = ur_uint(log2(blocksAlongSide)) + 1;
	//	this->levelInfo.resize(levelsCount);
	//	ur_float levelDistance = this->desc.DetailLevelDistance;
	//	for (ur_uint i = 0; i < levelsCount; ++i)
	//	{
	//		LevelInfo &level = this->levelInfo[levelsCount - i - 1];
	//		ur_uint blocksInRange = std::max(ur_uint(levelDistance / blockSizeMax + 0.5f), ur_uint(2));
	//		levelDistance = std::max(blockSizeMax * blocksInRange, levelDistance);
	//		level.Distance = levelDistance;
	//		levelDistance *= this->desc.RefinementProgression;
	//	}

	//	return Octree<Isosurface::Block>::Init(this->alignedBound);
	//}

	//void Isosurface::AdaptiveVolume::OnNodeChanged(Node* node, Event e)
	//{
	//	if (ur_null == node)
	//		return;

	//	AdaptiveVolume& volume = static_cast<AdaptiveVolume&>(node->GetTree());
	//	Builder *builder = volume.GetIsosurface().GetBuilder();
	//	if (builder != ur_null)
	//	{
	//		switch (e)
	//		{
	//		case Event::NodeAdded: builder->AddNode(node); break;
	//		case Event::NodeRemoved: builder->RemoveNode(node); break;
	//		}
	//	}
	//}

	//void Isosurface::AdaptiveVolume::RefinementByDistance(const ur_float3 &pos)
	//{
	//	if (this->levelInfo.empty())
	//		return;

	//	this->refinementPoint = pos;

	//	this->RefinementByDistance(pos, this->GetRoot());
	//}

	//void Isosurface::AdaptiveVolume::RefinementByDistance(const ur_float3 &pos, AdaptiveVolume::Node *node)
	//{
	//	if (ur_null == node || (node->GetLevel() + 1) >= this->GetLevelsCount())
	//		return;

	//	if (node->GetBBox().Distance(pos) <= this->levelInfo[node->GetLevel()].Distance)
	//	{
	//		node->Split();
	//		if (node->HasSubNodes())
	//		{
	//			for (ur_uint i = 0; i < Node::SubNodesCount; ++i)
	//			{
	//				this->RefinementByDistance(pos, node->GetSubNode(i));
	//			}
	//		}
	//	}
	//	else
	//	{
	//		node->Merge();
	//	}
	//}

	//ur_uint Isosurface::AdaptiveVolume::MaxRefinementLevel(const BoundingBox &bbox)
	//{
	//	if (ur_null == this->GetRoot() || !bbox.Intersects(this->GetRoot()->GetBBox()))
	//		return 0;

	//	ur_uint maxLevel = this->GetLevelsCount();
	//	
	//	this->MaxRefinementLevel(maxLevel, bbox, this->GetRoot());
	//	
	//	return maxLevel;
	//}

	//void Isosurface::AdaptiveVolume::MaxRefinementLevel(ur_uint &maxLevel, const BoundingBox &bbox, AdaptiveVolume::Node *node)
	//{
	//	if (node == ur_null ||
	//		node->GetLevel() > maxLevel ||
	//		node->GetBBox().Intersects(bbox) == false)
	//		return;

	//	if (node->HasSubNodes())
	//	{
	//		for (ur_uint i = 0; i < Node::SubNodesCount; ++i)
	//		{
	//			this->MaxRefinementLevel(maxLevel, bbox, node->GetSubNode(i));
	//		}
	//	}
	//	else
	//	{
	//		maxLevel = std::min(maxLevel, node->GetLevel());
	//	}
	//}

	//void Isosurface::AdaptiveVolume::GatherBlocks(const ur_uint level, const BoundingBox &bbox, BlockArray &blockArray)
	//{		
	//	if (0 == this->GetLevelsCount() || ur_null == this->GetRoot())
	//		return;

	//	const BoundingBox &rbbox = this->GetRoot()->GetBBox();
	//	BoundingBox ibbox = rbbox;
	//	ibbox.Min.SetMax(bbox.Min);
	//	ibbox.Max.SetMin(bbox.Max);
	//	if (ibbox.IsInsideOut())
	//		return;
	//	
	//	ibbox.Min -= rbbox.Min;
	//	ibbox.Max -= rbbox.Min;

	//	blockArray.blockResolution = this->desc.BlockResolution;
	//	blockArray.blockBorder = BorderSize;
	//	blockArray.blockSize = this->desc.BlockSize * float(1 << (this->GetLevelsCount() - level - 1));
	//	
	//	ur_int3 gridMin(
	//		(ur_int)floor(ibbox.Min.x / blockArray.blockSize.x),
	//		(ur_int)floor(ibbox.Min.y / blockArray.blockSize.y),
	//		(ur_int)floor(ibbox.Min.z / blockArray.blockSize.z));
	//	ur_int3 gridMax(
	//		(ur_int)ceil(ibbox.Max.x / blockArray.blockSize.x),
	//		(ur_int)ceil(ibbox.Max.y / blockArray.blockSize.y),
	//		(ur_int)ceil(ibbox.Max.z / blockArray.blockSize.z));
	//	blockArray.size.x = ur_uint(gridMax.x - gridMin.x);
	//	blockArray.size.y = ur_uint(gridMax.y - gridMin.y);
	//	blockArray.size.z = ur_uint(gridMax.z - gridMin.z);

	//	blockArray.bbox.Min.x = blockArray.blockSize.x * gridMin.x + rbbox.Min.x;
	//	blockArray.bbox.Min.y = blockArray.blockSize.y * gridMin.y + rbbox.Min.y;
	//	blockArray.bbox.Min.z = blockArray.blockSize.z * gridMin.z + rbbox.Min.z;
	//	blockArray.bbox.Max.x = blockArray.blockSize.x * gridMax.x + rbbox.Min.x;
	//	blockArray.bbox.Max.y = blockArray.blockSize.y * gridMax.y + rbbox.Min.y;
	//	blockArray.bbox.Max.z = blockArray.blockSize.z * gridMax.z + rbbox.Min.z;

	//	blockArray.blocks.resize(blockArray.size.x * blockArray.size.y * blockArray.size.z, ur_null);

	//	this->GatherBlocks(this->GetRoot(), level, blockArray);
	//}

	//void Isosurface::AdaptiveVolume::GatherBlocks(AdaptiveVolume::Node *node, const ur_uint level, BlockArray &blockArray)
	//{
	//	if (node == ur_null ||
	//		node->GetBBox().Intersects(blockArray.bbox) == false)
	//		return;

	//	if (node->GetLevel() == level)
	//	{
	//		ur_uint3 gridPos(
	//			(ur_uint)floor((node->GetBBox().Min.x - blockArray.bbox.Min.x) / blockArray.blockSize.x),
	//			(ur_uint)floor((node->GetBBox().Min.y - blockArray.bbox.Min.y) / blockArray.blockSize.y),
	//			(ur_uint)floor((node->GetBBox().Min.z - blockArray.bbox.Min.z) / blockArray.blockSize.z));
	//		if (gridPos.x < blockArray.size.x && gridPos.y < blockArray.size.y && gridPos.z < blockArray.size.z)
	//		{
	//			blockArray.blocks[gridPos.x + gridPos.y * blockArray.size.x + gridPos.z * blockArray.size.x * blockArray.size.y] = &node->GetData();
	//		}
	//	}
	//	else if (node->HasSubNodes())
	//	{
	//		for (ur_uint i = 0; i < Node::SubNodesCount; ++i)
	//		{
	//			this->GatherBlocks(node->GetSubNode(i), level, blockArray);
	//		}
	//	}
	//}

	//Isosurface::Block::ValueType Isosurface::BlockArray::Sample(const ur_float3 &point) const
	//{
	//	Block::ValueType val = (Block::ValueType)0;
	//	
	//	ur_float3 fpos(
	//		(point.x - this->bbox.Min.x) / this->blockSize.x,
	//		(point.y - this->bbox.Min.y) / this->blockSize.y,
	//		(point.z - this->bbox.Min.z) / this->blockSize.z);
	//	ur_int3 bpos(
	//		std::min((ur_int)this->size.x - 1, (ur_int)floor(fpos.x)),
	//		std::min((ur_int)this->size.y - 1, (ur_int)floor(fpos.y)),
	//		std::min((ur_int)this->size.z - 1, (ur_int)floor(fpos.z)));
	//	fpos.x -= bpos.x;
	//	fpos.y -= bpos.y;
	//	fpos.z -= bpos.z;
	//	
	//	Block *block = blocks[bpos.x + bpos.y * this->size.x + bpos.z * this->size.x * this->size.y];
	//	if (block != ur_null && !block->field.empty())
	//	{
	//		// following sampling algorithm considers that cell center is on the block's corner
	//		// (cell size = block size / (resolution - 1))
	//		const ur_uint3 &bres = this->blockResolution;
	//		ur_float3 cf(
	//			lerp(0.0f, ur_float(bres.x - 1), fpos.x),
	//			lerp(0.0f, ur_float(bres.y - 1), fpos.y),
	//			lerp(0.0f, ur_float(bres.z - 1), fpos.z));
	//		ur_int3 cp(
	//			std::min((ur_int)bres.x - 1, (ur_int)floor(cf.x)),
	//			std::min((ur_int)bres.y - 1, (ur_int)floor(cf.y)),
	//			std::min((ur_int)bres.z - 1, (ur_int)floor(cf.z)));
	//		cf.x -= cp.x;
	//		cf.y -= cp.y;
	//		cf.z -= cp.z;
	//		static const ur_int3 cofs[8] = {
	//			{ 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 },
	//			{ 0, 0, 1 }, { 1, 0, 1 }, { 0, 1, 1 }, { 1, 1, 1 }
	//		};
	//		const ur_uint3 fres(bres.x + this->blockBorder * 2, bres.y + this->blockBorder * 2, bres.z + this->blockBorder * 2);
	//		const ur_uint skipBorderOfs = this->blockBorder * (fres.x * fres.y + fres.x + 1);
	//		Block::ValueType cv[8];
	//		for (ur_uint i = 0; i < 8; ++i)
	//		{
	//			cv[i] = block->field[skipBorderOfs +
	//				(cp.x + cofs[i].x) + (cp.y + cofs[i].y) * fres.x + (cp.z + cofs[i].z) * fres.x * fres.y];
	//		}
	//		ur_float v0 = lerp(cv[0], cv[1], cf.x);
	//		ur_float v1 = lerp(cv[2], cv[3], cf.x);
	//		ur_float v2 = lerp(cv[4], cv[5], cf.x);
	//		ur_float v3 = lerp(cv[6], cv[7], cf.x);
	//		val = lerp(lerp(v0, v1, cf.y), lerp(v2, v3, cf.y), cf.z);
	//	}

	//	return val;
	//}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface::DataVolume
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::DataVolume::DataVolume(Isosurface &isosurface) :
		SubSystem(isosurface)
	{

	}

	Isosurface::DataVolume::~DataVolume()
	{

	}

	Result Isosurface::DataVolume::Read(ValueType *values, const ur_float3 *points, const ur_uint count, const BoundingBox &bbox)
	{
		return Result(NotImplemented);
	}

	Result Isosurface::DataVolume::Save(const std::string &fileName)
	{
		return Result(NotImplemented);
	}

	Result Isosurface::DataVolume::Save(const std::string &fileName, ur_float cellSize, ur_uint3 blockResolution)
	{
		return Result(NotImplemented);
	}

	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface::ProceduralGenerator
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::ProceduralGenerator::ProceduralGenerator(Isosurface &isosurface, const Algorithm &algorithm, const GenerateParams &generateParams) :
		DataVolume(isosurface)
	{
		this->bound = generateParams.bound;
		this->algorithm = algorithm;
		switch (this->algorithm)
		{
		case Algorithm::SphericalDistanceField: this->generateParams.reset(new SphericalDistanceFieldParams((const SphericalDistanceFieldParams&)generateParams)); break;
		case Algorithm::SimplexNoise: this->generateParams.reset(new SimplexNoiseParams((const SimplexNoiseParams&)generateParams)); break;
		}
	}

	Isosurface::ProceduralGenerator::~ProceduralGenerator()
	{

	}

	Result Isosurface::ProceduralGenerator::Read(ValueType *values, const ur_float3 *points, const ur_uint count, const BoundingBox &bbox)
	{
		// todo: implement optimized sampling
		// do early exit (return NotFound result) if we it is obious that isosurface does not intersect bbox

		Result res = Result(Success);

		// choose algorithm and generate a value
		switch (this->algorithm)
		{
		case Algorithm::SphericalDistanceField:
		{
			res = this->GenerateSphericalDistanceField(values, points, count, bbox);
		} break;
		case Algorithm::SimplexNoise:
		{
			res = this->GenerateSimplexNoise(values, points, count, bbox);
		} break;
		default:
		{
			res = Result(NotImplemented);
		}
		}

		return res;
	}

	Result Isosurface::ProceduralGenerator::GenerateSphericalDistanceField(ValueType *values, const ur_float3 *points, const ur_uint count, const BoundingBox &bbox)
	{
		const SphericalDistanceFieldParams &params = static_cast<const SphericalDistanceFieldParams&>(*this->generateParams.get());
		ur_float radius = params.radius;
		ur_float brad = (bbox.Max - bbox.Min).Length() * 0.5f;
		ur_float dist = (bbox.Center() - params.center).Length();
		if (dist + brad < radius ||
			dist - brad > radius)
			return Result(NotFound); // does not intersect isosurface

		if (ur_null == values || ur_null == points || 0 == count)
			return Result(InvalidArgs);

		ValueType *p_value = values;
		const ur_float3 *p_point = points;
		for (ur_uint i = 0; i < count; ++i, ++p_value, ++p_point)
		{
			*p_value = params.radius - (*p_point - params.center).Length();
		}

		return Result(Success);
	}

	#define CAVE_TEST 1
	Result Isosurface::ProceduralGenerator::GenerateSimplexNoise(ValueType *values, const ur_float3 *points, const ur_uint count, const BoundingBox &bbox)
	{
		const SimplexNoiseParams &params = static_cast<const SimplexNoiseParams&>(*this->generateParams.get());
		ur_float3 center = params.bound.Center();
		ur_float brad = (bbox.Max - bbox.Min).Length() * 0.5f;
		ur_float dist = (bbox.Center() - center).Length();
		#if (!CAVE_TEST)
		if (dist + brad < params.radiusMin ||
			dist - brad > params.radiusMax)
			return Result(NotFound); // does not intersect isosurface
		#endif

		if (ur_null == values || ur_null == points || 0 == count)
			return Result(InvalidArgs);

		// temp
		#if (CAVE_TEST)
		std::vector<SimplexNoiseParams::Octave> cave_octaves = {
			{ 0.800f, 6.0f, -1.0f, 1.0f },
			{ 0.400f, 16.0f, -0.5f, 0.1f },
			{ 0.10f, 64.0f, -1.0f, 0.2f },
		};
		#endif

		SimplexNoise noise;
		ur_float radius = (params.radiusMax + params.radiusMin) * 0.5f;
		ur_float distMax = params.radiusMax - radius;
		ValueType *p_value = values;
		const ur_float3 *p_point = points;
		for (ur_uint i = 0; i < count; ++i, ++p_value, ++p_point)
		{
			*p_value = radius - (*p_point - center).Length();
			for (auto &octave : params.octaves)
			{
				ur_float val = (ur_float)noise.Noise(
					ur_double(p_point->x / radius * octave.freq),
					ur_double(p_point->y / radius * octave.freq),
					ur_double(p_point->z / radius * octave.freq));
				val *= 2.0f;
				val = std::max(octave.clamp_min, val);
				val = std::min(octave.clamp_max, val);
				val *= octave.scale * distMax;
				*p_value += val;
			}
			
			#if (CAVE_TEST)
			// test: caves
			ur_float cave_value = 0.0f;
			for (auto &octave : cave_octaves)
			{
				ur_float val = (ur_float)noise.Noise(
					ur_double(p_point->x / radius * octave.freq),
					ur_double(p_point->y / radius * octave.freq),
					ur_double(p_point->z / radius * octave.freq));
				val *= 2.0f;
				//val = (0.07f - fabs(val));
				//val = val + 0.2f;
				val = std::max(octave.clamp_min, val);
				val = std::min(octave.clamp_max, val);
				val *= octave.scale * distMax;
				cave_value += val;
			}
			//cave_value *= -1.0f;
			ur_float d = std::max(0.0f, params.radiusMin - (*p_point - center).Length());
			cave_value += d;
			*p_value = std::min(*p_value, cave_value);
			#endif
		}

		return Result(Success);
	}

	Result Isosurface::ProceduralGenerator::Save(const std::string &fileName, ur_float cellSize, ur_uint3 blockResolution)
	{
		auto &storage = this->isosurface.GetRealm().GetStorage();
		std::unique_ptr<File> file;
		Result res = storage.Open(file, fileName, ur_uint(StorageAccess::Binary) | ur_uint(StorageAccess::Append));
		if (Failed(res))
			return LogResult(Failure, isosurface.GetRealm().GetLog(), Log::Error,
				"Isosurface::ProceduralGenerator::Save: failed to open " + fileName);

		// todo: generate and save to file: from coarse level to most refined, block by block
		
		BoundingBox bound = this->GetBound();
		ur_float3 boundSize = bound.Max - bound.Min;

		ur_uint3 volumeResolution(
			(ur_uint)ceil(boundSize.x / cellSize),
			(ur_uint)ceil(boundSize.y / cellSize),
			(ur_uint)ceil(boundSize.z / cellSize));

		ur_float3 volumeCellSize(
			boundSize.x / volumeResolution.x,
			boundSize.y / volumeResolution.y,
			boundSize.z / volumeResolution.z);

		ur_uint3 blocksCount(
			(ur_uint)ceil(ur_float(volumeResolution.x) / blockResolution.x),
			(ur_uint)ceil(ur_float(volumeResolution.y) / blockResolution.y),
			(ur_uint)ceil(ur_float(volumeResolution.z) / blockResolution.z));

		ur_uint levels = (ur_uint)floor(log2(blocksCount.GetMaxValue()) + 1);


		return res;
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

	Result Isosurface::Presentation::Update(const ur_float3 &refinementPoint, const ur_float4x4 &viewProj)
	{
		return Result(NotImplemented);
	}

	Result Isosurface::Presentation::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj)
	{
		return Result(NotImplemented);
	}

	Result Isosurface::Presentation::Render(GrafCommandList &grafCmdList, const ur_float4x4 &viewProj)
	{
		return Result(NotImplemented);
	}

	void Isosurface::Presentation::ShowImgui()
	{

	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface::HybridCubes
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	const Isosurface::HybridCubes::Edge Isosurface::HybridCubes::Tetrahedron::Edges[EdgesCount] = {
		{ 0, 1 }, { 1, 2 }, { 2, 0 }, { 0, 3 }, { 1, 3 }, { 2, 3 }
	};

	const ur_byte Isosurface::HybridCubes::Tetrahedron::EdgeOppositeId[EdgesCount] = {
		5, 3, 4, 1, 2, 0
	};

	const Isosurface::HybridCubes::Face Isosurface::HybridCubes::Tetrahedron::Faces[FacesCount] = {
		{ { 0, 1, 2 }, { 0, 1, 2 } },
		{ { 0, 3, 1 }, { 0, 3, 4 } },
		{ { 1, 3, 2 }, { 1, 4, 5 } },
		{ { 2, 3, 0 }, { 2, 5, 3 } }
	};

	const Isosurface::HybridCubes::Tetrahedron::SplitInfo Isosurface::HybridCubes::Tetrahedron::EdgeSplitInfo[EdgesCount] = {
		{ { { 0, 0xff, 2, 3 }, { 0xff, 1, 2, 3 } } },
		{ { { 0, 1, 0xff, 3 }, { 0, 0xff, 2, 3 } } },
		{ { { 0, 1, 0xff, 3 }, { 0xff, 1, 2, 3 } } },
		{ { { 0, 1, 2, 0xff }, { 0xff, 1, 2, 3 } } },
		{ { { 0, 1, 2, 0xff }, { 0, 0xff, 2, 3 } } },
		{ { { 0, 1, 2, 0xff }, { 0, 1, 0xff, 3 } } }
	};

	#if defined(UR_GRAF)
	Isosurface::HybridCubes::GrafMesh::GrafMesh()
	{
	}

	Isosurface::HybridCubes::GrafMesh::~GrafMesh()
	{	
		// safely delete GRAF objects
		if (this->VB != ur_null)
		{
			GrafRenderer* grafRenderer = this->VB->GetRealm().GetComponent<GrafRenderer>();
			grafRenderer->SafeDelete(this->VB.release());
		}
		if (this->IB != ur_null)
		{
			GrafRenderer* grafRenderer = this->IB->GetRealm().GetComponent<GrafRenderer>();
			grafRenderer->SafeDelete(this->IB.release());
		}	
	}
	#endif

	Isosurface::HybridCubes::Tetrahedron::Tetrahedron()
	{
		this->initialized = false;
		this->visible = true;
		this->level = 0;
		this->longestEdgeIdx = 0;
	}

	Isosurface::HybridCubes::Tetrahedron::~Tetrahedron()
	{
	}

	void Isosurface::HybridCubes::Tetrahedron::Init(const Vertex &v0, const Vertex &v1, const Vertex &v2, const Vertex &v3)
	{
		this->vertices[0] = v0;
		this->vertices[1] = v1;
		this->vertices[2] = v2;
		this->vertices[3] = v3;

		// compute the longest edge
		
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

		// compute bbox

		this->bbox = BoundingBox();
		this->bbox.Min.SetMin(v0); this->bbox.Min.SetMin(v1); this->bbox.Min.SetMin(v2); this->bbox.Min.SetMin(v3);
		this->bbox.Max.SetMax(v0); this->bbox.Max.SetMax(v1); this->bbox.Max.SetMax(v2); this->bbox.Max.SetMax(v3);

		// init sub hexahedra

		Vertex ve[EdgesCount];
		for (ur_uint i = 0; i < EdgesCount; ++i)
		{
			ve[i] = (this->vertices[Edges[i].vid[0]] + this->vertices[Edges[i].vid[1]]) / 2.0f;
		}
		Vertex vf[FacesCount];
		for (ur_uint i = 0; i < FacesCount; ++i)
		{
			vf[i] = (this->vertices[Faces[i].vid[0]] + this->vertices[Faces[i].vid[1]] + this->vertices[Faces[i].vid[2]]) / 3.0f;
		}
		Vertex vc = (v0 + v1 + v2 + v3) / 4;

		{
			Hexahedron &h = this->hexahedra[0];
			h.vertices[0] = v0;
			h.vertices[1] = ve[2];
			h.vertices[2] = ve[0];
			h.vertices[3] = vf[0];
			h.vertices[4] = ve[3];
			h.vertices[5] = vf[3];
			h.vertices[6] = vf[1];
			h.vertices[7] = vc;
		}
		{
			Hexahedron &h = this->hexahedra[1];
			h.vertices[0] = ve[2];
			h.vertices[1] = v2;
			h.vertices[2] = vf[0];
			h.vertices[3] = ve[1];
			h.vertices[4] = vf[3];
			h.vertices[5] = ve[5];
			h.vertices[6] = vc;
			h.vertices[7] = vf[2];
		}
		{
			Hexahedron &h = this->hexahedra[2];
			h.vertices[0] = ve[0];
			h.vertices[1] = vf[0];
			h.vertices[2] = v1;
			h.vertices[3] = ve[1];
			h.vertices[4] = vf[1];
			h.vertices[5] = vc;
			h.vertices[6] = ve[4];
			h.vertices[7] = vf[2];
		}
		{
			Hexahedron &h = this->hexahedra[3];
			h.vertices[0] = vf[3];
			h.vertices[1] = ve[5];
			h.vertices[2] = vc;
			h.vertices[3] = vf[2];
			h.vertices[4] = ve[3];
			h.vertices[5] = v3;
			h.vertices[6] = vf[1];
			h.vertices[7] = ve[4];
		}
	}

	void Isosurface::HybridCubes::Tetrahedron::Split(std::array<std::unique_ptr<Tetrahedron>, ChildrenCount> &subTetrahedra)
	{
		// create sub tetrahedra
		const ur_byte *ev = this->Edges[this->longestEdgeIdx].vid;
		Vector3 ecp = this->vertices[ev[0]] * 0.5f + this->vertices[ev[1]] * 0.5f;
		const SplitInfo &splitInfo = EdgeSplitInfo[this->longestEdgeIdx];
		for (ur_uint subIdx = 0; subIdx < ChildrenCount; ++subIdx)
		{
			std::unique_ptr<Tetrahedron> subTetrahedron(new Tetrahedron());
			subTetrahedron->level = this->level + 1;
			const ur_byte *vid = splitInfo.subTetrahedra[subIdx];
			subTetrahedron->Init(
				(vid[0] != 0xff ? this->vertices[vid[0]] : ecp),
				(vid[1] != 0xff ? this->vertices[vid[1]] : ecp),
				(vid[2] != 0xff ? this->vertices[vid[2]] : ecp),
				(vid[3] != 0xff ? this->vertices[vid[3]] : ecp));
			subTetrahedra[subIdx] = std::move(subTetrahedron);
		}
	}

	Isosurface::HybridCubes::Node::Node()
	{
	}

	Isosurface::HybridCubes::Node::Node(std::unique_ptr<Tetrahedron> tetrahedron)
	{
		this->tetrahedron = std::move(tetrahedron);
	}

	Isosurface::HybridCubes::Node::~Node()
	{
	}

	void Isosurface::HybridCubes::Node::Split(Node *cachedNode)
	{
		if (this->HasChildren() ||
			this->tetrahedron.get() == ur_null)
			return;

		// get cached sub tetrahedra or compute new ones
		std::array<std::shared_ptr<Tetrahedron>, Tetrahedron::ChildrenCount> subTetrahedra;
		if (cachedNode && cachedNode->HasChildren())
		{
			for (ur_uint subIdx = 0; subIdx < Tetrahedron::ChildrenCount; ++subIdx)
			{
				subTetrahedra[subIdx] = cachedNode->children[subIdx]->tetrahedron;
			}
		}
		else
		{
			std::array<std::unique_ptr<Tetrahedron>, Tetrahedron::ChildrenCount> newTetrahedra;
			this->tetrahedron->Split(newTetrahedra);
			for (ur_uint subIdx = 0; subIdx < Tetrahedron::ChildrenCount; ++subIdx)
			{
				subTetrahedra[subIdx] = std::move(newTetrahedra[subIdx]);
			}
		}
		
		// create sub nodes
		for (ur_uint subIdx = 0; subIdx < Tetrahedron::ChildrenCount; ++subIdx)
		{
			std::unique_ptr<Node> subNode(new Node());
			subNode->tetrahedron = std::move(subTetrahedra[subIdx]);
			this->children[subIdx] = std::move(subNode);
		}
	}

	void Isosurface::HybridCubes::Node::Merge()
	{
		if (!this->HasChildren())
			return;

		for (auto &child : this->children)
		{
			child = ur_null;
		}
	}

	Isosurface::HybridCubes::HybridCubes(Isosurface &isosurface, const Desc &desc) :
		Presentation(isosurface)
	{
		this->desc = desc;
		this->freezeUpdate = false;
		this->hideSurface = false;
		this->drawTetrahedra = false;
		this->hideEmptyTetrahedra = false;
		this->drawHexahedra = false;
		this->drawRefinementTree = false;
		memset(&this->stats, 0, sizeof(this->stats));
		this->jobBuildCounter = 0;
		this->jobBuildRequested = 0;
	}

	Isosurface::HybridCubes::~HybridCubes()
	{
		if (this->jobUpdate != ur_null)
		{
			this->jobUpdate->Interrupt();
			this->jobUpdate->Wait();
		}
		for (auto &job : this->jobBuild)
		{
			job->Interrupt();
			job->Wait();
		}
	}

	Result Isosurface::HybridCubes::Update(const ur_float3 &refinementPoint, const ur_float4x4 &viewProj)
	{
		if (ur_null == this->isosurface.GetData())
			return Result(NotInitialized);

		// init roots
		const BoundingBox &bbox = this->isosurface.GetData()->GetBound();
		if (ur_null == this->root[0].get())
		{
			std::unique_ptr<Tetrahedron> th(new Tetrahedron());
			th->Init(
				{ bbox.Min.x, bbox.Min.y, bbox.Min.z },
				{ bbox.Min.x, bbox.Max.y, bbox.Min.z },
				{ bbox.Max.x, bbox.Max.y, bbox.Min.z },
				{ bbox.Max.x, bbox.Max.y, bbox.Max.z }
			);
			this->root[0].reset(new Node(std::move(th)));
		}
		if (ur_null == this->root[1].get())
		{
			std::unique_ptr<Tetrahedron> th(new Tetrahedron());
			th->Init(
				{ bbox.Min.x, bbox.Min.y, bbox.Min.z },
				{ bbox.Max.x, bbox.Max.y, bbox.Min.z },
				{ bbox.Max.x, bbox.Min.y, bbox.Min.z },
				{ bbox.Max.x, bbox.Max.y, bbox.Max.z }
			);
			this->root[1].reset(new Node(std::move(th)));
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
			this->root[2].reset(new Node(std::move(th)));
		}
		if (ur_null == this->root[3].get())
		{
			std::unique_ptr<Tetrahedron> th(new Tetrahedron());
			th->Init(
				{ bbox.Min.x, bbox.Min.y, bbox.Min.z },
				{ bbox.Min.x, bbox.Max.y, bbox.Max.z },
				{ bbox.Min.x, bbox.Max.y, bbox.Min.z },
				{ bbox.Max.x, bbox.Max.y, bbox.Max.z }
			);
			this->root[3].reset(new Node(std::move(th)));
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
			this->root[4].reset(new Node(std::move(th)));
		}
		if (ur_null == this->root[5].get())
		{
			std::unique_ptr<Tetrahedron> th(new Tetrahedron());
			th->Init(
				{ bbox.Min.x, bbox.Min.y, bbox.Min.z },
				{ bbox.Max.x, bbox.Min.y, bbox.Max.z },
				{ bbox.Min.x, bbox.Min.y, bbox.Max.z },
				{ bbox.Max.x, bbox.Max.y, bbox.Max.z }
			);
			this->root[5].reset(new Node(std::move(th)));
		}
		if (ur_null == this->refinementTree.GetRoot())
		{
			this->refinementTree.Init(bbox);
			ur_float nodeSize = (bbox.Max - bbox.Min).Length();
			ur_float cellSize = (nodeSize / (this->desc.LatticeResolution.GetMaxValue() * 2));
			ur_uint levels = 0;
			while (cellSize > this->desc.CellSize)
			{
				cellSize *= 0.5;
				++levels;
			}
			ur_float levelDistance = this->desc.DetailLevelDistance;
			this->refinementDistance.resize(levels);
			for (ur_uint i = levels; i > 0; --i)
			{
				this->refinementDistance[i - 1] = levelDistance;
				levelDistance += levelDistance * 2.0f;
			}
		}

		if (this->freezeUpdate)
			return Success;

		Result res(Success);
		bool updateSync = false;
		if (updateSync)
		{
			memset(&this->stats, 0, sizeof(this->stats));

			// update refinement tree
			this->UpdateRefinementTree(refinementPoint, this->refinementTree.GetRoot());

			// update hierarchy
			for (auto &tetrahedron : this->root)
			{
				res &= this->Update(refinementPoint, tetrahedron.get(), ur_null, &this->stats);
			}

			// build meshes
			for (auto &entry : this->buildQueue)
			{
				res &= this->BuildMesh(entry.second, &this->stats);
			}
			this->buildQueue.clear();
		}
		else
		{
			auto &jobSystem = this->isosurface.GetRealm().GetJobSystem();

			// do update/build job(s)
			if ((this->jobUpdate == ur_null || this->jobUpdate->Finished()) &&
				(this->jobBuildCounter >= this->jobBuildRequested))
			{
				// make visible new meshes
				for (auto &jobCtx : this->jobBuildCtx)
				{
					jobCtx.second->visible = true;
				}

				// reset previous build job(s) data
				this->jobBuildCounter = 0;
				this->jobBuildRequested = 0;
				this->jobBuild.clear();
				this->jobBuildCtx.clear();

				// swap back and front data
				if (this->jobUpdate != ur_null &&
					this->jobUpdate->FinishedSuccessfully())
				{
					for (ur_uint ir = 0; ir < HybridCubes::RootsCount; ++ir)
					{
						this->root[ir].swap(this->rootBack[ir]);
					}
					memcpy(&this->stats, &this->statsBack, sizeof(this->stats));
				}
				
				// prepare update context
				this->updatePoint = refinementPoint;

				// start a new update
				this->jobUpdate = jobSystem.Add(JobPriority::Low, Job::DataPtr(this), [](Job::Context& ctx) -> void {

					Result result = Success;

					// reinterpret data ptr
					HybridCubes *presentation = reinterpret_cast<HybridCubes*>(ctx.data);
					auto &jobSystem = presentation->isosurface.GetRealm().GetJobSystem();
					
					// reset presentation
					presentation->buildQueue.clear();
					memset(&presentation->statsBack, 0, sizeof(presentation->statsBack));

					// update refinement tree
					presentation->UpdateRefinementTree(presentation->updatePoint, presentation->refinementTree.GetRoot());

					// update hierarchy
					for (ur_uint i = 0; i < HybridCubes::RootsCount; ++i)
					{
						result &= presentation->Update(
							presentation->updatePoint, presentation->rootBack[i].get(), presentation->root[i].get(),
							&presentation->statsBack);
					}

					// build meshes
					if (!presentation->buildQueue.empty())
					{
						ur_uint level = presentation->buildQueue.begin()->second->level;
						for (auto &entry : presentation->buildQueue)
						{
							if (entry.first != level)
								break; // one level per update iteration (to avoid seams)

							presentation->jobBuildRequested += 1;
							presentation->jobBuildCtx.push_back(std::pair<HybridCubes*, Tetrahedron*>(presentation, entry.second));
							auto &buildCtx = presentation->jobBuildCtx.back();

							// mesh building job
							presentation->jobBuild.push_back(jobSystem.Add(Job::DataPtr(&buildCtx), [](Job::Context& ctx) -> void {

								Result result = Success;

								std::pair<HybridCubes*, Tetrahedron*> *jobData = reinterpret_cast<decltype(jobData)>(ctx.data);
								HybridCubes *presentation = jobData->first;
								Tetrahedron *tetrahedron = jobData->second;
								tetrahedron->visible = false;

								result &= presentation->BuildMesh(tetrahedron, &presentation->statsBack);

								ctx.resultCode = result.Code;

								presentation->jobBuildCounter += 1;
							}));
						}
						presentation->buildQueue.clear();
						presentation->statsBack.buildQueue = presentation->jobBuildRequested;
					}

					ctx.resultCode = result.Code;
				});
			}
		}

		return res;
	}

	void Isosurface::HybridCubes::UpdateRefinementTree(const ur_float3 &refinementPoint, EmptyOctree::Node *node)
	{
		if (ur_null == node)
			return;

#if 1
		ur_float nodeSize = (node->GetBBox().Max - node->GetBBox().Min).Length();
		bool doSplit = (nodeSize / (this->desc.LatticeResolution.GetMaxValue() * 2) > this->desc.CellSize);
		if (doSplit)
		{
			ur_float refinementDistance = (node->GetBBox().Max - node->GetBBox().Min).Length() * 0.5f;
			doSplit = (node->GetBBox().Distance(refinementPoint) < refinementDistance);
			doSplit |= node->GetLevel() < 2; // TEMP: always split to minimal refinement level
		}
#else
		bool doSplit = (node->GetLevel() < this->refinementDistance.size());
		if (doSplit)
		{
			doSplit = (node->GetBBox().Distance(refinementPoint) < this->refinementDistance[node->GetLevel()]);
		}
#endif

		if (doSplit)
		{
			node->Split();
			if (node->HasSubNodes())
			{
				for (ur_uint i = 0; i < EmptyOctree::Node::SubNodesCount; ++i)
				{
					this->UpdateRefinementTree(refinementPoint, node->GetSubNode(i));
				}
			}
		}
		else
		{
			node->Merge();
		}
	}

	bool Isosurface::HybridCubes::CheckRefinementTree(const BoundingBox &bbox, EmptyOctree::Node *node)
	{
		if (ur_null == node || !bbox.Intersects(node->GetBBox()))
			return false;

		ur_float nodeSize = node->GetBBox().SizeMin();
		ur_float bboxSize = bbox.SizeMax();
		if (bboxSize > nodeSize)
			return true;

		//RectF r0, r1, ri;
		//{ // test XY
		//	r0.Min.x = bbox.Min.x; r0.Min.y = bbox.Min.y;
		//	r0.Max.x = bbox.Max.x; r0.Max.y = bbox.Max.y;
		//	r1.Min.x = node->GetBBox().Min.x; r1.Min.y = node->GetBBox().Min.y;
		//	r1.Max.x = node->GetBBox().Max.x; r1.Max.y = node->GetBBox().Max.y;
		//	if (r0.Intersection(r1, ri) && ri.Area() > std::numeric_limits<ur_float>::epsilon() &&
		//		(ri.Width() < r0.Width() || ri.Height() < r0.Height()))
		//		return true;
		//}
		//{ // test XZ
		//	r0.Min.x = bbox.Min.x; r0.Min.y = bbox.Min.z;
		//	r0.Max.x = bbox.Max.x; r0.Max.y = bbox.Max.z;
		//	r1.Min.x = node->GetBBox().Min.x; r1.Min.y = node->GetBBox().Min.z;
		//	r1.Max.x = node->GetBBox().Max.x; r1.Max.y = node->GetBBox().Max.z;
		//	if (r0.Intersection(r1, ri) && ri.Area() > std::numeric_limits<ur_float>::epsilon() &&
		//		(ri.Width() < r0.Width() || ri.Height() < r0.Height()))
		//		return true;
		//}
		//{ // test YZ
		//	r0.Min.x = bbox.Min.z; r0.Min.y = bbox.Min.y;
		//	r0.Max.x = bbox.Max.z; r0.Max.y = bbox.Max.y;
		//	r1.Min.x = node->GetBBox().Min.z; r1.Min.y = node->GetBBox().Min.y;
		//	r1.Max.x = node->GetBBox().Max.z; r1.Max.y = node->GetBBox().Max.y;
		//	if (r0.Intersection(r1, ri) && ri.Area() > std::numeric_limits<ur_float>::epsilon() &&
		//		(ri.Width() < r0.Width() || ri.Height() < r0.Height()))
		//		return true;
		//}
		
		for (ur_uint i = 0; i < EmptyOctree::Node::SubNodesCount; ++i)
		{
			if (this->CheckRefinementTree(bbox, node->GetSubNode(i)))
				return true;
		}
			
		return false;
	}

	Result Isosurface::HybridCubes::Update(const ur_float3 &refinementPoint, Node *node, Node *cachedNode, Stats *stats)
	{
		Result res(Success);
		if (ur_null == node ||
			ur_null == node->tetrahedron)
			return res;

		res &= this->UpdateLoD(refinementPoint, node, cachedNode);

		if (!node->tetrahedron->initialized)
		{
			this->buildQueue.insert(std::pair<ur_uint, Tetrahedron*>(node->tetrahedron->level, node->tetrahedron.get()));
		}

		if (node->HasChildren())
		{
			for (ur_uint subIdx = 0; subIdx < Tetrahedron::ChildrenCount; ++subIdx)
			{
				res &= this->Update(refinementPoint, node->children[subIdx].get(),
					(cachedNode ? cachedNode->children[subIdx].get() : ur_null), stats);
			}
		}

		// update stats
		if (stats != ur_null)
		{
			stats->tetrahedraCount += 1;
			stats->treeMemory += sizeof(Tetrahedron);
			stats->treeMemory += sizeof(node->tetrahedron->hexahedra);
			if (node->tetrahedron->initialized)
			{
				for (const auto &h : node->tetrahedron->hexahedra)
				{
					#if defined(UR_GRAF)
					if (h.grafMesh.VB) stats->meshVideoMemory += (ur_uint)h.grafMesh.VB->GetDesc().SizeInBytes;
					if (h.grafMesh.IB) stats->meshVideoMemory += (ur_uint)h.grafMesh.IB->GetDesc().SizeInBytes;
					#else
					if (h.gfxMesh.VB) stats->meshVideoMemory += h.gfxMesh.VB->GetDesc().Size;
					if (h.gfxMesh.IB) stats->meshVideoMemory += h.gfxMesh.IB->GetDesc().Size;
					#endif
				}
			}
		}

		return res;
	}

	Result Isosurface::HybridCubes::UpdateLoD(const ur_float3 &refinementPoint, Node *node, Node *cachedNode)
	{
		Result res(Success);
		if (ur_null == node ||
			ur_null == node->tetrahedron)
			return res;

		// update LoD by traversing refinement octree
		// current approach produces seamless partition, but due to it's conservative nature, the resulting mesh is overdetailed
		// todo: try doing proper LEB implementation, based on "dimonds" hierarchy or "terminal edge" bisection
#if 1
		bool doSplit = this->CheckRefinementTree(node->tetrahedron->bbox, this->refinementTree.GetRoot());
#else
		bool doSplit = false;
		const ur_float3 &ev0 = node->tetrahedron->vertices[Tetrahedron::Edges[node->tetrahedron->longestEdgeIdx].vid[0]];
		const ur_float3 &ev1 = node->tetrahedron->vertices[Tetrahedron::Edges[node->tetrahedron->longestEdgeIdx].vid[1]];
		ur_float edgeLen = (ev0 - ev1).Length();
		doSplit = (edgeLen / (this->desc.LatticeResolution.GetMaxValue() * 2) > this->desc.CellSize);
		if (doSplit)
		{
			ur_float3 evc = (ev0 + ev1) * 0.5f;
			doSplit = (evc - refinementPoint).Length() < edgeLen;
		}
#endif

		if (doSplit)
		{
			node->Split(cachedNode);
		}
		else
		{
			node->Merge();
		}

		return res;
	}

	Result Isosurface::HybridCubes::BuildMesh(Tetrahedron *tetrahedron, Stats *stats)
	{
		Result res = Result(Success);
		if (ur_null == tetrahedron)
			return res;

		for (auto &hexahedron : tetrahedron->hexahedra)
		{
			res &= this->MarchCubes(hexahedron);
		}

		tetrahedron->initialized = Succeeded(res);

		// update stats
		if (stats != ur_null && tetrahedron->initialized)
		{
			for (const auto &h : tetrahedron->hexahedra)
			{
				#if defined(UR_GRAF)
				if (h.grafMesh.VB) stats->meshVideoMemory += (ur_uint)h.grafMesh.VB->GetDesc().SizeInBytes;
				if (h.grafMesh.IB) stats->meshVideoMemory += (ur_uint)h.grafMesh.IB->GetDesc().SizeInBytes;
				#else
				if (h.gfxMesh.VB) stats->meshVideoMemory += h.gfxMesh.VB->GetDesc().Size;
				if (h.gfxMesh.IB) stats->meshVideoMemory += h.gfxMesh.IB->GetDesc().Size;
				#endif
			}
		}

		return res;
	}

	// Marching cubes lookup tables

	static const ur_uint MCEdgeVertices[12][2] = {
		{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
		{ 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
		{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
	};

	static const ur_uint MCEdgeTable[256] =
	{
		0x000, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c, 0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
		0x190, 0x099, 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c, 0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
		0x230, 0x339, 0x033, 0x13a, 0x636, 0x73f, 0x435, 0x53c, 0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
		0x3a0, 0x2a9, 0x1a3, 0x0aa, 0x7a6, 0x6af, 0x5a5, 0x4ac, 0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
		0x460, 0x569, 0x663, 0x76a, 0x066, 0x16f, 0x265, 0x36c, 0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
		0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0x0ff, 0x3f5, 0x2fc, 0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
		0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x055, 0x15c, 0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
		0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0x0cc, 0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
		0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc, 0x0cc, 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
		0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c, 0x15c, 0x055, 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
		0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc, 0x2fc, 0x3f5, 0x0ff, 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
		0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c, 0x36c, 0x265, 0x16f, 0x066, 0x76a, 0x663, 0x569, 0x460,
		0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac, 0x4ac, 0x5a5, 0x6af, 0x7a6, 0x0aa, 0x1a3, 0x2a9, 0x3a0,
		0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c, 0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x033, 0x339, 0x230,
		0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c, 0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x099, 0x190,
		0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c, 0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x000
	};

	static const ur_int MCTriangleTable[256][16] =
	{
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1 },
		{ 8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1 },
		{ 3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1 },
		{ 4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1 },
		{ 4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1 },
		{ 9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1 },
		{ 10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1 },
		{ 5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1 },
		{ 5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1 },
		{ 8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1 },
		{ 2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1 },
		{ 2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1 },
		{ 11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1 },
		{ 5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1 },
		{ 11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1 },
		{ 11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1 },
		{ 2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1 },
		{ 6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1 },
		{ 3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1 },
		{ 6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1 },
		{ 6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1 },
		{ 8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1 },
		{ 7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1 },
		{ 3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1 },
		{ 0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1 },
		{ 9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1 },
		{ 8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1 },
		{ 5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1 },
		{ 0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1 },
		{ 6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1 },
		{ 10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1 },
		{ 1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1 },
		{ 0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1 },
		{ 3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1 },
		{ 6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1 },
		{ 9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1 },
		{ 8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1 },
		{ 3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1 },
		{ 6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1 },
		{ 10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1 },
		{ 10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1 },
		{ 2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1 },
		{ 7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1 },
		{ 7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1 },
		{ 2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1 },
		{ 1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1 },
		{ 11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1 },
		{ 8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1 },
		{ 0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1 },
		{ 7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1 },
		{ 7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1 },
		{ 10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1 },
		{ 0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1 },
		{ 7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1 },
		{ 6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1 },
		{ 6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1 },
		{ 4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1 },
		{ 10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1 },
		{ 8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1 },
		{ 1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1 },
		{ 10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1 },
		{ 10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1 },
		{ 9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1 },
		{ 7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1 },
		{ 3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1 },
		{ 7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1 },
		{ 3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1 },
		{ 6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1 },
		{ 9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1 },
		{ 1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1 },
		{ 4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1 },
		{ 7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1 },
		{ 6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1 },
		{ 0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1 },
		{ 6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1 },
		{ 0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1 },
		{ 11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1 },
		{ 6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1 },
		{ 5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1 },
		{ 9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1 },
		{ 1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1 },
		{ 10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1 },
		{ 0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1 },
		{ 11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1 },
		{ 9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1 },
		{ 7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1 },
		{ 2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1 },
		{ 9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1 },
		{ 9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1 },
		{ 1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1 },
		{ 0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1 },
		{ 10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1 },
		{ 2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1 },
		{ 0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1 },
		{ 0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1 },
		{ 9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1 },
		{ 5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1 },
		{ 5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1 },
		{ 8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1 },
		{ 9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1 },
		{ 1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1 },
		{ 3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1 },
		{ 4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1 },
		{ 9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1 },
		{ 11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1 },
		{ 2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1 },
		{ 9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1 },
		{ 3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1 },
		{ 1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1 },
		{ 4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1 },
		{ 0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1 },
		{ 1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
	};

	Result Isosurface::HybridCubes::MarchCubes(Hexahedron &hexahedron)
	{
		if (ur_null == this->isosurface.GetData())
			return Result(NotInitialized);

		// early isosurface intersection test

		BoundingBox bbox;
		for (auto &v : hexahedron.vertices) { bbox.Expand(v); }

		if (this->isosurface.GetData()->Read(ur_null, ur_null, 0, bbox) == NotFound)
			return Result(Success); // does not intersect isosurface, nothing to extract here

		// compute hexahedron lattice points
		
		auto computeLinePoints = [](ur_float3 *points, const ur_uint count, const ur_uint step, const ur_float3 &p0, const ur_float3 &p1) {
			ur_float s = ur_float(count - 1);
			for (ur_uint i = 0; i < count; ++i)
			{
				ur_float3::Lerp(*points, p0, p1, ur_float(i) / s);
				points += step;
			}
		};

		ur_uint rowOfs = this->desc.LatticeResolution.x;
		ur_uint lastRowOfs = rowOfs * (this->desc.LatticeResolution.y - 1);
		ur_uint sliceOfs = rowOfs * this->desc.LatticeResolution.y;
		ur_uint lastSliceOfs = sliceOfs * (this->desc.LatticeResolution.z - 1);
		ur_uint latticeSize = sliceOfs * this->desc.LatticeResolution.z;
		std::vector<ur_float3> lattice(latticeSize);
		
		computeLinePoints(lattice.data(), this->desc.LatticeResolution.x, 1, hexahedron.vertices[0], hexahedron.vertices[1]);
		computeLinePoints(lattice.data() + lastRowOfs, this->desc.LatticeResolution.x, 1, hexahedron.vertices[2], hexahedron.vertices[3]);
		computeLinePoints(lattice.data() + lastSliceOfs, this->desc.LatticeResolution.x, 1, hexahedron.vertices[4], hexahedron.vertices[5]);
		computeLinePoints(lattice.data() + lastRowOfs + lastSliceOfs, this->desc.LatticeResolution.x, 1, hexahedron.vertices[6], hexahedron.vertices[7]);
		ur_float3 *p_col0 = lattice.data();
		ur_float3 *p_col1 = lattice.data() + lastSliceOfs;
		for (ur_uint ix = 0; ix < this->desc.LatticeResolution.x; ++ix, ++p_col0, ++p_col1)
		{
			computeLinePoints(p_col0, this->desc.LatticeResolution.y, rowOfs, *p_col0, *(p_col0 + lastRowOfs));
			computeLinePoints(p_col1, this->desc.LatticeResolution.y, rowOfs, *p_col1, *(p_col1 + lastRowOfs));
		}

		ur_float3 *p_row0 = lattice.data();
		ur_float3 *p_row1 = lattice.data() + lastSliceOfs;
		for (ur_uint iy = 0; iy < this->desc.LatticeResolution.y; ++iy)
		{
			p_col0 = p_row0;
			p_col1 = p_row1;
			for (ur_uint ix = 0; ix < this->desc.LatticeResolution.x; ++ix, ++p_col0, ++p_col1)
			{
				computeLinePoints(p_col0, this->desc.LatticeResolution.z, sliceOfs, *p_col0, *p_col1);
			}
			p_row0 += rowOfs;
			p_row1 += rowOfs;
		}

		// sample and cache values at lattice points

		static const DataVolume::ValueType ScalarFieldSurfaceValue = DataVolume::ValueType(0);
		std::vector<DataVolume::ValueType> samples(latticeSize);
		this->isosurface.GetData()->Read(samples.data(), lattice.data(), latticeSize, bbox);

		// march

		static const int NoVertexId = -1;
		std::vector<Isosurface::Vertex> vertexBuffer;
		std::vector<Isosurface::Index> indexBuffer;
		std::vector<ur_int3> edgeVertices(latticeSize, NoVertexId);
		ur_int *cellEdges[12];
		ur_float3 *cellPoints[8];
		DataVolume::ValueType *cellValues[8];
		ur_uint3 cellsCount = this->desc.LatticeResolution - 1;
		ur_float3 *p_slice = lattice.data();
		DataVolume::ValueType *p_sample_slice = samples.data();
		for (ur_uint iz = 0; iz < cellsCount.z; ++iz)
		{
			ur_float3 *p_row = p_slice;
			DataVolume::ValueType *p_sample_row = p_sample_slice;
			for (ur_uint iy = 0; iy < cellsCount.y; ++iy)
			{
				ur_float3 *p_cell = p_row;
				DataVolume::ValueType *p_sample = p_sample_row;
				for (ur_uint ix = 0; ix < cellsCount.x; ++ix, ++p_cell, ++p_sample)
				{
					// cell corner vertices
					cellPoints[0] = &p_cell[0];
					cellPoints[1] = &p_cell[1];
					cellPoints[2] = &p_cell[rowOfs + 1];
					cellPoints[3] = &p_cell[rowOfs];
					cellPoints[4] = &p_cell[sliceOfs];
					cellPoints[5] = &p_cell[sliceOfs + 1];
					cellPoints[6] = &p_cell[sliceOfs + rowOfs + 1];
					cellPoints[7] = &p_cell[sliceOfs + rowOfs];

					// cell corner samples
					cellValues[0] = &p_sample[0];
					cellValues[1] = &p_sample[1];
					cellValues[2] = &p_sample[rowOfs + 1];
					cellValues[3] = &p_sample[rowOfs];
					cellValues[4] = &p_sample[sliceOfs];
					cellValues[5] = &p_sample[sliceOfs + 1];
					cellValues[6] = &p_sample[sliceOfs + rowOfs + 1];
					cellValues[7] = &p_sample[sliceOfs + rowOfs];
					
					// lookup intersected edges
					ur_uint flagIdx = 0;
					for (ur_uint iv = 0; iv < 8; ++iv)
					{
						if (*cellValues[iv] <= ScalarFieldSurfaceValue) flagIdx |= (1 << iv);
					}
					const ur_uint &edgeFlags = MCEdgeTable[flagIdx];
					if (edgeFlags == 0)
						continue; // surface does not intersect any edge

					// init cell vertices refs
					ur_int3 *p_cellEdges = edgeVertices.data() + ix + iy * rowOfs + iz * sliceOfs;
					cellEdges[0] = &p_cellEdges[0].x;
					cellEdges[1] = &p_cellEdges[1].y;
					cellEdges[2] = &p_cellEdges[rowOfs].x;
					cellEdges[3] = &p_cellEdges[0].y;
					cellEdges[4] = &p_cellEdges[sliceOfs].x;
					cellEdges[5] = &p_cellEdges[sliceOfs + 1].y;
					cellEdges[6] = &p_cellEdges[sliceOfs + rowOfs].x;
					cellEdges[7] = &p_cellEdges[sliceOfs].y;
					cellEdges[8] = &p_cellEdges[0].z;
					cellEdges[9] = &p_cellEdges[1].z;
					cellEdges[10] = &p_cellEdges[rowOfs + 1].z;
					cellEdges[11] = &p_cellEdges[rowOfs].z;

					// compute intersection points
					for (ur_uint ie = 0; ie < 12; ++ie)
					{
						if ((edgeFlags & (1 << ie)) &&
							(NoVertexId == *cellEdges[ie]))
						{
							// compute new vertex
							DataVolume::ValueType &cv0 = *cellValues[MCEdgeVertices[ie][0]];
							DataVolume::ValueType &cv1 = *cellValues[MCEdgeVertices[ie][1]];
							ur_float lfactor = (ur_float)(ScalarFieldSurfaceValue - cv0) / (cv1 - cv0);
							ur_float3 &p0 = *cellPoints[MCEdgeVertices[ie][0]];
							ur_float3 &p1 = *cellPoints[MCEdgeVertices[ie][1]];
							ur_float3 p = ur_float3::Lerp(p0, p1, lfactor);
							*cellEdges[ie] = (ur_int)vertexBuffer.size();
							vertexBuffer.push_back({ p, 0.0f, 0xffffffff });
							// todo: compute corresponding normal here
							// as a gradient of adjacent samples
						}
					}

					// add triangles
					for (ur_uint itri = 0; itri < 5; ++itri)
					{
						if (MCTriangleTable[flagIdx][itri * 3] < 0)
							break;

						const ur_int &vi0 = MCTriangleTable[flagIdx][itri * 3 + 0];
						const ur_int &vi1 = MCTriangleTable[flagIdx][itri * 3 + 1];
						const ur_int &vi2 = MCTriangleTable[flagIdx][itri * 3 + 2];
						indexBuffer.push_back(*cellEdges[vi0]);
						indexBuffer.push_back(*cellEdges[vi1]);
						indexBuffer.push_back(*cellEdges[vi2]);
					}
				}
				p_row += rowOfs;
				p_sample_row += rowOfs;
			}
			p_slice += sliceOfs;
			p_sample_slice += sliceOfs;
		}

		// prepare graphics resources

		if (indexBuffer.size() < 3)
			return Result(Success); // no data

		#if defined(UR_GRAF)

		GrafRenderer *grafRenderer = this->isosurface.grafRenderer;
		GrafSystem *grafSystem = grafRenderer->GetGrafSystem();
		GrafDevice *grafDevice = grafRenderer->GetGrafDevice();

		// create vertex Buffer
		auto &grafVB = hexahedron.grafMesh.VB;
		Result res = grafSystem->CreateBuffer(grafVB);
		if (Succeeded(res))
		{
			GrafBufferDesc bufferDesc;
			bufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::VertexBuffer | (ur_uint)GrafBufferUsageFlag::TransferDst;
			bufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::GpuLocal;
			bufferDesc.SizeInBytes = (ur_size)vertexBuffer.size() * sizeof(Isosurface::Vertex);
			res = grafVB->Initialize(grafDevice, { bufferDesc });
			if (Succeeded(res))
			{
				//grafRenderer->Upload((ur_byte*)vertexBuffer.data(), grafVB.get(), bufferDesc.SizeInBytes);
				std::unique_ptr<GrafBuffer> grafUploadBuffer;
				res = grafSystem->CreateBuffer(grafUploadBuffer);
				if (Succeeded(res))
				{
					bufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::TransferSrc;
					bufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
					res = grafUploadBuffer->Initialize(grafDevice, { bufferDesc });
					if (Succeeded(res))
					{
						res = grafUploadBuffer->Write((ur_byte*)vertexBuffer.data(), bufferDesc.SizeInBytes);
						if (Succeeded(res))
						{
							std::unique_ptr<GrafCommandList> grafUploadCmdList;
							res = grafSystem->CreateCommandList(grafUploadCmdList);
							if (Succeeded(res))
							{
								res = grafUploadCmdList->Initialize(grafDevice);
								if (Succeeded(res))
								{
									grafUploadCmdList->Begin();
									grafUploadCmdList->Copy(grafUploadBuffer.get(), grafVB.get());
									grafUploadCmdList->End();
									grafDevice->Record(grafUploadCmdList.get());
									GrafCommandList *grafUploadCmdListPtr = grafUploadCmdList.release();
									GrafBuffer *grafUploadBufferPtr = grafUploadBuffer.release();
									grafRenderer->AddCommandListCallback(grafUploadCmdListPtr, {}, [grafUploadCmdListPtr, grafUploadBufferPtr](GrafCallbackContext& ctx) -> Result
									{
										delete grafUploadCmdListPtr;
										delete grafUploadBufferPtr;
										return Result(Success);
									});
								}
							}
						}
					}
				}
			}
		}
		if (Failed(res))
			return Result(Failure);

		// create index buffer
		auto &grafIB = hexahedron.grafMesh.IB;
		res = grafSystem->CreateBuffer(grafIB);
		if (Succeeded(res))
		{
			GrafBufferDesc bufferDesc;
			bufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::IndexBuffer | (ur_uint)GrafBufferUsageFlag::TransferDst;
			bufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::GpuLocal;
			bufferDesc.SizeInBytes = (ur_size)indexBuffer.size() * sizeof(Isosurface::Index);
			res = grafIB->Initialize(grafDevice, { bufferDesc });
			if (Succeeded(res))
			{
				//grafRenderer->Upload((ur_byte*)indexBuffer.data(), grafIB.get(), bufferDesc.SizeInBytes);
				std::unique_ptr<GrafBuffer> grafUploadBuffer;
				res = grafSystem->CreateBuffer(grafUploadBuffer);
				if (Succeeded(res))
				{
					bufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::TransferSrc;
					bufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
					res = grafUploadBuffer->Initialize(grafDevice, { bufferDesc });
					if (Succeeded(res))
					{
						res = grafUploadBuffer->Write((ur_byte*)indexBuffer.data(), bufferDesc.SizeInBytes);
						if (Succeeded(res))
						{
							std::unique_ptr<GrafCommandList> grafUploadCmdList;
							res = grafSystem->CreateCommandList(grafUploadCmdList);
							if (Succeeded(res))
							{
								res = grafUploadCmdList->Initialize(grafDevice);
								if (Succeeded(res))
								{
									grafUploadCmdList->Begin();
									grafUploadCmdList->Copy(grafUploadBuffer.get(), grafIB.get());
									grafUploadCmdList->End();
									grafDevice->Record(grafUploadCmdList.get());
									GrafCommandList *grafUploadCmdListPtr = grafUploadCmdList.release();
									GrafBuffer *grafUploadBufferPtr = grafUploadBuffer.release();
									grafRenderer->AddCommandListCallback(grafUploadCmdListPtr, {}, [grafUploadCmdListPtr, grafUploadBufferPtr](GrafCallbackContext& ctx) -> Result
									{
										delete grafUploadCmdListPtr;
										delete grafUploadBufferPtr;
										return Result(Success);
									});
								}
							}
						}
					}
				}
			}
		}
		if (Failed(res))
			return Result(Failure);

		#else

		GfxSystem *gfxSystem = this->isosurface.GetRealm().GetGfxSystem();
		if (ur_null == gfxSystem)
			return Result(Failure);

		// create vertex Buffer
		auto &gfxVB = hexahedron.gfxMesh.VB;
		Result res = gfxSystem->CreateBuffer(gfxVB);
		if (Succeeded(res))
		{
			GfxResourceData gfxRes = { vertexBuffer.data(), (ur_uint)vertexBuffer.size() * sizeof(Isosurface::Vertex), 0 };
			res = gfxVB->Initialize(gfxRes.RowPitch, sizeof(Isosurface::Vertex), GfxUsage::Immutable, (ur_uint)GfxBindFlag::VertexBuffer, 0, &gfxRes);
		}
		if (Failed(res))
			return Result(Failure);

		// create index buffer
		auto &gfxIB = hexahedron.gfxMesh.IB;
		res = gfxSystem->CreateBuffer(gfxIB);
		if (Succeeded(res))
		{
			GfxResourceData gfxRes = { indexBuffer.data(), (ur_uint)indexBuffer.size() * sizeof(Isosurface::Index), 0 };
			res = gfxIB->Initialize(gfxRes.RowPitch, sizeof(Isosurface::Index), GfxUsage::Immutable, (ur_uint)GfxBindFlag::IndexBuffer, 0, &gfxRes);
		}
		if (Failed(res))
			return Result(Failure);
		#endif

		return Result(Success);
	}

	Result Isosurface::HybridCubes::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj)
	{
	#if !defined(UR_GRAF)
		GenericRender *genericRender = this->isosurface.GetRealm().GetComponent<GenericRender>();
		ur_float4 frustumPlanes[6];
		viewProj.FrustumPlanes(frustumPlanes, true);
		this->stats.primitivesRendered = 0;

		for (auto &node : this->root)
		{
			this->Render(gfxContext, genericRender, frustumPlanes, node.get());
		}

		if (this->drawRefinementTree)
		{
			this->RenderOctree(genericRender, this->refinementTree.GetRoot());
		}

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	Result Isosurface::HybridCubes::Render(GrafCommandList &grafCmdList, const ur_float4x4 &viewProj)
	{
	#if defined(UR_GRAF)
		GenericRender *genericRender = this->isosurface.GetRealm().GetComponent<GenericRender>();
		ur_float4 frustumPlanes[6];
		viewProj.FrustumPlanes(frustumPlanes, true);
		this->stats.primitivesRendered = 0;

		for (auto &node : this->root)
		{
			this->Render(grafCmdList, genericRender, frustumPlanes, node.get());
		}

		if (this->drawRefinementTree)
		{
			this->RenderOctree(genericRender, this->refinementTree.GetRoot());
		}

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	Result Isosurface::HybridCubes::Render(GfxContext &gfxContext, GenericRender *genericRender, const ur_float4(&frustumPlanes)[6], Node *node)
	{
	#if !defined(UR_GRAF)
		if (ur_null == node ||
			ur_null == node->tetrahedron.get() ||
			!node->tetrahedron->bbox.Intersects(frustumPlanes))
			return Result(Success);

		if (node->HasChildren() &&
			node->children[0]->tetrahedron->initialized && node->children[0]->tetrahedron->visible &&
			node->children[1]->tetrahedron->initialized && node->children[1]->tetrahedron->visible)
		{
			for (auto &child : node->children)
			{
				this->Render(gfxContext, genericRender, frustumPlanes, child.get());
			}
		}
		else
		{
			for (auto &hexahedron : node->tetrahedron->hexahedra)
			{
				const auto &gfxVB = hexahedron.gfxMesh.VB;
				const auto &gfxIB = hexahedron.gfxMesh.IB;
				if (gfxVB != ur_null && gfxIB != ur_null && !this->hideSurface)
				{
					const ur_uint indexCount = (gfxIB.get() ? gfxIB->GetDesc().Size / sizeof(Isosurface::Index) : 0);
					this->stats.primitivesRendered += indexCount / 3;
					gfxContext.SetVertexBuffer(gfxVB.get(), 0);
					gfxContext.SetIndexBuffer(gfxIB.get());
					gfxContext.DrawIndexed(indexCount, 0, 0, 0, 0);
				}
			}

			if (this->drawTetrahedra)
			{
				if (!this->hideEmptyTetrahedra || node->tetrahedron->initialized && (
					node->tetrahedron->hexahedra[0].gfxMesh.VB != ur_null ||
					node->tetrahedron->hexahedra[1].gfxMesh.VB != ur_null ||
					node->tetrahedron->hexahedra[2].gfxMesh.VB != ur_null ||
					node->tetrahedron->hexahedra[3].gfxMesh.VB != ur_null))
				{
					RenderDebug(genericRender, node);
				}
			}
		}

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	Result Isosurface::HybridCubes::Render(GrafCommandList &grafCmdList, GenericRender *genericRender, const ur_float4(&frustumPlanes)[6], Node *node)
	{
	#if defined(UR_GRAF)
		if (ur_null == node ||
			ur_null == node->tetrahedron.get() ||
			!node->tetrahedron->bbox.Intersects(frustumPlanes))
			return Result(Success);

		if (node->HasChildren() &&
			node->children[0]->tetrahedron->initialized && node->children[0]->tetrahedron->visible &&
			node->children[1]->tetrahedron->initialized && node->children[1]->tetrahedron->visible)
		{
			for (auto &child : node->children)
			{
				this->Render(grafCmdList, genericRender, frustumPlanes, child.get());
			}
		}
		else
		{
			for (auto &hexahedron : node->tetrahedron->hexahedra)
			{
				const auto &grafVB = hexahedron.grafMesh.VB;
				const auto &grafIB = hexahedron.grafMesh.IB;
				if (grafVB != ur_null && grafIB != ur_null && !this->hideSurface)
				{
					const ur_uint indexCount = ur_uint(grafIB.get() ? grafIB->GetDesc().SizeInBytes / sizeof(Isosurface::Index) : 0);
					this->stats.primitivesRendered += indexCount / 3;
					grafCmdList.BindVertexBuffer(grafVB.get(), 0);
					grafCmdList.BindIndexBuffer(grafIB.get(), (sizeof(Isosurface::Index) == 16 ? GrafIndexType::UINT16 : GrafIndexType::UINT32));
					grafCmdList.DrawIndexed(indexCount, 1, 0, 0, 0);
				}
			}

			if (this->drawTetrahedra)
			{
				if (!this->hideEmptyTetrahedra || node->tetrahedron->initialized && (
					node->tetrahedron->hexahedra[0].grafMesh.VB != ur_null ||
					node->tetrahedron->hexahedra[1].grafMesh.VB != ur_null ||
					node->tetrahedron->hexahedra[2].grafMesh.VB != ur_null ||
					node->tetrahedron->hexahedra[3].grafMesh.VB != ur_null))
				{
					RenderDebug(genericRender, node);
				}
			}
		}

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	Result Isosurface::HybridCubes::RenderDebug(GenericRender *genericRender, Node *node)
	{
		if (ur_null == genericRender ||
			ur_null == node ||
			ur_null == node->tetrahedron.get())
			return Result(Success);

		static const ur_float4 s_debugColor = { 1.0f, 1.0f, 0.0f, 1.0f };
		for (const auto &edge : Tetrahedron::Edges)
		{
			genericRender->DrawLine(
				node->tetrahedron->vertices[edge.vid[0]],
				node->tetrahedron->vertices[edge.vid[1]],
				s_debugColor);
		}

		if (this->drawHexahedra)
		{
			static const ur_float4 s_hexaColor = { 0.0f, 1.0f, 0.0f, 1.0f };
			for (ur_uint ih = 0; ih < 4; ++ih)
			{
				const Hexahedron &h = node->tetrahedron->hexahedra[ih];
				genericRender->DrawLine(h.vertices[0], h.vertices[1], s_hexaColor);
				genericRender->DrawLine(h.vertices[2], h.vertices[3], s_hexaColor);
				genericRender->DrawLine(h.vertices[4], h.vertices[5], s_hexaColor);
				genericRender->DrawLine(h.vertices[6], h.vertices[7], s_hexaColor);
				genericRender->DrawLine(h.vertices[0], h.vertices[2], s_hexaColor);
				genericRender->DrawLine(h.vertices[1], h.vertices[3], s_hexaColor);
				genericRender->DrawLine(h.vertices[4], h.vertices[6], s_hexaColor);
				genericRender->DrawLine(h.vertices[5], h.vertices[7], s_hexaColor);
				genericRender->DrawLine(h.vertices[0], h.vertices[4], s_hexaColor);
				genericRender->DrawLine(h.vertices[1], h.vertices[5], s_hexaColor);
				genericRender->DrawLine(h.vertices[2], h.vertices[6], s_hexaColor);
				genericRender->DrawLine(h.vertices[3], h.vertices[7], s_hexaColor);
			}
		}

		return Result(Success);
	}

	Result Isosurface::HybridCubes::RenderOctree(GenericRender *genericRender, EmptyOctree::Node *node)
	{
		if (ur_null == genericRender ||
			ur_null == node)
			return Result(Success);

		static const ur_float4 boxColor(0.0f, 0.0f, 1.0f, 1.0f);
		genericRender->DrawWireBox(node->GetBBox().Min, node->GetBBox().Max, boxColor);

		if (node->HasSubNodes())
		{
			for (ur_uint i = 0; i < EmptyOctree::Node::SubNodesCount; ++i)
			{
				this->RenderOctree(genericRender, node->GetSubNode(i));
			}
		}

		return Result(Success);
	}

	void Isosurface::HybridCubes::ShowImgui()
	{
		ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
		if (ImGui::TreeNode("HybridCubes"))
		{
			ImGui::Checkbox("Freeze update", &this->freezeUpdate);
			ImGui::Checkbox("Hide surface", &this->hideSurface);
			ImGui::Checkbox("Draw tetrahedra", &this->drawTetrahedra);
			ImGui::Checkbox("Hide empty tetrahedra", &this->hideEmptyTetrahedra);
			ImGui::Checkbox("Draw hexahedra", &this->drawHexahedra);
			ImGui::Checkbox("Draw refinement tree", &this->drawRefinementTree);
			
			ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
			if (ImGui::TreeNode("Stats"))
			{
				ImGui::Text("tetrahedraCount:		%i", (int)this->stats.tetrahedraCount);
				ImGui::Text("treeMemory:			%i", (int)this->stats.treeMemory);
				ImGui::Text("meshVideoMemory:		%i", (int)this->stats.meshVideoMemory);
				ImGui::Text("primitivesRendered:	%i", (int)this->stats.primitivesRendered);
				ImGui::Text("buildQueue:			%i", (int)this->stats.buildQueue);
				ImGui::TreePop();
			}
			
			ImGui::TreePop();
		}
	}

	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::Isosurface(Realm &realm) :
		RealmEntity(realm)
	{
		this->drawWireframe = false;
		#if defined(UR_GRAF)
		this->grafRenderer = ur_null;
		#endif
	}

	Isosurface::~Isosurface()
	{
	}

	Result Isosurface::Init(std::unique_ptr<DataVolume> data, std::unique_ptr<Presentation> presentation)
	{
	#if !defined(UR_GRAF)
		Result res(Success);

		this->data = std::move(data);
		this->presentation = std::move(presentation);
		
		res = this->CreateGfxObjects();

		return res;
	#else
		return Result(NotImplemented);
	#endif
	}

	Result Isosurface::Init(std::unique_ptr<DataVolume> data, std::unique_ptr<Presentation> presentation, GrafRenderPass* grafRenderPass)
	{
	#if defined(UR_GRAF)
		Result res(Success);

		this->data = std::move(data);
		this->presentation = std::move(presentation);
		this->grafRenderer = this->GetRealm().GetComponent<GrafRenderer>();

		res = this->CreateGrafObjects(grafRenderPass);

		return res;
	#else
		return Result(NotImplemented);
	#endif
	}

	Result Isosurface::Update()
	{
		// temp: simple synchronous build process
		// support: sync/async construction

		// update presentation
		/*if (this->presentation.get() != ur_null)
		{
			this->presentation->Update();
		}*/

		return Result(Success);
	}

	#if defined(UR_GRAF)

	Result Isosurface::CreateGrafObjects(GrafRenderPass* grafRenderPass)
	{
		Result res = Result(Success);

		this->grafObjects = {}; // reset resources

		if (ur_null == this->grafRenderer)
			return ResultError(InvalidArgs, "Isosurface::CreateGrafObjects: invalid GrafRenderer");
		if (ur_null == grafRenderPass)
			return ResultError(InvalidArgs, "Isosurface::CreateGrafObjects: invalid GrafRenderPass");

		GrafSystem* grafSystem = this->grafRenderer->GetGrafSystem();
		GrafDevice* grafDevice = this->grafRenderer->GetGrafDevice();
		ur_uint frameCount = this->grafRenderer->GetRecordedFrameCount();

		// VS
		res = GrafUtils::CreateShaderFromFile(*grafDevice, "Isosurface_vs.spv", GrafShaderType::Vertex, this->grafObjects.VS);
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGrafObjects: failed to initialize VS");

		// PS
		res = GrafUtils::CreateShaderFromFile(*grafDevice, "Isosurface_ps.spv", GrafShaderType::Pixel, this->grafObjects.PS);
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGrafObjects: failed to initialize PS");

		// PS
		res = GrafUtils::CreateShaderFromFile(*grafDevice, "Isosurface_ps.spv", GrafShaderType::Pixel, this->grafObjects.PSDbg);
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGrafObjects: failed to initialize PSDbg");

		// shader descriptors layout
		res = grafSystem->CreateDescriptorTableLayout(this->grafObjects.shaderDescriptorLayout);
		if (Succeeded(res))
		{
			GrafDescriptorRangeDesc grafDescriptorRanges[] = {
				{ GrafDescriptorType::ConstantBuffer, 0, 2 },
				//{ GrafDescriptorType::Sampler, 0, 1 },
				//{ GrafDescriptorType::Texture, 0, 6 },
			};
			GrafDescriptorTableLayoutDesc grafDescriptorLayoutDesc = {
				GrafShaderStageFlags((ur_uint)GrafShaderStageFlag::Vertex | (ur_uint)GrafShaderStageFlag::Pixel),
				grafDescriptorRanges, ur_array_size(grafDescriptorRanges)
			};
			res = this->grafObjects.shaderDescriptorLayout->Initialize(grafDevice, { grafDescriptorLayoutDesc });
		}
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGrafObjects: failed to initialize descriptor table layout");

		// per frame descriptor tables
		this->grafObjects.shaderDescriptorTable.resize(frameCount);
		for (ur_uint iframe = 0; iframe < frameCount; ++iframe)
		{
			res = grafSystem->CreateDescriptorTable(this->grafObjects.shaderDescriptorTable[iframe]);
			if (Failed(res)) break;
			res = this->grafObjects.shaderDescriptorTable[iframe]->Initialize(grafDevice, { this->grafObjects.shaderDescriptorLayout.get() });
			if (Failed(res)) break;
		}
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGrafObjects: failed to initialize descriptor table(s)");

		// graphics pipeline configuration(s)
		res = grafSystem->CreatePipeline(this->grafObjects.pipelineSolid);
		if (Succeeded(res))
		{
			GrafShader* shaderStages[] = {
				this->grafObjects.VS.get(),
				this->grafObjects.PS.get()
			};
			GrafDescriptorTableLayout* descriptorLayouts[] = {
				this->grafObjects.shaderDescriptorLayout.get(),
			};
			GrafVertexElementDesc vertexElements[] = {
				{ GrafFormat::R32G32B32_SFLOAT, 0 },
				{ GrafFormat::R32G32B32_SFLOAT, 12 },
				{ GrafFormat::R8G8B8A8_UNORM, 24 },
			};
			GrafVertexInputDesc vertexInputs[] = { {
				GrafVertexInputType::PerVertex, 0, sizeof(Vertex),
				vertexElements, ur_array_size(vertexElements)
			} };
			GrafPipeline::InitParams pipelineParams = GrafPipeline::InitParams::Default;
			pipelineParams.RenderPass = grafRenderPass;
			pipelineParams.ShaderStages = shaderStages;
			pipelineParams.ShaderStageCount = ur_array_size(shaderStages);
			pipelineParams.DescriptorTableLayouts = descriptorLayouts;
			pipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
			pipelineParams.VertexInputDesc = vertexInputs;
			pipelineParams.VertexInputCount = ur_array_size(vertexInputs);
			pipelineParams.DepthTestEnable = true;
			pipelineParams.DepthWriteEnable = true;
			pipelineParams.DepthCompareOp = GrafCompareOp::LessOrEqual;
			pipelineParams.PrimitiveTopology = GrafPrimitiveTopology::TriangleList;
			res = this->grafObjects.pipelineSolid->Initialize(grafDevice, pipelineParams);
		}
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGrafObjects: failed to initialize pipeline object");

		return res;
	}

	#else

	Result Isosurface::CreateGfxObjects()
	{
		Result res = Result(Success);

		this->gfxObjects = GfxObjects(); // reset gfx resources

		// VS
		res = CreateVertexShaderFromFile(this->GetRealm(), this->gfxObjects.VS, "Isosurface_vs.cso");
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGfxObjects: failed to initialize VS");

		// PS
		res = CreatePixelShaderFromFile(this->GetRealm(), this->gfxObjects.PS, "Isosurface_ps.cso");
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGfxObjects: failed to initialize PS");

		// PSDbg
		res = CreatePixelShaderFromFile(this->GetRealm(), this->gfxObjects.PSDbg, "IsosurfaceDbg_ps.cso");
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGfxObjects: failed to initialize PSDbg");

		// Input Layout
		res = this->GetRealm().GetGfxSystem()->CreateInputLayout(this->gfxObjects.inputLayout);
		if (Succeeded(res))
		{
			GfxInputElement elements[] = {
				{ GfxSemantic::Position, 0, 0, GfxFormat::R32G32B32, GfxFormatView::Float, 0 },
				{ GfxSemantic::Normal, 0, 0, GfxFormat::R32G32B32, GfxFormatView::Float, 0 },
				{ GfxSemantic::Color, 0, 0, GfxFormat::R8G8B8A8, GfxFormatView::Unorm, 0 }
			};
			res = this->gfxObjects.inputLayout->Initialize(*this->gfxObjects.VS.get(), elements, ur_array_size(elements));
		}
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGfxObjects: failed to initialize input layout");

		// Pipeline State
		res = this->GetRealm().GetGfxSystem()->CreatePipelineState(this->gfxObjects.pipelineState);
		if (Succeeded(res))
		{
			this->gfxObjects.pipelineState->InputLayout = this->gfxObjects.inputLayout.get();
			this->gfxObjects.pipelineState->VertexShader = this->gfxObjects.VS.get();
			this->gfxObjects.pipelineState->PixelShader = this->gfxObjects.PS.get();
			GfxRenderState gfxRS = GfxRenderState::Default;
			gfxRS.RasterizerState.CullMode = GfxCullMode::CCW;
			gfxRS.RasterizerState.FillMode = GfxFillMode::Solid;
			gfxRS.SamplerState[0].AddressU = GfxTextureAddressMode::Wrap;
			gfxRS.SamplerState[0].AddressV = GfxTextureAddressMode::Wrap;
			res = this->gfxObjects.pipelineState->SetRenderState(gfxRS);
		}
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGfxObjects: failed to initialize pipeline state");

		res = this->GetRealm().GetGfxSystem()->CreatePipelineState(this->gfxObjects.wireframeState);
		if (Succeeded(res))
		{
			this->gfxObjects.wireframeState->InputLayout = this->gfxObjects.pipelineState->InputLayout;
			this->gfxObjects.wireframeState->VertexShader = this->gfxObjects.pipelineState->VertexShader;
			this->gfxObjects.wireframeState->PixelShader = this->gfxObjects.PSDbg.get();
			GfxRenderState gfxRS = this->gfxObjects.pipelineState->GetRenderState();
			gfxRS.RasterizerState.FillMode = GfxFillMode::Wireframe;
			gfxRS.RasterizerState.DepthBias = -10;
			res = this->gfxObjects.wireframeState->SetRenderState(gfxRS);
		}
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGfxObjects: failed to initialize wireframe state");

		// Constant Buffer
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxObjects.CB);
		if (Succeeded(res))
		{
			res = this->gfxObjects.CB->Initialize(sizeof(CommonCB), 0, GfxUsage::Dynamic,
				(ur_uint)GfxBindFlag::ConstantBuffer, (ur_uint)GfxAccessFlag::Write);
		}
		if (Failed(res))
			return ResultError(Failure, "Isosurface::CreateGfxObjects: failed to initialize constant buffer");

		return res;
	}

	#endif

	Result Isosurface::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const Atmosphere *atmosphere)
	{
	#if !defined(UR_GRAF)
		Result res(Success);

		// render isosurface

		if (this->presentation.get() != ur_null)
		{
			// texturing test
			#if 0
			static const bool firstTime = [&]{
				CreateTextureFromFile(this->GetRealm(), this->gfxObjects.albedoMaps[0], "Res/PBS_Stones_Big_AM.dds");
				CreateTextureFromFile(this->GetRealm(), this->gfxObjects.albedoMaps[1], "Res/PBS_Stones_Big_AM.dds");
				CreateTextureFromFile(this->GetRealm(), this->gfxObjects.albedoMaps[2], "Res/PBS_Stones_Big_AM.dds");
				CreateTextureFromFile(this->GetRealm(), this->gfxObjects.normalMaps[0], "Res/PBS_Stones_Big_NM.dds");
				CreateTextureFromFile(this->GetRealm(), this->gfxObjects.normalMaps[1], "Res/PBS_Stones_Big_NM.dds");
				CreateTextureFromFile(this->GetRealm(), this->gfxObjects.normalMaps[2], "Res/PBS_Stones_Big_NM.dds");
				return true;
			}();
			gfxContext.SetTexture(this->gfxObjects.albedoMaps[0].get(), 0);
			gfxContext.SetTexture(this->gfxObjects.albedoMaps[1].get(), 1);
			gfxContext.SetTexture(this->gfxObjects.albedoMaps[2].get(), 2);
			gfxContext.SetTexture(this->gfxObjects.normalMaps[0].get(), 3);
			gfxContext.SetTexture(this->gfxObjects.normalMaps[1].get(), 4);
			gfxContext.SetTexture(this->gfxObjects.normalMaps[2].get(), 5);
			#endif

			CommonCB cb;
			cb.ViewProj = viewProj;
			cb.CameraPos = cameraPos;
			cb.AtmoParams = (atmosphere != ur_null ? atmosphere->GetDesc() : Atmosphere::Desc::Invisible);
			GfxResourceData cbResData = { &cb, sizeof(CommonCB), 0 };
			gfxContext.UpdateBuffer(this->gfxObjects.CB.get(), GfxGPUAccess::WriteDiscard, &cbResData, 0, cbResData.RowPitch);
			gfxContext.SetConstantBuffer(this->gfxObjects.CB.get(), 0);
			gfxContext.SetPipelineState(this->gfxObjects.pipelineState.get());

			res = this->presentation->Render(gfxContext, viewProj);

			if (this->drawWireframe)
			{
				gfxContext.SetPipelineState(this->gfxObjects.wireframeState.get());
				this->presentation->Render(gfxContext, viewProj);
			}
		}

		return res;
	#else
		return Result(NotImplemented);
	#endif
	}

	Result Isosurface::Render(GrafCommandList &grafCmdList, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const Atmosphere *atmosphere)
	{
	#if defined(UR_GRAF)
		if (ur_null == this->grafRenderer || ur_null == this->grafObjects.pipelineSolid)
			return Result(NotInitialized);

		Result res(Success);

		// render isosurface

		if (this->presentation.get() != ur_null)
		{
			// fill CB
			CommonCB cbData;
			cbData.ViewProj = viewProj;
			cbData.CameraPos = cameraPos;
			cbData.AtmoParams = (atmosphere != ur_null ? atmosphere->GetDesc() : Atmosphere::Desc::Invisible);
			GrafBuffer* dynamicCB = this->grafRenderer->GetDynamicConstantBuffer();
			Allocation dynamicCBAlloc = grafRenderer->GetDynamicConstantBufferAllocation(sizeof(CommonCB));
			dynamicCB->Write((ur_byte*)&cbData, sizeof(cbData), 0, dynamicCBAlloc.Offset);

			// fill shader descriptors
			ur_uint frameIdx = this->grafRenderer->GetCurrentFrameId();
			GrafDescriptorTable *shaderDescriptorTable = this->grafObjects.shaderDescriptorTable[frameIdx].get();
			shaderDescriptorTable->SetConstantBuffer(0, dynamicCB, dynamicCBAlloc.Offset, dynamicCBAlloc.Size);

			// bind pipeline
			grafCmdList.BindPipeline(this->grafObjects.pipelineSolid.get());
			grafCmdList.BindDescriptorTable(shaderDescriptorTable, this->grafObjects.pipelineSolid.get());

			res = this->presentation->Render(grafCmdList, viewProj);

			/*if (this->drawWireframe)
			{
				grafCmdList.BindPipeline(this->grafObjects.pipelineDebug.get());
				this->presentation->Render(gfxContext, viewProj);
			}*/
		}

		return res;
	#else
		return Result(NotImplemented);
	#endif
	}

	void Isosurface::ShowImgui()
	{
		if (ImGui::CollapsingHeader("Isosurface"))
		{
			ImGui::Checkbox("Draw wireframe", &this->drawWireframe);
			if (this->presentation.get() != ur_null)
			{
				this->presentation->ShowImgui();
			}
		}
	}

} // end namespace UnlimRealms