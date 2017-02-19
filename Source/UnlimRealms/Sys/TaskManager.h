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


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base task class
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL Task
	{
	public:

		enum class Priority
		{
			Normal,
			High
		};

		Task();

		virtual ~Task();

		virtual void Execute() = 0;

		inline void Terminate();

		inline const Result& GetResult() const;

	public:

		bool terminate;
		Result result;

	private:

		friend class TaskManager;
		std::list<std::shared_ptr<Task>>::iterator *taskListHandle;
		std::list<std::shared_ptr<Task>>::iterator *execListHandle;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base task manager
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL TaskManager : public RealmEntity
	{
	public:

		TaskManager(Realm &realm);

		virtual ~TaskManager();

		void Schedule(std::shared_ptr<Task> &task, Task::Priority priority = Task::Priority::Normal);

		void Terminate(Task *task, bool waitTermination = false);

		void TerminateAll(bool waitTermination = false);

		virtual void Execute();

		void ExecuteImmediately();

	protected:

		std::shared_ptr<Task> FetchTask();

	private:

		std::mutex taskListMutex;
		std::list<std::shared_ptr<Task>> taskList;
		std::mutex execListMutex;
		std::list<std::shared_ptr<Task>> execList;
	};

} // end namespace UnlimRealms

#include "Sys/TaskManager.inline.h"