///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core/Math.h"
#include "Core/ResultTypes.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base octree
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class TData>
	class Octree
	{
	public:
		
		class Node
		{
		public:

			enum NodePosId
			{
				PosIdx_LeftBottomNear = 0,
				PosIdx_RightBottomNear,
				PosIdx_LeftTopNear,
				PosIdx_RightTopNear,
				PosIdx_LeftBottomFar,
				PosIdx_RightBottomFar,
				PosIdx_LeftTopFar,
				PosIdx_RightTopFar,
				SubNodesCount
			};

			Node(Octree &tree, Node *parent, const BoundingBox &bbox);

			~Node();

			void Split();

			void Merge();

			inline bool HasSubNodes() const;

			inline Octree& GetTree() { return this->tree; }

			inline Node* GetParent() const { return this->parent; }

			inline ur_uint GetLevel() const { return this->level; }

			inline const BoundingBox& GetBBox() const { return this->bbox; }

			inline const TData& GetData() const { return this->data; }

			inline TData& GetData() { return this->data; }

			inline Node* GetSubNode(ur_uint idx) const { return (idx < SubNodesCount ? this->subNodes[idx].get() : ur_null); }

		private:

			Octree &tree;
			Node *parent;
			ur_uint level;
			BoundingBox bbox;
			TData data;
			std::unique_ptr<Node> subNodes[SubNodesCount];
		};
	
		enum class Event
		{
			NodeAdded,
			NodeRemoved
		};
		typedef std::function<void(Node*, Event)> Handler;

		static const ur_uint DefaultDepth = 32;

		Octree(ur_uint depth = DefaultDepth, Handler handeler = ur_null);

		~Octree();

		Result Init(const BoundingBox &rootBB);

		inline ur_uint GetDepth() const { return this->depth; }

		inline Handler GetHandler() { return this->handler; }

		inline Node* GetRoot() const { return this->root.get(); }

		inline Node* FindNode(const BoundingBox &bbox) const
		{
			return FindNode(this->root.get(), bbox);
		}

		static Node* FindNode(Node *root, const BoundingBox &bbox)
		{
			if (ur_null == root)
				return ur_null;

			if (root->GetBBox() == bbox)
				return root;

			if (root->HasSubNodes())
			{
				for (ur_uint i = 0; i < Node::SubNodesCount; ++i)
				{
					Node *node = FindNode(root->GetSubNode(i), bbox);
					if (node != ur_null)
					{
						return node;
					}
				}
			}

			return ur_null;
		}

	private:

		ur_uint depth;
		Handler handler;
		std::unique_ptr<Node> root;
	};
	struct UR_DECL EmptyOctreeNodeData {};
	typedef Octree<EmptyOctreeNodeData> EmptyOctree;


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Noise generators
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	class UR_DECL PerlinNoise
	{
	public:

		static const ur_int64 perm[512];
		static const ur_int64 grad[12][3];

	public:

		PerlinNoise();

		~PerlinNoise();

		static ur_double Noise(ur_double x, ur_double y, ur_double z);

	protected:

		inline static ur_double fade(ur_double t)
		{
			return t * t * t * (t * (t * 6 - 15) + 10);
		}

		static ur_double grad_fn(ur_int64 hash, ur_double x, ur_double y, ur_double z)
		{
			ur_int32 h = (ur_int32(hash) & 15);
			ur_double u = (h < 8 ? x : y);
			ur_double v = (h < 4 ? y : (h == 12 || h == 14 ? x : z));
			return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
		}
	};


	class UR_DECL SimplexNoise
	{
	public:

		SimplexNoise();

		~SimplexNoise();

		static ur_double Noise(ur_double xin, ur_double yin);

		static ur_double Noise(ur_double xin, ur_double yin, ur_double zin);

	protected:

		static inline ur_int64 fastfloor(ur_double x)
		{
			return (x > 0 ? (ur_int64)x : (ur_int64)x - 1);
		}

		static inline ur_double dot(const ur_int64 g[3], ur_double x, ur_double y)
		{
			return (g[0] * x + g[1] * y);
		}

		static inline ur_double dot(const ur_int64 g[3], ur_double x, ur_double y, ur_double z)
		{
			return (g[0] * x + g[1] * y + g[2] * z);
		}
	};

} // end namespace UnlimRealms

#include "Algorithms.inline.h"