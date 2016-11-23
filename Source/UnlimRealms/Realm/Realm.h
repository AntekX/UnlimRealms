///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core/Core.h"

namespace UnlimRealms
{
	
	// referenced classes
	class Storage;
	class Input;
	class Log;
	class Canvas;
	class GfxSystem;


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Realm
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL Realm : public Composite
	{
	public:

		Realm();

		virtual ~Realm();

		virtual Result Initialize();

		template <class TStorage>
		void SetStorage(std::unique_ptr<TStorage> storage);

		template <class TLog>
		void SetLog(std::unique_ptr<TLog> log);

		template <class TInput>
		void SetInput(std::unique_ptr<TInput> input);

		template <class TCanvas>
		void SetCanvas(std::unique_ptr<TCanvas> canvas);

		template <class TGfxSystem>
		void SetGfxSystem(std::unique_ptr<TGfxSystem> gfxSystem);

		// base components access shortcuts

		inline Storage& GetStorage();

		inline Log& GetLog();

		inline Input* GetInput() const;

		inline Canvas* GetCanvas() const;

		inline GfxSystem* GetGfxSystem() const;
	
	private:

		void CreateDefaultStorage();

		void CreateDefaultLog();
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Basic realm entity
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL RealmEntity : public Component
	{
	public:

		RealmEntity(Realm &realm);
		
		virtual ~RealmEntity();

		inline Realm& GetRealm();

	private:

		Realm &realm;
	};

} // end namespace UnlimRealms

#include "Realm/Realm.inline.h"