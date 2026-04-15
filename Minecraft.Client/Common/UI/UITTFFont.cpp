#include "stdafx.h"
#include "UI.h"
#include "../../../Minecraft.World/StringHelpers.h"
#include "../../../Minecraft.World/File.h"
#include "UITTFFont.h"

UITTFFont::UITTFFont(const string &name, const string &path, S32 fallbackCharacter) 
	: m_strFontName(name)
	, pbData(nullptr)
{
	app.DebugPrintf("UITTFFont opening %s\n",path.c_str());

	std::vector<BYTE> fontBytes;
	if(!NativeDesktopReadFileBytes(path.c_str(), &fontBytes))
	{
		app.DebugPrintf("Failed to open TTF file: %s\n", path.c_str());
		assert(false);
		app.FatalLoadError();
		return;
	}

	if(!fontBytes.empty())
	{
		pbData =  (PBYTE) new BYTE[fontBytes.size()];
		std::memcpy(pbData, fontBytes.data(), fontBytes.size());

		IggyFontInstallTruetypeUTF8 ( (void *)pbData, IGGY_TTC_INDEX_none, m_strFontName.c_str(), -1, IGGY_FONTFLAG_none );

		IggyFontInstallTruetypeFallbackCodepointUTF8( m_strFontName.c_str(), -1, IGGY_FONTFLAG_none, fallbackCharacter );

		// 4J Stu - These are so we can use the default flash controls
		IggyFontInstallTruetypeUTF8 ( (void *)pbData, IGGY_TTC_INDEX_none, "Times New Roman", -1, IGGY_FONTFLAG_none );
		IggyFontInstallTruetypeUTF8 ( (void *)pbData, IGGY_TTC_INDEX_none, "Arial", -1, IGGY_FONTFLAG_none );
	}
}

UITTFFont::~UITTFFont()
{
}


string UITTFFont::getFontName()
{
	return m_strFontName;
}
