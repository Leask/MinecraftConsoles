#pragma once

#include <lce_abi/lce_abi.h>

#define MAX_SOCIALPOST_CAPTION 60
#define MAX_SOCIALPOST_DESC 100

enum ESocialNetwork
{
    eFacebook = 0,
    eNumSocialNetworks
};

class CSocialManager
{
private:
    CSocialManager();
    CSocialManager(const CSocialManager&);
    CSocialManager& operator=(const CSocialManager&);

    static CSocialManager* m_pInstance;

public:
    static CSocialManager* Instance();

    void Initialise();
    void Tick();
    bool RefreshPostingCapability();
    bool IsTitleAllowedToPostAnything();
    bool IsTitleAllowedToPostImages();
    bool IsTitleAllowedToPostLinks();
    bool AreAllUsersAllowedToPostImages();
    bool PostLinkToSocialNetwork(
        ESocialNetwork eSocialNetwork,
        DWORD dwUserIndex,
        bool bUsingKinect);
    bool PostImageToSocialNetwork(
        ESocialNetwork eSocialNetwork,
        DWORD dwUserIndex,
        bool bUsingKinect);
    void SetSocialPostText(LPCWSTR title, LPCWSTR caption, LPCWSTR desc);
};
