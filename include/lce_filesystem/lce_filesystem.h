#pragma once

#include <cstddef>

bool FileOrDirectoryExists(const char* path);
bool FileExists(const char* path);
bool DirectoryExists(const char* path);
bool GetFirstFileInDirectory(const char* directory, char* outFilePath, size_t outFilePathSize);
