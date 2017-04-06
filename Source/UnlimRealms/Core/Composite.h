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
	// Non copyable base class
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL NonCopyable
	{
	public:

		NonCopyable() {}

	private:

		NonCopyable(const NonCopyable&) {}

		NonCopyable& operator = (NonCopyable&) { return *this; }
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base Composite Object Component
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL Component
	{
	public:

		typedef ur_size UID;

		static const UID UndefinedUID = 0;

		Component();

		virtual ~Component();

		// run time UID generation
		template <class T>
		inline static UID GetUID();

	private:

		static UID lastUID;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base Composite Object
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL Composite : public NonCopyable, public Component
	{
	public:

		Composite();

		~Composite();

		template <class T, class ... Args>
		inline bool AddComponent(Args&&... args);
		
		template <class TBase, class TImpl, class ... Args>
		inline bool AddComponent(Args&&... args);
		
		inline bool AddComponent(Component::UID uid, std::unique_ptr<Component> &component);

		template <class T>
		inline bool RemoveComponent();
		
		inline bool RemoveComponent(Component::UID uid);

		inline void RemoveComponents();

		template <class T>
		inline bool HasComponent() const;
		
		inline bool HasComponent(Component::UID uid) const;

		template <class T>
		inline T* GetComponent() const;
		
		inline Component* GetComponent(Component::UID uid) const;

	private:

		typedef std::unordered_map<Component::UID, std::unique_ptr<Component>> ComponentArray;

		ComponentArray components;
	};

} // end namespace UnlimRealms

#include "Composite.inline.h"