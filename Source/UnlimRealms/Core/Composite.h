///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common.h"
#include "Core/BaseTypes.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base Composite Object Component
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL Component
	{
	public:

		typedef ur_size UID;
		static const UID UndefinedUID = 0;

		Component();

		Component(const Component &v) {}

		Component& operator=(const Component &v) { return *this; }
		
		~Component();

		// run time UID generation
		template <class T>
		static UID GetUID()
		{
			static_assert(std::is_base_of<Component, T>(), "Component::GetUID: class type is not a component");
			static const UID TypeUID = ++lastUID;
			return TypeUID;
		}

	private:

		static UID lastUID;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base Composite Object
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL Composite : public virtual Component
	{
	public:

		Composite();
		
		~Composite();

		/*template <class T>
		bool AddComponent()
		{
			static_assert(std::is_base_of<Component, T>(), "Composite::AddComponent: class type is not a component");
			return this->AddComponent(Component::GetUID<T>(), std::unique_ptr<Component>(new T()));
		}

		bool AddComponent(Component::UID uid, std::unique_ptr<Component> &component)
		{
			if (this->HasComponent(uid))
				return false;
			this->components[uid] = std::move(component);
			return true;
		}

		template <class T>
		bool RemoveComponent()
		{
			static_assert(std::is_base_of<Component, T>(), "Composite::RemoveComponent: class type is not a component");
			return this->RemoveComponent(Component::GetUID<T>());
		}

		bool RemoveComponent(Component::UID uid)
		{
			auto &ientry = this->components.find(uid);
			if (ientry == this->components.end())
				return false;
			this->components.erase(ientry);
			return true;
		}

		template <class T>
		bool HasComponent()
		{
			static_assert(std::is_base_of<Component, T>(), "Composite::HasComponent: class type is not a component");
			return this->HasComponent(Component::GetUID<T>());
		}

		bool HasComponent(Component::UID uid)
		{
			return (this->components.count(uid) > 0);
		}

		template <class T>
		T* GetComponent()
		{
			static_assert(std::is_base_of<Component, T>(), "Composite::GetComponent: class type is not a component");
			return static_cast<T*>(this->GetComponent(Component::GetUID<T>()));
		}

		Component* GetComponent(Component::UID uid)
		{
			auto &ientry = this->components.find(uid);
			if (ientry == this->components.end())
				return ur_null;
			return ientry->second.get();
		}*/

	private:

		/*struct Entry
		{
			Entry() {}
			Entry(const Entry &) {}
			std::unique_ptr<Component> component;
		};*/

		// TODO

		class ComponentUPtr : public std::unique_ptr<Component>
		{
			ComponentUPtr() {}

			ComponentUPtr(const ComponentUPtr &v) {}
		};

		typedef std::unordered_map<Component::UID, ComponentUPtr> ComponentArray;

		//// note: OOD implementation
		//// can be rewriten to DOD if required
		ComponentArray components;
	};

} // end namespace UnlimRealms