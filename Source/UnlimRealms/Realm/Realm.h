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
	class JobSystem;
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

		virtual Result Deinitialize();

		template <class TStorage>
		void SetStorage(std::unique_ptr<TStorage> storage);

		template <class TLog>
		void SetLog(std::unique_ptr<TLog> log);

		template <class TInput>
		void SetInput(std::unique_ptr<TInput> input);

		template <class TJobSystem>
		void SetJobSystem(std::unique_ptr<TJobSystem> jobSystem);

		template <class TCanvas>
		void SetCanvas(std::unique_ptr<TCanvas> canvas);

		template <class TGfxSystem>
		void SetGfxSystem(std::unique_ptr<TGfxSystem> gfxSystem);

		// base components access shortcuts

		inline Storage& GetStorage();

		inline Log& GetLog();

		inline JobSystem& GetJobSystem();

		inline Input* GetInput() const;

		inline Canvas* GetCanvas() const;

		inline GfxSystem* GetGfxSystem() const;
	
	protected:

		virtual std::unique_ptr<Storage> CreateDefaultStorage();

		virtual std::unique_ptr<Log> CreateDefaultLog();

		virtual std::unique_ptr<JobSystem> CreateDefaultJobSystem();

	private:

		std::unique_ptr<Log> log;
		std::unique_ptr<Storage> storage;
		std::unique_ptr<JobSystem> jobSystem;
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