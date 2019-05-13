#pragma once

//#include "core/application/Application.h"
#include <chrono>
#include <thread>
#include "core/Core.h"

namespace Time {
	using nanoseconds = std::ratio<1, 1000000000>;
	using microseconds = std::ratio<1, 1000000>;
	using milliseconds = std::ratio<1, 1000>;
	using seconds = std::ratio<1, 1>;
	using minutes = std::ratio<60, 1>;
	using hours = std::ratio<3600, 1>;

	using time_unit = Time::nanoseconds;

	// This would have used 32 bit integer to encode hh:mm:ss:SSS but since mm:ss:SSS together take up 22 bits, that
	// would leave only 10 bits for the hour, or a maximum encodable time value of 1023:59:59.999 (~43 days). In a game,
	// it is entirely reasonable to expect some values to exceed this limit; for example, save files. With the 42 bit hour,
	// this enables a theoreticle maximum time value of only ~502 million years. This should hopefully suffice for most games.
	static const uint64 MILLI_BITMASK =			0x3FF;				// 0b0000000000000000000000000000000000000000000000000000001111111111;
	static const uint64 SECOND_BITMASK =		0xFC00;				// 0b0000000000000000000000000000000000000000000000001111110000000000;
	static const uint64 MINUTE_BITMASK =		0x3F0000;			// 0b0000000000000000000000000000000000000000001111110000000000000000;
	static const uint64 HOUR_BITMASK =			0xFFFFFFFFFFC00000;	// 0b1111111111111111111111111111111111111111110000000000000000000000;

	static uint64 TIMEZONE_OFFSET_MINUTES = 0;

	template<typename UnitFrom, typename UnitTo, typename Type>
	Type time_cast(Type time) {
		using fdur = std::chrono::duration<Type, UnitFrom>;
		using tdur = std::chrono::duration<Type, UnitTo>;
		return std::chrono::duration_cast<tdur>(fdur(time)).count();
	}

	template<typename Unit = time_unit, typename Type = uint64>
	Type now() {
		using clock = std::chrono::high_resolution_clock;
		return time_cast<clock::period, Unit, Type>(clock::now().time_since_epoch().count());
	}

	template<typename Unit = time_unit, typename Type = uint64>
	Type systemTime() {
		using clock = std::chrono::system_clock;
		return time_cast<clock::period, Unit, Type>(clock::now().time_since_epoch().count());
	}

	template<typename Unit = milliseconds, typename Type = uint64>
	void sleep(Type amount) {
		std::this_thread::sleep_for(std::chrono::duration<Type, Unit>(amount));
	}

	template<typename Unit = time_unit, typename Type = uint64>
	uint64 getTimeStamp(Type time, bool wrapHours = true) {
		uint64 encoded = 0;

		time += time_cast<minutes, Unit>(TIMEZONE_OFFSET_MINUTES);

		encoded |= (((uint64) time_cast<Unit, milliseconds>(time) % 1000) << 0) & Time::MILLI_BITMASK;
		encoded |= (((uint64) time_cast<Unit, seconds>(time) % 60) << 10) & Time::SECOND_BITMASK;
		encoded |= (((uint64) time_cast<Unit, minutes>(time) % 60) << 16) & Time::MINUTE_BITMASK;
		if (wrapHours) {
			encoded |= (((uint64)time_cast<Unit, hours>(time) % 24) << 22) & Time::HOUR_BITMASK;
		} else {
			encoded |= (((uint64)time_cast<Unit, hours>(time)) << 22) & Time::HOUR_BITMASK;
		}

		return encoded;
	}

	std::string decodeTimestampStr(uint64 encoded);

	std::string getCurrentTimestampStr(bool wrapHours = true);
};
