#include "stdafx.h"
#include "CompassItem.h"
#if !defined(_NATIVE_DESKTOP)
#include "..\Minecraft.Client\Minecraft.h"
#include "..\Minecraft.Client\MultiPlayerLocalPlayer.h"
#endif
#include "net.minecraft.world.h"

#ifdef __PSVITA__
const wstring CompassItem::TEXTURE_PLAYER_ICON[XUSER_MAX_COUNT] = {L"compassP0"};
#else
const wstring CompassItem::TEXTURE_PLAYER_ICON[XUSER_MAX_COUNT] = {L"compassP0",L"compassP1",L"compassP2",L"compassP3"};
#endif

CompassItem::CompassItem(int id) : Item(id)
{
	icons = nullptr;
}

// 4J Added so that we can override the icon id used to calculate the texture UV's for each player

Icon *CompassItem::getIcon(int auxValue)
{
	Icon *icon = Item::getIcon(auxValue);
#if defined(_NATIVE_DESKTOP)
	if (icons != nullptr && auxValue == 0)
	{
		icon = icons[0];
	}
#else
	Minecraft *pMinecraft = Minecraft::GetInstance();

	if( pMinecraft->player != nullptr && auxValue == 0 )
	{
		icon = icons[pMinecraft->player->GetXboxPad()];
	}
#endif
	return icon;
}

void CompassItem::registerIcons(IconRegister *iconRegister)
{
	Item::registerIcons(iconRegister);

	icons = new Icon *[XUSER_MAX_COUNT];

	for (int i = 0; i < XUSER_MAX_COUNT; i++)
	{
		icons[i] = iconRegister->registerIcon(TEXTURE_PLAYER_ICON[i]);
	}
}
