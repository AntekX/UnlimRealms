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
	class UR_DECL Realm
	{
	public:

		Realm();

		virtual ~Realm();

		virtual Result Initialize(); 

		void SetStorage(std::unique_ptr<Storage> storage);

		void SetLog(std::unique_ptr<Log> log);

		void SetInput(std::unique_ptr<Input> input);

		void SetCanvas(std::unique_ptr<Canvas> canvas);

		void SetGfxSystem(std::unique_ptr<GfxSystem> gfxSystem);

		inline Storage& GetStorage();

		inline Log& GetLog();

		inline Input* GetInput() const;

		inline Canvas* GetCanvas() const;

		inline GfxSystem* GetGfxSystem() const;
	
	private:

		void CreateDefaultStorage();

		void CreateDefaultLog();

		std::unique_ptr<Storage> storage;
		std::unique_ptr<Log> log;
		std::unique_ptr<Input> input;
		std::unique_ptr<Canvas> canvas;
		std::unique_ptr<GfxSystem> gfxSystem;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Basic realm entity
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL RealmEntity
	{
	public:

		RealmEntity(Realm &realm);
		
		~RealmEntity();

		inline Realm& GetRealm();

	private:

		Realm &realm;
	};

} // end namespace UnlimRealms

#include "Realm/Realm.inline.h"