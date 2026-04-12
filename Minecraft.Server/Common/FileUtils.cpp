#include "FileUtils.h"

#include <filesystem>
#include <unistd.h>
#include <stdio.h>

#include <lce_time/lce_time.h>
#include <lce_abi/lce_abi.h>

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
				return fopen(path.c_str(), mode);
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

				return fsync(fileno(file)) == 0;
			}
		}

		unsigned long long GetCurrentUtcFileTime()
		{
			static const unsigned long long kFileTimeToUnixEpoch100Ns =
				116444736000000000ULL;
			return (LceGetUnixTimeMilliseconds() * 10000ULL) +
				kFileTimeToUnixEpoch100Ns;
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

			std::error_code renameError;
			std::filesystem::rename(tmpPath, nativePath, renameError);
			if (!renameError)
			{
				return true;
			}

			// Keep the temp file on failure so the original file remains recoverable and the caller can inspect the write result.
			return false;
		}
	}
}
