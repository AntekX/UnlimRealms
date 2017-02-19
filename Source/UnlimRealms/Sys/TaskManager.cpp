///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Sys/TaskManager.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Task
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	
	Task::Task() :
		terminate(false),
		result(Undefined),
		taskListHandle(ur_null),
		execListHandle(ur_null)
	{

	}

	Task::~Task()
	{

	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// TaskManager
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	TaskManager::TaskManager(Realm &realm) :
		RealmEntity(realm)
	{

	}

	TaskManager::~TaskManager()
	{

	}

	void TaskManager::Schedule(std::shared_ptr<Task> &task, Task::Priority priority)
	{
		if (ur_null == task.get())
			return;

		std::lock_guard<std::mutex> lock(this->taskListMutex);

		if (Task::Priority::High == priority)
		{
			task->taskListHandle = &this->taskList.emplace(this->taskList.begin(), task);
		}
		else
		{
			task->taskListHandle = &this->taskList.emplace(this->taskList.end(), task);
		}
	}

	void TaskManager::Terminate(Task *task, bool waitTermination)
	{
		if (ur_null == task)
			return;

		{
			std::lock_guard<std::mutex> lock(this->taskListMutex);
			if (task->taskListHandle != ur_null)
			{
				// remove idle task
				this->taskList.erase(*task->taskListHandle);
				task->taskListHandle = ur_null;

				// no termination is required, early exit
				return;
			}
		}

		{
			std::lock_guard<std::mutex> lock(this->execListMutex);
			if (task->execListHandle != ur_null)
			{
				// terminate 
				task->Terminate();
				// todo: wait if required

				// remove from execution list
				this->execList.erase(*task->execListHandle);
				task->execListHandle = ur_null;
			}
		}
	}

	void TaskManager::TerminateAll(bool waitTermination)
	{
		{
			std::lock_guard<std::mutex> lock(this->taskListMutex);
			for (auto &task : this->taskList)
			{
				if (task.get() != ur_null)
				{
					task->taskListHandle = ur_null;
				}
			}
			this->taskList.clear();
		}

		{
			std::lock_guard<std::mutex> lock(this->execListMutex);
			for (auto &task : this->execList)
			{
				if (task.get() != ur_null)
				{
					task->Terminate();
					// todo: wait if required
					task->execListHandle = ur_null;
				}
			}
			this->execList.clear();
		}
	}

	std::shared_ptr<Task> TaskManager::FetchTask()
	{
		std::shared_ptr<Task> task(ur_null);

		{
			// fetch from task list
			std::lock_guard<std::mutex> lock(this->taskListMutex);
			if (!this->taskList.empty())
			{
				std::shared_ptr<Task> task = *this->taskList.begin();
				this->taskList.pop_front();
			}
		}

		{
			// add to execution list
			std::lock_guard<std::mutex> lock(this->execListMutex);
			
		}

		return task;
	}

	void TaskManager::Execute()
	{
		this->ExecuteImmediately();
	}

	void TaskManager::ExecuteImmediately()
	{
		// execute scheduled tasks synchronously
		std::lock_guard<std::mutex> lock(this->taskListMutex);
		for (auto &task : this->taskList)
		{
			task->Execute();
			task->taskListHandle = ur_null;
		}
		this->taskList.clear();
	}

} // end namespace UnlimRealms