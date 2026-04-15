#include "stdafx.h"

#include "../Minecraft.World/StringHelpers.h"
#include "../Minecraft.World/compression.h"

#include "ArchiveFile.h"

void ArchiveFile::_readHeader(DataInputStream *dis)
{
	int numberOfFiles = dis->readInt();

	for (int i = 0; i < numberOfFiles; i++)
	{
		MetaData *meta = new MetaData();
		meta->filename = dis->readUTF();
		meta->ptr = dis->readInt();
		meta->filesize = dis->readInt();

		// Filenames preceeded by an asterisk have been compressed.
		if (meta->filename[0] == '*')
		{
			meta->filename = meta->filename.substr(1);
			meta->isCompressed = true;
		} 
		else meta->isCompressed = false;

		m_index.insert( pair<wstring,PMetaData>(meta->filename,meta) );
	}
}

ArchiveFile::ArchiveFile(File file)
{
	m_cachedData = nullptr;
	m_sourcefile = file;
	app.DebugPrintf("Loading archive file...\n");
#ifndef _CONTENT_PACKAGE
	const std::string archivePath =
		NativeDesktopWideToUtf8String(file.getPath().c_str());
	app.DebugPrintf("archive file - %s\n", archivePath.c_str());
#endif

	if(!file.exists())
	{
		app.DebugPrintf("Failed to load archive file!\n");//,file.getPath());
		app.FatalLoadError();
	}

	FileInputStream fis(file);

#if defined _XBOX_ONE || defined __ORBIS__ || defined _WINDOWS64
	byteArray readArray(static_cast<unsigned int>(file.length()));
	fis.read(readArray,0,file.length());

	ByteArrayInputStream bais(readArray);
	DataInputStream dis(&bais);

	m_cachedData = readArray.data;
#else
	DataInputStream dis(&fis);
#endif

	_readHeader(&dis);

	dis.close();
	fis.close();
#if defined _XBOX_ONE || defined __ORBIS__ || defined _WINDOWS64
	bais.reset();
#endif
	app.DebugPrintf("Finished loading archive file\n");
}

ArchiveFile::~ArchiveFile()
{
	delete m_cachedData;
}

vector<wstring> *ArchiveFile::getFileList()
{
	vector<wstring> *out = new vector<wstring>();
	
	for ( const auto& it : m_index )
		out->push_back( it.first );

	return out;
}

bool ArchiveFile::hasFile(const wstring &filename)
{
	return m_index.find(filename) != m_index.end();
}

int ArchiveFile::getFileSize(const wstring &filename)
{
	return hasFile(filename) ? m_index.at(filename)->filesize : -1;
}

byteArray ArchiveFile::getFile(const wstring &filename)
{
	byteArray out;
	auto it = m_index.find(filename);

	if(it == m_index.end())
	{
		app.DebugPrintf("Couldn't find file in archive\n");
		app.DebugPrintf("Failed to find file '%ls' in archive\n", filename.c_str());
#ifndef _CONTENT_PACKAGE
		__debugbreak();
#endif
		app.FatalLoadError();
	}
	else
	{
		PMetaData data = it->second;

#if defined _XBOX_ONE || defined __ORBIS__ || defined _WINDOWS64
		out = byteArray(data->filesize );

		memcpy( out.data, m_cachedData + data->ptr, data->filesize );
#else
		const std::string sourcePath =
			NativeDesktopWideToUtf8String(m_sourcefile.getPath().c_str());
		std::vector<BYTE> fileBytes;
		if (!NativeDesktopReadFileBytesAt(
			sourcePath.c_str(),
			static_cast<long>(data->ptr),
			static_cast<std::size_t>(data->filesize),
			&fileBytes))
		{
			app.DebugPrintf("bad archive read\n");
			app.FatalLoadError();
		}
		else
		{
			PBYTE pbData = new BYTE[data->filesize];
			std::memcpy(pbData, fileBytes.data(), fileBytes.size());
			out = byteArray(pbData, data->filesize);
		}
#endif

		// Compressed filenames are preceeded with an asterisk.
		if ( data->isCompressed && out.data != nullptr )
		{
			/* 4J-JEV:
			* If a compressed file is accessed before compression object is 
			* initialized it will crash here (Compression::getCompression).
			*/
			///4 279 553 556

			ByteArrayInputStream bais(out);
			DataInputStream dis(&bais);
			unsigned int decompressedSize = dis.readInt();
			dis.close();

			PBYTE uncompressedBuffer = new BYTE[decompressedSize];
			Compression::getCompression()->Decompress(uncompressedBuffer, &decompressedSize, out.data+4, out.length-4);

			delete [] out.data;

			out.data = uncompressedBuffer;
			out.length = decompressedSize;
		}

		assert(out.data != nullptr); // THERE IS NO FILE WITH THIS NAME!

	}

	return out;
}
