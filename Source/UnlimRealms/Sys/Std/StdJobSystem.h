///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Sys/JobSystem.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Standard library based JobSystem implemntation
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL StdJobSystem : public JobSystem
	{
	public:

		StdJobSystem(Realm &realm);

		virtual ~StdJobSystem();

	private:

		virtual void OnJobAdded() override;

		virtual void OnJobRemoved() override;

		static void ThreadFunction(StdJobSystem *jobSystem);

		std::vector<std::unique_ptr<std::thread>> threads;
		size_t jobCount;
		std::condition_variable jobCountCondition;
		std::mutex jobCountMutex;

		ur_bool shutdown;
	};

} // end namespace UnlimRealms