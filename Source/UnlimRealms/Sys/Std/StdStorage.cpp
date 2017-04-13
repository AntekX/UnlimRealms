///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Sys/Std/StdStorage.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// StdStorage
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	StdStorage::StdStorage(Realm &realm) :
		Storage(realm)
	{
	}

	StdStorage::~StdStorage()
	{
	}

	Result StdStorage::OnInitialize()
	{
		// nothing to initialize
		return Result(Success);
	}

	Result StdStorage::Open(std::unique_ptr<File> &file, const std::string &name, const ur_uint accessFlags)
	{
		std::unique_ptr<StdFile> stdFile(new StdFile(*this, name));
		Result result = stdFile->Open(accessFlags);
		if (Succeeded(result))
		{
			file = std::move(stdFile);
		}
		return result;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// StdFile
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	StdFile::StdFile(Storage &storage, const std::string &name) :
		File(storage, name)
	{
	}

	StdFile::~StdFile()
	{
		if (this->stream != ur_null)
		{
			this->Close();
		}
	}

	Result StdFile::Open(const ur_uint accessFlags)
	{
		unsigned int mode = 0;
		if (accessFlags & ur_uint(StorageAccess::Read)) mode |= std::fstream::in;
		if (accessFlags & ur_uint(StorageAccess::Write)) mode |= std::fstream::out | std::fstream::trunc;
		if (accessFlags & ur_uint(StorageAccess::Append)) mode = (mode ^ std::fstream::trunc) | std::fstream::out;
		if (accessFlags & ur_uint(StorageAccess::Binary)) mode |= std::fstream::binary;

		std::unique_ptr<std::fstream> newStream(new std::fstream());
		newStream->open(this->GetName(), mode);
		if (newStream->fail())
			return Result(Failure);
		
		this->stream = std::move(newStream);

		return Result(Success);
	}

	Result StdFile::Close()
	{
		if (ur_null == this->stream)
			return Result(NotInitialized);

		this->stream->close();
		this->stream.reset(ur_null);

		return Result(Success);
	}

	ur_size StdFile::GetSize()
	{
		if (ur_null == this->stream)
			return 0;

		std::streampos initialPos = this->stream->tellg();
		this->stream->seekg(0, std::fstream::end);
		ur_size size = this->stream->tellg();
		this->stream->seekg(initialPos);

		return (ur_size)size;
	}

	Result StdFile::Read(const ur_size size, ur_byte *buffer)
	{
		if (ur_null == this->stream)
			return Result(NotInitialized);

		this->stream->read((char*)buffer, size);
		if (this->stream->fail())
			return Result(Failure);

		return Result(Success);
	}

	Result StdFile::Write(const ur_size size, const ur_byte *buffer)
	{
		if (ur_null == this->stream)
			return Result(NotInitialized);
		
		this->stream->write((const char*)buffer, size);
		if (this->stream->fail())
			return Result(Failure);

		return Result(Success);
	}

	Result StdFile::Read(std::string &text)
	{
		if (ur_null == this->stream)
			return Result(NotInitialized);

		*this->stream >> text;
		if (this->stream->fail())
			return Result(Failure);

		return Result(Success);
	}

	Result StdFile::Write(const std::string &text)
	{
		if (ur_null == this->stream)
			return Result(NotInitialized);

		*this->stream << text;
		if (this->stream->fail())
			return Result(Failure);

		return Result(Success);
	}

} // end namespace UnlimRealms