///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base system log
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class UR_DECL Log : public RealmEntity
	{
	public:

		enum Priority
		{
			Error,
			Warning,
			Note
		};

		// todo: use Storage reference instead
		typedef std::iostream Stream;
		typedef std::fstream FileStream;

		Log(Realm &realm);

		Log(Realm &realm, const std::string &outputFile);

		Log(Realm &realm, Stream *stream);

		~Log();

		void SetPriority(Priority priority);

		void Write(const std::string &text, Priority priority = Note);

		void WriteLine(const std::string &text, Priority priority = Note);

	private:

		Priority priorityLevel;
		Stream *stream;
		std::unique_ptr<FileStream> fileStream;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Extended operation result, supports auto logging
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	struct UR_DECL LogResult : public Result
	{
	public:

		LogResult();

		LogResult(UID code);

		LogResult(UID code, Log &log, Log::Priority priority, const std::string &msg);
	};


	// helper shortcuts
	#define Succeeded(result) (Success == (result).Code)
	#define Failed(result) (Success != (result).Code)
	#define LogWrite(text, priority) GetRealm().GetLog().WriteLine(text, priority)
	#define LogError(text) GetRealm().GetLog().WriteLine(text, Log::Error)
	#define LogWarning(text) GetRealm().GetLog().WriteLine(text, Log::Warning)
	#define LogNote(text) GetRealm().GetLog().WriteLine(text, Log::Note)
	#define ResultError(code, msg) LogResult(code, GetRealm().GetLog(), Log::Error, msg)
	#define ResultWarning(code, msg) LogResult(code, GetRealm().GetLog(), Log::Warning, msg)
	#define ResultNote(code, msg) LogResult(code, GetRealm().GetLog(), Log::Note, msg)

} // end namespace UnlimRealms