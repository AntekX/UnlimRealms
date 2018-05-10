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
	// Job
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	
	Job::Job(JobSystem &jobSystem, DataPtr data, Callback callback) :
		JobSystemEntity(jobSystem),
		data(data),
		callback(callback)
	{
		this->interrupt = false;
		this->state = State::Pending;
		this->progress = 0.0f;
		this->resultCode = Undefined;
	}

	Job::~Job()
	{

	}

	void Job::Execute()
	{
		if (this->callback != ur_null)
		{
			this->state = State::InProgress;
			this->callback( Context(this->data, this->interrupt, this->progress, this->resultCode) );
		}
		else
		{
			this->resultCode = InvalidArgs;
		}
		this->state = State::Finished;
	}

	void Job::Interrupt()
	{
		ur_bool expectedState = true;
		ur_bool desiredState = false;
		if (this->interrupt.compare_exchange_strong(expectedState, desiredState))
		{
			this->jobSystem.Remove(*this);
		}
	}

	void Job::Wait()
	{
		while (!this->Finished()) {};
	}

	void Job::WaitProgress(ur_float expectedProgress)
	{
		while (this->progress.load() < expectedProgress) {}
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// JobSystem
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	JobSystem::JobSystem(Realm &realm) :
		RealmEntity(realm)
	{

	}

	JobSystem::~JobSystem()
	{

	}

	std::shared_ptr<Job> JobSystem::Add(Job::DataPtr jobData, Job::Callback jobCallback)
	{
		return this->Add(JobPriority::Normal, jobData, jobCallback);
	}

	std::shared_ptr<Job> JobSystem::Add(JobPriority priority, Job::DataPtr jobData, Job::Callback jobCallback)
	{
		auto &job = std::make_shared<Job>(*this, jobData, jobCallback);
		if (!this->Add(job, priority))
		{
			job = ur_null;
		}
		return job;
	}

	ur_bool JobSystem::Add(std::shared_ptr<Job> job, JobPriority priority)
	{
		bool res = false;

		if (job == ur_null || job->Interrupted())
			return res; // nothing to do

		{ // locked scope: add to queue
			std::lock_guard<std::mutex> lockQueue(this->queueMutex);
			if (job->systemHandle.listPtr == ur_null)
			{
				auto &queue = this->priorityQueue[(ur_int)priority];
				queue.jobs.push_front(job);
				job->systemHandle.listPtr = &queue.jobs;
				job->systemHandle.iter = queue.jobs.begin();
				this->OnJobAdded();
				res = true;
			}
		}

		return res;
	}

	ur_bool JobSystem::Remove(Job &job)
	{
		bool res = false;

		{ // locked scope: remove from queue
			std::lock_guard<std::mutex> lockQueue(this->queueMutex);
			if (job.systemHandle.listPtr != ur_null)
			{
				auto &queue = *reinterpret_cast<JobQueue*>(job.systemHandle.listPtr);
				queue.jobs.erase(job.systemHandle.iter);
				job.systemHandle.listPtr = ur_null;
				this->OnJobRemoved();
				res = true;
			}
		}

		return res;
	}

	void JobSystem::Interrupt(Job &job)
	{
		job.Interrupt();
	}

	std::shared_ptr<Job> JobSystem::FetchJob()
	{
		std::shared_ptr<Job> job(ur_null);
		
		{ // locked scope: find a job
			std::lock_guard<std::mutex> lockQueue(this->queueMutex);
			for (auto &queue : this->priorityQueue)
			{
				if (!queue.jobs.empty())
				{
					job = queue.jobs.back();
					queue.jobs.pop_back();
					job->systemHandle.listPtr = ur_null;
					this->OnJobRemoved();
					break;
				}
			}
		}

		return job;
	}

	void JobSystem::OnJobAdded()
	{
		// base implementation: synchronous execution
		auto &job = this->FetchJob();
		job->Execute();
	}

	void JobSystem::OnJobRemoved()
	{
	}

} // end namespace UnlimRealms