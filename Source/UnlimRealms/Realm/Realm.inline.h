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

	template <class TJobSystem>
	void Realm::SetJobSystem(std::unique_ptr<TJobSystem> jobSystem)
	{
		static_assert(std::is_base_of<JobSystem, TJobSystem>(), "Realm::SetJobSystem: invalid implementation class type");
		this->jobSystem = std::move(jobSystem);
	}

	template <class TInput>
	void Realm::SetInput(std::unique_ptr<TInput> input)
	{
		static_assert(std::is_base_of<Input, TInput>(), "Realm::SetInput: invalid implementation class type");
		this->RemoveComponent<Input>();
		auto componentPtr = std::unique_ptr<Component>(static_cast<Component*>(input.release()));
		this->AddComponent(Component::GetUID<Input>(), componentPtr);
	}

	template <class TCanvas>
	void Realm::SetCanvas(std::unique_ptr<TCanvas> canvas)
	{
		static_assert(std::is_base_of<Canvas, TCanvas>(), "Realm::SetCanvas: invalid implementation class type");
		this->RemoveComponent<Canvas>();
		auto componentPtr = std::unique_ptr<Component>(static_cast<Component*>(canvas.release()));
		this->AddComponent(Component::GetUID<Canvas>(), componentPtr);
	}

	template <class TGfxSystem>
	void Realm::SetGfxSystem(std::unique_ptr<TGfxSystem> gfxSystem)
	{
		static_assert(std::is_base_of<GfxSystem, TGfxSystem>(), "Realm::SetGfxSystem: invalid implementation class type");
		this->RemoveComponent<GfxSystem>();
		auto componentPtr = std::unique_ptr<Component>(static_cast<Component*>(gfxSystem.release()));
		this->AddComponent(Component::GetUID<GfxSystem>(), componentPtr);
	}

	inline Realm& RealmEntity::GetRealm()
	{
		return this->realm;
	}

} // end namespace UnlimRealms