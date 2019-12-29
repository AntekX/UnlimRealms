///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core/ResultTypes.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL Allocation
	{
		ur_size Offset;
		ur_size Size;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Linear Allocator
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL LinearAllocator
	{
	public:

		LinearAllocator();

		~LinearAllocator();

		void Init(ur_size size, ur_size alignment = 1);

		void Resize(ur_size size);

		Allocation Allocate(ur_size allocSize);

	private:

		ur_size size;
		ur_size alignment;
		ur_size offset;
	};

} // end namespace UnlimRealms
