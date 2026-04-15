#include "stdafx.h"
#include "FileFilter.h"
#include "McRegionLevelStorageSource.h"
#include "File.h"

#include <chrono>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <system_error>

namespace
{
	std::filesystem::path NativePathFromWide(const std::wstring& path)
	{
		return std::filesystem::path(wstringtofilename(path));
	}

	std::wstring NativeFileNameFromPath(const std::filesystem::path& path)
	{
		return convStringToWstring(path.filename().string());
	}

	bool NativeLogFilesystemFailure(
		const char* operation,
		const std::error_code& errorCode)
	{
#ifndef _CONTENT_PACKAGE
		if( errorCode )
		{
			printf(
				"%s - Error code %d (%s)\n",
				operation,
				errorCode.value(),
				errorCode.message().c_str());
		}
		else
		{
			printf("%s - Operation failed\n", operation);
		}
#else
		(void)operation;
		(void)errorCode;
#endif
		return false;
	}

	int64_t NativeWindowsFileTime(std::filesystem::file_time_type time)
	{
		const auto systemTime =
			std::chrono::time_point_cast<std::chrono::nanoseconds>(
				time - std::filesystem::file_time_type::clock::now() +
				std::chrono::system_clock::now());
		const auto ticksSinceUnixEpoch =
			std::chrono::duration_cast<std::chrono::nanoseconds>(
				systemTime.time_since_epoch()).count();
		if( ticksSinceUnixEpoch < 0 )
		{
			return 0;
		}

		const std::uint64_t windowsTicks =
			static_cast<std::uint64_t>(ticksSinceUnixEpoch / 100) +
			116444736000000000ULL;
		return static_cast<int64_t>(windowsTicks);
	}
}

const wchar_t File::pathSeparator = L'\\';
const std::wstring File::pathRoot = L""; // Path root after pathSeparator has been removed

//Creates a new File instance from a parent abstract pathname and a child pathname string.
File::File( const File &parent, const std::wstring& child )
{
	m_abstractPathName = parent.getPath() + pathSeparator + child;
}

//Creates a new File instance by converting the given pathname string into an abstract pathname.
File::File( const wstring& pathname ) //: parent( nullptr )
{
	// #ifndef _CONTENT_PACKAGE
	// 	char buf[256];
	// 	wcstombs(buf, pathname.c_str(), 256);
	// 	printf("File::File - %s\n",buf);
	// #endif
	if( pathname.empty() )
		m_abstractPathName = wstring( L"" );
	else
		m_abstractPathName = pathname;

	/*
	vector<wstring> path = stringSplit( pathname, pathSeparator );

	if( path.back().compare( pathRoot ) != 0 )		
	m_abstractPathName = path.back();
	else
	m_abstractPathName = L"";

	path.pop_back();

	if( path.size() > 0 )
	{
	// If the last member of the vector is the root then just stop
	if( path.back().compare( pathRoot ) != 0 )
	this->parent = new File( &path );
	else
	this->parent = nullptr;
	}
	*/
}

File::File( const std::wstring& parent, const std::wstring& child  ) //: m_abstractPathName( child  )
{
	m_abstractPathName = pathRoot + pathSeparator + parent + pathSeparator + child;
	//this->parent = new File( parent );
}

//Creates a new File instance by converting the given path vector into an abstract pathname.
/*
File::File( vector<wstring> *path ) : parent( nullptr )
{
m_abstractPathName = path->back();
path->pop_back();

if( path->size() > 0 )
{
// If the last member of the vector is the root then just stop
if( path->back().compare( pathRoot ) != 0 )
this->parent = new File( path );
else
this->parent = nullptr;
}
}
*/

//Deletes the file or directory denoted by this abstract pathname. If this pathname denotes a directory,
//then the directory must be empty in order to be deleted.
//Returns:
//true if and only if the file or directory is successfully deleted; false otherwise
bool File::_delete()
{
	std::error_code errorCode;
	const bool removed =
		std::filesystem::remove(NativePathFromWide(getPath()), errorCode);
	if( errorCode || !removed )
	{
		return NativeLogFilesystemFailure("File::_delete", errorCode);
	}

	return true;
}

//Creates the directory named by this abstract pathname.
//Returns:
//true if and only if the directory was created; false otherwise
bool File::mkdir() const
{
	std::error_code errorCode;
	const bool created =
		std::filesystem::create_directory(
			NativePathFromWide(getPath()),
			errorCode);
	return !errorCode && created;
}


//Creates the directory named by this abstract pathname, including any
//necessary but nonexistent parent directories.  Note that if this
//operation fails it may have succeeded in creating some of the necessary
//parent directories.
//
//@return  <code>true</code> if and only if the directory was created,
//         along with all necessary parent directories; <code>false</code>
//         otherwise
//
//@throws  SecurityException
//         If a security manager exists and its <code>{@link
//         java.lang.SecurityManager#checkRead(java.lang.String)}</code>
//         method does not permit verification of the existence of the 
//         named directory and all necessary parent directories; or if
//         the <code>{@link 
//         java.lang.SecurityManager#checkWrite(java.lang.String)}</code>
//         method does not permit the named directory and all necessary
//         parent directories to be created
//
bool File::mkdirs() const
{
	std::error_code errorCode;
	const std::filesystem::path path = NativePathFromWide(getPath());
	std::filesystem::create_directories(path, errorCode);
	if( errorCode )
	{
		return false;
	}

	return std::filesystem::is_directory(path, errorCode) && !errorCode;
}

/*
File *File::getParent() const 
{
return (File *) parent;
}
*/

//Tests whether the file or directory denoted by this abstract pathname exists.
//Returns:
//true if and only if the file or directory denoted by this abstract pathname exists; false otherwise
bool File::exists() const
{
	std::error_code errorCode;
	return std::filesystem::exists(NativePathFromWide(getPath()), errorCode) &&
		!errorCode;
}

//Tests whether the file denoted by this abstract pathname is a normal file. A file is normal if it is not a directory and,
//in addition, satisfies other system-dependent criteria. Any non-directory file created by a Java application is guaranteed to be a normal file.
//Returns:
//true if and only if the file denoted by this abstract pathname exists and is a normal file; false otherwise
bool File::isFile() const
{
	return exists() && !isDirectory();
}

//Renames the file denoted by this abstract pathname.
//Whether or not this method can move a file from one filesystem to another is platform-dependent.
//The return value should always be checked to make sure that the rename operation was successful.
//
//Parameters:
//dest - The new abstract pathname for the named file
//Returns:
//true if and only if the renaming succeeded; false otherwise
bool File::renameTo(File dest)
{
	std::error_code errorCode;
	std::filesystem::rename(
		NativePathFromWide(getPath()),
		NativePathFromWide(dest.getPath()),
		errorCode);
	if( errorCode )
	{
		return NativeLogFilesystemFailure("File::renameTo", errorCode);
	}

	return true;
}

//Returns an array of abstract pathnames denoting the files in the directory denoted by this abstract pathname.
//If this abstract pathname does not denote a directory, then this method returns null. Otherwise an array of File objects is returned,
//one for each file or directory in the directory. Pathnames denoting the directory itself and the directory's parent directory
//are not included in the result. Each resulting abstract pathname is constructed from this abstract pathname using 
//the File(File, String) constructor. Therefore if this pathname is absolute then each resulting pathname is absolute;
//if this pathname is relative then each resulting pathname will be relative to the same directory.
//
//There is no guarantee that the name strings in the resulting array will appear in any specific order; they are not,
//in particular, guaranteed to appear in alphabetical order.
//
//Returns:
//An array of abstract pathnames denoting the files and directories in the directory denoted by this abstract pathname.
//The array will be empty if the directory is empty. Returns null if this abstract pathname does not denote a directory,
//or if an I/O error occurs.
std::vector<File *> *File::listFiles() const
{
	std::vector<File *> *vOutput = new vector<File *>();

	// TODO 4J Stu - Also need to check for I/O errors?
	if( !isDirectory() )
		return vOutput;

	std::error_code errorCode;
	const std::filesystem::path directory = NativePathFromWide(getPath());
	std::filesystem::directory_iterator it(directory, errorCode);
	const std::filesystem::directory_iterator end;
	while( !errorCode && it != end )
	{
		vOutput->push_back(
			new File(*this, NativeFileNameFromPath(it->path())));
		it.increment(errorCode);
	}

	return vOutput;
}

//Returns an array of abstract pathnames denoting the files and directories in the directory denoted by this abstract pathname that
//satisfy the specified filter. The behavior of this method is the same as that of the listFiles() method, except that the pathnames
//in the returned array must satisfy the filter. If the given filter is null then all pathnames are accepted. Otherwise, a pathname
//satisfies the filter if and only if the value true results when the FileFilter.accept(java.io.File) method of the filter is invoked
//on the pathname.
//Parameters:
//filter - A file filter
//Returns:
//An array of abstract pathnames denoting the files and directories in the directory denoted by this abstract pathname.
//The array will be empty if the directory is empty. Returns null if this abstract pathname does not denote a directory, or if an I/O error occurs.
std::vector<File *> *File::listFiles(FileFilter *filter) const
{
	// TODO 4J Stu - Also need to check for I/O errors?
	if( !isDirectory() )
		return nullptr;

	std::vector<File *> *vOutput = new std::vector<File *>();

	std::error_code errorCode;
	const std::filesystem::path directory = NativePathFromWide(getPath());
	std::filesystem::directory_iterator it(directory, errorCode);
	const std::filesystem::directory_iterator end;
	while( !errorCode && it != end )
	{
		File* candidate =
			new File(*this, NativeFileNameFromPath(it->path()));
		if( filter == nullptr || filter->accept(candidate) )
		{
			vOutput->push_back(candidate);
		}
		else
		{
			delete candidate;
		}
		it.increment(errorCode);
	}

	return vOutput;
}

//Tests whether the file denoted by this abstract pathname is a directory.
//Returns:
//true if and only if the file denoted by this abstract pathname exists and is a directory; false otherwise
bool File::isDirectory() const
{
	std::error_code errorCode;
	return std::filesystem::is_directory(
		NativePathFromWide(getPath()),
		errorCode) && !errorCode;
}

//Returns the length of the file denoted by this abstract pathname. The return value is unspecified if this pathname denotes a directory.
//Returns:
//The length, in bytes, of the file denoted by this abstract pathname, or 0L if the file does not exist
int64_t File::length()
{
	std::error_code errorCode;
	const std::filesystem::path path = NativePathFromWide(getPath());
	if( std::filesystem::is_directory(path, errorCode) || errorCode )
	{
		return 0l;
	}

	const std::uintmax_t fileSize =
		std::filesystem::file_size(path, errorCode);
	if( errorCode )
	{
		return 0l;
	}

	return static_cast<int64_t>(fileSize);
}

//Returns the time that the file denoted by this abstract pathname was last modified.
//Returns:
//A long value representing the time the file was last modified, measured in milliseconds since the epoch (00:00:00 GMT, January 1, 1970),
//or 0L if the file does not exist or if an I/O error occurs
int64_t File::lastModified()
{
	std::error_code errorCode;
	const std::filesystem::path path = NativePathFromWide(getPath());
	if( std::filesystem::is_directory(path, errorCode) || errorCode )
	{
		return 0l;
	}

	const std::filesystem::file_time_type lastWrite =
		std::filesystem::last_write_time(path, errorCode);
	if( errorCode )
	{
		return 0l;
	}

	return NativeWindowsFileTime(lastWrite);
}

const std::wstring File::getPath() const
{
	/*
	wstring path;
	if ( parent != nullptr)
	path = parent->getPath();
	else
	path = wstring(pathRoot);

	path.push_back( pathSeparator );
	path.append(m_abstractPathName);
	*/
	return m_abstractPathName;
}

std::wstring File::getName() const
{
	size_t sep = m_abstractPathName.find_last_of(L"\\/");
	return m_abstractPathName.substr( sep + 1, m_abstractPathName.length() );
}

bool File::eq_test(const File &x, const File &y)
{
	return x.getPath().compare( y.getPath() ) == 0;
}

// 4J TODO JEV, a better hash function may be nessesary.
int File::hash_fnct(const File &k)
{
	int hashCode = 0;

	//if (k->parent != nullptr)
	//	hashCode = hash_fnct(k->getParent());

	wchar_t *ref = (wchar_t *) k.m_abstractPathName.c_str();

	for (unsigned int i = 0; i < k.m_abstractPathName.length(); i++)
	{
		hashCode += ((hashCode * 33) + ref[i]) % 149;
	}

	return (int) hashCode;
}

int FileKeyHash::operator() (const File &k) const 
{ return File::hash_fnct(k); }

bool FileKeyEq::operator() (const File &x, const File &y) const 
{ return File::eq_test(x,y); }
