#pragma once

#include <cstdint>
#include <string>

class PerformanceTimer
{
private:
	std::uint64_t m_startTimeNs;

public:
	PerformanceTimer();
	void Reset();
	void PrintElapsedTime(const std::wstring &description);
};
