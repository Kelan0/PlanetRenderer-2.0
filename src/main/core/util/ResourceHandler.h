#pragma once

#include "core/Core.h"

#ifdef _WIN32
#define FILE_SEPARATOR '\\'
#else
#define FILE_SEPARATOR '/'
#endif

class Shader;
//class Texture;

class ResourceHandler {
private:
	std::string workingDirectory;
	std::string resourceDirectory;
	std::string shaderDirectory;

public:
	ResourceHandler(char* exec);

	~ResourceHandler();

	std::string formatFilePath(std::string file) const;

	bool loadFile(std::string file, std::string& dest, bool logError = true, bool formatted = false) const;

	bool loadFileAttemptPaths(std::vector<std::string> paths, std::string& dest, bool logError = true, bool formatted = false) const;

	Shader* loadShader(uint32 type, std::string file) const;

	//    Texture* loadTexture(const char* file) const;
};

