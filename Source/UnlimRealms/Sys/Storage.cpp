///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Sys/Storage.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Storage
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Storage::Storage(Realm &realm) :
		RealmEntity(realm)
	{
	}

	Storage::~Storage()
	{
	}

	Result Storage::Initialize()
	{
		return this->OnInitialize();
	}

	Result Storage::OnInitialize()
	{
		return Result(NotImplemented);
	}

	Result Storage::Open(const std::string &name, const ur_uint accessFlags, std::unique_ptr<File> &file)
	{
		file.reset(new File(*this, name));
		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// File
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	File::File(Storage &storage, const std::string &name) :
		storage(storage),
		name(name)
	{
	}

	File::~File()
	{
	}

	Result File::Open(const ur_uint accessFlags)
	{
		return Result(NotImplemented);
	}

	Result File::Close()
	{
		return Result(NotImplemented);
	}

	ur_size File::GetSize()
	{
		return 0;
	}

	Result File::Read(const ur_size size, ur_byte *buffer)
	{
		return Result(NotImplemented);
	}

	Result File::Write(const ur_size size, const ur_byte *buffer)
	{
		return Result(NotImplemented);
	}

	Result File::Read(std::string &text)
	{
		return Result(NotImplemented);
	}

	Result File::Write(const std::string &text)
	{
		return Result(NotImplemented);
	}

} // end namespace UnlimRealms