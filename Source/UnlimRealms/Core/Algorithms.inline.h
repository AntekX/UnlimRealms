///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base octree
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <class TData>
	Octree<TData>::Octree(ur_uint depth, Handler handler)
	{
		this->depth = depth;
		this->handler = handler;
	}

	template <class TData>
	Octree<TData>::~Octree()
	{
		if (this->root.get() != ur_null)
		{
			if (this->GetHandler() != ur_null)
			{
				this->GetHandler()(this->root.get(), Event::NodeRemoved);
			}
			this->root.reset(ur_null);
		}
	}

	template <class TData>
	Result Octree<TData>::Init(const BoundingBox &rootBB)
	{
		this->root.reset(new Node(*this, ur_null, rootBB));
		if (this->GetHandler() != ur_null)
		{
			this->GetHandler()(this->root.get(), Event::NodeAdded);
		}
		return Result(Success);
	}

	template <class TData>
	Octree<TData>::Node::Node(Octree &tree, Node *parent, const BoundingBox &bbox) :
		tree(tree), parent(parent), bbox(bbox)
	{
		this->level = (this->parent != ur_null ? this->parent->level + 1 : 0);
	}

	template <class TData>
	Octree<TData>::Node::~Node()
	{

	}

	template <class TData>
	void Octree<TData>::Node::Split()
	{
		if (this->level >= this->tree.GetDepth() ||
			this->HasSubNodes())
			return;

		static const ur_float3 sub_ofs[SubNodesCount] = {
			{ 0.0f, 0.0f, 0.0f }, { 0.5f, 0.0f, 0.0f }, { 0.0f, 0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f },
			{ 0.0f, 0.0f, 0.5f }, { 0.5f, 0.0f, 0.5f }, { 0.0f, 0.5f, 0.5f }, { 0.5f, 0.5f, 0.5f }
		};
		BoundingBox subBox;
		for (ur_uint i = 0; i < SubNodesCount; ++i)
		{
			subBox.Min = this->bbox.Min * (sub_ofs[i] * -1.0f + 1.0f) + this->bbox.Max * sub_ofs[i];
			subBox.Max = this->bbox.Min * ((sub_ofs[i] + 0.5f) * -1.0f + 1.0f) + this->bbox.Max * (sub_ofs[i] + 0.5f);
			this->subNodes[i].reset(new Node(this->tree, this, subBox));
			if (this->tree.GetHandler() != ur_null)
			{
				this->tree.GetHandler()(this->subNodes[i].get(), Event::NodeAdded);
			}
		}
	}

	template <class TData>
	void Octree<TData>::Node::Merge()
	{
		if (!this->HasSubNodes())
			return;

		for (ur_uint i = 0; i < SubNodesCount; ++i)
		{
			this->subNodes[i]->Merge();
			if (this->tree.GetHandler() != ur_null)
			{
				this->tree.GetHandler()(this->subNodes[i].get(), Event::NodeRemoved);
			}
			this->subNodes[i].reset(ur_null);
		}
	}

	template <class TData>
	inline bool Octree<TData>::Node::HasSubNodes() const
	{
		// enough to check the first node pointer only,
		// split/merge functions always allocate/deallocate all subnodes
		return (this->subNodes[0].get() != ur_null);
	}

} // end namespace UnlimRealms
