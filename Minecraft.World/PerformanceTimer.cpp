#include "PerformanceTimer.h"

#include <lce_time/lce_time.h>

#include <cstdio>
#include <string>

namespace
{
	std::string NarrowDescription(const std::wstring &description)
	{
		std::string narrowed;
		narrowed.reserve(description.size());
		for (size_t i = 0; i < description.size(); ++i)
		{
			const wchar_t ch = description[i];
			narrowed.push_back(ch >= 0 && ch <= 0x7f
				? static_cast<char>(ch)
				: '?');
		}
		return narrowed;
	}
}

PerformanceTimer::PerformanceTimer()
	: m_startTimeNs(0)
{
#ifndef _CONTENT_PACKAGE
	Reset();
#endif
}

void PerformanceTimer::Reset()
{
#ifndef _CONTENT_PACKAGE
	m_startTimeNs = LceGetMonotonicNanoseconds();
#endif
}

void PerformanceTimer::PrintElapsedTime(const std::wstring &description)
{
#ifndef _CONTENT_PACKAGE
	const std::uint64_t newTimeNs = LceGetMonotonicNanoseconds();
	const double elapsedTime = static_cast<double>(newTimeNs - m_startTimeNs)
		/ 1000000000.0;
	const std::string narrowed = NarrowDescription(description);
	std::printf("TIMER: %s: Elapsed time %.6f\n",
		narrowed.c_str(),
		elapsedTime);
#endif
}
