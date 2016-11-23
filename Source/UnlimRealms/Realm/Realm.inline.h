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
		this->RemoveComponent<Storage>();
		this->AddComponent<Storage, TStorage>(*this);
	}

	template <class TLog>
	void Realm::SetLog(std::unique_ptr<TLog> log)
	{
		static_assert(std::is_base_of<Log, TLog>(), "Realm::SetLog: invalid implementation class type");
		this->RemoveComponent<Log>();
		this->AddComponent<Log, TLog>(*this);
	}

	template <class TInput>
	void Realm::SetInput(std::unique_ptr<TInput> input)
	{
		static_assert(std::is_base_of<Input, TInput>(), "Realm::SetInput: invalid implementation class type");
		this->RemoveComponent<Input>();
		this->AddComponent<Input, TInput>(*this);
	}

	template <class TCanvas>
	void Realm::SetCanvas(std::unique_ptr<TCanvas> canvas)
	{
		static_assert(std::is_base_of<Canvas, TCanvas>(), "Realm::SetCanvas: invalid implementation class type");
		this->RemoveComponent<Canvas>();
		this->AddComponent<Canvas, TCanvas>(*this);
	}

	template <class TGfxSystem>
	void Realm::SetGfxSystem(std::unique_ptr<TGfxSystem> gfxSystem)
	{
		static_assert(std::is_base_of<GfxSystem, TGfxSystem>(), "Realm::SetGfxSystem: invalid implementation class type");
		this->RemoveComponent<GfxSystem>();
		this->AddComponent<GfxSystem, TGfxSystem>(*this);
	}

	inline Storage& Realm::GetStorage()
	{
		return *this->GetComponent<Storage>();
	}

	inline Log& Realm::GetLog()
	{
		return *this->GetComponent<Log>();
	}

	inline Input* Realm::GetInput() const
	{
		return this->GetComponent<Input>();
	}

	inline Canvas* Realm::GetCanvas() const
	{
		return this->GetComponent<Canvas>();
	}

	inline GfxSystem* Realm::GetGfxSystem() const
	{
		return this->GetComponent<GfxSystem>();
	}

	inline Realm& RealmEntity::GetRealm()
	{
		return this->realm;
	}

} // end namespace UnlimRealms