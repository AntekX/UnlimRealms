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

	// forward declarations
	class File;


	enum class UR_DECL StorageAccess : ur_byte
	{
		Read = (1 << 0),
		Write = (1 << 1),
		Append = (1 << 2),
		Binary = (1 << 3)
	};

	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base system storage
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL Storage : public RealmEntity
	{
	public:

		Storage(Realm &realm);

		virtual ~Storage();

		Result Initialize();

		virtual Result Open(const std::string &name, const ur_uint accessFlags, std::unique_ptr<File> &file);

	protected:

		virtual Result OnInitialize();
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base system storage file
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL File
	{
	public:

		File(Storage &storage, const std::string &name);

		virtual ~File();

		virtual Result Open(const ur_uint accessFlags);

		virtual Result Close();

		virtual ur_size GetSize();

		virtual Result Read(const ur_size size, ur_byte *buffer);

		virtual Result Write(const ur_size size, const ur_byte *buffer);

		virtual Result Read(std::string &text);

		virtual Result Write(const std::string &text);

		inline Storage& GetStorage();

		inline const std::string& GetName();

	private:

		Storage &storage;
		std::string name;
	};

} // end namespace UnlimRealms

#include "Sys/Storage.inline.h"