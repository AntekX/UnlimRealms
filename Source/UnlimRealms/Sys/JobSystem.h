///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"

namespace UnlimRealms
{

	// forward declarations
	class JobSystem;
	class Job;


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Job System Entity
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL JobSystemEntity
	{
	public:

		friend class JobSystem;

		explicit JobSystemEntity(JobSystem &jobSystem) : jobSystem(jobSystem) {}

	protected:

		JobSystem &jobSystem;

	private:
	
		struct QueueHandle
		{
			void* listPtr;
			void* iterPtr;
			QueueHandle() : listPtr(ur_null), iterPtr(ur_null) {}
		} systemHandle;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Job
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL Job : public JobSystemEntity
	{
	public:

		enum class UR_DECL State
		{
			Pending,
			InProgress,
			Finished
		};

		typedef void* DataPtr;

		struct UR_DECL Context
		{
			DataPtr data;
			ur_bool &interrupt;
			std::atomic<ur_float> &progress;
			std::atomic<Result::UID> &resultCode;

			Context(DataPtr &data,
				ur_bool &interrupt, std::atomic<ur_float> &progress, std::atomic<Result::UID> &resultCode) :
				data(data), interrupt(interrupt), progress(progress), resultCode(resultCode)
			{
			}
		};

		typedef std::function<void(Job::Context&)> Callback;

		Job(JobSystem &jobSystem, DataPtr data, Callback callback);

		~Job();

		void Execute();

		void Interrupt();

		void Wait();

		inline ur_float GetProgress() const;

		inline State GetState() const;

		inline ur_bool InProgress() const;

		inline ur_bool Finished() const;

		inline ur_bool Interrupted() const;
		
		inline Result::UID GetResultCode() const;

		inline ur_bool FinishedSuccessfully() const;

	private:

		Callback callback;
		DataPtr data;
		ur_bool interrupt;
		std::atomic<State> state;
		std::atomic<ur_float> progress;
		std::atomic<Result::UID> resultCode;
	};

	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class JobPriority
	{
		High,
		Normal,
		Low,
		Count
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base Job System
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL JobSystem : public RealmEntity
	{
	public:

		JobSystem(Realm &realm);

		virtual ~JobSystem();

		std::shared_ptr<Job> Add(Job::DataPtr jobData, Job::Callback jobCallback);

		std::shared_ptr<Job> Add(JobPriority priority, Job::DataPtr jobData, Job::Callback jobCallback);

		ur_bool Add(std::shared_ptr<Job> job, JobPriority priority = JobPriority::Normal);

		ur_bool Remove(Job &job);

		void Interrupt(Job &job);

	protected:

		std::shared_ptr<Job> FetchJob();

		virtual void OnJobAdded();

		virtual void OnJobRemoved();

	private:

		struct JobQueue
		{
			std::list<std::shared_ptr<Job>> jobs;
		};
		JobQueue priorityQueue[(ur_int)JobPriority::Count];
		std::mutex queueMutex;
	};

} // end namespace UnlimRealms

#include "Sys/JobSystem.inline.h"