#include "stdafx.h"
#include "net.minecraft.world.entity.player.h"
#if !defined(_NATIVE_DESKTOP)
#include "../Minecraft.Client/ServerPlayer.h"
#include "../Minecraft.Client/PlayerConnection.h"
#include <qnet.h>
#endif
#include "PacketListener.h"
#include "InputOutputStream.h"
#include "PlayerInfoPacket.h"



PlayerInfoPacket::PlayerInfoPacket()
{
	m_networkSmallId = 0;
	m_playerColourIndex = -1;
	m_playerPrivileges = 0;
	m_entityId = -1;
}

PlayerInfoPacket::PlayerInfoPacket(BYTE networkSmallId, short playerColourIndex, unsigned int playerPrivileges)
{
	m_networkSmallId = networkSmallId;
	m_playerColourIndex = playerColourIndex;
	m_playerPrivileges = playerPrivileges;
	m_entityId = -1;
}

PlayerInfoPacket::PlayerInfoPacket(shared_ptr<ServerPlayer> player)
{
#if defined(_NATIVE_DESKTOP)
	(void)player;
	m_networkSmallId = 0;
	m_playerColourIndex = -1;
	m_playerPrivileges = 0;
	m_entityId = -1;
#else
	m_networkSmallId = 0;
	if(player->connection != nullptr && player->connection->getNetworkPlayer() != nullptr) m_networkSmallId = player->connection->getNetworkPlayer()->GetSmallId();
	m_playerColourIndex = player->getPlayerIndex();
	m_playerPrivileges = player->getAllPlayerGamePrivileges();
	m_entityId = player->entityId;
#endif
}

void PlayerInfoPacket::read(DataInputStream *dis)
{
	m_networkSmallId = dis->readByte();
	m_playerColourIndex = dis->readShort();
	m_playerPrivileges = dis->readInt();
	m_entityId = dis->readInt();
}

void PlayerInfoPacket::write(DataOutputStream *dos)
{
	dos->writeByte(m_networkSmallId);
	dos->writeShort(m_playerColourIndex);
	dos->writeInt(m_playerPrivileges);
	dos->writeInt(m_entityId);
}

void PlayerInfoPacket::handle(PacketListener *listener)
{
	listener->handlePlayerInfo(shared_from_this());
}

int PlayerInfoPacket::getEstimatedSize()
{
	return 2 + 2 + 4 + 4;
}
