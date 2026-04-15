#include "stdafx.h"

#include <cstdio>

#include "File.h"
#include "NbtSlotFile.h"

namespace
{
    void ReadBytes(std::FILE *file, void *data, size_t size)
    {
        if (file == nullptr)
        {
            return;
        }

        std::fread(data, 1, size, file);
    }

    template <class T>
    void ReadValue(std::FILE *file, T &value)
    {
        ReadBytes(file, &value, sizeof(value));
    }

    void WriteBytes(std::FILE *file, const void *data, size_t size)
    {
        if (file == nullptr)
        {
            return;
        }

        std::fwrite(data, 1, size, file);
    }

    template <class T>
    void WriteValue(std::FILE *file, const T &value)
    {
        WriteBytes(file, &value, sizeof(value));
    }
}


byteArray NbtSlotFile::READ_BUFFER(1024*1024);
int64_t NbtSlotFile::largest = 0;

NbtSlotFile::NbtSlotFile(File file)
{
	totalFileSlots = 0;
	fileSlotMapLength = ZonedChunkStorage::CHUNKS_PER_ZONE * ZonedChunkStorage::CHUNKS_PER_ZONE;
	fileSlotMap = new vector<int> *[fileSlotMapLength];

	const bool needsHeader = !file.exists() || file.length() == 0;
	const char *path = wstringtofilename(file.getPath());
	raf = std::fopen(path, "r+b");
	if (raf == nullptr)
	{
		raf = std::fopen(path, "w+b");
    }

	if (raf == nullptr)
	{
		app.FatalLoadError();
	}

	if (needsHeader)
	{
		writeHeader();
	}

    readHeader();

    for (int i = 0; i < fileSlotMapLength; i++)
	{
        fileSlotMap[i] = new vector<int>;
    }

    for (int fileSlot = 0; fileSlot < totalFileSlots; fileSlot++)
	{
        seekSlotHeader(fileSlot);
		short slot = 0;
		ReadValue(raf, slot);
        if (slot == 0)
		{
			freeFileSlots.push_back(fileSlot);
        } else if (slot < 0)
		{
			fileSlotMap[(-slot) - 1]->push_back(fileSlot);
        } else {
            fileSlotMap[slot - 1]->push_back(fileSlot);
        }
    }
}

void NbtSlotFile::readHeader()
{
	std::fseek(raf,0,SEEK_SET);
    int magic = 0;
	ReadValue(raf, magic);
//    if (magic != MAGIC_NUMBER) throw new IOException("Bad magic number: " + magic);		// 4J - TODO
    short version = 0;
	ReadValue(raf, version);
//    if (version != 0) throw new IOException("Bad version number: " + version);		// 4J - TODO
	ReadValue(raf, totalFileSlots);
}

void NbtSlotFile::writeHeader()
{
	const int magic = MAGIC_NUMBER;
	short version = 0;
	std::fseek(raf,0,SEEK_SET);
	WriteValue(raf, magic);
	WriteValue(raf, version);
	WriteValue(raf, totalFileSlots);
}

void NbtSlotFile::seekSlotHeader(int fileSlot)
{
    int target = FILE_HEADER_SIZE + fileSlot * (FILE_SLOT_SIZE + FILE_SLOT_HEADER_SIZE);
	std::fseek(raf,target,SEEK_SET);
}

void NbtSlotFile::seekSlot(int fileSlot)
{
    int target = FILE_HEADER_SIZE + fileSlot * (FILE_SLOT_SIZE + FILE_SLOT_HEADER_SIZE);
	std::fseek(raf,target+FILE_SLOT_HEADER_SIZE,SEEK_SET);
}

vector<CompoundTag *> *NbtSlotFile::readAll(int slot)
{
    vector<CompoundTag *> *tags = new vector<CompoundTag *>;
    vector<int> *fileSlots = fileSlotMap[slot];
    int skipped = 0;

	for (int c : *fileSlots)
	{
        int pos = 0;
        int continuesAt = -1;
        int expectedSlot = slot + 1;
        do
		{
            seekSlotHeader(c);
            short oldSlot = 0;
			ReadValue(raf, oldSlot);
            short size = 0;
			ReadValue(raf, size);
			ReadValue(raf, continuesAt);
			int lastSlot = 0;
			ReadValue(raf, lastSlot);

            seekSlot(c);
            if (expectedSlot > 0 && oldSlot == -expectedSlot)
			{
                skipped++;
                goto fileSlotLoop;	// 4J - used to be continue fileSlotLoop, with for loop labelled as fileSlotLoop
            }

			ReadBytes(raf, READ_BUFFER.data + pos, size);

            if (continuesAt >= 0)
			{
                pos += size;
                c = continuesAt;
                expectedSlot = -slot - 1;
            }
        } while (continuesAt >= 0);
        tags->push_back(NbtIo::decompress(READ_BUFFER));
fileSlotLoop:
		continue;
    }

    return tags;

}

int NbtSlotFile::getFreeSlot()
{    int fileSlot;

// 4J - removed - don't see how toReplace can ever have anything in here, and might not be initialised
//    if (toReplace->size() > 0)
//	{
//		fileSlot = toReplace->back();
//		toReplace->pop_back();
//    } else

	if (freeFileSlots.size() > 0)
	{
        fileSlot = freeFileSlots.back();
		freeFileSlots.pop_back();
    }
	else
	{
        fileSlot = totalFileSlots++;
        writeHeader();
    }

    return fileSlot;

}
void NbtSlotFile::replaceSlot(int slot, vector<CompoundTag *> *tags)
{
	toReplace = fileSlotMap[slot];
    fileSlotMap[slot] = new vector<int>();

	for (auto tag : *tags)
	{
        byteArray compressed = NbtIo::compress(tag);
        if (compressed.length > largest)
		{
			wchar_t buf[256];
            largest = compressed.length;
#ifndef _CONTENT_PACKAGE
			swprintf(buf, 256, L"New largest: %I64d (%ls)\n",largest,tag->getString(L"id").c_str() );
			OutputDebugStringW(buf);
#endif
        }

        int pos = 0;
        int remaining = compressed.length;
        if (remaining == 0) continue;

        int nextFileSlot = getFreeSlot();
        short currentSlot = slot + 1;
        int lastFileSlot = -1;

        while (remaining > 0)
		{
            int fileSlot = nextFileSlot;
            fileSlotMap[slot]->push_back(fileSlot);

            short toWrite = remaining;
            if (toWrite > FILE_SLOT_SIZE)
			{
                toWrite = FILE_SLOT_SIZE;
            }

            remaining -= toWrite;
            if (remaining > 0)
			{
                nextFileSlot = getFreeSlot();
            }
			else
			{
                nextFileSlot = -1;
            }

            seekSlotHeader(fileSlot);
			WriteValue(raf, currentSlot);
			WriteValue(raf, toWrite);
			WriteValue(raf, nextFileSlot);
			WriteValue(raf, lastFileSlot);

            seekSlot(fileSlot);
			WriteBytes(raf, compressed.data + pos, toWrite);

            if (remaining > 0)
			{
                lastFileSlot = fileSlot;
                pos += toWrite;
                currentSlot = -slot - 1;
            }
        }
		delete[] compressed.data;
    }

	for (int c : *toReplace)
	{
		freeFileSlots.push_back(c);

        seekSlotHeader(c);
		short zero = 0;
		WriteValue(raf, zero);
    }

    toReplace->clear();

}

void NbtSlotFile::close()
{
	if (raf != nullptr)
	{
		std::fclose(raf);
		raf = nullptr;
	}
}
