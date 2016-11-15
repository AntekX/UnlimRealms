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
			this->desc.PartitionProgression < 1.0f)
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
			levelDistance *= this->desc.PartitionProgression;
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

	void Isosurface::AdaptiveVolume::PartitionByDistance(const ur_float3 &pos)
	{
		if (this->levelInfo.empty())
			return;

		this->partitionPoint = pos;

		this->PartitionByDistance(pos, this->GetRoot());
	}

	void Isosurface::AdaptiveVolume::PartitionByDistance(const ur_float3 &pos, AdaptiveVolume::Node *node)
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
					this->PartitionByDistance(pos, node->GetSubNode(i));
				}
			}
		}
		else
		{
			node->Merge();
		}
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
	// Isosurface::Triangulator
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::Triangulator::Triangulator(Isosurface &isosurface) :
		SubSystem(isosurface)
	{

	}

	Isosurface::Triangulator::~Triangulator()
	{

	}

	Result Isosurface::Triangulator::Construct(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox)
	{
		return Result(NotImplemented);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface::SurfaceNet
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::SurfaceNet::SurfaceNet(Isosurface &isosurface) :
		Triangulator(isosurface)
	{

	}

	Isosurface::SurfaceNet::~SurfaceNet()
	{

	}

	Result Isosurface::SurfaceNet::Construct(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox)
	{
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
		auto &gfxVB = block.gfxVB;
		Result res = gfxSystem->CreateBuffer(gfxVB);
		if (Succeeded(res))
		{
			GfxResourceData gfxRes = { vertices.data(), (ur_uint)vertices.size() * sizeof(Vertex), 0 };
			res = gfxVB->Initialize(gfxRes.RowPitch, GfxUsage::Immutable, (ur_uint)GfxBindFlag::VertexBuffer, 0, &gfxRes);
		}
		if (Failed(res))
			return Result(Failure);

		// create index buffer
		auto &gfxIB = block.gfxIB;
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


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface::HybridTetrahedra
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::HybridTetrahedra::HybridTetrahedra(Isosurface &isosurface) :
		Triangulator(isosurface)
	{

	}

	Isosurface::HybridTetrahedra::~HybridTetrahedra()
	{

	}

	Result Isosurface::HybridTetrahedra::Construct(AdaptiveVolume &volume, Block &block, const BoundingBox &bbox)
	{
		// TODO:
		return Result(NotImplemented);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface::Builder
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::Builder::Builder(Isosurface &isosurface) :
		SubSystem(isosurface)
	{
		this->buildCache.state = BuildCacheEntry::State::Idle;
	}

	Isosurface::Builder::~Builder()
	{

	}

	void Isosurface::Builder::AddNode(AdaptiveVolume::Node *node)
	{
		if (ur_null == node)
			return;

		this->newNodes.insert(node);

		AdaptiveVolume& volume = static_cast<AdaptiveVolume&>(node->GetTree());
		//ur_float priority = node->GetBBox().Distance(volume.GetPartitionPoint());
		ur_float priority = (ur_float)node->GetLevel();
		this->priorityQueue.insert(std::make_pair(priority, node));
	}

	void Isosurface::Builder::RemoveNode(AdaptiveVolume::Node *node)
	{
		auto ientry = std::find_if(this->priorityQueue.begin(), this->priorityQueue.end(), [&node](auto &entry) -> bool {
			return (entry.second == node);
		});
		if (ientry != this->priorityQueue.end())
		{
			this->priorityQueue.erase(ientry);
		}

		this->newNodes.erase(node);
	}

	Result Isosurface::Builder::Build()
	{
		// acquire sub systems
		AdaptiveVolume *volume = this->isosurface.GetVolume();
		Loader *loader = this->isosurface.GetLoader();
		Triangulator *triangulater = this->isosurface.GetTriangulator();
		if (ur_null == volume ||
			ur_null == loader ||
			ur_null == triangulater)
			return Result(NotInitialized);

		bool syncMode = false;
		if (syncMode)
		{
			// temp: simple synchronous build process
			// support: sync/async construction
			for (AdaptiveVolume::Node *node : this->newNodes)
			{
				loader->Load(*volume, node->GetData(), node->GetBBox());
				triangulater->Construct(*volume, node->GetData(), node->GetBBox());
			}
			this->newNodes.clear();
			this->priorityQueue.clear();
		}
		else
		{
			// todo: rewrite
			// temp: simple one thread async builder test
			ur_size idx = 0;
			if (BuildCacheEntry::State::Finished == this->buildCache.state)
			{
				this->thread.join();
				AdaptiveVolume::Node *node = volume->FindNode(this->buildCache.bbox);
				if (node != ur_null)
				{
					node->GetData().field = std::move(this->buildCache.block.field);
					node->GetData().gfxVB = std::move(this->buildCache.block.gfxVB);
					node->GetData().gfxIB = std::move(this->buildCache.block.gfxIB);
				}
				this->buildCache.bbox.Min = this->buildCache.bbox.Max = 0.0f;
				this->buildCache.state = BuildCacheEntry::State::Idle;
			}

			if (BuildCacheEntry::State::Idle == this->buildCache.state &&
				this->newNodes.size() > 0)
			{
				auto ientry = this->priorityQueue.begin();
				this->buildCache.bbox = ientry->second->GetBBox();
				this->buildCache.state = BuildCacheEntry::State::Working;
				this->newNodes.erase(ientry->second);
				this->priorityQueue.erase(ientry);
				this->thread = std::thread(BuildBlock, volume, &this->buildCache);
			}
		}

		return Result(Success);
	}

	void Isosurface::Builder::BuildBlock(AdaptiveVolume *volume, BuildCacheEntry *cacheEntry)
	{
		if (volume != ur_null)
		{
			Isosurface &isosurface = volume->GetIsosurface();
			Loader *loader = isosurface.GetLoader();
			Triangulator *triangulater = isosurface.GetTriangulator();

			if (loader != ur_null)
			{
				loader->Load(*volume, cacheEntry->block, cacheEntry->bbox);
			}

			if (triangulater != ur_null)
			{
				triangulater->Construct(*volume, cacheEntry->block, cacheEntry->bbox);
			}
		}

		cacheEntry->state = BuildCacheEntry::State::Finished;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Isosurface
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Isosurface::Isosurface(Realm &realm) :
		RealmEntity(realm),
		debugRender(realm)
	{
		this->drawDebugBounds = true;
		this->drawWireframe = false;
		memset(&this->stats, 0, sizeof(this->stats));
	}

	Isosurface::~Isosurface()
	{
		// explicit deinitialization, order matters:
		// volume calls handler function, which expects builder to be valid or at least null
		this->volume.reset(ur_null);
		this->builder.reset(ur_null);
		this->loader.reset(ur_null);
		this->triangulator.reset(ur_null);
	}

	Result Isosurface::Init(const AdaptiveVolume::Desc &desc, std::unique_ptr<Loader> loader, std::unique_ptr<Triangulator> triangulator)
	{
		Result res(Success);

		this->volume.reset(new AdaptiveVolume(*this));
		this->builder.reset(new Builder(*this));
		this->loader = std::move(loader);
		this->triangulator = std::move(triangulator);
		
		res = this->volume->Init(desc);
		if (Failed(res))
			return ResultError(Failure, "Isosurface::Init: failed to initialize volume");
		
		this->debugRender.Init();

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

	Result Isosurface::RenderSurface(GfxContext &gfxContext, AdaptiveVolume::Node *volumeNode)
	{
		if (ur_null == volumeNode)
			return Result(InvalidArgs);

		bool hasSubSurface = volumeNode->HasSubNodes();
		if (hasSubSurface)
		{
			for (ur_uint i = 0; i < AdaptiveVolume::Node::SubNodesCount; ++i)
			{
				const Block &subBlock = volumeNode->GetSubNode(i)->GetData();
				hasSubSurface &= !subBlock.field.empty();
				if (!hasSubSurface) break;
			}
		}

		if (hasSubSurface)
		{
			for (ur_uint i = 0; i < AdaptiveVolume::Node::SubNodesCount; ++i)
			{
				this->RenderSurface(gfxContext, volumeNode->GetSubNode(i));
			}
		}
		else
		{
			const Block &surfBlock = volumeNode->GetData();
			const ur_uint indexCount = (surfBlock.gfxIB.get() ? surfBlock.gfxIB->GetDesc().Size / sizeof(Index) : 0);
			gfxContext.SetVertexBuffer(surfBlock.gfxVB.get(), 0, sizeof(Vertex), 0);
			gfxContext.SetIndexBuffer(surfBlock.gfxIB.get(), sizeof(Index) * 8, 0);
			gfxContext.DrawIndexed(indexCount, 0, 0, 0, 0);
		}

		return Result(Success);
	}

	Result Isosurface::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj)
	{
		Result res(Success);
		if (ur_null == this->volume)
			return res;

		// render isosurface

		CommonCB cb;
		cb.viewProj = viewProj;
		GfxResourceData cbResData = { &cb, sizeof(CommonCB), 0 };
		gfxContext.UpdateBuffer(this->gfxObjects->CB.get(), GfxGPUAccess::WriteDiscard, false, &cbResData, 0, cbResData.RowPitch);
		const RectI &canvasBound = this->GetRealm().GetCanvas()->GetBound();
		GfxViewPort viewPort = { 0.0f, 0.0f, (float)canvasBound.Width(), (float)canvasBound.Height(), 0.0f, 1.0f };
		gfxContext.SetViewPort(&viewPort);
		gfxContext.SetConstantBuffer(this->gfxObjects->CB.get(), 0);
		gfxContext.SetPipelineState(this->gfxObjects->pipelineState.get());

		res = this->RenderSurface(gfxContext, this->volume->GetRoot());

		// render debug info

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

		return res;
	}

	void Isosurface::DrawDebugTreeBounds(const AdaptiveVolume::Node *node)
	{
		if (ur_null == node)
			return;

		if (node->HasSubNodes())
		{
			for (ur_uint i = 0; i < AdaptiveVolume::Node::SubNodesCount; ++i)
			{
				this->DrawDebugTreeBounds(node->GetSubNode(i));
			}
		}
		else
		{
			static ur_float4 DebugColor[2] = {
				{ 0.5f, 0.5f, 0.55f, 1.0f },
				{ 1.0f, 1.0f, 0.0f, 1.0f }
			};
			bool hasSurface = (node->GetData().gfxIB.get() != ur_null && node->GetData().gfxIB->GetDesc().Size > 0);
			this->debugRender.DrawBox(node->GetBBox().Min, node->GetBBox().Max, DebugColor[hasSurface ? 1 : 0]);
		}
	}

	void Isosurface::GatherStats()
	{
		memset(&this->stats, 0, sizeof(this->stats));
		this->GatherStats(this->volume->GetRoot());
	}

	void Isosurface::GatherStats(const AdaptiveVolume::Node *node)
	{
		if (ur_null == node)
			return;

		this->stats.volumeNodes += 1;

		this->stats.sysMemory += (ur_uint)node->GetData().field.size() * sizeof(Block::ValueType);
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
		else if (node->GetData().gfxIB.get() != ur_null && node->GetData().gfxIB->GetDesc().Size > 0)
		{
			this->stats.surfaceNodes += 1;
		}
	}

} // end namespace UnlimRealms