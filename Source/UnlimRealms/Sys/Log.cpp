///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Sys/Log.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Log
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Log::Log(Realm &realm) :
		RealmEntity(realm),
		stream(ur_null),
		fileStream(ur_null)
	{
	}

	Log::Log(Realm &realm, const std::string &outputFile) :
		RealmEntity(realm),
		stream(ur_null),
		fileStream(ur_null)
	{
		this->fileStream.reset(new std::fstream());
		this->fileStream->open(outputFile.c_str(), std::fstream::in | std::fstream::out | std::fstream::trunc);
		if (!(this->fileStream->flags() & std::ios::failbit))
		{
			this->stream = this->fileStream.get();
		}
	}

	Log::Log(Realm &realm, Stream *stream) :
		RealmEntity(realm),
		stream(stream),
		fileStream(ur_null)
	{
	}

	Log::~Log()
	{
		if (this->fileStream != ur_null)
		{
			this->fileStream->close();
		}
	}

	void Log::SetPriority(Priority priority)
	{
		this->priorityLevel = priority;
	}

	void Log::Write(const std::string &text, Priority priority)
	{
		if (ur_null == this->stream ||
			priority > this->priorityLevel)
			return;

		*this->stream << text;
	}

	void Log::WriteLine(const std::string &text, Priority priority)
	{
		if (ur_null == this->stream ||
			priority > this->priorityLevel)
			return;

		*this->stream << text << "\n";

		#ifdef _WINDOWS
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> strConverter;
		std::wstring textW = strConverter.from_bytes(text);
		OutputDebugString(textW.c_str());
		#endif
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LogResult
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	LogResult::LogResult()
	{
	}

	LogResult::LogResult(UID code) :
		Result(code)
	{

	}

	LogResult::LogResult(UID code, Log &log, Log::Priority priority, const std::string &msg) :
		Result(code)
	{
		log.WriteLine(msg, priority);
	}


} // end namespace UnlimRealms