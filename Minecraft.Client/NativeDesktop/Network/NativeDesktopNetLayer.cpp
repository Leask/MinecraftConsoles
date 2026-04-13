// Code implemented by LCEMP, credit if used on other repos
// https://github.com/LCEMP/LCEMP

#include "stdafx.h"

#include "NativeDesktopNetLayer.h"
#include "../../Common/Network/PlatformNetworkManagerStub.h"
#include "../../../Minecraft.World/Socket.h"
#include <lce_net/lce_net.h>
#include <lce_time/lce_time.h>
#if defined(MINECRAFT_SERVER_BUILD)
#include "../../../Minecraft.Server/Access/Access.h"
#include "../../../Minecraft.Server/ServerLogManager.h"
#endif
#include "../../../Minecraft.World/DisconnectPacket.h"
#include "../../Minecraft.h"

#include <cerrno>
#include <cstdio>
#include <string>

#define NATIVE_DESKTOP_NET_TRACE(message) \
	std::fprintf(stderr, "NativeDesktop network: %s\n", message)
#define NATIVE_DESKTOP_NET_TRACEF(format, ...) \
	std::fprintf(stderr, "NativeDesktop network: " format "\n", __VA_ARGS__)

static bool RecvExact(SOCKET sock, BYTE* buf, int len);

namespace
{
int GetNetLastError()
{
	return LceNetGetLastError();
}

void CloseNetSocket(SOCKET socketHandle)
{
	if (socketHandle != INVALID_SOCKET)
	{
		LceNetCloseSocket(static_cast<LceSocketHandle>(socketHandle));
	}
}
}

SOCKET NativeDesktopNetLayer::s_listenSocket = INVALID_SOCKET;
SOCKET NativeDesktopNetLayer::s_hostConnectionSocket = INVALID_SOCKET;
HANDLE NativeDesktopNetLayer::s_acceptThread = nullptr;
HANDLE NativeDesktopNetLayer::s_clientRecvThread = nullptr;

bool NativeDesktopNetLayer::s_isHost = false;
bool NativeDesktopNetLayer::s_connected = false;
bool NativeDesktopNetLayer::s_active = false;
bool NativeDesktopNetLayer::s_initialized = false;

BYTE NativeDesktopNetLayer::s_localSmallId = 0;
BYTE NativeDesktopNetLayer::s_hostSmallId = 0;
unsigned int NativeDesktopNetLayer::s_nextSmallId = XUSER_MAX_COUNT;

CRITICAL_SECTION NativeDesktopNetLayer::s_sendLock;
CRITICAL_SECTION NativeDesktopNetLayer::s_connectionsLock;

std::vector<NativeDesktopRemoteConnection> NativeDesktopNetLayer::s_connections;

SOCKET NativeDesktopNetLayer::s_advertiseSock = INVALID_SOCKET;
HANDLE NativeDesktopNetLayer::s_advertiseThread = nullptr;
volatile bool NativeDesktopNetLayer::s_advertising = false;
NativeDesktopLANBroadcast NativeDesktopNetLayer::s_advertiseData = {};
CRITICAL_SECTION NativeDesktopNetLayer::s_advertiseLock;
int NativeDesktopNetLayer::s_hostGamePort = NATIVE_DESKTOP_NET_DEFAULT_PORT;

SOCKET NativeDesktopNetLayer::s_discoverySock = INVALID_SOCKET;
HANDLE NativeDesktopNetLayer::s_discoveryThread = nullptr;
volatile bool NativeDesktopNetLayer::s_discovering = false;
CRITICAL_SECTION NativeDesktopNetLayer::s_discoveryLock;
std::vector<NativeDesktopLANSession> NativeDesktopNetLayer::s_discoveredSessions;

CRITICAL_SECTION NativeDesktopNetLayer::s_disconnectLock;
std::vector<BYTE> NativeDesktopNetLayer::s_disconnectedSmallIds;

CRITICAL_SECTION NativeDesktopNetLayer::s_freeSmallIdLock;
std::vector<BYTE> NativeDesktopNetLayer::s_freeSmallIds;
SOCKET NativeDesktopNetLayer::s_smallIdToSocket[256];
CRITICAL_SECTION NativeDesktopNetLayer::s_smallIdToSocketLock;

SOCKET NativeDesktopNetLayer::s_splitScreenSocket[XUSER_MAX_COUNT] = { INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET };
BYTE NativeDesktopNetLayer::s_splitScreenSmallId[XUSER_MAX_COUNT] = { 0xFF, 0xFF, 0xFF, 0xFF };
HANDLE NativeDesktopNetLayer::s_splitScreenRecvThread[XUSER_MAX_COUNT] = {nullptr, nullptr, nullptr, nullptr};

bool g_NativeDesktopMultiplayerHost = false;
bool g_NativeDesktopMultiplayerJoin = false;
int g_NativeDesktopMultiplayerPort = NATIVE_DESKTOP_NET_DEFAULT_PORT;
char g_NativeDesktopMultiplayerIP[256] = "127.0.0.1";
bool g_NativeDesktopDedicatedServer = false;
int g_NativeDesktopDedicatedServerPort = NATIVE_DESKTOP_NET_DEFAULT_PORT;
char g_NativeDesktopDedicatedServerBindIP[256] = "";
bool g_NativeDesktopDedicatedServerLanAdvertise = true;

bool NativeDesktopNetLayer::Initialize()
{
	if (s_initialized) return true;

	if (!LceNetInitialize())
	{
		app.DebugPrintf("Network initialization failed: %d\n", GetNetLastError());
		return false;
	}

	InitializeCriticalSection(&s_sendLock);
	InitializeCriticalSection(&s_connectionsLock);
	InitializeCriticalSection(&s_advertiseLock);
	InitializeCriticalSection(&s_discoveryLock);
	InitializeCriticalSection(&s_disconnectLock);
	InitializeCriticalSection(&s_freeSmallIdLock);
	InitializeCriticalSection(&s_smallIdToSocketLock);
	for (int i = 0; i < 256; i++)
		s_smallIdToSocket[i] = INVALID_SOCKET;

	s_initialized = true;

	// Dedicated Server does not use LAN session discovery and therefore does not initiate discovery.
	if (!g_NativeDesktopDedicatedServer)
	{
		StartDiscovery();
	}

	return true;
}

void NativeDesktopNetLayer::Shutdown()
{
	NATIVE_DESKTOP_NET_TRACE("Shutdown begin");
	StopAdvertising();
	NATIVE_DESKTOP_NET_TRACE("StopAdvertising complete");
	StopDiscovery();
	NATIVE_DESKTOP_NET_TRACE("StopDiscovery complete");

	s_active = false;
	s_connected = false;

	if (s_listenSocket != INVALID_SOCKET)
	{
		CloseNetSocket(s_listenSocket);
		s_listenSocket = INVALID_SOCKET;
	}

	if (s_hostConnectionSocket != INVALID_SOCKET)
	{
		CloseNetSocket(s_hostConnectionSocket);
		s_hostConnectionSocket = INVALID_SOCKET;
	}

	// Stop accept loop first so no new RecvThread can be created while shutting down.
	if (s_acceptThread != nullptr)
	{
		NATIVE_DESKTOP_NET_TRACE("accept thread wait begin");
		WaitForSingleObject(s_acceptThread, 2000);
		CloseHandle(s_acceptThread);
		s_acceptThread = nullptr;
		NATIVE_DESKTOP_NET_TRACE("accept thread wait end");
	}

	std::vector<HANDLE> recvThreads;
	EnterCriticalSection(&s_connectionsLock);
	NATIVE_DESKTOP_NET_TRACEF(
		"connections close begin count=%zu",
		s_connections.size());
	for (size_t i = 0; i < s_connections.size(); i++)
	{
		s_connections[i].active = false;
		if (s_connections[i].tcpSocket != INVALID_SOCKET)
		{
			CloseNetSocket(s_connections[i].tcpSocket);
			s_connections[i].tcpSocket = INVALID_SOCKET;
		}
		if (s_connections[i].recvThread != nullptr)
		{
			recvThreads.push_back(s_connections[i].recvThread);
			s_connections[i].recvThread = nullptr;
		}
	}
	LeaveCriticalSection(&s_connectionsLock);
	NATIVE_DESKTOP_NET_TRACEF(
		"connections close end recvThreads=%zu",
		recvThreads.size());

	// Wait for all host-side receive threads to exit before destroying state.
	for (size_t i = 0; i < recvThreads.size(); i++)
	{
		NATIVE_DESKTOP_NET_TRACEF("recv thread %zu wait begin", i);
		WaitForSingleObject(recvThreads[i], 2000);
		CloseHandle(recvThreads[i]);
		NATIVE_DESKTOP_NET_TRACEF("recv thread %zu wait end", i);
	}

	EnterCriticalSection(&s_connectionsLock);
	s_connections.clear();
	LeaveCriticalSection(&s_connectionsLock);
	NATIVE_DESKTOP_NET_TRACE("connections clear end");

	if (s_clientRecvThread != nullptr)
	{
		NATIVE_DESKTOP_NET_TRACE("client recv thread wait begin");
		WaitForSingleObject(s_clientRecvThread, 2000);
		CloseHandle(s_clientRecvThread);
		s_clientRecvThread = nullptr;
		NATIVE_DESKTOP_NET_TRACE("client recv thread wait end");
	}

	for (int i = 0; i < XUSER_MAX_COUNT; i++)
	{
		if (s_splitScreenSocket[i] != INVALID_SOCKET)
		{
			CloseNetSocket(s_splitScreenSocket[i]);
			s_splitScreenSocket[i] = INVALID_SOCKET;
		}
		if (s_splitScreenRecvThread[i] != nullptr)
		{
			NATIVE_DESKTOP_NET_TRACEF(
				"split-screen recv thread %d wait begin",
				i);
			WaitForSingleObject(s_splitScreenRecvThread[i], 2000);
			CloseHandle(s_splitScreenRecvThread[i]);
			s_splitScreenRecvThread[i] = nullptr;
			NATIVE_DESKTOP_NET_TRACEF(
				"split-screen recv thread %d wait end",
				i);
		}
		s_splitScreenSmallId[i] = 0xFF;
	}

	if (s_initialized)
	{
		NATIVE_DESKTOP_NET_TRACE("state clear begin");
		EnterCriticalSection(&s_disconnectLock);
		s_disconnectedSmallIds.clear();
		LeaveCriticalSection(&s_disconnectLock);

		EnterCriticalSection(&s_freeSmallIdLock);
		s_freeSmallIds.clear();
		LeaveCriticalSection(&s_freeSmallIdLock);

		DeleteCriticalSection(&s_sendLock);
		DeleteCriticalSection(&s_connectionsLock);
		DeleteCriticalSection(&s_advertiseLock);
		DeleteCriticalSection(&s_discoveryLock);
		DeleteCriticalSection(&s_disconnectLock);
		DeleteCriticalSection(&s_freeSmallIdLock);
		DeleteCriticalSection(&s_smallIdToSocketLock);
		LceNetShutdown();
		s_initialized = false;
		NATIVE_DESKTOP_NET_TRACE("state clear end");
	}
	NATIVE_DESKTOP_NET_TRACE("Shutdown end");
}

bool NativeDesktopNetLayer::HostGame(int port, const char* bindIp)
{
	if (!s_initialized && !Initialize()) return false;

	s_isHost = true;
	s_localSmallId = 0;
	s_hostSmallId = 0;
	s_nextSmallId = XUSER_MAX_COUNT;
	s_hostGamePort = port;

	EnterCriticalSection(&s_freeSmallIdLock);
	s_freeSmallIds.clear();
	LeaveCriticalSection(&s_freeSmallIdLock);
	EnterCriticalSection(&s_smallIdToSocketLock);
	for (int i = 0; i < 256; i++)
		s_smallIdToSocket[i] = INVALID_SOCKET;
	LeaveCriticalSection(&s_smallIdToSocketLock);

	const char* resolvedBindIp = (bindIp != nullptr && bindIp[0] != 0) ? bindIp : nullptr;
	const LceSocketHandle listenHandle = LceNetOpenTcpSocket();
	if (listenHandle == LCE_INVALID_SOCKET)
	{
		app.DebugPrintf("socket() failed: %d\n", GetNetLastError());
		return false;
	}
	s_listenSocket = static_cast<SOCKET>(listenHandle);

	if (!LceNetSetSocketReuseAddress(listenHandle, true))
	{
		app.DebugPrintf("setsockopt(SO_REUSEADDR) failed: %d\n", GetNetLastError());
		CloseNetSocket(s_listenSocket);
		s_listenSocket = INVALID_SOCKET;
		return false;
	}
	if (!LceNetBindIpv4(listenHandle, resolvedBindIp, port))
	{
		app.DebugPrintf("bind() failed: %d\n", GetNetLastError());
		CloseNetSocket(s_listenSocket);
		s_listenSocket = INVALID_SOCKET;
		return false;
	}
	if (!LceNetListen(listenHandle, SOMAXCONN))
	{
		app.DebugPrintf("listen() failed: %d\n", GetNetLastError());
		CloseNetSocket(s_listenSocket);
		s_listenSocket = INVALID_SOCKET;
		return false;
	}
#if defined(_NATIVE_DESKTOP)
	if (!LceNetSetSocketRecvTimeout(listenHandle, 500))
	{
		app.DebugPrintf("listen timeout setup failed: %d\n", GetNetLastError());
		CloseNetSocket(s_listenSocket);
		s_listenSocket = INVALID_SOCKET;
		return false;
	}

	s_active = true;
	s_connected = true;

	s_acceptThread = CreateThread(nullptr, 0, AcceptThreadProc, nullptr, 0, nullptr);

	app.DebugPrintf("NativeDesktop LAN: Hosting on %s:%d\n",
		resolvedBindIp != nullptr ? resolvedBindIp : "*",
		port);
	return true;
}

bool NativeDesktopNetLayer::JoinGame(const char* ip, int port)
{
	if (!s_initialized && !Initialize()) return false;

	s_isHost = false;
	s_hostSmallId = 0;
	s_connected = false;
	s_active = false;

	if (s_hostConnectionSocket != INVALID_SOCKET)
	{
		CloseNetSocket(s_hostConnectionSocket);
		s_hostConnectionSocket = INVALID_SOCKET;
	}

	// Wait for old client recv thread to fully exit before starting a new connection.
	// Without this, the old thread can read from the new socket (s_hostConnectionSocket
	// is a global) and steal bytes from the new connection's TCP stream, causing
	// packet stream misalignment on reconnect.
	if (s_clientRecvThread != nullptr)
	{
		WaitForSingleObject(s_clientRecvThread, 5000);
		CloseHandle(s_clientRecvThread);
		s_clientRecvThread = nullptr;
	}

	char resolvedIp[64] = {};
	if (!LceNetResolveIpv4(ip, port, resolvedIp, sizeof(resolvedIp)))
	{
		app.DebugPrintf("resolve failed for %s:%d\n", ip, port);
		return false;
	}

	bool connected = false;
	BYTE assignedSmallId = 0;
	const int maxAttempts = 12;

	for (int attempt = 0; attempt < maxAttempts; ++attempt)
	{
		const LceSocketHandle connectionHandle = LceNetOpenTcpSocket();
		if (connectionHandle == LCE_INVALID_SOCKET)
		{
			app.DebugPrintf("socket() failed: %d\n", GetNetLastError());
			break;
		}
		s_hostConnectionSocket = static_cast<SOCKET>(connectionHandle);

		LceNetSetSocketNoDelay(connectionHandle, true);

		if (!LceNetConnectIpv4(connectionHandle, resolvedIp, port))
		{
			int err = GetNetLastError();
			app.DebugPrintf("connect() to %s:%d failed (attempt %d/%d): %d\n", resolvedIp, port, attempt + 1, maxAttempts, err);
			CloseNetSocket(s_hostConnectionSocket);
			s_hostConnectionSocket = INVALID_SOCKET;
			Sleep(200);
			continue;
		}

		BYTE assignBuf[1];
		if (!LceNetRecvAll(connectionHandle, assignBuf, 1))
		{
			app.DebugPrintf("Failed to receive small ID assignment from host (attempt %d/%d)\n", attempt + 1, maxAttempts);
			CloseNetSocket(s_hostConnectionSocket);
			s_hostConnectionSocket = INVALID_SOCKET;
			Sleep(200);
			continue;
		}

		if (assignBuf[0] == NATIVE_DESKTOP_SMALLID_REJECT)
		{
			BYTE rejectBuf[5];
			if (!RecvExact(s_hostConnectionSocket, rejectBuf, 5))
			{
				app.DebugPrintf("Failed to receive reject reason from host\n");
				CloseNetSocket(s_hostConnectionSocket);
				s_hostConnectionSocket = INVALID_SOCKET;
				Sleep(200);
				continue;
			}
			// rejectBuf[0] = packet id (255), rejectBuf[1..4] = 4-byte big-endian reason
			int reason = ((rejectBuf[1] & 0xff) << 24) | ((rejectBuf[2] & 0xff) << 16) |
				((rejectBuf[3] & 0xff) << 8) | (rejectBuf[4] & 0xff);
			Minecraft::GetInstance()->connectionDisconnected(ProfileManager.GetPrimaryPad(), (DisconnectPacket::eDisconnectReason)reason);
			CloseNetSocket(s_hostConnectionSocket);
			s_hostConnectionSocket = INVALID_SOCKET;
			return false;
		}

		assignedSmallId = assignBuf[0];
		connected = true;
		break;
	}

	if (!connected)
	{
		return false;
	}
	s_localSmallId = assignedSmallId;

	// Save the host IP and port so JoinSplitScreen can connect to the same host
	// regardless of how the connection was initiated (UI vs command line).
	strncpy_s(g_NativeDesktopMultiplayerIP, sizeof(g_NativeDesktopMultiplayerIP), resolvedIp, _TRUNCATE);
	g_NativeDesktopMultiplayerPort = port;

	app.DebugPrintf("NativeDesktop LAN: Connected to %s:%d, assigned smallId=%d\n", resolvedIp, port, s_localSmallId);

	s_active = true;
	s_connected = true;

	s_clientRecvThread = CreateThread(nullptr, 0, ClientRecvThreadProc, nullptr, 0, nullptr);

	return true;
}

bool NativeDesktopNetLayer::SendOnSocket(SOCKET sock, const void* data, int dataSize)
{
	if (sock == INVALID_SOCKET || dataSize <= 0 || dataSize > NATIVE_DESKTOP_NET_MAX_PACKET_SIZE) return false;

	// TODO: s_sendLock is a single global lock for ALL sockets. If one client's
	// send() blocks (TCP window full, slow WiFi), every other write thread stalls
	// waiting for this lock — no data flows to any player until the slow send
	// completes. This scales badly with player count (8+ players = noticeable).
	// Fix: replace with per-socket locks indexed by smallId (s_perSocketSendLock[256]).
	// The lock only needs to prevent interleaving of header+payload on the SAME socket;
	// sends to different sockets are independent and should never block each other.
	EnterCriticalSection(&s_sendLock);

	BYTE header[4];
	header[0] = static_cast<BYTE>((dataSize >> 24) & 0xFF);
	header[1] = static_cast<BYTE>((dataSize >> 16) & 0xFF);
	header[2] = static_cast<BYTE>((dataSize >> 8) & 0xFF);
	header[3] = static_cast<BYTE>(dataSize & 0xFF);
	const bool sentHeader = LceNetSendAll(
		static_cast<LceSocketHandle>(sock),
		header,
		sizeof(header));
	const bool sentPayload = sentHeader && LceNetSendAll(
		static_cast<LceSocketHandle>(sock),
		data,
		dataSize);

	LeaveCriticalSection(&s_sendLock);
	return sentPayload;
}

bool NativeDesktopNetLayer::SendToSmallId(BYTE targetSmallId, const void* data, int dataSize)
{
	if (!s_active) return false;

	if (s_isHost)
	{
		SOCKET sock = GetSocketForSmallId(targetSmallId);
		if (sock == INVALID_SOCKET) return false;
		return SendOnSocket(sock, data, dataSize);
	}
	else
	{
		return SendOnSocket(s_hostConnectionSocket, data, dataSize);
	}
}

SOCKET NativeDesktopNetLayer::GetSocketForSmallId(BYTE smallId)
{
	EnterCriticalSection(&s_smallIdToSocketLock);
	SOCKET sock = s_smallIdToSocket[smallId];
	LeaveCriticalSection(&s_smallIdToSocketLock);
	return sock;
}

void NativeDesktopNetLayer::ClearSocketForSmallId(BYTE smallId)
{
	EnterCriticalSection(&s_smallIdToSocketLock);
	s_smallIdToSocket[smallId] = INVALID_SOCKET;
	LeaveCriticalSection(&s_smallIdToSocketLock);
}

// Send reject handshake: sentinel 0xFF + DisconnectPacket wire format (1 byte id 255 + 4 byte big-endian reason). Then caller closes socket.
static void SendRejectWithReason(SOCKET clientSocket, DisconnectPacket::eDisconnectReason reason)
{
	BYTE buf[6];
	buf[0] = NATIVE_DESKTOP_SMALLID_REJECT;
	buf[1] = (BYTE)255; // DisconnectPacket packet id
	int r = (int)reason;
	buf[2] = (BYTE)((r >> 24) & 0xff);
	buf[3] = (BYTE)((r >> 16) & 0xff);
	buf[4] = (BYTE)((r >> 8) & 0xff);
	buf[5] = (BYTE)(r & 0xff);
	LceNetSendAll(static_cast<LceSocketHandle>(clientSocket), buf, sizeof(buf));
}

static bool RecvExact(SOCKET sock, BYTE* buf, int len)
{
	return LceNetRecvAll(static_cast<LceSocketHandle>(sock), buf, len);
}

void NativeDesktopNetLayer::HandleDataReceived(BYTE fromSmallId, BYTE toSmallId, unsigned char* data, unsigned int dataSize)
{
	INetworkPlayer* pPlayerFrom = g_NetworkManager.GetPlayerBySmallId(fromSmallId);
	INetworkPlayer* pPlayerTo = g_NetworkManager.GetPlayerBySmallId(toSmallId);

	if (pPlayerFrom == nullptr || pPlayerTo == nullptr)
	{
		app.DebugPrintf("NET RECV: DROPPED %u bytes from=%d to=%d (player NULL: from=%p to=%p)\n",
			dataSize, fromSmallId, toSmallId, pPlayerFrom, pPlayerTo);
		return;
	}

	if (s_isHost)
	{
		::Socket* pSocket = pPlayerFrom->GetSocket();
		if (pSocket != nullptr)
			pSocket->pushDataToQueue(data, dataSize, false);
		else
			app.DebugPrintf("NET RECV: DROPPED %u bytes, host pSocket NULL for from=%d\n", dataSize, fromSmallId);
	}
	else
	{
		::Socket* pSocket = pPlayerTo->GetSocket();
		if (pSocket != nullptr)
			pSocket->pushDataToQueue(data, dataSize, true);
		else
			app.DebugPrintf("NET RECV: DROPPED %u bytes, client pSocket NULL for to=%d\n", dataSize, toSmallId);
	}
}

DWORD WINAPI NativeDesktopNetLayer::AcceptThreadProc(LPVOID param)
{
	while (s_active)
	{
		char remoteIpBuffer[64] = {};
		int remotePort = 0;
		const LceSocketHandle acceptedHandle = LceNetAcceptIpv4(
			static_cast<LceSocketHandle>(s_listenSocket),
			remoteIpBuffer,
			sizeof(remoteIpBuffer),
			&remotePort);
		(void)remotePort;
		SOCKET clientSocket = static_cast<SOCKET>(acceptedHandle);
		if (acceptedHandle == LCE_INVALID_SOCKET)
		{
#if defined(_NATIVE_DESKTOP)
			const int error = GetNetLastError();
			if (s_active && (error == EAGAIN || error == EWOULDBLOCK))
			{
				continue;
			}
#endif
			if (s_active)
				app.DebugPrintf("accept() failed: %d\n", GetNetLastError());
			break;
		}

		LceNetSetSocketNoDelay(acceptedHandle, true);

#if defined(MINECRAFT_SERVER_BUILD)
		const std::string remoteIp = remoteIpBuffer;
		const bool hasRemoteIp = !remoteIp.empty();
		const char *remoteIpForLog = hasRemoteIp ? remoteIp.c_str() : "unknown";
		if (g_NativeDesktopDedicatedServer)
		{
			ServerRuntime::ServerLogManager::OnIncomingTcpConnection(remoteIpForLog);
			if (hasRemoteIp && ServerRuntime::Access::IsIpBanned(remoteIp))
			{
				ServerRuntime::ServerLogManager::OnRejectedTcpConnection(remoteIpForLog, ServerRuntime::ServerLogManager::eTcpRejectReason_BannedIp);
				SendRejectWithReason(clientSocket, DisconnectPacket::eDisconnect_Banned);
				CloseNetSocket(clientSocket);
				continue;
			}
		}
#endif

		extern QNET_STATE _iQNetStubState;
		if (_iQNetStubState != QNET_STATE_GAME_PLAY)
		{
#if defined(MINECRAFT_SERVER_BUILD)
			if (g_NativeDesktopDedicatedServer)
			{
				ServerRuntime::ServerLogManager::OnRejectedTcpConnection(remoteIpForLog, ServerRuntime::ServerLogManager::eTcpRejectReason_GameNotReady);
			}
			else
#endif
			{
				app.DebugPrintf("NativeDesktop LAN: Rejecting connection, game not ready\n");
			}
			CloseNetSocket(clientSocket);
			continue;
		}

		extern CPlatformNetworkManagerStub* g_pPlatformNetworkManager;
		if (g_pPlatformNetworkManager != nullptr && !g_pPlatformNetworkManager->CanAcceptMoreConnections())
		{
#if defined(MINECRAFT_SERVER_BUILD)
			if (g_NativeDesktopDedicatedServer)
			{
				ServerRuntime::ServerLogManager::OnRejectedTcpConnection(remoteIpForLog, ServerRuntime::ServerLogManager::eTcpRejectReason_ServerFull);
			}
			else
#endif
			{
				app.DebugPrintf("NativeDesktop LAN: Rejecting connection, server at max players\n");
			}
			SendRejectWithReason(clientSocket, DisconnectPacket::eDisconnect_ServerFull);
			CloseNetSocket(clientSocket);
			continue;
		}

		BYTE assignedSmallId;
		EnterCriticalSection(&s_freeSmallIdLock);
		if (!s_freeSmallIds.empty())
		{
			assignedSmallId = s_freeSmallIds.back();
			s_freeSmallIds.pop_back();
		}
		else if (s_nextSmallId < (unsigned int)MINECRAFT_NET_MAX_PLAYERS)
		{
			assignedSmallId = (BYTE)s_nextSmallId++;
		}
		else
		{
			LeaveCriticalSection(&s_freeSmallIdLock);
#if defined(MINECRAFT_SERVER_BUILD)
			if (g_NativeDesktopDedicatedServer)
			{
				ServerRuntime::ServerLogManager::OnRejectedTcpConnection(remoteIpForLog, ServerRuntime::ServerLogManager::eTcpRejectReason_ServerFull);
			}
			else
#endif
			{
				app.DebugPrintf("NativeDesktop LAN: Server full, rejecting connection\n");
			}
			SendRejectWithReason(clientSocket, DisconnectPacket::eDisconnect_ServerFull);
			CloseNetSocket(clientSocket);
			continue;
		}
		LeaveCriticalSection(&s_freeSmallIdLock);

		BYTE assignBuf[1] = { assignedSmallId };
		if (!LceNetSendAll(static_cast<LceSocketHandle>(clientSocket), assignBuf, 1))
		{
			app.DebugPrintf("Failed to send small ID to client\n");
			CloseNetSocket(clientSocket);
			PushFreeSmallId(assignedSmallId);
			continue;
		}

		NativeDesktopRemoteConnection conn;
		conn.tcpSocket = clientSocket;
		conn.smallId = assignedSmallId;
		conn.active = true;
		conn.recvThread = nullptr;

		EnterCriticalSection(&s_connectionsLock);
		s_connections.push_back(conn);
		int connIdx = static_cast<int>(s_connections.size()) - 1;
		LeaveCriticalSection(&s_connectionsLock);

#if defined(MINECRAFT_SERVER_BUILD)
		if (g_NativeDesktopDedicatedServer)
		{
			ServerRuntime::ServerLogManager::OnAcceptedTcpConnection(assignedSmallId, remoteIpForLog);
		}
		else
#endif
		{
			app.DebugPrintf("NativeDesktop LAN: Client connected, assigned smallId=%d\n", assignedSmallId);
		}

		EnterCriticalSection(&s_smallIdToSocketLock);
		s_smallIdToSocket[assignedSmallId] = clientSocket;
		LeaveCriticalSection(&s_smallIdToSocketLock);

		IQNetPlayer* qnetPlayer = &IQNet::m_player[assignedSmallId];

		extern void NativeDesktop_SetupRemoteQNetPlayer(IQNetPlayer * player, BYTE smallId, bool isHost, bool isLocal);
		NativeDesktop_SetupRemoteQNetPlayer(qnetPlayer, assignedSmallId, false, false);

		extern CPlatformNetworkManagerStub* g_pPlatformNetworkManager;
		g_pPlatformNetworkManager->NotifyPlayerJoined(qnetPlayer);

		DWORD* threadParam = new DWORD;
		*threadParam = connIdx;
		HANDLE hThread = CreateThread(nullptr, 0, RecvThreadProc, threadParam, 0, nullptr);

		EnterCriticalSection(&s_connectionsLock);
		if (connIdx < static_cast<int>(s_connections.size()))
			s_connections[connIdx].recvThread = hThread;
		LeaveCriticalSection(&s_connectionsLock);
	}
	return 0;
}

DWORD WINAPI NativeDesktopNetLayer::RecvThreadProc(LPVOID param)
{
	DWORD connIdx = *static_cast<DWORD *>(param);
	delete static_cast<DWORD *>(param);

	EnterCriticalSection(&s_connectionsLock);
	if (connIdx >= static_cast<DWORD>(s_connections.size()))
	{
		LeaveCriticalSection(&s_connectionsLock);
		return 0;
	}
	SOCKET sock = s_connections[connIdx].tcpSocket;
	BYTE clientSmallId = s_connections[connIdx].smallId;
	LeaveCriticalSection(&s_connectionsLock);

	std::vector<BYTE> recvBuf;
	recvBuf.resize(NATIVE_DESKTOP_NET_RECV_BUFFER_SIZE);

	while (s_active)
	{
		BYTE header[4];
		if (!RecvExact(sock, header, 4))
		{
			app.DebugPrintf("NativeDesktop LAN: Client smallId=%d disconnected (header)\n", clientSmallId);
			break;
		}

		int packetSize =
			(static_cast<uint32_t>(header[0]) << 24) |
			(static_cast<uint32_t>(header[1]) << 16) |
			(static_cast<uint32_t>(header[2]) << 8) |
			static_cast<uint32_t>(header[3]);

		if (packetSize <= 0 || packetSize > NATIVE_DESKTOP_NET_MAX_PACKET_SIZE)
		{
			app.DebugPrintf("NativeDesktop LAN: Invalid packet size %d from client smallId=%d (max=%d)\n",
				packetSize,
				clientSmallId,
				(int)NATIVE_DESKTOP_NET_MAX_PACKET_SIZE);
			break;
		}

		if (static_cast<int>(recvBuf.size()) < packetSize)
		{
			recvBuf.resize(packetSize);
			app.DebugPrintf("NativeDesktop LAN: Resized host recv buffer to %d bytes for client smallId=%d\n", packetSize, clientSmallId);
		}

		if (!RecvExact(sock, &recvBuf[0], packetSize))
		{
			app.DebugPrintf("NativeDesktop LAN: Client smallId=%d disconnected (body)\n", clientSmallId);
			break;
		}

		HandleDataReceived(clientSmallId, s_hostSmallId, &recvBuf[0], packetSize);
	}

	EnterCriticalSection(&s_connectionsLock);
	for (size_t i = 0; i < s_connections.size(); i++)
	{
		if (s_connections[i].smallId == clientSmallId)
		{
			s_connections[i].active = false;
			if (s_connections[i].tcpSocket != INVALID_SOCKET)
			{
				CloseNetSocket(s_connections[i].tcpSocket);
				s_connections[i].tcpSocket = INVALID_SOCKET;
			}
			break;
		}
	}
	LeaveCriticalSection(&s_connectionsLock);

	EnterCriticalSection(&s_disconnectLock);
	s_disconnectedSmallIds.push_back(clientSmallId);
	LeaveCriticalSection(&s_disconnectLock);

	return 0;
}

bool NativeDesktopNetLayer::PopDisconnectedSmallId(BYTE* outSmallId)
{
	bool found = false;
	EnterCriticalSection(&s_disconnectLock);
	if (!s_disconnectedSmallIds.empty())
	{
		*outSmallId = s_disconnectedSmallIds.back();
		s_disconnectedSmallIds.pop_back();
		found = true;
	}
	LeaveCriticalSection(&s_disconnectLock);
	return found;
}

void NativeDesktopNetLayer::PushFreeSmallId(BYTE smallId)
{
	// SmallIds 0..(XUSER_MAX_COUNT-1) are permanently reserved for the host's
	// local pads and must never be recycled to remote clients.
	if (smallId < (BYTE)XUSER_MAX_COUNT)
		return;

	EnterCriticalSection(&s_freeSmallIdLock);
	// Guard against double-recycle: the reconnect path (queueSmallIdForRecycle) and
	// the DoWork disconnect path can both push the same smallId. If we allow duplicates,
	// AcceptThread will hand out the same smallId to two different connections.
	bool alreadyFree = false;
	for (size_t i = 0; i < s_freeSmallIds.size(); i++)
	{
		if (s_freeSmallIds[i] == smallId) { alreadyFree = true; break; }
	}
	if (!alreadyFree)
		s_freeSmallIds.push_back(smallId);
	LeaveCriticalSection(&s_freeSmallIdLock);
}

void NativeDesktopNetLayer::CloseConnectionBySmallId(BYTE smallId)
{
	EnterCriticalSection(&s_connectionsLock);
	for (size_t i = 0; i < s_connections.size(); i++)
	{
		if (s_connections[i].smallId == smallId && s_connections[i].active && s_connections[i].tcpSocket != INVALID_SOCKET)
		{
			CloseNetSocket(s_connections[i].tcpSocket);
			s_connections[i].tcpSocket = INVALID_SOCKET;
			app.DebugPrintf("NativeDesktop LAN: Force-closed TCP connection for smallId=%d\n", smallId);
			break;
		}
	}
	LeaveCriticalSection(&s_connectionsLock);
}

BYTE NativeDesktopNetLayer::GetSplitScreenSmallId(int padIndex)
{
	if (padIndex <= 0 || padIndex >= XUSER_MAX_COUNT) return 0xFF;
	return s_splitScreenSmallId[padIndex];
}

SOCKET NativeDesktopNetLayer::GetLocalSocket(BYTE senderSmallId)
{
	if (senderSmallId == s_localSmallId)
		return s_hostConnectionSocket;
	for (int i = 1; i < XUSER_MAX_COUNT; i++)
	{
		if (s_splitScreenSmallId[i] == senderSmallId && s_splitScreenSocket[i] != INVALID_SOCKET)
			return s_splitScreenSocket[i];
	}
	return INVALID_SOCKET;
}

bool NativeDesktopNetLayer::JoinSplitScreen(int padIndex, BYTE* outSmallId)
{
	if (!s_active || s_isHost || padIndex <= 0 || padIndex >= XUSER_MAX_COUNT)
		return false;

	if (s_splitScreenSocket[padIndex] != INVALID_SOCKET)
	{
		return false;
	}

	char resolvedIp[64] = {};
	if (!LceNetResolveIpv4(
		g_NativeDesktopMultiplayerIP,
		g_NativeDesktopMultiplayerPort,
		resolvedIp,
		sizeof(resolvedIp)))
	{
		app.DebugPrintf("NativeDesktop LAN: Split-screen resolve failed for %s:%d\n", g_NativeDesktopMultiplayerIP, g_NativeDesktopMultiplayerPort);
		return false;
	}

	const LceSocketHandle splitHandle = LceNetOpenTcpSocket();
	SOCKET sock = static_cast<SOCKET>(splitHandle);
	if (splitHandle == LCE_INVALID_SOCKET)
	{
		return false;
	}

	LceNetSetSocketNoDelay(splitHandle, true);

	if (!LceNetConnectIpv4(splitHandle, resolvedIp, g_NativeDesktopMultiplayerPort))
	{
		app.DebugPrintf("NativeDesktop LAN: Split-screen connect() failed: %d\n", GetNetLastError());
		CloseNetSocket(sock);
		return false;
	}

	BYTE assignBuf[1];
	if (!RecvExact(sock, assignBuf, 1))
	{
		app.DebugPrintf("NativeDesktop LAN: Split-screen failed to receive smallId\n");
		CloseNetSocket(sock);
		return false;
	}

	if (assignBuf[0] == NATIVE_DESKTOP_SMALLID_REJECT)
	{
		BYTE rejectBuf[5];
		RecvExact(sock, rejectBuf, 5);
		app.DebugPrintf("NativeDesktop LAN: Split-screen connection rejected\n");
		CloseNetSocket(sock);
		return false;
	}

	BYTE assignedSmallId = assignBuf[0];
	s_splitScreenSocket[padIndex] = sock;
	s_splitScreenSmallId[padIndex] = assignedSmallId;
	*outSmallId = assignedSmallId;

	app.DebugPrintf("NativeDesktop LAN: Split-screen pad %d connected, assigned smallId=%d\n", padIndex, assignedSmallId);

	int* threadParam = new int;
	*threadParam = padIndex;
	s_splitScreenRecvThread[padIndex] = CreateThread(nullptr, 0, SplitScreenRecvThreadProc, threadParam, 0, nullptr);
	if (s_splitScreenRecvThread[padIndex] == nullptr)
	{
		delete threadParam;
		CloseNetSocket(sock);
		s_splitScreenSocket[padIndex] = INVALID_SOCKET;
		s_splitScreenSmallId[padIndex] = 0xFF;
		app.DebugPrintf("NativeDesktop LAN: CreateThread failed for split-screen pad %d\n", padIndex);
		return false;
	}

	return true;
}

void NativeDesktopNetLayer::CloseSplitScreenConnection(int padIndex)
{
	if (padIndex <= 0 || padIndex >= XUSER_MAX_COUNT) return;

	if (s_splitScreenSocket[padIndex] != INVALID_SOCKET)
	{
		CloseNetSocket(s_splitScreenSocket[padIndex]);
		s_splitScreenSocket[padIndex] = INVALID_SOCKET;
	}
	s_splitScreenSmallId[padIndex] = 0xFF;
	if (s_splitScreenRecvThread[padIndex] != nullptr)
	{
		WaitForSingleObject(s_splitScreenRecvThread[padIndex], 2000);
		CloseHandle(s_splitScreenRecvThread[padIndex]);
		s_splitScreenRecvThread[padIndex] = nullptr;
	}
}

DWORD WINAPI NativeDesktopNetLayer::SplitScreenRecvThreadProc(LPVOID param)
{
	int padIndex = *(int*)param;
	delete (int*)param;

	SOCKET sock = s_splitScreenSocket[padIndex];
	BYTE localSmallId = s_splitScreenSmallId[padIndex];
	std::vector<BYTE> recvBuf;
	recvBuf.resize(NATIVE_DESKTOP_NET_RECV_BUFFER_SIZE);

	while (s_active && s_splitScreenSocket[padIndex] != INVALID_SOCKET)
	{
		BYTE header[4];
		if (!RecvExact(sock, header, 4))
		{
			app.DebugPrintf("NativeDesktop LAN: Split-screen pad %d disconnected from host\n", padIndex);
			break;
		}

		int packetSize = ((uint32_t)header[0] << 24) | ((uint32_t)header[1] << 16) |
			((uint32_t)header[2] << 8) | ((uint32_t)header[3]);
		if (packetSize <= 0 || packetSize > NATIVE_DESKTOP_NET_MAX_PACKET_SIZE)
		{
			app.DebugPrintf("NativeDesktop LAN: Split-screen pad %d invalid packet size %d\n", padIndex, packetSize);
			break;
		}

		if ((int)recvBuf.size() < packetSize)
			recvBuf.resize(packetSize);

		if (!RecvExact(sock, &recvBuf[0], packetSize))
		{
			app.DebugPrintf("NativeDesktop LAN: Split-screen pad %d disconnected from host (body)\n", padIndex);
			break;
		}

		HandleDataReceived(s_hostSmallId, localSmallId, &recvBuf[0], packetSize);
	}

	EnterCriticalSection(&s_disconnectLock);
	s_disconnectedSmallIds.push_back(localSmallId);
	LeaveCriticalSection(&s_disconnectLock);

	return 0;
}

DWORD WINAPI NativeDesktopNetLayer::ClientRecvThreadProc(LPVOID param)
{
	std::vector<BYTE> recvBuf;
	recvBuf.resize(NATIVE_DESKTOP_NET_RECV_BUFFER_SIZE);

	while (s_active && s_hostConnectionSocket != INVALID_SOCKET)
	{
		BYTE header[4];
		if (!RecvExact(s_hostConnectionSocket, header, 4))
		{
			app.DebugPrintf("NativeDesktop LAN: Disconnected from host (header)\n");
			break;
		}

		int packetSize = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];

		if (packetSize <= 0 || packetSize > NATIVE_DESKTOP_NET_MAX_PACKET_SIZE)
		{
			app.DebugPrintf("NativeDesktop LAN: Invalid packet size %d from host (max=%d)\n",
				packetSize,
				(int)NATIVE_DESKTOP_NET_MAX_PACKET_SIZE);
			break;
		}

		if (static_cast<int>(recvBuf.size()) < packetSize)
		{
			recvBuf.resize(packetSize);
			app.DebugPrintf("NativeDesktop LAN: Resized client recv buffer to %d bytes\n", packetSize);
		}

		if (!RecvExact(s_hostConnectionSocket, &recvBuf[0], packetSize))
		{
			app.DebugPrintf("NativeDesktop LAN: Disconnected from host (body)\n");
			break;
		}

		HandleDataReceived(s_hostSmallId, s_localSmallId, &recvBuf[0], packetSize);
	}

	s_connected = false;
	return 0;
}

bool NativeDesktopNetLayer::StartAdvertising(int gamePort, const wchar_t* hostName, unsigned int gameSettings, unsigned int texPackId, unsigned char subTexId, unsigned short netVer)
{
	if (s_advertising) return true;
	if (!s_initialized) return false;

	EnterCriticalSection(&s_advertiseLock);
	const bool broadcastReady = LceLanBuildBroadcast(
		gamePort,
		hostName,
		gameSettings,
		texPackId,
		subTexId,
		netVer,
		1,
		MINECRAFT_NET_MAX_PLAYERS,
		false,
		&s_advertiseData);
	s_hostGamePort = gamePort;
	LeaveCriticalSection(&s_advertiseLock);
	if (!broadcastReady)
	{
		app.DebugPrintf("NativeDesktop LAN: Failed to build advertise payload for port %d\n",
			gamePort);
		return false;
	}

	const LceSocketHandle advertiseSocket = LceNetOpenUdpSocket();
	if (advertiseSocket == LCE_INVALID_SOCKET)
	{
		app.DebugPrintf("NativeDesktop LAN: Failed to create advertise socket: %d\n", GetNetLastError());
		return false;
	}
	s_advertiseSock = static_cast<SOCKET>(advertiseSocket);

	if (!LceNetSetSocketBroadcast(advertiseSocket, true))
	{
		app.DebugPrintf("NativeDesktop LAN: Failed to enable broadcast on advertise socket: %d\n",
			GetNetLastError());
		CloseNetSocket(s_advertiseSock);
		s_advertiseSock = INVALID_SOCKET;
		return false;
	}

	s_advertising = true;
	s_advertiseThread = CreateThread(nullptr, 0, AdvertiseThreadProc, nullptr, 0, nullptr);

	app.DebugPrintf("NativeDesktop LAN: Started advertising on UDP port %d\n",
		LCE_LAN_DISCOVERY_PORT);
	return true;
}

void NativeDesktopNetLayer::StopAdvertising()
{
	s_advertising = false;

	if (s_advertiseSock != INVALID_SOCKET)
	{
		CloseNetSocket(s_advertiseSock);
		s_advertiseSock = INVALID_SOCKET;
	}

	if (s_advertiseThread != nullptr)
	{
		WaitForSingleObject(s_advertiseThread, 2000);
		CloseHandle(s_advertiseThread);
		s_advertiseThread = nullptr;
	}
}

void NativeDesktopNetLayer::UpdateAdvertisePlayerCount(BYTE count)
{
	EnterCriticalSection(&s_advertiseLock);
	s_advertiseData.playerCount = count;
	LeaveCriticalSection(&s_advertiseLock);
}

void NativeDesktopNetLayer::UpdateAdvertiseMaxPlayers(BYTE maxPlayers)
{
	EnterCriticalSection(&s_advertiseLock);
	s_advertiseData.maxPlayers = maxPlayers;
	LeaveCriticalSection(&s_advertiseLock);
}

void NativeDesktopNetLayer::UpdateAdvertiseJoinable(bool joinable)
{
	EnterCriticalSection(&s_advertiseLock);
	s_advertiseData.isJoinable = joinable ? 1 : 0;
	LeaveCriticalSection(&s_advertiseLock);
}

DWORD WINAPI NativeDesktopNetLayer::AdvertiseThreadProc(LPVOID param)
{
	while (s_advertising)
	{
		EnterCriticalSection(&s_advertiseLock);
		NativeDesktopLANBroadcast data = s_advertiseData;
		LeaveCriticalSection(&s_advertiseLock);

		const int sent = LceNetSendToIpv4(
			static_cast<LceSocketHandle>(s_advertiseSock),
			"255.255.255.255",
			LCE_LAN_DISCOVERY_PORT,
			&data,
			static_cast<int>(sizeof(data)));

		if (sent < 0 && s_advertising)
		{
			app.DebugPrintf("NativeDesktop LAN: Broadcast sendto failed: %d\n", GetNetLastError());
		}

		Sleep(1000);
	}

	return 0;
}

bool NativeDesktopNetLayer::StartDiscovery()
{
	if (s_discovering) return true;
	if (!s_initialized) return false;

	const LceSocketHandle discoverySocket = LceNetOpenUdpSocket();
	if (discoverySocket == LCE_INVALID_SOCKET)
	{
		app.DebugPrintf("NativeDesktop LAN: Failed to create discovery socket: %d\n", GetNetLastError());
		return false;
	}
	s_discoverySock = static_cast<SOCKET>(discoverySocket);

	if (!LceNetSetSocketReuseAddress(discoverySocket, true) ||
		!LceNetBindIpv4(discoverySocket, nullptr, LCE_LAN_DISCOVERY_PORT))
	{
		app.DebugPrintf("NativeDesktop LAN: Discovery bind failed: %d\n", GetNetLastError());
		CloseNetSocket(s_discoverySock);
		s_discoverySock = INVALID_SOCKET;
		return false;
	}

	if (!LceNetSetSocketRecvTimeout(discoverySocket, 500))
	{
		app.DebugPrintf("NativeDesktop LAN: Discovery timeout setup failed: %d\n",
			GetNetLastError());
		CloseNetSocket(s_discoverySock);
		s_discoverySock = INVALID_SOCKET;
		return false;
	}

	s_discovering = true;
	s_discoveryThread = CreateThread(nullptr, 0, DiscoveryThreadProc, nullptr, 0, nullptr);

	app.DebugPrintf("NativeDesktop LAN: Listening for LAN games on UDP port %d\n",
		LCE_LAN_DISCOVERY_PORT);
	return true;
}

void NativeDesktopNetLayer::StopDiscovery()
{
	s_discovering = false;

	if (s_discoverySock != INVALID_SOCKET)
	{
		CloseNetSocket(s_discoverySock);
		s_discoverySock = INVALID_SOCKET;
	}

	if (s_discoveryThread != nullptr)
	{
		WaitForSingleObject(s_discoveryThread, 2000);
		CloseHandle(s_discoveryThread);
		s_discoveryThread = nullptr;
	}

	EnterCriticalSection(&s_discoveryLock);
	s_discoveredSessions.clear();
	LeaveCriticalSection(&s_discoveryLock);
}

std::vector<NativeDesktopLANSession> NativeDesktopNetLayer::GetDiscoveredSessions()
{
	std::vector<NativeDesktopLANSession> result;
	EnterCriticalSection(&s_discoveryLock);
	result = s_discoveredSessions;
	LeaveCriticalSection(&s_discoveryLock);
	return result;
}

DWORD WINAPI NativeDesktopNetLayer::DiscoveryThreadProc(LPVOID param)
{
	char recvBuf[512];

	while (s_discovering)
	{
		char senderIP[64] = {};
		int senderPort = 0;
		const int recvLen = LceNetRecvFromIpv4(
			static_cast<LceSocketHandle>(s_discoverySock),
			recvBuf,
			sizeof(recvBuf),
			senderIP,
			sizeof(senderIP),
			&senderPort);

		if (recvLen < 0)
		{
			continue;
		}

		if (recvLen < static_cast<int>(sizeof(NativeDesktopLANBroadcast)))
			continue;

		const std::uint64_t nowMs = LceGetMonotonicMilliseconds();
		NativeDesktopLANSession session = {};
		if (!LceLanDecodeBroadcast(
			recvBuf,
			static_cast<std::size_t>(recvLen),
			senderIP,
			nowMs,
			&session))
		{
			continue;
		}

		EnterCriticalSection(&s_discoveryLock);
		bool addedSession = false;
		LceLanUpsertSession(session, &s_discoveredSessions, &addedSession);
		if (addedSession)
		{
			app.DebugPrintf("NativeDesktop LAN: Discovered game \"%ls\" at %s:%d\n",
				session.hostName, session.hostIP, session.hostPort);
		}
		std::vector<NativeDesktopLANSession> expiredSessions;
		LceLanPruneExpiredSessions(
			nowMs,
			LCE_LAN_SESSION_TIMEOUT_MS,
			&s_discoveredSessions,
			&expiredSessions);
		for (size_t i = 0; i < expiredSessions.size(); ++i)
		{
			app.DebugPrintf("NativeDesktop LAN: Session \"%ls\" at %s timed out\n",
				expiredSessions[i].hostName, expiredSessions[i].hostIP);
		}
		LeaveCriticalSection(&s_discoveryLock);
	}

	return 0;
}

#endif
