#include "stdafx.h"

#include <cstdio>
#include <limits>

#include "File.h"
#include "FileInputStream.h"

//Creates a FileInputStream by opening a connection to an actual file, the file named by the File object file in the file system.
//A new FileDescriptor object is created to represent this file connection.
//First, if there is a security manager, its checkRead method is called with the path represented by the file argument as its argument.
//
//If the named file does not exist, is a directory rather than a regular file, or for some other reason cannot be opened for reading
//then a FileNotFoundException is thrown.
//
//Parameters:
//file - the file to be opened for reading.
//Throws:
//FileNotFoundException - if the file does not exist, is a directory rather than a regular file, or for some other reason cannot be
//opened for reading.
//SecurityException - if a security manager exists and its checkRead method denies read access to the file.
FileInputStream::FileInputStream(const File &file)
	: m_fileHandle(nullptr)
{
	const char *pchFilename=wstringtofilename(file.getPath());
	m_fileHandle = std::fopen(pchFilename, "rb");

	if( m_fileHandle == nullptr )
	{
		// TODO 4J Stu - Any form of error/exception handling
		//__debugbreak();
		app.FatalLoadError();
	}
}

FileInputStream::~FileInputStream()
{
	if( m_fileHandle != nullptr )
		std::fclose( m_fileHandle );
}

//Reads a byte of data from this input stream. This method blocks if no input is yet available.
//Returns:
//the next byte of data, or -1 if the end of the file is reached.
int FileInputStream::read()
{
	int byteRead = std::fgetc(m_fileHandle);

	if( byteRead == EOF )
	{
		if( std::ferror(m_fileHandle) != 0 )
		{
			// TODO 4J Stu - Some kind of error handling
			app.FatalLoadError();
		}

		return -1;
	}

	return byteRead;
}

//Reads up to b.length bytes of data from this input stream into an array of bytes. This method blocks until some input is available.
//Parameters:
//b - the buffer into which the data is read.
//Returns:
//the total number of bytes read into the buffer, or -1 if there is no more data because the end of the file has been reached.
int FileInputStream::read(byteArray b)
{
	if( b.length == 0 )
	{
		return 0;
	}

	size_t numberOfBytesRead = std::fread(
		b.data,
		1,
		b.length,
		m_fileHandle
		);

	if( numberOfBytesRead == 0 )
	{
		if( std::ferror(m_fileHandle) != 0 )
		{
			// TODO 4J Stu - Some kind of error handling
			app.FatalLoadError();
		}

		// File pointer is past the end of the file
		return -1;
	}

	return static_cast<int>(numberOfBytesRead);
}

//Reads up to len bytes of data from this input stream into an array of bytes. If len is not zero, the method blocks until some input
//is available; otherwise, no bytes are read and 0 is returned.
//Parameters:
//b - the buffer into which the data is read.
//off - the start offset in the destination array b
//len - the maximum number of bytes read.
//Returns:
//the total number of bytes read into the buffer, or -1 if there is no more data because the end of the file has been reached.
int FileInputStream::read(byteArray b, unsigned int offset, unsigned int length)
{
	// 4J Stu - We don't want to read any more than the array buffer can hold
	assert( length <= ( b.length - offset ) );

	if( length == 0 )
	{
		return 0;
	}

	size_t numberOfBytesRead = std::fread(
		&b[offset],
		1,
		length,
		m_fileHandle
		);

	if( numberOfBytesRead == 0 )
	{
		if( std::ferror(m_fileHandle) != 0 )
		{
			// TODO 4J Stu - Some kind of error handling
			app.FatalLoadError();
		}

		// File pointer is past the end of the file
		return -1;
	}

	return static_cast<int>(numberOfBytesRead);
}

//Closes this file input stream and releases any system resources associated with the stream.
//If this stream has an associated channel then the channel is closed as well.
void FileInputStream::close()
{
	if(m_fileHandle==nullptr)
	{
		//printf("\n\nFileInputStream::close - TRYING TO CLOSE AN INVALID FILE\n\n");
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


//Skips n bytes of input from this input stream. Fewer bytes might be skipped if the end of the input stream is reached. The actual number k of bytes to be skipped is equal to the smaller of n and count-pos. The value k is added into pos and k is returned.
//Overrides:
//skip in class InputStream
//Parameters:
//n - the number of bytes to be skipped.
//Returns:
//the actual number of bytes skipped.
int64_t FileInputStream::skip(int64_t n)
{
	if( n <= 0 ||
		n > static_cast<int64_t>(std::numeric_limits<long>::max()) )
	{
		return 0;
	}

	long initialPos = std::ftell(m_fileHandle);
	if( initialPos < 0 )
	{
		return 0;
	}

	if( std::fseek(m_fileHandle, static_cast<long>(n), SEEK_CUR) != 0 )
	{
		return 0;
	}

	long finalPos = std::ftell(m_fileHandle);
	if( finalPos < initialPos )
	{
		return 0;
	}

	return static_cast<int64_t>(finalPos - initialPos);
}
