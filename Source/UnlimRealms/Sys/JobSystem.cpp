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
	
	Job::Job(JobSystem &jobSystem, Callback callback, DataPtr data) :
		JobSystemEntity(jobSystem),
		callback(callback),
		data(data)
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
		if (!this->interrupt)
		{
			this->interrupt = true;
			this->jobSystem.Remove(*this);
		}
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

	ur_bool JobSystem::Add(Job::Callback jobCallback, Job::DataPtr jobData, JobPriority priority)
	{
		return this->Add( std::make_shared<Job>(*this, jobCallback, jobData), priority );
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
				job->systemHandle.listPtr = reinterpret_cast<void*>(&queue);
				job->systemHandle.iterPtr = reinterpret_cast<void*>(&queue.jobs.begin());
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
				auto &iterator = *reinterpret_cast<decltype(queue.jobs)::iterator*>(job.systemHandle.iterPtr);
				queue.jobs.erase(iterator);
				job.systemHandle.listPtr = ur_null;
				job.systemHandle.iterPtr = ur_null;
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
					job->systemHandle.iterPtr = ur_null;
					this->OnJobRemoved();
					break;
				}
			}
		}

		return job;
	}

	void JobSystem::OnJobAdded()
	{
	}

	void JobSystem::OnJobRemoved()
	{
	}

} // end namespace UnlimRealms