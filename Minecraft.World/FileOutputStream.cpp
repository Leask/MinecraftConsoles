#include "stdafx.h"

#include <cstdio>

#include "File.h"
#include "FileOutputStream.h"

//Creates a file output stream to write to the file represented by the specified File object. A new FileDescriptor object is
//created to represent this file connection.
//First, if there is a security manager, its checkWrite method is called with the path represented by the file argument as its argument.
//
//If the file exists but is a directory rather than a regular file, does not exist but cannot be created, or cannot be opened
//for any other reason then a FileNotFoundException is thrown.
//
//Parameters:
//file - the file to be opened for writing.
FileOutputStream::FileOutputStream(const File &file) : m_fileHandle( nullptr )
{
	if( file.exists() && file.isDirectory())
	{
		// TODO 4J Stu - FileNotFoundException
		return;
	}

	const char *pchFilename=wstringtofilename(file.getPath());
	m_fileHandle = std::fopen(pchFilename, "r+b");
	if( m_fileHandle == nullptr )
	{
		m_fileHandle = std::fopen(pchFilename, "w+b");
	}

	if( m_fileHandle == nullptr )
	{
		// TODO 4J Stu - Any form of error/exception handling
	}
}

FileOutputStream::~FileOutputStream()
{
	if( m_fileHandle != nullptr )
		std::fclose( m_fileHandle );
}

//Writes the specified byte to this file output stream. Implements the write method of OutputStream.
//Parameters:
//b - the byte to be written.
void FileOutputStream::write(unsigned int b)
{	
	if( m_fileHandle == nullptr )
	{
		return;
	}

	byte value = static_cast<byte>(b);

	int result = std::fputc(value, m_fileHandle);

	if( result == EOF )
	{
		// TODO 4J Stu - Some kind of error handling
	}
}

//Writes b.length bytes from the specified byte array to this file output stream.
//Parameters:
//b - the data.
void FileOutputStream::write(byteArray b)
{
	if( m_fileHandle == nullptr || b.length == 0 )
	{
		return;
	}

	size_t numberOfBytesWritten = std::fwrite(
		b.data,
		1,
		b.length,
		m_fileHandle
		);

	if( numberOfBytesWritten != b.length  )
	{
		// TODO 4J Stu - Some kind of error handling
	}
}

//Writes len bytes from the specified byte array starting at offset off to this file output stream.
//Parameters:
//b - the data.
//off - the start offset in the data.
//len - the number of bytes to write.
void FileOutputStream::write(byteArray b, unsigned int offset, unsigned int length)
{
	// 4J Stu - We don't want to write any more than the array buffer holds
	assert( length <= ( b.length - offset ) );

	if( m_fileHandle == nullptr || length == 0 )
	{
		return;
	}

	size_t numberOfBytesWritten = std::fwrite(
		&b[offset],
		1,
		length,
		m_fileHandle
		);

	if( numberOfBytesWritten != length  )
	{
		// TODO 4J Stu - Some kind of error handling
	}
}
//
//Closes this file output stream and releases any system resources associated with this stream.
//This file output stream may no longer be used for writing bytes.
//If this stream has an associated channel then the channel is closed as well.
void FileOutputStream::close()
{
	if( m_fileHandle == nullptr )
	{
		return;
	}

	int result = std::fclose( m_fileHandle );

	if( result != 0 )
	{
		// TODO 4J Stu - Some kind of error handling
	}

	// Stop the dtor from trying to close it again
	m_fileHandle = nullptr;
}
