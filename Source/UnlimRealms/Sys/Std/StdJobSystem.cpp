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
		ur_size priorityCount = ur_size(JobPriority::Count);
		threads.resize(std::max(ur_int(std::thread::hardware_concurrency()) - 1, ur_int(priorityCount)));
		// note: threads that accept all priority types (starting from the lowest) must be available in the first place,
		// more exclusive high priority threads reserved in the end, if hardware concurrency is high enough
		for (ur_size it = 0; it < threads.size(); ++it)
		{
			JobPriority threadJobPriorityMin = JobPriority((priorityCount - 1) - (it % priorityCount));
			threads[it].reset( new std::thread(ThreadFunction, this, threadJobPriorityMin) );
		}
	}

	StdJobSystem::~StdJobSystem()
	{
		this->shutdown = true;
		this->jobCountCondition.notify_all();
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

	void StdJobSystem::OnJobRemoved()
	{
		{ // update jobs counter
			std::lock_guard<std::mutex> lock(this->jobCountMutex);
			this->jobCount -= 1;
		}
	}

	void StdJobSystem::ThreadFunction(StdJobSystem *jobSystem, JobPriority jobPriorityMin)
	{
		if (jobSystem != ur_null)
		{
			bool &shutdown = jobSystem->shutdown;
			while (!shutdown)
			{
				{ // block until we have a job to do
					size_t &jobCount = jobSystem->jobCount;
					std::unique_lock<std::mutex> jobCountLock(jobSystem->jobCountMutex);
					jobSystem->jobCountCondition.wait(jobCountLock, [&jobCount, &shutdown] {
						return (jobCount > 0 || shutdown);
					});
				}

				// fetch a job and do it
				std::shared_ptr<Job> job = jobSystem->FetchJob(jobPriorityMin);
				if (job != nullptr)
				{
					job->Execute();
				}
			}
		}
	}

} // end namespace UnlimRealms