///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Sys/Std/StdJobSystem.h"

namespace UnlimRealms
{

	StdJobSystem::StdJobSystem(Realm &realm) :
		JobSystem(realm)
	{
		this->shutdown = false;
		this->jobCount = 0;
		threads.resize(std::thread::hardware_concurrency());
		for (auto &thread : this->threads)
		{
			thread.reset( new std::thread() );
		}
	}

	StdJobSystem::~StdJobSystem()
	{
		this->shutdown = true;
		for (auto &thread : this->threads)
		{
			thread->join();
		}
		this->threads.clear();
	}

	void StdJobSystem::OnJobAdded()
	{
		{ // update jobs counter
			std::lock_guard<std::mutex> lock(this->jobCountMutex);
			this->jobCount += 1;
		}
		// notify & wake working threads
		this->jobCountCondition.notify_all();
	}

	void StdJobSystem::ThreadFunction(StdJobSystem *jobSystem)
	{
		if (jobSystem != ur_null)
		{
			while (!jobSystem->shutdown)
			{
				// block until we have a job to do
				size_t &jobCount = jobSystem->jobCount;
				std::unique_lock<std::mutex> jobCountLock(jobSystem->jobCountMutex);
				jobSystem->jobCountCondition.wait(jobCountLock, [&jobCount] { return (jobCount > 0); });

				// fetch a job and do it
				// todo:
			}
		}
	}

} // end namespace UnlimRealms