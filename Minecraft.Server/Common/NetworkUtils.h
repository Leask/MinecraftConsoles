#pragma once

#include "StringUtils.h"
#include <lce_net/lce_net.h>

namespace ServerRuntime
{
	namespace NetworkUtils
	{
		inline std::string NormalizeIpToken(const std::string &ip)
		{
			return StringUtils::ToLowerAscii(StringUtils::TrimAscii(ip));
		}

		inline bool IsIpLiteral(const std::string &text)
		{
			const std::string trimmed = StringUtils::TrimAscii(text);
			return LceNetStringIsIpLiteral(trimmed.c_str());
		}
	}
}
