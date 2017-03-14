///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Resources/Resources.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Utilities
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	Result CreateVertexShaderFromFile(Realm &realm, std::unique_ptr<GfxVertexShader> &resOutput, const std::string &resName)
	{
		if (ur_null == realm.GetGfxSystem())
			return NotInitialized;

		std::unique_ptr<File> file;
		Result res = realm.GetStorage().Open(resName, ur_uint(StorageAccess::Read) | ur_uint(StorageAccess::Binary), file);
		if (Succeeded(res))
		{
			ur_size sizeInBytes = file->GetSize();
			std::unique_ptr<ur_byte[]> bytecode(new ur_byte[sizeInBytes]);
			file->Read(sizeInBytes, bytecode.get());
			res = realm.GetGfxSystem()->CreateVertexShader(resOutput);
			if (Succeeded(res))
			{
				res = resOutput->Initialize(std::move(bytecode), sizeInBytes);
			}
		}

		return res;
	}

	Result CreatePixelShaderFromFile(Realm &realm, std::unique_ptr<GfxPixelShader> &resOutput, const std::string &resName)
	{
		if (ur_null == realm.GetGfxSystem())
			return NotInitialized;

		std::unique_ptr<File> file;
		Result res = realm.GetStorage().Open(resName, ur_uint(StorageAccess::Read) | ur_uint(StorageAccess::Binary), file);
		if (Succeeded(res))
		{
			ur_size sizeInBytes = file->GetSize();
			std::unique_ptr<ur_byte[]> bytecode(new ur_byte[sizeInBytes]);
			file->Read(sizeInBytes, bytecode.get());
			res = realm.GetGfxSystem()->CreatePixelShader(resOutput);
			if (Succeeded(res))
			{
				res = resOutput->Initialize(std::move(bytecode), sizeInBytes);
			}
		}

		return res;
	}

} // end namespace UnlimRealms