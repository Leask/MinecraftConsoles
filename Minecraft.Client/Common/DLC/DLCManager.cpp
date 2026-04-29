#include "stdafx.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include "DLCManager.h"
#include "DLCPack.h"
#include "DLCFile.h"
#include "../../../Minecraft.World/StringHelpers.h"
#include "../../Minecraft.h"
#include "../../TexturePackRepository.h"
#include "Common/UI/UI.h"

namespace
{
	enum class NativeReadResult
	{
		Ok,
		Missing,
		TooLarge,
		Error,
	};

	unsigned int SwapUInt32(unsigned int value)
	{
		return ((value & 0x000000FF) << 24) |
			((value & 0x0000FF00) << 8) |
			((value & 0x00FF0000) >> 8) |
			((value & 0xFF000000) >> 24);
	}

	unsigned short SwapUInt16(unsigned short value)
	{
		return static_cast<unsigned short>(
			((value & 0x00FF) << 8) |
			((value & 0xFF00) >> 8));
	}

	unsigned int ReadUInt32(const BYTE *ptr, bool swapEndian)
	{
		unsigned int value = 0;
		memcpy(&value, ptr, sizeof(value));
		return swapEndian ? SwapUInt32(value) : value;
	}

	unsigned short ReadUInt16(const BYTE *ptr, bool swapEndian)
	{
		unsigned short value = 0;
		memcpy(&value, ptr, sizeof(value));
		return swapEndian ? SwapUInt16(value) : value;
	}

	wstring ReadDLCUTF16String(
		const BYTE *ptr,
		DWORD charCount,
		bool swapEndian)
	{
		wstring value;
		value.reserve(charCount);
		for(DWORD i = 0; i < charCount; ++i)
		{
			const unsigned short c =
				ReadUInt16(ptr + i * sizeof(unsigned short), swapEndian);
			if(c == 0)
			{
				break;
			}
			value.push_back(static_cast<wchar_t>(c));
		}
		return value;
	}

	DWORD DLCParamRecordBytes(DWORD charCount)
	{
		return sizeof(C4JStorage::DLC_FILE_PARAM) +
			charCount * sizeof(unsigned short);
	}

	DWORD DLCFileDetailsRecordBytes(DWORD charCount)
	{
		return sizeof(C4JStorage::DLC_FILE_DETAILS) +
			charCount * sizeof(unsigned short);
	}

	string NormalizeNativePath(string path)
	{
		std::replace(path.begin(), path.end(), '\\', '/');
		return path;
	}

	NativeReadResult ReadNativeDLCFileBytes(
		const string &path,
		PBYTE *outData,
		DWORD *outLength)
	{
		if(outData == nullptr || outLength == nullptr)
		{
			return NativeReadResult::Error;
		}

		*outData = nullptr;
		*outLength = 0;

		const string nativePath = NormalizeNativePath(path);
		FILE *file = std::fopen(nativePath.c_str(), "rb");
		if(file == nullptr)
		{
			return NativeReadResult::Missing;
		}

		NativeReadResult result = NativeReadResult::Error;
		if(std::fseek(file, 0, SEEK_END) == 0)
		{
			const long fileSize = std::ftell(file);
			if(fileSize >= 0 &&
				static_cast<unsigned long>(fileSize) >
					std::numeric_limits<DWORD>::max())
			{
				result = NativeReadResult::TooLarge;
			}
			else if(fileSize >= 0 && std::fseek(file, 0, SEEK_SET) == 0)
			{
				const DWORD byteCount = static_cast<DWORD>(fileSize);
				PBYTE data = new BYTE[byteCount];
				const std::size_t bytesRead =
					byteCount == 0
						? 0
						: std::fread(data, 1, byteCount, file);
				if(bytesRead == byteCount)
				{
					*outData = data;
					*outLength = byteCount;
					result = NativeReadResult::Ok;
				}
				else
				{
					delete [] data;
				}
			}
		}

		std::fclose(file);
		return result;
	}
}

const WCHAR *DLCManager::wchTypeNamesA[]=
{
	L"XMLVERSION",
	L"DISPLAYNAME",
	L"THEMENAME",
	L"FREE", 
	L"CREDIT",
	L"CAPEPATH",
	L"BOX",
	L"ANIM",
	L"PACKID",
	L"NETHERPARTICLECOLOUR",
	L"ENCHANTTEXTCOLOUR",
	L"ENCHANTTEXTFOCUSCOLOUR",
	L"DATAPATH",
	L"PACKVERSION",
};

DLCManager::DLCManager()
{
	//m_bNeedsUpdated = true;
	m_bNeedsCorruptCheck = true;
}

DLCManager::~DLCManager()
{
	for ( DLCPack *pack : m_packs )
	{
		if ( pack )
			delete pack;
	}
}

DLCManager::EDLCParameterType DLCManager::getParameterType(const wstring &paramName)
{
	EDLCParameterType type = e_DLCParamType_Invalid;

	for(DWORD i = 0; i < e_DLCParamType_Max; ++i)
	{
		if(paramName.compare(wchTypeNamesA[i]) == 0)
		{
			type = static_cast<EDLCParameterType>(i);
			break;
		}
	}

	return type;
}

DWORD DLCManager::getPackCount(EDLCType type /*= e_DLCType_All*/)
{
	DWORD packCount = 0;
	if( type != e_DLCType_All )
	{
		for( DLCPack *pack : m_packs )
		{
			if( pack && pack->getDLCItemsCount(type) > 0 )
			{
				++packCount;
			}
		}
	}
	else
	{
		packCount = static_cast<DWORD>(m_packs.size());
	}
	return packCount;
}

void DLCManager::addPack(DLCPack *pack)
{
	m_packs.push_back(pack);
}

void DLCManager::removePack(DLCPack *pack)
{
	if(pack != nullptr)
	{
		auto it = find(m_packs.begin(), m_packs.end(), pack);
		if(it != m_packs.end() ) m_packs.erase(it);
		delete pack;
	}
}

void DLCManager::removeAllPacks(void)
{
	for( DLCPack *pack : m_packs )
	{
		if ( pack )
			delete pack;
	}

	m_packs.clear();
}

void DLCManager::LanguageChanged(void)
{
	for( DLCPack *pack : m_packs )
	{
		// update the language
		pack->UpdateLanguage();
	}
}

DLCPack *DLCManager::getPack(const wstring &name)
{
	DLCPack *pack = nullptr;
	//DWORD currentIndex = 0;
	for( DLCPack * currentPack : m_packs )
	{
		wstring wsName=currentPack->getName();

		if(wsName.compare(name) == 0)
		{
			pack = currentPack;
			break;
		}
	}
	return pack;
}

#ifdef _XBOX_ONE
DLCPack *DLCManager::getPackFromProductID(const wstring &productID)
{
	DLCPack *pack = nullptr;
	for( DLCPack *currentPack : m_packs )
	{
		wstring wsName=currentPack->getPurchaseOfferId();

		if(wsName.compare(productID) == 0)
		{
			pack = currentPack;
			break;
		}
	}
	return pack;
}
#endif

DLCPack *DLCManager::getPack(DWORD index, EDLCType type /*= e_DLCType_All*/)
{
	DLCPack *pack = nullptr;
	if( type != e_DLCType_All )
	{
		DWORD currentIndex = 0;
		for( DLCPack *currentPack : m_packs )
		{
			if(currentPack->getDLCItemsCount(type)>0)
			{
				if(currentIndex == index)
				{
					pack = currentPack;
					break;
				}
				++currentIndex;
			}
		}
	}
	else
	{
		if(index >= m_packs.size())
		{
			app.DebugPrintf("DLCManager: Trying to access a DLC pack beyond the range of valid packs\n");
			__debugbreak();
		}
		pack = m_packs[index];
	}

	return pack;
}

DWORD DLCManager::getPackIndex(DLCPack *pack, bool &found, EDLCType type /*= e_DLCType_All*/)
{
	DWORD foundIndex = 0;
	found = false;
	if(pack == nullptr)
	{
		app.DebugPrintf("DLCManager: Attempting to find the index for a nullptr pack\n");
		//__debugbreak();
		return foundIndex;
	}
	if( type != e_DLCType_All )
	{
		DWORD index = 0;
		for( DLCPack *thisPack : m_packs )
		{
			if(thisPack->getDLCItemsCount(type)>0)
			{
				if(thisPack == pack)
				{
					found = true;
					foundIndex = index;
					break;
				}
				++index;
			}
		}
	}
	else
	{
		DWORD index = 0;
		for( DLCPack *thisPack : m_packs )
		{
			if(thisPack == pack)
			{
				found = true;
				foundIndex = index;
				break;
			}
			++index;
		}
	}
	return foundIndex;
}

DWORD DLCManager::getPackIndexContainingSkin(const wstring &path, bool &found)
{
	DWORD foundIndex = 0;
	found = false;
	DWORD index = 0;
	for( DLCPack *pack : m_packs )
	{
		if(pack->getDLCItemsCount(e_DLCType_Skin)>0)
		{
			if(pack->doesPackContainSkin(path))
			{
				foundIndex = index;
				found = true;
				break;
			}
			++index;
		}
	}
	return foundIndex;
}

DLCPack *DLCManager::getPackContainingSkin(const wstring &path)
{
	DLCPack *foundPack = nullptr;
	for( DLCPack *pack : m_packs )
	{
		if(pack->getDLCItemsCount(e_DLCType_Skin)>0)
		{
			if(pack->doesPackContainSkin(path))
			{
				foundPack = pack;
				break;
			}
		}
	}
	return foundPack;
}

DLCSkinFile *DLCManager::getSkinFile(const wstring &path)
{
	DLCSkinFile *foundSkinfile = nullptr;
	for( DLCPack *pack : m_packs )
	{
		foundSkinfile=pack->getSkinFile(path);
		if(foundSkinfile!=nullptr)
		{
			break;
		}
	}
	return foundSkinfile;
}

DWORD DLCManager::checkForCorruptDLCAndAlert(bool showMessage /*= true*/)
{
	DWORD corruptDLCCount = m_dwUnnamedCorruptDLCCount;	
	DLCPack *firstCorruptPack = nullptr;

	for( DLCPack *pack : m_packs )
	{
		if( pack->IsCorrupt() )
		{
			++corruptDLCCount;
			if(firstCorruptPack == nullptr) firstCorruptPack = pack;
		}
	}

	if(corruptDLCCount > 0 && showMessage)
	{
		UINT uiIDA[1];
		uiIDA[0]=IDS_CONFIRM_OK;
		if(corruptDLCCount == 1 && firstCorruptPack != nullptr)
		{
			// pass in the pack format string
			WCHAR wchFormat[132];
			swprintf(wchFormat, 132, L"%ls\n\n%%ls", firstCorruptPack->getName().c_str());

			C4JStorage::EMessageResult result = ui.RequestErrorMessage( IDS_CORRUPT_DLC_TITLE, IDS_CORRUPT_DLC, uiIDA,1,ProfileManager.GetPrimaryPad(),nullptr,nullptr,wchFormat);

		}
		else
		{
			C4JStorage::EMessageResult result = ui.RequestErrorMessage( IDS_CORRUPT_DLC_TITLE, IDS_CORRUPT_DLC_MULTIPLE, uiIDA,1,ProfileManager.GetPrimaryPad());
		}
	}

	SetNeedsCorruptCheck(false);

	return corruptDLCCount;
}

bool DLCManager::readDLCDataFile(DWORD &dwFilesProcessed, const wstring &path, DLCPack *pack, bool fromArchive)
{
	return readDLCDataFile( dwFilesProcessed, wstringtofilename(path), pack, fromArchive);
}


bool DLCManager::readDLCDataFile(DWORD &dwFilesProcessed, const string &path, DLCPack *pack, bool fromArchive)
{
	wstring wPath = convStringToWstring(path);
	if (fromArchive && app.getArchiveFileSize(wPath) >= 0)
	{
		byteArray bytes = app.getArchiveFile(wPath);
		return processDLCDataFile(dwFilesProcessed, bytes.data, bytes.length, pack);
	}
	else if (fromArchive) return false;

	PBYTE pbData = nullptr;
	DWORD bytesRead = 0;
	const NativeReadResult readResult =
		ReadNativeDLCFileBytes(path, &pbData, &bytesRead);
	if( readResult == NativeReadResult::Missing )
	{
		app.DebugPrintf("Failed to open DLC data file %s\n", path.c_str());
		if( dwFilesProcessed == 0 ) removePack(pack);
		assert(false);
		return false;
	}
	if( readResult != NativeReadResult::Ok )
	{
		// Corrupt or some other error. In any case treat as corrupt
		app.DebugPrintf("Failed to read %s from DLC content package\n", path.c_str());
		pack->SetIsCorrupt( true );
		SetNeedsCorruptCheck(true);
		return false;
	}
	return processDLCDataFile(dwFilesProcessed, pbData, bytesRead, pack);
}

bool DLCManager::processDLCDataFile(DWORD &dwFilesProcessed, PBYTE pbData, DWORD dwLength, DLCPack *pack)
{
	unordered_map<int, EDLCParameterType> parameterMapping;
	unsigned int uiCurrentByte=0;

	// File format defined in the DLC_Creator
	// File format: Version 2
	// unsigned long, version number
	// unsigned long, t = number of parameter types
	// t * DLC_FILE_PARAM structs mapping strings to id's
	// unsigned long, n = number of files
	// n * DLC_FILE_DETAILS describing each file in the pack
	// n * files of the form
	// // unsigned long, p = number of parameters
	// // p * DLC_FILE_PARAM describing each parameter for this file
	// // ulFileSize bytes of data blob of the file added
	unsigned int uiVersion=ReadUInt32(pbData, false);
	uiCurrentByte+=sizeof(int);

	bool bSwapEndian = false;
	unsigned int uiVersionSwapped = SwapUInt32(uiVersion);
	if(uiVersion <= CURRENT_DLC_VERSION_NUM)
	{
		bSwapEndian = false;
	}
	else if(uiVersionSwapped <= CURRENT_DLC_VERSION_NUM)
	{
		bSwapEndian = true;
	}
	else
	{
		if(pbData!=nullptr) delete [] pbData;
		app.DebugPrintf("Unknown DLC version of %d\n", uiVersion);
		return false;
	}
	pack->SetDataPointer(pbData);
	unsigned int uiParameterCount=ReadUInt32(&pbData[uiCurrentByte], bSwapEndian);
	uiCurrentByte+=sizeof(int);
	C4JStorage::DLC_FILE_PARAM *pParams = (C4JStorage::DLC_FILE_PARAM *)&pbData[uiCurrentByte];
	bool bXMLVersion = false;
	//DWORD dwwchCount=0;
	for(unsigned int i=0;i<uiParameterCount;i++)
	{
		pParams->dwType = bSwapEndian ? SwapUInt32(pParams->dwType) : pParams->dwType;
		pParams->dwWchCount = bSwapEndian ? SwapUInt32(pParams->dwWchCount) : pParams->dwWchCount;

		// Map DLC strings to application strings, then store the DLC index mapping to application index
		wstring parameterName = ReadDLCUTF16String(
			reinterpret_cast<const BYTE *>(pParams->wchData),
			pParams->dwWchCount,
			bSwapEndian);
		EDLCParameterType type = getParameterType(parameterName);
		if( type != e_DLCParamType_Invalid )
		{
			parameterMapping[pParams->dwType] = type;
			if(type == e_DLCParamType_XMLVersion)
			{
				bXMLVersion = true;
			}
		}
		uiCurrentByte += DLCParamRecordBytes(pParams->dwWchCount);
		pParams = (C4JStorage::DLC_FILE_PARAM *)&pbData[uiCurrentByte];
	}
	//ulCurrentByte+=ulParameterCount * sizeof(C4JStorage::DLC_FILE_PARAM);

	if(bXMLVersion)
	{
		uiCurrentByte += sizeof(int);
	}

	unsigned int uiFileCount=ReadUInt32(&pbData[uiCurrentByte], bSwapEndian);
	uiCurrentByte+=sizeof(int);
	C4JStorage::DLC_FILE_DETAILS *pFile = (C4JStorage::DLC_FILE_DETAILS *)&pbData[uiCurrentByte];

	DWORD dwTemp=uiCurrentByte;
	for(unsigned int i=0;i<uiFileCount;i++)
	{
		pFile->dwWchCount = bSwapEndian ? SwapUInt32(pFile->dwWchCount) : pFile->dwWchCount;
		dwTemp += DLCFileDetailsRecordBytes(pFile->dwWchCount);
		pFile = (C4JStorage::DLC_FILE_DETAILS *)&pbData[dwTemp];
	}
	PBYTE pbTemp=((PBYTE )pFile);//+ sizeof(C4JStorage::DLC_FILE_DETAILS)*ulFileCount;
	pFile = (C4JStorage::DLC_FILE_DETAILS *)&pbData[uiCurrentByte];

	for(unsigned int i=0;i<uiFileCount;i++)
	{
		pFile->dwType = bSwapEndian ? SwapUInt32(pFile->dwType) : pFile->dwType;
		pFile->uiFileSize = bSwapEndian ? SwapUInt32(pFile->uiFileSize) : pFile->uiFileSize;
		wstring fileName = ReadDLCUTF16String(
			reinterpret_cast<const BYTE *>(pFile->wchFile),
			pFile->dwWchCount,
			bSwapEndian);

		EDLCType type = static_cast<EDLCType>(pFile->dwType);

		DLCFile *dlcFile = nullptr;
		DLCPack *dlcTexturePack = nullptr;

		if(type == e_DLCType_TexturePack)
		{
			dlcTexturePack = new DLCPack(pack->getName(), pack->getLicenseMask());
		}
		else if(type != e_DLCType_PackConfig)
		{
			dlcFile = pack->addFile(type, fileName);
		}

		// Params
		uiParameterCount=ReadUInt32(pbTemp, bSwapEndian);
		pbTemp+=sizeof(int);
		pParams = (C4JStorage::DLC_FILE_PARAM *)pbTemp;
		for(unsigned int j=0;j<uiParameterCount;j++)
		{
			//DLCManager::EDLCParameterType paramType = DLCManager::e_DLCParamType_Invalid;
			pParams->dwType = bSwapEndian ? SwapUInt32(pParams->dwType) : pParams->dwType;
			pParams->dwWchCount = bSwapEndian ? SwapUInt32(pParams->dwWchCount) : pParams->dwWchCount;
			wstring parameterValue = ReadDLCUTF16String(
				reinterpret_cast<const BYTE *>(pParams->wchData),
				pParams->dwWchCount,
				bSwapEndian);

			auto it = parameterMapping.find(pParams->dwType);

			if(it != parameterMapping.end() )
			{
				if(type == e_DLCType_PackConfig)
				{
					pack->addParameter(it->second, parameterValue);
				}
				else
				{
					if(dlcFile != nullptr) dlcFile->addParameter(it->second, parameterValue);
					else if(dlcTexturePack != nullptr) dlcTexturePack->addParameter(it->second, parameterValue);
				}
			}
			pbTemp += DLCParamRecordBytes(pParams->dwWchCount);
			pParams = (C4JStorage::DLC_FILE_PARAM *)pbTemp;
		}
		//pbTemp+=ulParameterCount * sizeof(C4JStorage::DLC_FILE_PARAM);

		if(dlcTexturePack != nullptr)
		{
			DWORD texturePackFilesProcessed = 0;
			bool validPack = processDLCDataFile(texturePackFilesProcessed,pbTemp,pFile->uiFileSize,dlcTexturePack);
			pack->SetDataPointer(nullptr); // If it's a child pack, it doesn't own the data
			if(!validPack || texturePackFilesProcessed == 0)
			{
				delete dlcTexturePack;
				dlcTexturePack = nullptr;
			}
			else
			{
				pack->addChildPack(dlcTexturePack);

				if(dlcTexturePack->getDLCItemsCount(e_DLCType_Texture) > 0)
				{
					Minecraft::GetInstance()->skins->addTexturePackFromDLC(dlcTexturePack, dlcTexturePack->GetPackId() );
				}
			}
			++dwFilesProcessed;
		}
		else if(dlcFile != nullptr)
		{
			// Data
			dlcFile->addData(pbTemp,pFile->uiFileSize);

			// TODO - 4J Stu Remove the need for this vSkinNames vector, or manage it differently
			switch(pFile->dwType)
			{
			case e_DLCType_Skin:
				app.vSkinNames.push_back(fileName);
				break;
			}

			++dwFilesProcessed;
		}

		// Move the pointer to the start of the next files data;
		pbTemp+=pFile->uiFileSize;
		uiCurrentByte += DLCFileDetailsRecordBytes(pFile->dwWchCount);

		pFile=(C4JStorage::DLC_FILE_DETAILS *)&pbData[uiCurrentByte];
	}

	if( pack->getDLCItemsCount(e_DLCType_GameRules) > 0
		|| pack->getDLCItemsCount(e_DLCType_GameRulesHeader) > 0)
	{
		app.m_gameRules.loadGameRules(pack);
	}

	if(pack->getDLCItemsCount(e_DLCType_Audio) > 0)
	{
		//app.m_Audio.loadAudioDetails(pack);
	}
	// TODO Should be able to delete this data, but we can't yet due to how it is added to the Memory textures (MEM_file)

	return true;
}

DWORD DLCManager::retrievePackIDFromDLCDataFile(const string &path, DLCPack *pack)
{
	DWORD packId = 0;

	PBYTE pbData = nullptr;
	DWORD bytesRead = 0;
	const NativeReadResult readResult =
		ReadNativeDLCFileBytes(path, &pbData, &bytesRead);
	if( readResult == NativeReadResult::Missing )
	{
		return 0;
	}
	if( readResult != NativeReadResult::Ok )
	{
		// Corrupt or some other error. In any case treat as corrupt
		app.DebugPrintf("Failed to read %s from DLC content package\n", path.c_str());
		return 0;
	}
	packId=retrievePackID(pbData, bytesRead, pack);
	delete [] pbData;

	return packId;
}

DWORD DLCManager::retrievePackID(PBYTE pbData, DWORD dwLength, DLCPack *pack)
{
	DWORD packId=0;
	bool bPackIDSet=false;
	unordered_map<int, EDLCParameterType> parameterMapping;
	unsigned int uiCurrentByte=0;

	// File format defined in the DLC_Creator
	// File format: Version 2
	// unsigned long, version number
	// unsigned long, t = number of parameter types
	// t * DLC_FILE_PARAM structs mapping strings to id's
	// unsigned long, n = number of files
	// n * DLC_FILE_DETAILS describing each file in the pack
	// n * files of the form
	// // unsigned long, p = number of parameters
	// // p * DLC_FILE_PARAM describing each parameter for this file
	// // ulFileSize bytes of data blob of the file added
	unsigned int uiVersion=ReadUInt32(pbData, false);
	uiCurrentByte+=sizeof(int);

	bool bSwapEndian = false;
	unsigned int uiVersionSwapped = SwapUInt32(uiVersion);
	if(uiVersion <= CURRENT_DLC_VERSION_NUM)
	{
		bSwapEndian = false;
	}
	else if(uiVersionSwapped <= CURRENT_DLC_VERSION_NUM)
	{
		bSwapEndian = true;
	}
	else
	{
		app.DebugPrintf("Unknown DLC version of %d\n", uiVersion);
		return 0;
	}
	pack->SetDataPointer(pbData);
	unsigned int uiParameterCount=ReadUInt32(&pbData[uiCurrentByte], bSwapEndian);
	uiCurrentByte+=sizeof(int);
	C4JStorage::DLC_FILE_PARAM *pParams = (C4JStorage::DLC_FILE_PARAM *)&pbData[uiCurrentByte];
	bool bXMLVersion = false;
	for(unsigned int i=0;i<uiParameterCount;i++)
	{
		pParams->dwType = bSwapEndian ? SwapUInt32(pParams->dwType) : pParams->dwType;
		pParams->dwWchCount = bSwapEndian ? SwapUInt32(pParams->dwWchCount) : pParams->dwWchCount;

		// Map DLC strings to application strings, then store the DLC index mapping to application index
		wstring parameterName = ReadDLCUTF16String(
			reinterpret_cast<const BYTE *>(pParams->wchData),
			pParams->dwWchCount,
			bSwapEndian);
		EDLCParameterType type = getParameterType(parameterName);
		if( type != e_DLCParamType_Invalid )
		{
			parameterMapping[pParams->dwType] = type;
			if(type == e_DLCParamType_XMLVersion)
			{
				bXMLVersion = true;
			}
		}
		uiCurrentByte += DLCParamRecordBytes(pParams->dwWchCount);
		pParams = (C4JStorage::DLC_FILE_PARAM *)&pbData[uiCurrentByte];
	}

	if(bXMLVersion)
	{
		uiCurrentByte += sizeof(int);
	}

	unsigned int uiFileCount=ReadUInt32(&pbData[uiCurrentByte], bSwapEndian);
	uiCurrentByte+=sizeof(int);
	C4JStorage::DLC_FILE_DETAILS *pFile = (C4JStorage::DLC_FILE_DETAILS *)&pbData[uiCurrentByte];

	DWORD dwTemp=uiCurrentByte;
	for(unsigned int i=0;i<uiFileCount;i++)
	{
		pFile->dwWchCount = bSwapEndian ? SwapUInt32(pFile->dwWchCount) : pFile->dwWchCount;
		dwTemp += DLCFileDetailsRecordBytes(pFile->dwWchCount);
		pFile = (C4JStorage::DLC_FILE_DETAILS *)&pbData[dwTemp];
	}
	PBYTE pbTemp=((PBYTE )pFile);
	pFile = (C4JStorage::DLC_FILE_DETAILS *)&pbData[uiCurrentByte];

	for(unsigned int i=0;i<uiFileCount;i++)
	{
		pFile->dwType = bSwapEndian ? SwapUInt32(pFile->dwType) : pFile->dwType;
		pFile->uiFileSize = bSwapEndian ? SwapUInt32(pFile->uiFileSize) : pFile->uiFileSize;

		EDLCType type = static_cast<EDLCType>(pFile->dwType);

		// Params
		uiParameterCount=ReadUInt32(pbTemp, bSwapEndian);
		pbTemp+=sizeof(int);
		pParams = (C4JStorage::DLC_FILE_PARAM *)pbTemp;
		for(unsigned int j=0;j<uiParameterCount;j++)
		{
			pParams->dwType = bSwapEndian ? SwapUInt32(pParams->dwType) : pParams->dwType;
			pParams->dwWchCount = bSwapEndian ? SwapUInt32(pParams->dwWchCount) : pParams->dwWchCount;
			wstring parameterValue = ReadDLCUTF16String(
				reinterpret_cast<const BYTE *>(pParams->wchData),
				pParams->dwWchCount,
				bSwapEndian);

			auto it = parameterMapping.find(pParams->dwType);

			if(it != parameterMapping.end() )
			{
				if(type==e_DLCType_PackConfig)
				{
					if(it->second==e_DLCParamType_PackId)
					{				
						std::wstringstream ss;
						// 4J Stu - numbered using decimal to make it easier for artists/people to number manually
						ss << std::dec << parameterValue.c_str();
						ss >> packId;
						bPackIDSet=true;
						break;
					}
				}
			}
			pbTemp += DLCParamRecordBytes(pParams->dwWchCount);
			pParams = (C4JStorage::DLC_FILE_PARAM *)pbTemp;
		}

		if(bPackIDSet) break;
		// Move the pointer to the start of the next files data;
		pbTemp+=pFile->uiFileSize;
		uiCurrentByte += DLCFileDetailsRecordBytes(pFile->dwWchCount);

		pFile=(C4JStorage::DLC_FILE_DETAILS *)&pbData[uiCurrentByte];
	}

	parameterMapping.clear();
	return packId;
}
