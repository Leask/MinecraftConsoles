#include "stdafx.h"

#include <algorithm>
#include <mutex>
#include <unordered_map>

#include "../Connection.h"
#include "../ConsoleSaveFileOriginal.h"
#include "../EffectCommand.h"
#include "../EnchantItemCommand.h"
#include "../EntityEvent.h"
#include "../ExperienceCommand.h"
#include "../File.h"
#include "../GiveItemCommand.h"
#include "../InputOutputStream.h"
#include "../Level.h"
#include "../PotionBrewing.h"
#include "../QuartzBlockTile.h"
#include "../SharedConstants.h"
#include "../SkullTileEntity.h"
#include "../Socket.h"
#include "../TallGrass.h"
#include "../TimeCommand.h"
#include "../ToggleDownfallCommand.h"
#include "../net.minecraft.network.packet.h"
#include "../net.minecraft.world.level.tile.h"
#include "../../Minecraft.Client/ServerConnection.h"

namespace
{
    struct NativeSaveFileData
    {
        std::vector<FileEntry*> files;
    std::unordered_map<FileEntry*, std::vector<BYTE>> data;
    };

    std::unordered_map<ConsoleSaveFileOriginal*, NativeSaveFileData> g_saveData;
    std::unordered_map<Socket*, INetworkPlayer*> g_socketPlayers;
    INetworkPlayer* g_hostSocketPlayer = nullptr;

    std::mutex& SocketPlayerMutex()
    {
        static std::mutex* mutex = new std::mutex();
        return *mutex;
    }

    NativeSaveFileData& SaveData(ConsoleSaveFileOriginal* save)
    {
        return g_saveData[save];
    }
}

unsigned int ConsoleSaveFileOriginal::pagesCommitted = 0;
void* ConsoleSaveFileOriginal::pvHeap = nullptr;

#define DEFINE_NATIVE_STATIC(Member) decltype(Member) Member

DEFINE_NATIVE_STATIC(EntityEvent::USE_ITEM_COMPLETE);
DEFINE_NATIVE_STATIC(AnimatePacket::SWING);
DEFINE_NATIVE_STATIC(AnimatePacket::WAKE_UP);
DEFINE_NATIVE_STATIC(AnimatePacket::EAT);
DEFINE_NATIVE_STATIC(AnimatePacket::CRITICAL_HIT);
DEFINE_NATIVE_STATIC(AnimatePacket::MAGIC_CRITICAL_HIT);
DEFINE_NATIVE_STATIC(AddEntityPacket::ITEM);
DEFINE_NATIVE_STATIC(AddEntityPacket::ARROW);
DEFINE_NATIVE_STATIC(AddEntityPacket::SNOWBALL);
DEFINE_NATIVE_STATIC(AddEntityPacket::FIREBALL);
DEFINE_NATIVE_STATIC(AddEntityPacket::EGG);
DEFINE_NATIVE_STATIC(AddEntityPacket::THROWN_EXPBOTTLE);
DEFINE_NATIVE_STATIC(AddEntityPacket::THROWN_ENDERPEARL);
DEFINE_NATIVE_STATIC(AddEntityPacket::EYEOFENDERSIGNAL);
DEFINE_NATIVE_STATIC(AddEntityPacket::THROWN_POTION);
DEFINE_NATIVE_STATIC(AddEntityPacket::BOAT);
DEFINE_NATIVE_STATIC(AddEntityPacket::MINECART);
DEFINE_NATIVE_STATIC(AddEntityPacket::PRIMED_TNT);
DEFINE_NATIVE_STATIC(AddEntityPacket::FALLING);
DEFINE_NATIVE_STATIC(AddEntityPacket::ITEM_FRAME);
DEFINE_NATIVE_STATIC(AddEntityPacket::LEASH_KNOT);
DEFINE_NATIVE_STATIC(AddEntityPacket::ENDER_CRYSTAL);
DEFINE_NATIVE_STATIC(AddEntityPacket::FIREWORKS);
DEFINE_NATIVE_STATIC(AddEntityPacket::FISH_HOOK);
DEFINE_NATIVE_STATIC(SharedConstants::NETWORK_PROTOCOL_VERSION);
DEFINE_NATIVE_STATIC(PotionBrewing::POTION_ID_SPLASH_DAMAGE);
DEFINE_NATIVE_STATIC(QuartzBlockTile::TYPE_LINES_Y);
DEFINE_NATIVE_STATIC(SkullTileEntity::TYPE_WITHER);
DEFINE_NATIVE_STATIC(ClientCommandPacket::PERFORM_RESPAWN);
DEFINE_NATIVE_STATIC(ContainerOpenPacket::CONTAINER);
DEFINE_NATIVE_STATIC(ContainerOpenPacket::WORKBENCH);
DEFINE_NATIVE_STATIC(ContainerOpenPacket::FURNACE);
DEFINE_NATIVE_STATIC(ContainerOpenPacket::TRAP);
DEFINE_NATIVE_STATIC(ContainerOpenPacket::ENCHANTMENT);
DEFINE_NATIVE_STATIC(ContainerOpenPacket::BREWING_STAND);
DEFINE_NATIVE_STATIC(ContainerOpenPacket::TRADER_NPC);
DEFINE_NATIVE_STATIC(ContainerOpenPacket::BEACON);
DEFINE_NATIVE_STATIC(ContainerOpenPacket::REPAIR_TABLE);
DEFINE_NATIVE_STATIC(ContainerOpenPacket::HOPPER);
DEFINE_NATIVE_STATIC(ContainerOpenPacket::DROPPER);
DEFINE_NATIVE_STATIC(ContainerOpenPacket::HORSE);
DEFINE_NATIVE_STATIC(ContainerOpenPacket::FIREWORKS);
DEFINE_NATIVE_STATIC(SetEntityLinkPacket::RIDING);
DEFINE_NATIVE_STATIC(SetEntityLinkPacket::LEASH);
DEFINE_NATIVE_STATIC(TileEditorOpenPacket::SIGN);
DEFINE_NATIVE_STATIC(TileEntityDataPacket::TYPE_MOB_SPAWNER);
DEFINE_NATIVE_STATIC(TileEntityDataPacket::TYPE_ADV_COMMAND);
DEFINE_NATIVE_STATIC(TileEntityDataPacket::TYPE_BEACON);
DEFINE_NATIVE_STATIC(TileEntityDataPacket::TYPE_SKULL);
DEFINE_NATIVE_STATIC(Level::maxBuildHeight);
DEFINE_NATIVE_STATIC(Sapling::TYPE_DEFAULT);
DEFINE_NATIVE_STATIC(Sapling::TYPE_EVERGREEN);
DEFINE_NATIVE_STATIC(Sapling::TYPE_BIRCH);
DEFINE_NATIVE_STATIC(Sapling::TYPE_JUNGLE);
DEFINE_NATIVE_STATIC(ChestTile::TYPE_TRAP);
DEFINE_NATIVE_STATIC(TallGrass::FERN);

#undef DEFINE_NATIVE_STATIC

EGameCommand TimeCommand::getId()
{
    return eGameCommand_Time;
}

int TimeCommand::getPermissionLevel()
{
    return 2;
}

void TimeCommand::execute(shared_ptr<CommandSender> source, byteArray commandData)
{
    (void)source;
    (void)commandData;
}

void TimeCommand::doSetTime(shared_ptr<CommandSender> source, int value)
{
    (void)source;
    (void)value;
}

void TimeCommand::doAddTime(shared_ptr<CommandSender> source, int value)
{
    (void)source;
    (void)value;
}

shared_ptr<GameCommandPacket> TimeCommand::preparePacket(bool night)
{
    (void)night;
    return std::make_shared<GameCommandPacket>(eGameCommand_Time, byteArray());
}

EGameCommand GiveItemCommand::getId()
{
    return eGameCommand_Give;
}

int GiveItemCommand::getPermissionLevel()
{
    return 2;
}

void GiveItemCommand::execute(shared_ptr<CommandSender> source,
                              byteArray commandData)
{
    (void)source;
    (void)commandData;
}

shared_ptr<GameCommandPacket> GiveItemCommand::preparePacket(
    shared_ptr<Player> player,
    int item,
    int amount,
    int aux,
    const wstring& tag)
{
    (void)player;
    (void)item;
    (void)amount;
    (void)aux;
    (void)tag;
    return std::make_shared<GameCommandPacket>(eGameCommand_Give, byteArray());
}

EGameCommand ExperienceCommand::getId()
{
    return eGameCommand_Experience;
}

int ExperienceCommand::getPermissionLevel()
{
    return 2;
}

void ExperienceCommand::execute(shared_ptr<CommandSender> source,
                                byteArray commandData)
{
    (void)source;
    (void)commandData;
}

EGameCommand EnchantItemCommand::getId()
{
    return eGameCommand_EnchantItem;
}

int EnchantItemCommand::getPermissionLevel()
{
    return 2;
}

void EnchantItemCommand::execute(shared_ptr<CommandSender> source,
                                 byteArray commandData)
{
    (void)source;
    (void)commandData;
}

shared_ptr<GameCommandPacket> EnchantItemCommand::preparePacket(
    shared_ptr<Player> player,
    int enchantmentId,
    int enchantmentLevel)
{
    (void)player;
    (void)enchantmentId;
    (void)enchantmentLevel;
    return std::make_shared<GameCommandPacket>(
        eGameCommand_EnchantItem,
        byteArray());
}

EGameCommand ToggleDownfallCommand::getId()
{
    return eGameCommand_ToggleDownfall;
}

int ToggleDownfallCommand::getPermissionLevel()
{
    return 2;
}

void ToggleDownfallCommand::execute(shared_ptr<CommandSender> source,
                                    byteArray commandData)
{
    (void)source;
    (void)commandData;
}

void ToggleDownfallCommand::doToggleDownfall()
{
}

shared_ptr<GameCommandPacket> ToggleDownfallCommand::preparePacket()
{
    return std::make_shared<GameCommandPacket>(
        eGameCommand_ToggleDownfall,
        byteArray());
}

EGameCommand EffectCommand::getId()
{
    return eGameCommand_Effect;
}

int EffectCommand::getPermissionLevel()
{
    return 2;
}

wstring EffectCommand::getUsage(CommandSender* source)
{
    (void)source;
    return L"";
}

void EffectCommand::execute(shared_ptr<CommandSender> source,
                            byteArray commandData)
{
    (void)source;
    (void)commandData;
}

wstring EffectCommand::getPlayerNames()
{
    return L"";
}

bool EffectCommand::isValidWildcardPlayerArgument(wstring args,
                                                  int argumentIndex)
{
    (void)args;
    (void)argumentIndex;
    return false;
}

ConsoleSaveFileOriginal::ConsoleSaveFileOriginal(
    const wstring& fileName,
    LPVOID pvSaveData,
    DWORD fileSize,
    bool forceCleanSave,
    ESavePlatform plat)
{
    (void)pvSaveData;
    (void)fileSize;
    (void)forceCleanSave;
    InitializeCriticalSectionAndSpinCount(&m_lock, 5120);
    m_fileName = fileName;
    pvSaveMem = nullptr;
    header.setPlatform(plat);
}

ConsoleSaveFileOriginal::~ConsoleSaveFileOriginal()
{
    auto found = g_saveData.find(this);
    if (found != g_saveData.end())
    {
        for (FileEntry* file : found->second.files)
        {
            delete file;
        }
        g_saveData.erase(found);
    }
    DeleteCriticalSection(&m_lock);
}

FileEntry* ConsoleSaveFileOriginal::createFile(const ConsoleSavePath& fileName)
{
    FileEntry* file = new FileEntry();
    wcsncpy_s(file->data.filename, fileName.getName().c_str(), _TRUNCATE);
    file->data.length = 0;
    file->data.startOffset = 0;
    file->currentFilePointer = 0;
    SaveData(this).files.push_back(file);
    SaveData(this).data[file] = {};
    return file;
}

void ConsoleSaveFileOriginal::deleteFile(FileEntry* file)
{
    NativeSaveFileData& save = SaveData(this);
    save.data.erase(file);
    save.files.erase(
        std::remove(save.files.begin(), save.files.end(), file),
        save.files.end());
    delete file;
}

void ConsoleSaveFileOriginal::setFilePointer(
    FileEntry* file,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod)
{
    (void)lpDistanceToMoveHigh;
    if (file == nullptr)
    {
        return;
    }

    LONG base = 0;
    if (dwMoveMethod == FILE_CURRENT)
    {
        base = static_cast<LONG>(file->currentFilePointer);
    }
    else if (dwMoveMethod == FILE_END)
    {
        base = static_cast<LONG>(file->data.length);
    }

    const LONG next = std::max<LONG>(0, base + lDistanceToMove);
    file->currentFilePointer = static_cast<unsigned int>(next);
}

BOOL ConsoleSaveFileOriginal::writeFile(
    FileEntry* file,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten)
{
    if (lpNumberOfBytesWritten != nullptr)
    {
        *lpNumberOfBytesWritten = 0;
    }
    if (file == nullptr || lpBuffer == nullptr)
    {
        return FALSE;
    }

    std::vector<BYTE>& bytes = SaveData(this).data[file];
    const size_t end = file->currentFilePointer + nNumberOfBytesToWrite;
    if (bytes.size() < end)
    {
        bytes.resize(end);
    }
    std::memcpy(
        bytes.data() + file->currentFilePointer,
        lpBuffer,
        nNumberOfBytesToWrite);
    file->currentFilePointer += nNumberOfBytesToWrite;
    file->data.length = static_cast<unsigned int>(bytes.size());
    file->updateLastModifiedTime();
    if (lpNumberOfBytesWritten != nullptr)
    {
        *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
    }
    return TRUE;
}

BOOL ConsoleSaveFileOriginal::zeroFile(
    FileEntry* file,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten)
{
    std::vector<BYTE> zeroes(nNumberOfBytesToWrite);
    return writeFile(
        file,
        zeroes.data(),
        nNumberOfBytesToWrite,
        lpNumberOfBytesWritten);
}

BOOL ConsoleSaveFileOriginal::readFile(
    FileEntry* file,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead)
{
    if (lpNumberOfBytesRead != nullptr)
    {
        *lpNumberOfBytesRead = 0;
    }
    if (file == nullptr || lpBuffer == nullptr)
    {
        return FALSE;
    }

    std::vector<BYTE>& bytes = SaveData(this).data[file];
    if (file->currentFilePointer >= bytes.size())
    {
        return TRUE;
    }

    const size_t available = bytes.size() - file->currentFilePointer;
    const size_t toRead = std::min<size_t>(available, nNumberOfBytesToRead);
    std::memcpy(lpBuffer, bytes.data() + file->currentFilePointer, toRead);
    file->currentFilePointer += static_cast<unsigned int>(toRead);
    if (lpNumberOfBytesRead != nullptr)
    {
        *lpNumberOfBytesRead = static_cast<DWORD>(toRead);
    }
    return TRUE;
}

BOOL ConsoleSaveFileOriginal::closeHandle(FileEntry* file)
{
    (void)file;
    return TRUE;
}

void ConsoleSaveFileOriginal::finalizeWrite()
{
}

bool ConsoleSaveFileOriginal::doesFileExist(ConsoleSavePath file)
{
    NativeSaveFileData& save = SaveData(this);
    for (FileEntry* entry : save.files)
    {
        if (file.getName() == entry->data.filename)
        {
            return true;
        }
    }
    return false;
}

void ConsoleSaveFileOriginal::Flush(bool autosave, bool updateThumbnail)
{
    (void)autosave;
    (void)updateThumbnail;
}

#ifndef _CONTENT_PACKAGE
void ConsoleSaveFileOriginal::DebugFlushToFile(
    void* compressedData,
    unsigned int compressedDataSize)
{
    (void)compressedData;
    (void)compressedDataSize;
}
#endif

unsigned int ConsoleSaveFileOriginal::getSizeOnDisk()
{
    unsigned int total = 0;
    for (const auto& entry : SaveData(this).data)
    {
        total += static_cast<unsigned int>(entry.second.size());
    }
    return total;
}

wstring ConsoleSaveFileOriginal::getFilename()
{
    return m_fileName;
}

vector<FileEntry*>* ConsoleSaveFileOriginal::getFilesWithPrefix(
    const wstring& prefix)
{
    vector<FileEntry*>* files = new vector<FileEntry*>();
    for (FileEntry* file : SaveData(this).files)
    {
        if (wstring(file->data.filename).find(prefix) == 0)
        {
            files->push_back(file);
        }
    }
    return files;
}

vector<FileEntry*>* ConsoleSaveFileOriginal::getRegionFilesByDimension(
    unsigned int dimensionIndex)
{
    (void)dimensionIndex;
    return new vector<FileEntry*>();
}

int ConsoleSaveFileOriginal::getSaveVersion()
{
    return SAVE_FILE_VERSION_NUMBER;
}

int ConsoleSaveFileOriginal::getOriginalSaveVersion()
{
    return SAVE_FILE_VERSION_NUMBER;
}

void ConsoleSaveFileOriginal::LockSaveAccess()
{
    EnterCriticalSection(&m_lock);
}

void ConsoleSaveFileOriginal::ReleaseSaveAccess()
{
    LeaveCriticalSection(&m_lock);
}

ESavePlatform ConsoleSaveFileOriginal::getSavePlatform()
{
    return header.getSavePlatform();
}

bool ConsoleSaveFileOriginal::isSaveEndianDifferent()
{
    return false;
}

void ConsoleSaveFileOriginal::setLocalPlatform()
{
    header.setLocalPlatform();
}

void ConsoleSaveFileOriginal::setPlatform(ESavePlatform plat)
{
    header.setPlatform(plat);
}

ByteOrder ConsoleSaveFileOriginal::getSaveEndian()
{
    return header.getSaveEndian();
}

ByteOrder ConsoleSaveFileOriginal::getLocalEndian()
{
    return FileHeader::getLocalEndian();
}

void ConsoleSaveFileOriginal::setEndian(ByteOrder endian)
{
    header.setEndian(endian);
}

bool ConsoleSaveFileOriginal::isLocalEndianDifferent(ESavePlatform plat)
{
    return header.isLocalEndianDifferent(plat);
}

void ConsoleSaveFileOriginal::ConvertRegionFile(File sourceFile)
{
    (void)sourceFile;
}

void ConsoleSaveFileOriginal::ConvertToLocalPlatform()
{
    header.setLocalPlatform();
}

void* ConsoleSaveFileOriginal::getWritePointer(FileEntry* file)
{
    if (file == nullptr)
    {
        return nullptr;
    }

    std::vector<BYTE>& bytes = SaveData(this).data[file];
    if (bytes.empty())
    {
        return nullptr;
    }
    return bytes.data();
}

int Connection::readThreads = 0;
int Connection::writeThreads = 0;
int Connection::readSizes[256] = {};
int Connection::writeSizes[256] = {};

Connection::Connection(
    Socket* socketValue,
    const wstring& id,
    PacketListener* listener)
    : socket(socketValue),
      address(nullptr),
      dis(nullptr),
      bufferedDos(nullptr),
      byteArrayDos(nullptr),
      baos(nullptr),
      sos(nullptr),
      running(true),
      packetListener(listener),
      quitting(false),
      readThread(nullptr),
      writeThread(nullptr),
      m_hWakeReadThread(nullptr),
      m_hWakeWriteThread(nullptr),
      saqThreadID(0),
      closeThreadID(0),
      disconnected(false),
      disconnectReason(DisconnectPacket::eDisconnect_Quitting),
      disconnectReasonObjects(nullptr),
      noInputTicks(0),
      estimatedRemaining(0),
      tickCount(0),
      fakeLag(0),
      slowWriteDelay(0)
{
    InitializeCriticalSectionAndSpinCount(&incoming_cs, 5120);
    InitializeCriticalSectionAndSpinCount(&threadCounterLock, 5120);
    InitializeCriticalSectionAndSpinCount(&writeLock, 5120);

    (void)id;
    if (socket != nullptr && packetListener != nullptr)
    {
        socket->setSoTimeout(30000);
        socket->setTrafficClass(IPTOS_THROUGHPUT | IPTOS_LOWDELAY);

        bool serverListener = packetListener->isServerPacketListener();
        dis = new DataInputStream(socket->getInputStream(serverListener));
        sos = socket->getOutputStream(serverListener);
        bufferedDos = new DataOutputStream(
            new BufferedOutputStream(sos, SEND_BUFFER_SIZE));
        baos = new ByteArrayOutputStream(SEND_BUFFER_SIZE);
        byteArrayDos = new DataOutputStream(baos);
    }
}

Connection::~Connection()
{
    running = false;
    if (dis != nullptr)
    {
        dis->close();
        delete dis;
        dis = nullptr;
    }
    if (bufferedDos != nullptr)
    {
        bufferedDos->close();
        bufferedDos->deleteChildStream();
        delete bufferedDos;
        bufferedDos = nullptr;
    }
    delete byteArrayDos;
    byteArrayDos = nullptr;
    delete baos;
    baos = nullptr;

    DeleteCriticalSection(&incoming_cs);
    DeleteCriticalSection(&threadCounterLock);
    DeleteCriticalSection(&writeLock);
}

void Connection::_init()
{
}

void Connection::setListener(PacketListener* listener)
{
    packetListener = listener;
}

void Connection::send(shared_ptr<Packet> packet)
{
    queueSend(packet);
}

void Connection::queueSend(shared_ptr<Packet> packet)
{
    if (quitting || packet == nullptr)
    {
        return;
    }
    EnterCriticalSection(&writeLock);
    estimatedRemaining += packet->getEstimatedSize() + 1;
    outgoing.push(packet);
    LeaveCriticalSection(&writeLock);
}

bool Connection::writeTick()
{
    if (!running || quitting || bufferedDos == nullptr)
    {
        return false;
    }

    shared_ptr<Packet> packet;
    EnterCriticalSection(&writeLock);
    if (!outgoing.empty())
    {
        packet = outgoing.front();
        outgoing.pop();
    }
    else if (!outgoing_slow.empty())
    {
        packet = outgoing_slow.front();
        outgoing_slow.pop();
    }
    if (packet != nullptr)
    {
        estimatedRemaining -= packet->getEstimatedSize() + 1;
    }
    LeaveCriticalSection(&writeLock);

    if (packet == nullptr)
    {
        return false;
    }

    if (packet->shouldDelay && byteArrayDos != nullptr && baos != nullptr &&
        sos != nullptr)
    {
        packet->shouldDelay = false;
        baos->reset();
        Packet::writePacket(packet, byteArrayDos);
        sos->writeWithFlags(
            baos->buf,
            0,
            baos->size(),
            0);
    }
    else
    {
        Packet::writePacket(packet, bufferedDos);
        bufferedDos->flush();
    }

    writeSizes[packet->getId()] += packet->getEstimatedSize() + 1;
    return true;
}

void Connection::flush()
{
    while (writeTick())
    {
    }
    if (bufferedDos != nullptr)
    {
        bufferedDos->flush();
    }
}

bool Connection::readTick()
{
    if (!running || dis == nullptr || packetListener == nullptr)
    {
        return false;
    }

    shared_ptr<Packet> packet =
        Packet::readPacket(dis, packetListener->isServerPacketListener());
    if (packet == nullptr)
    {
        return false;
    }

    readSizes[packet->getId()] += packet->getEstimatedSize() + 1;
    EnterCriticalSection(&incoming_cs);
    if (!quitting)
    {
        incoming.push(packet);
    }
    LeaveCriticalSection(&incoming_cs);
    return true;
}

void Connection::close(DisconnectPacket::eDisconnectReason reason, ...)
{
    if (!running && disconnected)
    {
        return;
    }

    disconnectReason = reason;
    disconnected = true;
    running = false;

    if (dis != nullptr)
    {
        dis->close();
        delete dis;
        dis = nullptr;
    }
    if (bufferedDos != nullptr)
    {
        bufferedDos->close();
        bufferedDos->deleteChildStream();
        delete bufferedDos;
        bufferedDos = nullptr;
    }
    delete byteArrayDos;
    byteArrayDos = nullptr;
    delete baos;
    baos = nullptr;
    if (socket != nullptr)
    {
        socket->close(packetListener != nullptr &&
            packetListener->isServerPacketListener());
        socket = nullptr;
    }
}

void Connection::tick()
{
    if (estimatedRemaining > 1024 * 1024)
    {
        close(DisconnectPacket::eDisconnect_Overflow);
    }

    ++tickCount;
    if (tickCount % 20 == 0)
    {
        send(std::make_shared<KeepAlivePacket>());
    }

    flush();

    int readCount = 0;
    while (readTick() && readCount++ < 1000)
    {
    }

    std::vector<shared_ptr<Packet>> packetsToHandle;
    EnterCriticalSection(&incoming_cs);
    while (!disconnected && !incoming.empty())
    {
        packetsToHandle.push_back(incoming.front());
        incoming.pop();
    }
    LeaveCriticalSection(&incoming_cs);

    for (size_t i = 0; i < packetsToHandle.size(); ++i)
    {
        packetsToHandle[i]->handle(packetListener);
    }

    if (socket != nullptr && socket->isClosing())
    {
        close(DisconnectPacket::eDisconnect_Closed);
    }

    if (disconnected && packetListener != nullptr)
    {
        EnterCriticalSection(&incoming_cs);
        bool empty = incoming.empty();
        LeaveCriticalSection(&incoming_cs);
        if (empty)
        {
            packetListener->onDisconnect(
                disconnectReason,
                disconnectReasonObjects);
            disconnected = false;
        }
    }
}

SocketAddress* Connection::getRemoteAddress()
{
    return nullptr;
}

void Connection::sendAndQuit()
{
    quitting = true;
    close(DisconnectPacket::eDisconnect_Closed);
}

int Connection::countDelayedPackets()
{
    return static_cast<int>(outgoing_slow.size());
}

int Connection::runRead(void* lpParam)
{
    (void)lpParam;
    return 0;
}

int Connection::runWrite(void* lpParam)
{
    (void)lpParam;
    return 0;
}

int Connection::runClose(void* lpParam)
{
    (void)lpParam;
    return 0;
}

int Connection::runSendAndQuit(void* lpParam)
{
    (void)lpParam;
    return 0;
}

CRITICAL_SECTION Socket::s_hostQueueLock[2];
std::queue<byte> Socket::s_hostQueue[2];
Socket::SocketOutputStreamLocal* Socket::s_hostOutStream[2] = {};
Socket::SocketInputStreamLocal* Socket::s_hostInStream[2] = {};
ServerConnection* Socket::s_serverConnection = nullptr;

Socket::SocketInputStreamLocal::SocketInputStreamLocal(int queueIdx)
    : m_streamOpen(true), m_queueIdx(queueIdx)
{
}

int Socket::SocketInputStreamLocal::read()
{
    if (!m_streamOpen)
    {
        return -1;
    }

    int result = -1;
    EnterCriticalSection(&s_hostQueueLock[m_queueIdx]);
    if (!s_hostQueue[m_queueIdx].empty())
    {
        result = s_hostQueue[m_queueIdx].front();
        s_hostQueue[m_queueIdx].pop();
    }
    LeaveCriticalSection(&s_hostQueueLock[m_queueIdx]);
    return result;
}

int Socket::SocketInputStreamLocal::read(byteArray b)
{
    return read(b, 0, b.length);
}

int Socket::SocketInputStreamLocal::read(
    byteArray b,
    unsigned int offset,
    unsigned int length)
{
    if (!m_streamOpen)
    {
        return -1;
    }

    EnterCriticalSection(&s_hostQueueLock[m_queueIdx]);
    if (s_hostQueue[m_queueIdx].size() < length)
    {
        LeaveCriticalSection(&s_hostQueueLock[m_queueIdx]);
        return -1;
    }
    for (unsigned int i = 0; i < length; ++i)
    {
        b[offset + i] = s_hostQueue[m_queueIdx].front();
        s_hostQueue[m_queueIdx].pop();
    }
    LeaveCriticalSection(&s_hostQueueLock[m_queueIdx]);
    return static_cast<int>(length);
}

void Socket::SocketInputStreamLocal::close()
{
    m_streamOpen = false;
    EnterCriticalSection(&s_hostQueueLock[m_queueIdx]);
    std::queue<byte> empty;
    std::swap(s_hostQueue[m_queueIdx], empty);
    LeaveCriticalSection(&s_hostQueueLock[m_queueIdx]);
}

Socket::SocketOutputStreamLocal::SocketOutputStreamLocal(int queueIdx)
    : m_streamOpen(true), m_queueIdx(queueIdx)
{
}

void Socket::SocketOutputStreamLocal::write(unsigned int b)
{
    if (!m_streamOpen)
    {
        return;
    }

    EnterCriticalSection(&s_hostQueueLock[m_queueIdx]);
    s_hostQueue[m_queueIdx].push(static_cast<byte>(b));
    LeaveCriticalSection(&s_hostQueueLock[m_queueIdx]);
}

void Socket::SocketOutputStreamLocal::write(byteArray b)
{
    write(b, 0, b.length);
}

void Socket::SocketOutputStreamLocal::write(
    byteArray b,
    unsigned int offset,
    unsigned int length)
{
    if (!m_streamOpen)
    {
        return;
    }

    EnterCriticalSection(&s_hostQueueLock[m_queueIdx]);
    for (unsigned int i = 0; i < length; ++i)
    {
        s_hostQueue[m_queueIdx].push(b[offset + i]);
    }
    LeaveCriticalSection(&s_hostQueueLock[m_queueIdx]);
}

void Socket::SocketOutputStreamLocal::close()
{
    m_streamOpen = false;
    EnterCriticalSection(&s_hostQueueLock[m_queueIdx]);
    std::queue<byte> empty;
    std::swap(s_hostQueue[m_queueIdx], empty);
    LeaveCriticalSection(&s_hostQueueLock[m_queueIdx]);
}

Socket::SocketInputStreamNetwork::SocketInputStreamNetwork(
    Socket* socketValue,
    int queueIdx)
    : m_streamOpen(true), m_queueIdx(queueIdx), m_socket(socketValue)
{
}

int Socket::SocketInputStreamNetwork::read()
{
    if (!m_streamOpen || m_socket == nullptr)
    {
        return -1;
    }

    int result = -1;
    EnterCriticalSection(&m_socket->m_queueLockNetwork[m_queueIdx]);
    if (!m_socket->m_queueNetwork[m_queueIdx].empty())
    {
        result = m_socket->m_queueNetwork[m_queueIdx].front();
        m_socket->m_queueNetwork[m_queueIdx].pop();
    }
    LeaveCriticalSection(&m_socket->m_queueLockNetwork[m_queueIdx]);
    return result;
}

int Socket::SocketInputStreamNetwork::read(byteArray b)
{
    return read(b, 0, b.length);
}

int Socket::SocketInputStreamNetwork::read(
    byteArray b,
    unsigned int offset,
    unsigned int length)
{
    if (!m_streamOpen || m_socket == nullptr)
    {
        return -1;
    }

    EnterCriticalSection(&m_socket->m_queueLockNetwork[m_queueIdx]);
    if (m_socket->m_queueNetwork[m_queueIdx].size() < length)
    {
        LeaveCriticalSection(&m_socket->m_queueLockNetwork[m_queueIdx]);
        return -1;
    }
    for (unsigned int i = 0; i < length; ++i)
    {
        b[offset + i] = m_socket->m_queueNetwork[m_queueIdx].front();
        m_socket->m_queueNetwork[m_queueIdx].pop();
    }
    LeaveCriticalSection(&m_socket->m_queueLockNetwork[m_queueIdx]);
    return static_cast<int>(length);
}

void Socket::SocketInputStreamNetwork::close()
{
    m_streamOpen = false;
}

Socket::SocketOutputStreamNetwork::SocketOutputStreamNetwork(
    Socket* socketValue,
    int queueIdx)
    : m_streamOpen(true), m_queueIdx(queueIdx), m_socket(socketValue)
{
}

void Socket::SocketOutputStreamNetwork::write(unsigned int b)
{
    if (!m_streamOpen)
    {
        return;
    }

    byte value = static_cast<byte>(b);
    byteArray bytes;
    bytes.data = &value;
    bytes.length = 1;
    write(bytes, 0, 1);
}

void Socket::SocketOutputStreamNetwork::write(byteArray b)
{
    write(b, 0, b.length);
}

void Socket::SocketOutputStreamNetwork::write(
    byteArray b,
    unsigned int offset,
    unsigned int length)
{
    writeWithFlags(b, offset, length, 0);
}

void Socket::SocketOutputStreamNetwork::writeWithFlags(
    byteArray b,
    unsigned int offset,
    unsigned int length,
    int flags)
{
    (void)flags;
    if (!m_streamOpen || m_socket == nullptr || length == 0)
    {
        return;
    }

    if (m_socket->m_hostLocal)
    {
        int queueIdx = m_queueIdx == SOCKET_CLIENT_END
            ? SOCKET_SERVER_END
            : SOCKET_CLIENT_END;

        EnterCriticalSection(&m_socket->m_queueLockNetwork[queueIdx]);
        for (unsigned int i = 0; i < length; ++i)
        {
            m_socket->m_queueNetwork[queueIdx].push(b[offset + i]);
        }
        LeaveCriticalSection(&m_socket->m_queueLockNetwork[queueIdx]);
        return;
    }

    // NativeDesktop currently supports the local host loopback path. Non-local
    // QNet delivery is kept as an inert stub until remote gameplay is ported.
}

void Socket::SocketOutputStreamNetwork::close()
{
    m_streamOpen = false;
}

INetworkPlayer* Socket::getPlayer()
{
    std::lock_guard<std::mutex> lock(SocketPlayerMutex());
    auto found = g_socketPlayers.find(this);
    if (found != g_socketPlayers.end())
    {
        return found->second;
    }
    if (m_hostServerConnection && m_hostLocal)
    {
        return g_hostSocketPlayer;
    }
    return nullptr;
}

void Socket::setPlayer(INetworkPlayer* player)
{
    std::lock_guard<std::mutex> lock(SocketPlayerMutex());
    g_socketPlayers[this] = player;
    networkPlayerSmallId = player == nullptr ? 0 : player->GetSmallId();
    if (m_hostServerConnection && m_hostLocal)
    {
        g_hostSocketPlayer = player;
    }
}

void Socket::Initialise(ServerConnection* serverConnection)
{
    s_serverConnection = serverConnection;

    static bool init = false;
    if (!init)
    {
        init = true;
        for (int i = 0; i < 2; ++i)
        {
            InitializeCriticalSectionAndSpinCount(&s_hostQueueLock[i], 5120);
            s_hostOutStream[i] = new SocketOutputStreamLocal(i);
            s_hostInStream[i] = new SocketInputStreamLocal(i);
        }
        return;
    }

    for (int i = 0; i < 2; ++i)
    {
        if (TryEnterCriticalSection(&s_hostQueueLock[i]))
        {
            std::queue<byte> empty;
            std::swap(s_hostQueue[i], empty);
            LeaveCriticalSection(&s_hostQueueLock[i]);
        }
        if (s_hostOutStream[i] != nullptr)
        {
            s_hostOutStream[i]->m_streamOpen = true;
        }
        if (s_hostInStream[i] != nullptr)
        {
            s_hostInStream[i]->m_streamOpen = true;
        }
    }
}

Socket::Socket(bool response)
    : m_hostServerConnection(true),
      m_hostLocal(true),
      m_end(response ? SOCKET_SERVER_END : SOCKET_CLIENT_END),
      networkPlayerSmallId(0),
      m_socketClosedEvent(nullptr),
      createdOk(true)
{
    if (s_hostInStream[0] == nullptr)
    {
        Initialise(s_serverConnection);
    }

    m_endClosed[0] = false;
    m_endClosed[1] = false;
    InitializeCriticalSectionAndSpinCount(&m_queueLockNetwork[0], 5120);
    InitializeCriticalSectionAndSpinCount(&m_queueLockNetwork[1], 5120);
    m_inputStream[0] = nullptr;
    m_inputStream[1] = nullptr;
    m_outputStream[0] = nullptr;
    m_outputStream[1] = nullptr;

    if (!response && s_serverConnection != nullptr)
    {
        Socket* serverSocket = new Socket(true);
        s_serverConnection->NewIncomingSocket(serverSocket);
    }
}

Socket::Socket(INetworkPlayer* player, bool response, bool hostLocal)
    : m_hostServerConnection(false),
      m_hostLocal(hostLocal),
      m_end(response || hostLocal ? SOCKET_SERVER_END : SOCKET_CLIENT_END),
      networkPlayerSmallId(0),
      m_socketClosedEvent(new C4JThread::Event),
      createdOk(true)
{
    for (int i = 0; i < 2; ++i)
    {
        InitializeCriticalSectionAndSpinCount(&m_queueLockNetwork[i], 5120);
        m_queueNetwork[i] = std::queue<byte>();
        m_inputStream[i] = nullptr;
        m_outputStream[i] = nullptr;
        m_endClosed[i] = false;
    }

    if (!response || hostLocal)
    {
        m_inputStream[SOCKET_CLIENT_END] =
            new SocketInputStreamNetwork(this, SOCKET_CLIENT_END);
        m_outputStream[SOCKET_CLIENT_END] =
            new SocketOutputStreamNetwork(this, SOCKET_CLIENT_END);
        m_end = SOCKET_CLIENT_END;
    }
    if (response || hostLocal)
    {
        m_inputStream[SOCKET_SERVER_END] =
            new SocketInputStreamNetwork(this, SOCKET_SERVER_END);
        m_outputStream[SOCKET_SERVER_END] =
            new SocketOutputStreamNetwork(this, SOCKET_SERVER_END);
        m_end = SOCKET_SERVER_END;
    }

    setPlayer(player);
}

SocketAddress* Socket::getRemoteSocketAddress()
{
    return nullptr;
}

void Socket::pushDataToQueue(
    const BYTE* pbData,
    DWORD dwDataSize,
    bool fromHost)
{
    int queueIdx = fromHost ? SOCKET_CLIENT_END : SOCKET_SERVER_END;
    if (queueIdx != m_end && !m_hostLocal)
    {
        return;
    }

    EnterCriticalSection(&m_queueLockNetwork[queueIdx]);
    if (m_queueNetwork[queueIdx].size() + dwDataSize <= 2 * 1024 * 1024)
    {
        for (DWORD i = 0; i < dwDataSize; ++i)
        {
            m_queueNetwork[queueIdx].push(pbData[i]);
        }
    }
    LeaveCriticalSection(&m_queueLockNetwork[queueIdx]);
}

void Socket::addIncomingSocket(Socket* socket)
{
    if (s_serverConnection != nullptr)
    {
        s_serverConnection->NewIncomingSocket(socket);
    }
}

InputStream* Socket::getInputStream(bool isServerConnection)
{
    if (!m_hostServerConnection)
    {
        if (m_hostLocal)
        {
            return isServerConnection
                ? static_cast<InputStream*>(m_inputStream[SOCKET_SERVER_END])
                : static_cast<InputStream*>(m_inputStream[SOCKET_CLIENT_END]);
        }
        return m_inputStream[m_end];
    }

    return s_hostInStream[m_end];
}

void Socket::setSoTimeout(int a)
{
    (void)a;
}

void Socket::setTrafficClass(int a)
{
    (void)a;
}

Socket::SocketOutputStream* Socket::getOutputStream(bool isServerConnection)
{
    if (!m_hostServerConnection)
    {
        if (m_hostLocal)
        {
            return isServerConnection
                ? m_outputStream[SOCKET_SERVER_END]
                : m_outputStream[SOCKET_CLIENT_END];
        }
        return m_outputStream[m_end];
    }

    return s_hostOutStream[1 - m_end];
}

bool Socket::close(bool isServerConnection)
{
    bool allClosed = false;
    if (m_hostLocal)
    {
        int end = isServerConnection ? SOCKET_SERVER_END : SOCKET_CLIENT_END;
        m_endClosed[end] = true;
        allClosed = m_endClosed[SOCKET_CLIENT_END] &&
            m_endClosed[SOCKET_SERVER_END];
    }
    else
    {
        m_endClosed[m_end] = true;
        allClosed = true;
    }

    if (allClosed && m_socketClosedEvent != nullptr)
    {
        m_socketClosedEvent->Set();
    }
    if (allClosed)
    {
        createdOk = false;
    }
    {
        std::lock_guard<std::mutex> lock(SocketPlayerMutex());
        g_socketPlayers.erase(this);
    }
    return allClosed;
}
