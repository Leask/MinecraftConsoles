#include "FileUtils.h"

#include <filesystem>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>

#include <lce_time/lce_time.h>
#include <lce_win32/lce_win32.h>

namespace ServerRuntime
{
	namespace FileUtils
	{
		namespace
		{
			static std::filesystem::path ToNativePath(const std::string &filePath)
			{
				return std::filesystem::u8path(filePath);
			}

			static FILE *OpenPathFile(
				const std::filesystem::path &path,
				const char *mode)
			{
#if defined(_WIN32)
				const std::wstring wideMode =
					std::filesystem::path(mode).wstring();
				return _wfopen(path.c_str(), wideMode.c_str());
#else
				return fopen(path.c_str(), mode);
#endif
			}

			static bool FlushFileToStorage(FILE *file)
			{
				if (file == nullptr)
				{
					return false;
				}

				if (fflush(file) != 0)
				{
					return false;
				}

#if defined(_WIN32)
				return _commit(_fileno(file)) == 0;
#else
				return fsync(fileno(file)) == 0;
#endif
			}
		}

		unsigned long long GetCurrentUtcFileTime()
		{
			static const unsigned long long kWindowsToUnixEpoch100Ns =
				116444736000000000ULL;
			return (LceGetUnixTimeMilliseconds() * 10000ULL) +
				kWindowsToUnixEpoch100Ns;
		}

		bool ReadTextFile(const std::string &filePath, std::string *outText)
		{
			if (outText == nullptr)
			{
				return false;
			}

			outText->clear();

			const std::filesystem::path nativePath = ToNativePath(filePath);
			if (nativePath.empty())
			{
				return false;
			}

			FILE *inFile = OpenPathFile(nativePath, "rb");
			if (inFile == nullptr)
			{
				return false;
			}

			if (fseek(inFile, 0, SEEK_END) != 0)
			{
				fclose(inFile);
				return false;
			}

			long fileSize = ftell(inFile);
			if (fileSize < 0)
			{
				fclose(inFile);
				return false;
			}

			if (fseek(inFile, 0, SEEK_SET) != 0)
			{
				fclose(inFile);
				return false;
			}

			if (fileSize == 0)
			{
				fclose(inFile);
				return true;
			}

			outText->resize((size_t)fileSize);
			size_t bytesRead = fread(&(*outText)[0], 1, (size_t)fileSize, inFile);
			fclose(inFile);

			if (bytesRead != (size_t)fileSize)
			{
				outText->clear();
				return false;
			}

			return true;
		}

		bool WriteTextFileAtomic(const std::string &filePath, const std::string &text)
		{
			const std::filesystem::path nativePath = ToNativePath(filePath);
			if (nativePath.empty())
			{
				return false;
			}

			std::filesystem::path tmpPath = nativePath;
			tmpPath += ".tmp";

			FILE *outFile = OpenPathFile(tmpPath, "wb");
			if (outFile == nullptr)
			{
				return false;
			}

			if (!text.empty())
			{
				size_t bytesWritten = fwrite(text.data(), 1, text.size(), outFile);
				if (bytesWritten != text.size())
				{
					fclose(outFile);
					std::error_code cleanupError;
					std::filesystem::remove(tmpPath, cleanupError);
					return false;
				}
			}

			if (!FlushFileToStorage(outFile))
			{
				fclose(outFile);
				std::error_code cleanupError;
				std::filesystem::remove(tmpPath, cleanupError);
				return false;
			}
			fclose(outFile);

#if defined(_WIN32)
			if (std::filesystem::exists(nativePath) &&
				std::filesystem::is_regular_file(nativePath))
			{
				// Replace the destination without deleting the last known-good file first.
				if (ReplaceFileW(
						nativePath.c_str(),
						tmpPath.c_str(),
						nullptr,
						REPLACEFILE_IGNORE_MERGE_ERRORS,
						nullptr,
						nullptr))
				{
					return true;
				}
			}

			if (MoveFileExW(
					tmpPath.c_str(),
					nativePath.c_str(),
					MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
			{
				return true;
			}
#else
			std::error_code renameError;
			std::filesystem::rename(tmpPath, nativePath, renameError);
			if (!renameError)
			{
				return true;
			}
#endif

			// Keep the temp file on failure so the original file remains recoverable and the caller can inspect the write result.
			return false;
		}
	}
}
