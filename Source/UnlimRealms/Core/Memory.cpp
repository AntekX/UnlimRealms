///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Core/Memory.h"

namespace UnlimRealms
{

	LinearAllocator::LinearAllocator() :
		size(0),
		alignment(1),
		offset(0)
	{
	}

	LinearAllocator::~LinearAllocator()
	{
	}

	void LinearAllocator::Init(ur_size size, ur_size alignment)
	{
		this->alignment = alignment;
		this->Resize(size);
	}

	void LinearAllocator::Resize(ur_size size)
	{
		this->size = ((size + alignment - 1) / alignment) * alignment;
	}

	Allocation LinearAllocator::Allocate(ur_size allocSize)
	{
		allocSize = ((allocSize + alignment - 1) / alignment) * alignment;

		Allocation alloc = {};
		if (allocSize > this->size)
			return alloc;

		if (this->offset + allocSize > this->size)
		{
			this->offset = 0;
		}

		alloc.Offset = this->offset;
		alloc.Size = allocSize;
		this->offset += allocSize;
		
		return alloc;
	}

} // end namespace UnlimRealms