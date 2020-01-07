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
		threads.resize(std::max(int(std::thread::hardware_concurrency()) - 1, 1));
		// if there are enough threads, first two will be reserved to exclusively process only High & Normal priority jobs (to avoid stalling run time threads)
		ur_size maxExclusivePriority = (ur_size)std::max(ur_int(0), ur_int(JobPriority::Count) - ur_int(threads.size()));
		for (ur_size it = 0; it < threads.size(); ++it)
		{
			JobPriority threadJobPriorityMin = JobPriority(std::min(std::max(maxExclusivePriority, it), ur_size(JobPriority::Count)));
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