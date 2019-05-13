#include "core/util/Time.h"


namespace Time {
	std::string decodeTimestampStr(uint64 encoded) {
		uint64 h = (encoded & Time::HOUR_BITMASK) >> 22;
		uint64 m = (encoded & Time::MINUTE_BITMASK) >> 16;
		uint64 s = (encoded & Time::SECOND_BITMASK) >> 10;
		uint64 S = (encoded & Time::MILLI_BITMASK) >> 0;

		char buf[25];
		snprintf(buf, sizeof(buf), "%02llu:%02llu:%02llu.%03llu", h, m, s, S);
		return std::string(buf);
	}

	std::string getCurrentTimestampStr(bool wrapHours) {
		return decodeTimestampStr(getTimeStamp(systemTime(), wrapHours));
	}
}