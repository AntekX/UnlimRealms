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
		static const UID TypeUID = typeid(T).hash_code();//++lastUID;
		return TypeUID;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Composite
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <class T, class ... Args>
	bool Composite::AddComponent(Args&&... args)
	{
		static_assert(std::is_base_of<Component, T>(), "Composite::AddComponent: class type is not a component");
		return this->AddComponent(Component::GetUID<T>(), std::unique_ptr<Component>(new T(args...)));
	}

	template <class TBase, class TImpl, class ... Args>
	inline bool Composite::AddComponent(Args&&... args)
	{
		static_assert(std::is_base_of<Component, TBase>(), "Composite::AddComponent: TBase is not a component");
		static_assert(std::is_base_of<TBase, TImpl>(), "Composite::AddComponent: TBase is not a base type for TImpl");
		return this->AddComponent(Component::GetUID<TBase>(), std::unique_ptr<Component>(new TImpl(args...)));
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

	void Composite::RemoveComponents()
	{
		this->components.clear();
	}

	template <class T>
	bool Composite::HasComponent() const
	{
		static_assert(std::is_base_of<Component, T>(), "Composite::HasComponent: class type is not a component");
		return this->HasComponent(Component::GetUID<T>());
	}

	bool Composite::HasComponent(Component::UID uid) const
	{
		return (this->components.count(uid) > 0);
	}

	template <class T>
	T* Composite::GetComponent() const
	{
		static_assert(std::is_base_of<Component, T>(), "Composite::GetComponent: class type is not a component");
		return static_cast<T*>(this->GetComponent(Component::GetUID<T>()));
	}

	Component* Composite::GetComponent(Component::UID uid) const
	{
		auto &ientry = this->components.find(uid);
		if (ientry == this->components.end())
			return ur_null;
		return static_cast<Component*>(ientry->second.get());
	}

} // end namespace UnlimRealms
