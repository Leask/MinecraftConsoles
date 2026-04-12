#pragma once

#include "../Common/UI/UIController.h"

class ConsoleUIController : public UIController
{
public:
    void init(S32 w = 1280, S32 h = 720)
    {
        std::fprintf(stderr, "NativeDesktop UI: preInit begin\n");
        preInit(w, h);
        std::fprintf(stderr, "NativeDesktop UI: postInit begin\n");
        postInit();
        std::fprintf(stderr, "NativeDesktop UI: postInit ready\n");
    }

    void render() override
    {
        renderScenes();
    }

    void shutdown() {}

    void beginIggyCustomDraw4J(
        IggyCustomDrawCallbackRegion* region,
        CustomDrawData* customDrawRegion) override
    {
        (void)region;
        (void)customDrawRegion;
    }

    CustomDrawData* setupCustomDraw(
        UIScene* scene,
        IggyCustomDrawCallbackRegion* region) override
    {
        CustomDrawData* customDrawRegion = calculateCustomDraw(region);
        setupCustomDrawGameStateAndMatrices(scene, customDrawRegion);
        return customDrawRegion;
    }

    CustomDrawData* calculateCustomDraw(
        IggyCustomDrawCallbackRegion* region) override
    {
        CustomDrawData* customDrawRegion = new CustomDrawData();
        if (region != nullptr)
        {
            customDrawRegion->x0 = region->x0;
            customDrawRegion->x1 = region->x1;
            customDrawRegion->y0 = region->y0;
            customDrawRegion->y1 = region->y1;
        }
        return customDrawRegion;
    }

    void endCustomDraw(IggyCustomDrawCallbackRegion* region) override
    {
        (void)region;
        endCustomDrawGameStateAndMatrices();
    }

    static void handleUnlockFullVersionCallback() {}

    void setTileOrigin(S32 xPos, S32 yPos) override
    {
        (void)xPos;
        (void)yPos;
    }

    GDrawTexture* getSubstitutionTexture(int textureId) override
    {
        (void)textureId;
        return nullptr;
    }

    void destroySubstitutionTexture(
        void* destroyCallBackData,
        GDrawTexture* handle) override
    {
        (void)destroyCallBackData;
        (void)handle;
    }
};

inline ConsoleUIController ui;
