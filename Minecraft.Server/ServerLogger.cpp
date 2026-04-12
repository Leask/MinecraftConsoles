#include "ServerLogger.h"
#include "Common/StringUtils.h"
#include "vendor/linenoise/linenoise.h"

#include <chrono>
#include <cctype>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

namespace ServerRuntime
{
static int g_minLogLevel = (int)eServerLogLevel_Info;

static const char *NormalizeCategory(const char *category)
{
	if (category == NULL || category[0] == 0)
	{
		return "server";
	}
	return category;
}

static const char *LogLevelToString(EServerLogLevel level)
{
	switch (level)
	{
	case eServerLogLevel_Debug:
		return "DEBUG";
	case eServerLogLevel_Info:
		return "INFO";
	case eServerLogLevel_Warn:
		return "WARN";
	case eServerLogLevel_Error:
		return "ERROR";
	default:
		return "INFO";
	}
}

static bool EqualsIgnoreCase(const char *lhs, const char *rhs)
{
	if (lhs == NULL || rhs == NULL)
	{
		return lhs == rhs;
	}

	while (*lhs != 0 && *rhs != 0)
	{
		if (std::tolower((unsigned char)*lhs) !=
			std::tolower((unsigned char)*rhs))
		{
			return false;
		}

		++lhs;
		++rhs;
	}

	return *lhs == 0 && *rhs == 0;
}

static const char *LogLevelToAnsiColor(EServerLogLevel level)
{
	switch (level)
	{
	case eServerLogLevel_Debug:
		return "\033[36m";
	case eServerLogLevel_Warn:
		return "\033[33m";
	case eServerLogLevel_Error:
		return "\033[31m";
	case eServerLogLevel_Info:
	default:
		return "\033[37m";
	}
}

static void BuildTimestamp(char *buffer, size_t bufferSize)
{
	if (buffer == NULL || bufferSize == 0)
	{
		return;
	}

	const std::chrono::system_clock::time_point now =
		std::chrono::system_clock::now();
	const std::time_t nowSeconds =
		std::chrono::system_clock::to_time_t(now);
	std::tm localTime = {};
	localtime_r(&nowSeconds, &localTime);
	const unsigned milliseconds =
		(unsigned)(std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch()).count() % 1000ULL);
	snprintf(
		buffer,
		bufferSize,
		"%04u-%02u-%02u %02u:%02u:%02u.%03u",
		(unsigned)(localTime.tm_year + 1900),
		(unsigned)(localTime.tm_mon + 1),
		(unsigned)localTime.tm_mday,
		(unsigned)localTime.tm_hour,
		(unsigned)localTime.tm_min,
		(unsigned)localTime.tm_sec,
		milliseconds);
}

static bool ShouldLog(EServerLogLevel level)
{
	return ((int)level >= g_minLogLevel);
}

static void WriteLogLine(EServerLogLevel level, const char *category, const char *message)
{
	if (!ShouldLog(level))
	{
		return;
	}

	linenoiseExternalWriteBegin();

	const char *safeCategory = NormalizeCategory(category);
	const char *safeMessage = (message != NULL) ? message : "";

	char timestamp[32] = {};
	BuildTimestamp(timestamp, sizeof(timestamp));

	bool hasColorConsole = false;
	hasColorConsole = isatty(fileno(stdout)) != 0;
	if (hasColorConsole)
	{
		fputs(LogLevelToAnsiColor(level), stdout);
	}

	printf(
		"[%s][%s][%s] %s\n",
		timestamp,
		LogLevelToString(level),
		safeCategory,
		safeMessage);
	fflush(stdout);

	if (hasColorConsole)
	{
		fputs("\033[0m", stdout);
	}

	linenoiseExternalWriteEnd();
}

static void WriteLogLineV(EServerLogLevel level, const char *category, const char *format, va_list args)
{
	char messageBuffer[2048] = {};
	if (format == NULL)
	{
		WriteLogLine(level, category, "");
		return;
	}

	vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
	messageBuffer[sizeof(messageBuffer) - 1] = 0;
	WriteLogLine(level, category, messageBuffer);
}

bool TryParseServerLogLevel(const char *value, EServerLogLevel *outLevel)
{
	if (value == NULL || outLevel == NULL)
	{
		return false;
	}

	if (EqualsIgnoreCase(value, "debug"))
	{
		*outLevel = eServerLogLevel_Debug;
		return true;
	}
	if (EqualsIgnoreCase(value, "info"))
	{
		*outLevel = eServerLogLevel_Info;
		return true;
	}
	if (EqualsIgnoreCase(value, "warn") || EqualsIgnoreCase(value, "warning"))
	{
		*outLevel = eServerLogLevel_Warn;
		return true;
	}
	if (EqualsIgnoreCase(value, "error"))
	{
		*outLevel = eServerLogLevel_Error;
		return true;
	}

	return false;
}

void SetServerLogLevel(EServerLogLevel level)
{
	if (level < eServerLogLevel_Debug)
	{
		level = eServerLogLevel_Debug;
	}
	else if (level > eServerLogLevel_Error)
	{
		level = eServerLogLevel_Error;
	}

	g_minLogLevel = (int)level;
}

EServerLogLevel GetServerLogLevel()
{
	return (EServerLogLevel)g_minLogLevel;
}

void LogDebug(const char *category, const char *message)
{
	WriteLogLine(eServerLogLevel_Debug, category, message);
}

void LogInfo(const char *category, const char *message)
{
	WriteLogLine(eServerLogLevel_Info, category, message);
}

void LogWarn(const char *category, const char *message)
{
	WriteLogLine(eServerLogLevel_Warn, category, message);
}

void LogError(const char *category, const char *message)
{
	WriteLogLine(eServerLogLevel_Error, category, message);
}

void LogDebugf(const char *category, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	WriteLogLineV(eServerLogLevel_Debug, category, format, args);
	va_end(args);
}

void LogInfof(const char *category, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	WriteLogLineV(eServerLogLevel_Info, category, format, args);
	va_end(args);
}

void LogWarnf(const char *category, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	WriteLogLineV(eServerLogLevel_Warn, category, format, args);
	va_end(args);
}

void LogErrorf(const char *category, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	WriteLogLineV(eServerLogLevel_Error, category, format, args);
	va_end(args);
}

void LogStartupStep(const char *message)
{
	LogInfo("startup", message);
}

void LogWorldIO(const char *message)
{
	LogInfo("world-io", message);
}

void LogWorldName(const char *prefix, const std::wstring &name)
{
	std::string utf8 = StringUtils::WideToUtf8(name);
	LogInfof("world-io", "%s: %s", (prefix != NULL) ? prefix : "name", utf8.c_str());
}
}
