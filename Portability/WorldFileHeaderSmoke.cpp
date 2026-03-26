#include "WorldFileHeaderSmoke.h"

#include <vector>

#include "Minecraft.World/stdafx.h"
#include "Minecraft.World/FileHeader.h"

namespace
{
    class SmokeFileHeader : public FileHeader
    {
    public:
        using FileHeader::AddFile;
        using FileHeader::GetFileSize;
        using FileHeader::ReadHeader;
        using FileHeader::WriteHeader;
        using FileHeader::fileExists;
        using FileHeader::getLocalEndian;
        using FileHeader::getOriginalSaveVersion;
        using FileHeader::getSaveEndian;
        using FileHeader::getSavePlatform;
        using FileHeader::getSaveVersion;
        using FileHeader::getFilesWithPrefix;
    };
}

WorldFileHeaderSmokeResult RunWorldFileHeaderSmoke()
{
    WorldFileHeaderSmokeResult result = {};

    SmokeFileHeader writer;
    FileEntry *level = writer.AddFile(L"level.dat", 64);
    FileEntry *entities = writer.AddFile(L"entities.dat", 128);
    FileEntry *duplicate = writer.AddFile(L"level.dat", 777);

    const unsigned int totalSize = writer.GetFileSize();
    std::vector<unsigned char> saveBytes(totalSize, 0);
    writer.WriteHeader(saveBytes.data());

    SmokeFileHeader reader;
    reader.ReadHeader(saveBytes.data(), SAVE_FILE_PLATFORM_LOCAL);

    std::vector<FileEntry *> *allFiles = reader.getFilesWithPrefix(L"");
    std::vector<FileEntry *> *levelFiles = reader.getFilesWithPrefix(L"level");

    result.duplicateReused = duplicate == level;
    result.roundTripOk =
        reader.fileExists(L"level.dat") &&
        reader.fileExists(L"entities.dat") &&
        reader.getSavePlatform() == SAVE_FILE_PLATFORM_LOCAL &&
        reader.getSaveEndian() == reader.getLocalEndian() &&
        reader.getSaveVersion() == SAVE_FILE_VERSION_NUMBER &&
        reader.getOriginalSaveVersion() == SAVE_FILE_VERSION_NUMBER;
    result.prefixesOk =
        allFiles != nullptr &&
        levelFiles != nullptr &&
        levelFiles->size() == 1;
    result.offsetsOk =
        level != nullptr &&
        entities != nullptr &&
        level->data.startOffset == SAVE_FILE_HEADER_SIZE &&
        entities->data.startOffset == SAVE_FILE_HEADER_SIZE + 64;
    result.fileCount =
        allFiles != nullptr ? static_cast<int>(allFiles->size()) : 0;
    result.totalSize = totalSize;

    delete allFiles;
    delete levelFiles;

    return result;
}
