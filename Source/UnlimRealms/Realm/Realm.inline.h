///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	template <class TStorage>
	void Realm::SetStorage(std::unique_ptr<TStorage> storage)
	{
		static_assert(std::is_base_of<Storage, TStorage>(), "Realm::SetStorage: invalid implementation class type");
		this->storage = std::move(storage);
	}

	template <class TLog>
	void Realm::SetLog(std::unique_ptr<TLog> log)
	{
		static_assert(std::is_base_of<Log, TLog>(), "Realm::SetLog: invalid implementation class type");
		this->log = std::move(log);
	}

	template <class TTaskManager>
	void Realm::SetTaskManager(std::unique_ptr<TTaskManager> taskManager)
	{
		static_assert(std::is_base_of<TaskManager, TTaskManager>(), "Realm::SetTaskManager: invalid implementation class type");
		this->taskManager = std::move(taskManager);
	}

	template <class TInput>
	void Realm::SetInput(std::unique_ptr<TInput> input)
	{
		static_assert(std::is_base_of<Input, TInput>(), "Realm::SetInput: invalid implementation class type");
		this->RemoveComponent<Input>();
		this->AddComponent(Component::GetUID<Input>(), std::unique_ptr<Component>(static_cast<Component*>(input.release())));
	}

	template <class TCanvas>
	void Realm::SetCanvas(std::unique_ptr<TCanvas> canvas)
	{
		static_assert(std::is_base_of<Canvas, TCanvas>(), "Realm::SetCanvas: invalid implementation class type");
		this->RemoveComponent<Canvas>();
		this->AddComponent(Component::GetUID<Canvas>(), std::unique_ptr<Component>(static_cast<Component*>(canvas.release())));
	}

	template <class TGfxSystem>
	void Realm::SetGfxSystem(std::unique_ptr<TGfxSystem> gfxSystem)
	{
		static_assert(std::is_base_of<GfxSystem, TGfxSystem>(), "Realm::SetGfxSystem: invalid implementation class type");
		this->RemoveComponent<GfxSystem>();
		this->AddComponent(Component::GetUID<GfxSystem>(), std::unique_ptr<Component>(static_cast<Component*>(gfxSystem.release())));
	}

	inline Realm& RealmEntity::GetRealm()
	{
		return this->realm;
	}

} // end namespace UnlimRealms