#include "Logger.h"
#include <stdarg.h>
#include "core/util/Time.h"
#include <GL/glew.h>

Logger::Logger() {}

Logger::~Logger() {}

void Logger::info(const char* str, ...) {
	std::ostringstream ss;
	ss << "[" << getFormattedTimestamp() << "][INFO]: " << str << "\n";

	va_list arg;
	va_start(arg, str);
	vfprintf(stdout, ss.str().c_str(), arg);
	va_end(arg);
}

void Logger::warn(const char* str, ...) {
	std::ostringstream ss;
	ss << "[" << getFormattedTimestamp() << "][WARN]: " << str << "\n";

	va_list arg;
	va_start(arg, str);
	vfprintf(stderr, ss.str().c_str(), arg);
	va_end(arg);
}

void Logger::error(const char* str, ...) {
	std::ostringstream ss;
	ss << "[" << getFormattedTimestamp() << "][ERROR]: " << str << "\n";

	va_list arg;
	va_start(arg, str);
	vfprintf(stderr, ss.str().c_str(), arg);
	va_end(arg);
}

std::string Logger::getFormattedTimestamp() const {
	return Time::getCurrentTimestampStr();
}

const char* Logger::getGLErrorString(int32 error) const {
	if (error == GL_NO_ERROR) return "GL_NO_ERROR";
	if (error == GL_INVALID_VALUE) return "GL_INVALID_VALUE";
	if (error == GL_INVALID_OPERATION) return "GL_INVALID_OPERATION";
	if (error == GL_INVALID_ENUM) return "GL_INVALID_OPERATION";
	if (error == GL_INVALID_FRAMEBUFFER_OPERATION) return "GL_INVALID_FRAMEBUFFER_OPERATION";
	if (error == GL_OUT_OF_MEMORY) return "GL_OUT_OF_MEMORY";
	if (error == GL_STACK_UNDERFLOW) return "GL_STACK_UNDERFLOW";
	if (error == GL_STACK_OVERFLOW) return "GL_STACK_OVERFLOW";

	return "UNKNOWN_ERROR";
}
