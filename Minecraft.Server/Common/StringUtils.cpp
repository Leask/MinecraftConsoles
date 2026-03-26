#include "StringUtils.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <lce_time/lce_time.h>

namespace ServerRuntime
{
	namespace StringUtils
	{
		namespace
		{
			static void AppendUtf8CodePoint(std::string *output, unsigned int codePoint)
			{
				if (output == NULL)
				{
					return;
				}

				if (codePoint <= 0x7F)
				{
					output->push_back((char)codePoint);
					return;
				}

				if (codePoint <= 0x7FF)
				{
					output->push_back((char)(0xC0 | (codePoint >> 6)));
					output->push_back((char)(0x80 | (codePoint & 0x3F)));
					return;
				}

				if (codePoint <= 0xFFFF)
				{
					output->push_back((char)(0xE0 | (codePoint >> 12)));
					output->push_back((char)(0x80 | ((codePoint >> 6) & 0x3F)));
					output->push_back((char)(0x80 | (codePoint & 0x3F)));
					return;
				}

				if (codePoint <= 0x10FFFF)
				{
					output->push_back((char)(0xF0 | (codePoint >> 18)));
					output->push_back((char)(0x80 | ((codePoint >> 12) & 0x3F)));
					output->push_back((char)(0x80 | ((codePoint >> 6) & 0x3F)));
					output->push_back((char)(0x80 | (codePoint & 0x3F)));
				}
			}

			static bool ReadWideCodePoint(
				const std::wstring &value,
				size_t *index,
				unsigned int *outCodePoint)
			{
				if (index == NULL || outCodePoint == NULL || *index >= value.size())
				{
					return false;
				}

				const unsigned int first = (unsigned int)value[*index];
				++(*index);

				if (sizeof(wchar_t) == 2 &&
					first >= 0xD800 &&
					first <= 0xDBFF &&
					*index < value.size())
				{
					const unsigned int second = (unsigned int)value[*index];
					if (second >= 0xDC00 && second <= 0xDFFF)
					{
						++(*index);
						*outCodePoint =
							0x10000U +
							(((first - 0xD800U) << 10) | (second - 0xDC00U));
						return true;
					}
				}

				*outCodePoint = first;
				return true;
			}

			static void AppendWideCodePoint(std::wstring *output, unsigned int codePoint)
			{
				if (output == NULL)
				{
					return;
				}

				if (sizeof(wchar_t) == 2 && codePoint > 0xFFFF)
				{
					const unsigned int surrogate = codePoint - 0x10000U;
					output->push_back((wchar_t)(0xD800U + (surrogate >> 10)));
					output->push_back((wchar_t)(0xDC00U + (surrogate & 0x3FFU)));
					return;
				}

				output->push_back((wchar_t)codePoint);
			}

			static bool DecodeUtf8CodePoint(
				const unsigned char *value,
				size_t length,
				size_t *index,
				unsigned int *outCodePoint)
			{
				if (value == NULL || index == NULL || outCodePoint == NULL || *index >= length)
				{
					return false;
				}

				const unsigned char first = value[*index];
				if ((first & 0x80) == 0)
				{
					*outCodePoint = first;
					++(*index);
					return true;
				}

				unsigned int codePoint = 0;
				size_t expectedBytes = 0;
				if ((first & 0xE0) == 0xC0)
				{
					codePoint = first & 0x1F;
					expectedBytes = 2;
				}
				else if ((first & 0xF0) == 0xE0)
				{
					codePoint = first & 0x0F;
					expectedBytes = 3;
				}
				else if ((first & 0xF8) == 0xF0)
				{
					codePoint = first & 0x07;
					expectedBytes = 4;
				}
				else
				{
					return false;
				}

				if (*index + expectedBytes > length)
				{
					return false;
				}

				for (size_t offset = 1; offset < expectedBytes; ++offset)
				{
					const unsigned char continuation = value[*index + offset];
					if ((continuation & 0xC0) != 0x80)
					{
						return false;
					}

					codePoint = (codePoint << 6) | (continuation & 0x3F);
				}

				*index += expectedBytes;
				*outCodePoint = codePoint;
				return true;
			}

			static std::string ConvertWideToUtf8(const std::wstring &value)
			{
				std::string utf8;
				utf8.reserve(value.size());
				size_t index = 0;
				while (index < value.size())
				{
					unsigned int codePoint = 0;
					if (!ReadWideCodePoint(value, &index, &codePoint))
					{
						return std::string();
					}

					AppendUtf8CodePoint(&utf8, codePoint);
				}

				return utf8;
			}

			static std::wstring ConvertUtf8ToWide(const char *value)
			{
				std::wstring wide;
				if (value == NULL)
				{
					return wide;
				}

				const unsigned char *bytes =
					reinterpret_cast<const unsigned char *>(value);
				const size_t length = strlen(value);
				size_t index = 0;
				while (index < length)
				{
					unsigned int codePoint = 0;
					if (!DecodeUtf8CodePoint(bytes, length, &index, &codePoint))
					{
						wide.clear();
						for (size_t fallbackIndex = 0; fallbackIndex < length; ++fallbackIndex)
						{
							wide.push_back((wchar_t)bytes[fallbackIndex]);
						}
						return wide;
					}

					AppendWideCodePoint(&wide, codePoint);
				}

				return wide;
			}

			static bool BuildUtcTime(std::tm *outTime)
			{
				if (outTime == NULL)
				{
					return false;
				}

				const std::time_t seconds =
					static_cast<std::time_t>(LceGetUnixTimeMilliseconds() / 1000ULL);
#if defined(_WIN32)
				return gmtime_s(outTime, &seconds) == 0;
#else
				return gmtime_r(&seconds, outTime) != NULL;
#endif
			}
		}

		std::string WideToUtf8(const std::wstring &value)
		{
			if (value.empty())
			{
				return std::string();
			}

			return ConvertWideToUtf8(value);
		}

		std::wstring Utf8ToWide(const char *value)
		{
			if (value == NULL || value[0] == 0)
			{
				return std::wstring();
			}

			return ConvertUtf8ToWide(value);
		}

		std::wstring Utf8ToWide(const std::string &value)
		{
			return Utf8ToWide(value.c_str());
		}

		std::string StripUtf8Bom(const std::string &value)
		{
			if (value.size() >= 3 &&
				(unsigned char)value[0] == 0xEF &&
				(unsigned char)value[1] == 0xBB &&
				(unsigned char)value[2] == 0xBF)
			{
				return value.substr(3);
			}

			return value;
		}

		std::string TrimAscii(const std::string &value)
		{
			size_t start = 0;
			while (start < value.length() && std::isspace((unsigned char)value[start]))
			{
				++start;
			}

			size_t end = value.length();
			while (end > start && std::isspace((unsigned char)value[end - 1]))
			{
				--end;
			}

			return value.substr(start, end - start);
		}

		std::string ToLowerAscii(const std::string &value)
		{
			std::string lowered = value;
			for (size_t i = 0; i < lowered.length(); ++i)
			{
				lowered[i] = (char)std::tolower((unsigned char)lowered[i]);
			}
			return lowered;
		}

		std::string JoinTokens(const std::vector<std::string> &tokens, size_t startIndex, const char *separator)
		{
			if (startIndex >= tokens.size())
			{
				return std::string();
			}

			const auto joinSeparator = std::string((separator != nullptr) ? separator : " ");
			size_t totalLength = 0;
			for (size_t i = startIndex; i < tokens.size(); ++i)
			{
				totalLength += tokens[i].size();
			}

			totalLength += (tokens.size() - startIndex - 1) * joinSeparator.size();
			std::string joined;
			joined.reserve(totalLength);
			for (size_t i = startIndex; i < tokens.size(); ++i)
			{
				if (!joined.empty())
				{
					joined += joinSeparator;
				}

				joined += tokens[i];
			}

			return joined;
		}

		bool StartsWithIgnoreCase(const std::string &value, const std::string &prefix)
		{
			if (prefix.size() > value.size())
			{
				return false;
			}

			for (size_t i = 0; i < prefix.size(); ++i)
			{
				unsigned char a = (unsigned char)value[i];
				unsigned char b = (unsigned char)prefix[i];
				if (std::tolower(a) != std::tolower(b))
				{
					return false;
				}
			}

			return true;
		}

		bool TryParseUnsignedLongLong(const std::string &value, unsigned long long *outValue)
		{
			if (outValue == nullptr)
			{
				return false;
			}

			const std::string trimmed = TrimAscii(value);
			if (trimmed.empty())
			{
				return false;
			}

			errno = 0;
			char *end = nullptr;
			const unsigned long long parsed = strtoull(trimmed.c_str(), &end, 0);
			if (end == trimmed.c_str() || errno != 0)
			{
				return false;
			}

			while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
			{
				++end;
			}

			if (*end != 0)
			{
				return false;
			}

			*outValue = parsed;
			return true;
		}

		std::string GetCurrentUtcTimestampIso8601()
		{
			std::tm utc = {};
			if (!BuildUtcTime(&utc))
			{
				return std::string();
			}

			char created[64] = {};
			snprintf(
				created,
				sizeof(created),
				"%04u-%02u-%02uT%02u:%02u:%02uZ",
				(unsigned)(utc.tm_year + 1900),
				(unsigned)(utc.tm_mon + 1),
				(unsigned)utc.tm_mday,
				(unsigned)utc.tm_hour,
				(unsigned)utc.tm_min,
				(unsigned)utc.tm_sec);
			return created;
		}

		bool TryParseUtcTimestampIso8601(
			const std::string &value,
			unsigned long long *outFileTime)
		{
			if (outFileTime == nullptr)
			{
				return false;
			}

			const std::string trimmed = TrimAscii(value);
			if (trimmed.empty())
			{
				return false;
			}

			unsigned year = 0;
			unsigned month = 0;
			unsigned day = 0;
			unsigned hour = 0;
			unsigned minute = 0;
			unsigned second = 0;
			if (sscanf(
					trimmed.c_str(),
					"%4u-%2u-%2uT%2u:%2u:%2uZ",
					&year,
					&month,
					&day,
					&hour,
					&minute,
					&second) != 6)
			{
				return false;
			}

			std::tm utc = {};
			utc.tm_year = (int)year - 1900;
			utc.tm_mon = (int)month - 1;
			utc.tm_mday = (int)day;
			utc.tm_hour = (int)hour;
			utc.tm_min = (int)minute;
			utc.tm_sec = (int)second;
			utc.tm_isdst = 0;

#if defined(_WIN32)
			const std::time_t seconds = _mkgmtime(&utc);
#else
			const std::time_t seconds = timegm(&utc);
#endif
			if (seconds < 0)
			{
				return false;
			}

			static const unsigned long long kWindowsToUnixEpoch100Ns =
				116444736000000000ULL;
			*outFileTime =
				(static_cast<unsigned long long>(seconds) * 10000000ULL) +
				kWindowsToUnixEpoch100Ns;
			return true;
		}
	}
}
