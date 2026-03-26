#pragma once

#include "FileUtils.h"
#include "StringUtils.h"

#include "../vendor/nlohmann/json.hpp"

#include <filesystem>
#include <stdio.h>

namespace ServerRuntime
{
	namespace AccessStorageUtils
	{
		inline bool IsRegularFile(const std::string &path)
		{
			std::error_code error;
			return std::filesystem::is_regular_file(
				std::filesystem::u8path(path),
				error);
		}

		inline bool EnsureJsonListFileExists(const std::string &path)
		{
			return IsRegularFile(path) || FileUtils::WriteTextFileAtomic(path, "[]\n");
		}

		inline bool TryGetStringField(const nlohmann::ordered_json &object, const char *key, std::string *outValue)
		{
			if (key == nullptr || outValue == nullptr || !object.is_object())
			{
				return false;
			}

			const auto it = object.find(key);
			if (it == object.end() || !it->is_string())
			{
				return false;
			}

			*outValue = it->get<std::string>();
			return true;
		}

		inline std::string NormalizeXuid(const std::string &xuid)
		{
			const std::string trimmed = StringUtils::TrimAscii(xuid);
			if (trimmed.empty())
			{
				return "";
			}

			unsigned long long numericXuid = 0;
			if (StringUtils::TryParseUnsignedLongLong(trimmed, &numericXuid))
			{
				if (numericXuid == 0ULL)
				{
					return "";
				}

				char buffer[32] = {};
				snprintf(buffer, sizeof(buffer), "0x%016llx", numericXuid);
				return buffer;
			}

			return StringUtils::ToLowerAscii(trimmed);
		}

		inline std::string BuildPathFromBaseDirectory(const std::string &baseDirectory, const char *fileName)
		{
			if (fileName == nullptr || fileName[0] == 0)
			{
				return "";
			}

			if (baseDirectory.empty() || baseDirectory == ".")
			{
				return std::filesystem::u8path(fileName).u8string();
			}

			std::filesystem::path basePath = std::filesystem::u8path(baseDirectory);
			basePath /= std::filesystem::u8path(fileName);
			return basePath.u8string();
		}
	}
}
