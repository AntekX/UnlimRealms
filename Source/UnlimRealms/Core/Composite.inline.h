///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Component
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <class T>
	Component::UID Component::GetUID()
	{
		static_assert(std::is_base_of<Component, T>(), "Component::GetUID: class type is not a component");
		static const UID TypeUID = ++lastUID;
		return TypeUID;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Composite
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <class T>
	bool Composite::AddComponent()
	{
		static_assert(std::is_base_of<Component, T>(), "Composite::AddComponent: class type is not a component");
		return this->AddComponent(Component::GetUID<T>(), std::unique_ptr<Component>(new T()));
	}

	bool Composite::AddComponent(Component::UID uid, std::unique_ptr<Component> &component)
	{
		if (this->HasComponent(uid))
			return false;
		this->components[uid] = std::move(component);
		return true;
	}

	template <class T>
	bool Composite::RemoveComponent()
	{
		static_assert(std::is_base_of<Component, T>(), "Composite::RemoveComponent: class type is not a component");
		return this->RemoveComponent(Component::GetUID<T>());
	}

	bool Composite::RemoveComponent(Component::UID uid)
	{
		auto &ientry = this->components.find(uid);
		if (ientry == this->components.end())
			return false;
		this->components.erase(ientry);
		return true;
	}

	template <class T>
	bool Composite::HasComponent()
	{
		static_assert(std::is_base_of<Component, T>(), "Composite::HasComponent: class type is not a component");
		return this->HasComponent(Component::GetUID<T>());
	}

	bool Composite::HasComponent(Component::UID uid)
	{
		return (this->components.count(uid) > 0);
	}

	template <class T>
	T* Composite::GetComponent()
	{
		static_assert(std::is_base_of<Component, T>(), "Composite::GetComponent: class type is not a component");
		return static_cast<T*>(this->GetComponent(Component::GetUID<T>()));
	}

	Component* Composite::GetComponent(Component::UID uid)
	{
		auto &ientry = this->components.find(uid);
		if (ientry == this->components.end())
			return ur_null;
		return ientry->second.get();
	}

} // end namespace UnlimRealms
