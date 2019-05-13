#pragma once

#include "core/Core.h"

class Logger {
public:

	Logger();

	~Logger();

	/**
	 * Log info to the console without a line brek.
	 */
	void info(const char* str, ...);

	/**
	 * Log a warning to the console without a line break.
	 */
	void warn(const char* str, ...);

	/**
	 * Log an error to the console without a line break.
	 */
	void error(const char* str, ...);

	std::string getFormattedTimestamp() const;

	const char* getGLErrorString(int32 error) const;
};