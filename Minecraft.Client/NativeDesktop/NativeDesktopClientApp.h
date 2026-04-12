#pragma once

#include <cstdlib>
#include <cstring>

class NativeDesktopMinecraftApp : public CMinecraftApp
{
public:
    enum ECommerceState
    {
        eCommerce_State_Offline,
        eCommerce_State_Online,
        eCommerce_State_Error,
        eCommerce_State_Init,
        eCommerce_State_Init_Pending,
        eCommerce_State_GetCategories,
        eCommerce_State_GetCategories_Pending,
        eCommerce_State_GetProductList,
        eCommerce_State_GetProductList_Pending,
        eCommerce_State_AddProductInfoDetailed,
        eCommerce_State_AddProductInfoDetailed_Pending,
        eCommerce_State_RegisterDLC,
        eCommerce_State_Checkout,
        eCommerce_State_Checkout_WaitingForSession,
        eCommerce_State_Checkout_SessionStarted,
        eCommerce_State_Checkout_Pending,
        eCommerce_State_DownloadAlreadyPurchased,
        eCommerce_State_DownloadAlreadyPurchased_WaitingForSession,
        eCommerce_State_DownloadAlreadyPurchased_SessionStarted,
        eCommerce_State_DownloadAlreadyPurchased_Pending,
        eCommerce_State_UpgradeTrial,
        eCommerce_State_UpgradeTrial_WaitingForSession,
        eCommerce_State_UpgradeTrial_SessionStarted,
        eCommerce_State_UpgradeTrial_Pending
    };

    int GetCommerceState() const
    {
        return eCommerce_State_Offline;
    }

    ULONGLONG GetDLCInfoTexturesFullOffer(int iIndex)
    {
        (void)iIndex;
        return 0;
    }

    DLC_INFO* GetDLCInfoForFullOfferID(ULONGLONG ullOfferID)
    {
        (void)ullOfferID;
        return nullptr;
    }

    eDLCContentType GetDLCTypeFromName(char* dlcName)
    {
        if (dlcName == nullptr || std::strlen(dlcName) < 3)
        {
            return e_DLC_NotDefined;
        }

        char dlcType[3];
        dlcType[0] = dlcName[1];
        dlcType[1] = dlcName[2];
        dlcType[2] = '\0';

        if (std::strcmp(dlcType, "SP") == 0)
        {
            return e_DLC_SkinPack;
        }
        if (std::strcmp(dlcType, "GP") == 0)
        {
            return e_DLC_Gamerpics;
        }
        if (std::strcmp(dlcType, "TH") == 0)
        {
            return e_DLC_Themes;
        }
        if (std::strcmp(dlcType, "AV") == 0)
        {
            return e_DLC_AvatarItems;
        }
        if (std::strcmp(dlcType, "MP") == 0)
        {
            return e_DLC_MashupPacks;
        }
        if (std::strcmp(dlcType, "TP") == 0)
        {
            return e_DLC_TexturePacks;
        }

        return e_DLC_NotDefined;
    }

    int GetiConfigFromName(char* name)
    {
        if (name == nullptr)
        {
            return 0;
        }

        const std::size_t length = std::strlen(name);
        if (length < 4)
        {
            return 0;
        }

        char config[5];
        config[0] = name[length - 4];
        config[1] = name[length - 3];
        config[2] = name[length - 2];
        config[3] = name[length - 1];
        config[4] = '\0';
        return std::atoi(config);
    }

    void SetRichPresenceContext(int iPad, int contextId) override
    {
        (void)iPad;
        (void)contextId;
    }

    void CaptureSaveThumbnail() override {}
    void GetSaveThumbnail(PBYTE* data, DWORD* size) override
    {
        if (data != nullptr)
        {
            *data = nullptr;
        }
        if (size != nullptr)
        {
            *size = 0;
        }
    }
    void ReleaseSaveThumbnail() override {}
    void GetScreenshot(int iPad, PBYTE* data, DWORD* size) override
    {
        (void)iPad;
        GetSaveThumbnail(data, size);
    }
    void ReadBannedList(
        int iPad,
        eTMSAction action = static_cast<eTMSAction>(0),
        bool callback = false) override
    {
        (void)iPad;
        (void)action;
        (void)callback;
    }
    int LoadLocalTMSFile(WCHAR* tmsFile) override
    {
        (void)tmsFile;
        return -1;
    }
    int LoadLocalTMSFile(WCHAR* tmsFile, eFileExtensionType extension) override
    {
        (void)extension;
        return LoadLocalTMSFile(tmsFile);
    }
    void FreeLocalTMSFiles(eTMSFileType type) override
    {
        (void)type;
    }
    int GetLocalTMSFileIndex(
        WCHAR* tmsFile,
        bool filenameIncludesExtension,
        eFileExtensionType extension) override
    {
        (void)tmsFile;
        (void)filenameIncludesExtension;
        (void)extension;
        return -1;
    }
};

using CConsoleMinecraftApp = NativeDesktopMinecraftApp;

inline NativeDesktopMinecraftApp app;
