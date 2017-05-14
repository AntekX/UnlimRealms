///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Resources/Resources.h"
#include "Sys/Log.h"
#include "3rdParty/ResIL/include/IL/il.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ResIL texture loading utilities implementation
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	Result CreateTextureFromFile(Realm &realm, std::unique_ptr<GfxTexture> &resOutput, const std::string &resName)
	{
		if (ur_null == realm.GetGfxSystem())
			return NotInitialized;

		ILconst_string ilResName;
		#ifdef _UNICODE
		// convert name
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> strConverter;
		std::wstring resNameW = strConverter.from_bytes(resName);
		ilResName = resNameW.c_str();
		#else
		ilResName = resName.c_str();
		#endif

		ILuint imageId;
		ilInit();
		ilGenImages(1, &imageId);
		ilBindImage(imageId);
		ilSetInteger(IL_KEEP_DXTC_DATA, IL_TRUE);
		ilSetInteger(IL_DECOMPRESS_DXTC, IL_FALSE);
		if (ilLoadImage(ilResName) == IL_FALSE)
			return LogResult(Failure, realm.GetLog(), Log::Error, "CreateTextureFromFile: failed to load image " + resName);

		ILint ilWidth, ilHeight, ilMips, ilFormat, ilDXTFormat, ilBpp, ilBpc;
		ilGetImageInteger(IL_IMAGE_WIDTH, &ilWidth);
		ilGetImageInteger(IL_IMAGE_HEIGHT, &ilHeight);
		ilGetImageInteger(IL_IMAGE_FORMAT, &ilFormat);
		ilGetImageInteger(IL_IMAGE_BPP, &ilBpp);
		ilGetImageInteger(IL_IMAGE_BPC, &ilBpc);
		ilGetImageInteger(IL_NUM_MIPMAPS, &ilMips);
		ilGetImageInteger(IL_DXTC_DATA_FORMAT, &ilDXTFormat);

		std::vector<GfxResourceData> gfxTexData(ilMips + 1);
		gfxTexData[0].Ptr = (void*)ilGetData();
		gfxTexData[0].RowPitch = (ur_uint)ilWidth * ilBpp * ilBpc;
		gfxTexData[0].SlicePitch = 0;

		GfxTextureDesc gfxTexDesc;
		gfxTexDesc.Width = (ur_uint)ilWidth;
		gfxTexDesc.Height = (ur_uint)ilHeight;
		gfxTexDesc.Levels = (ur_uint)ilMips + 1;
		gfxTexDesc.Format = GfxFormat::R8G8B8A8;
		gfxTexDesc.FormatView = GfxFormatView::Unorm;
		gfxTexDesc.Usage = GfxUsage::Immutable;
		gfxTexDesc.BindFlags = (ur_uint)GfxBindFlag::ShaderResource;
		gfxTexDesc.AccessFlags = 0;
		if (ilDXTFormat != IL_DXT_NO_COMP)
		{
			switch (ilDXTFormat)
			{
			case IL_DXT1: gfxTexDesc.Format = GfxFormat::BC1; break;
			case IL_DXT5: gfxTexDesc.Format = GfxFormat::BC3; break;
			}
			gfxTexData[0].Ptr = (void*)ilGetDXTCData();
		}
		for (ur_uint i = 1; i < gfxTexDesc.Levels; ++i)
		{
			gfxTexData[i].RowPitch = gfxTexData[i - 1].RowPitch / 2;
			gfxTexData[i].Ptr = (void*)(ilDXTFormat != IL_DXT_NO_COMP ? ilGetMipDXTCData(i - 1) : ilGetMipData(i - 1));
		}

		std::unique_ptr<GfxTexture> texture;
		if (Succeeded(realm.GetGfxSystem()->CreateTexture(texture)))
		{
			texture->Initialize(gfxTexDesc, gfxTexData.data());
		}

		ilDeleteImages(1, &imageId);

		resOutput = std::move(texture);

		return Success;
	}

} // end namespace UnlimRealms