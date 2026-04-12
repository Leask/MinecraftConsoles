#include "stdafx.h"
#include "NetworkPlayerNative.h"

NetworkPlayerNative::NetworkPlayerNative(IQNetPlayer *qnetPlayer)
{
	m_qnetPlayer = qnetPlayer;
	m_pSocket = nullptr;
}

unsigned char NetworkPlayerNative::GetSmallId()
{
	return m_qnetPlayer->GetSmallId();
}

void NetworkPlayerNative::SendData(INetworkPlayer *player, const void *pvData, int dataSize, bool lowPriority, bool ack)
{
	DWORD flags;
	flags = QNET_SENDDATA_RELIABLE | QNET_SENDDATA_SEQUENTIAL;
	if( lowPriority ) flags |= QNET_SENDDATA_LOW_PRIORITY | QNET_SENDDATA_SECONDARY;
	m_qnetPlayer->SendData(static_cast<NetworkPlayerNative *>(player)->m_qnetPlayer, pvData, dataSize, flags);
}

int NetworkPlayerNative::GetOutstandingAckCount()
{
	return 0;
}

bool NetworkPlayerNative::IsSameSystem(INetworkPlayer *player)
{
	return ( m_qnetPlayer->IsSameSystem(static_cast<NetworkPlayerNative *>(player)->m_qnetPlayer) == TRUE );
}

int NetworkPlayerNative::GetSendQueueSizeBytes( INetworkPlayer *player, bool lowPriority )
{
	DWORD flags = QNET_GETSENDQUEUESIZE_BYTES;
	if( lowPriority ) flags |= QNET_GETSENDQUEUESIZE_SECONDARY_TYPE;
	return m_qnetPlayer->GetSendQueueSize(player ? static_cast<NetworkPlayerNative *>(player)->m_qnetPlayer : nullptr , flags);
}

int NetworkPlayerNative::GetSendQueueSizeMessages( INetworkPlayer *player, bool lowPriority )
{
	DWORD flags = QNET_GETSENDQUEUESIZE_MESSAGES;
	if( lowPriority ) flags |= QNET_GETSENDQUEUESIZE_SECONDARY_TYPE;
	return m_qnetPlayer->GetSendQueueSize(player ? static_cast<NetworkPlayerNative *>(player)->m_qnetPlayer : nullptr , flags);
}

int NetworkPlayerNative::GetCurrentRtt()
{
	return m_qnetPlayer->GetCurrentRtt();
}

bool NetworkPlayerNative::IsHost()
{
	return ( m_qnetPlayer->IsHost() == TRUE );
}

bool NetworkPlayerNative::IsGuest()
{
	return ( m_qnetPlayer->IsGuest() == TRUE );
}

bool NetworkPlayerNative::IsLocal()
{
	return ( m_qnetPlayer->IsLocal() == TRUE );
}

int NetworkPlayerNative::GetSessionIndex()
{
	return m_qnetPlayer->GetSessionIndex();
}

bool NetworkPlayerNative::IsTalking()
{
	return ( m_qnetPlayer->IsTalking() == TRUE );
}

bool NetworkPlayerNative::IsMutedByLocalUser(int userIndex)
{
	return ( m_qnetPlayer->IsMutedByLocalUser(userIndex) == TRUE );
}

bool NetworkPlayerNative::HasVoice()
{
	return ( m_qnetPlayer->HasVoice() == TRUE );
}

bool NetworkPlayerNative::HasCamera()
{
	return ( m_qnetPlayer->HasCamera() == TRUE );
}

int NetworkPlayerNative::GetUserIndex()
{
	return m_qnetPlayer->GetUserIndex();
}

void NetworkPlayerNative::SetSocket(Socket *pSocket)
{
	m_pSocket = pSocket;
}

Socket *NetworkPlayerNative::GetSocket()
{
	return m_pSocket;
}

PlayerUID NetworkPlayerNative::GetUID()
{
	return m_qnetPlayer->GetXuid();
}

const wchar_t *NetworkPlayerNative::GetOnlineName()
{
	return m_qnetPlayer->GetGamertag();
}

std::wstring NetworkPlayerNative::GetDisplayName()
{
	return m_qnetPlayer->GetGamertag();
}

IQNetPlayer *NetworkPlayerNative::GetQNetPlayer()
{
	return m_qnetPlayer;
}

void NetworkPlayerNative::SentChunkPacket()
{
	m_lastChunkPacketTime = System::currentTimeMillis();
}

int NetworkPlayerNative::GetTimeSinceLastChunkPacket_ms()
{
	// If we haven't ever sent a packet, return maximum
	if( m_lastChunkPacketTime == 0 )
	{
		return INT_MAX;
	}

	const int64_t currentTime = System::currentTimeMillis();
	return static_cast<int>(currentTime - m_lastChunkPacketTime);
}