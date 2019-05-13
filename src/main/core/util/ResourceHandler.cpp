#include "ResourceHandler.h"
#include "core/application/Application.h"

#include "core/engine/renderer/ShaderProgram.h"
#include <GL/glew.h>


ResourceHandler::ResourceHandler(char* exec) {

	/*
	echo Deleting $(SolutionDir)$(Platform)\$(Configuration)\res
	del /s /q $(SolutionDir)$(Platform)\$(Configuration)\res
	echo Copying "$(ProjectDir)res" to "$(SolutionDir)$(Platform)\$(Configuration)\res"
	xcopy $(ProjectDir)res  $(SolutionDir)$(Platform)\$(Configuration)\res /e /i /y /s
	echo Copying "C:\Library\bin\$(Platform)\*.dll" to "$(SolutionDir)$(Platform)\$(Configuration)"
	xcopy C:\Library\bin\$(Platform)\*.dll  $(SolutionDir)$(Platform)\$(Configuration) /e /i /y /s
	*/

	std::string cwd(exec);

	size_t i = cwd.rfind(FILE_SEPARATOR, cwd.length());
	if (i != std::string::npos) {
		cwd = cwd.substr(0, i);
	}
	else {
		logWarn("Could not get current working directory. Any resources and other relative access paths may be unavailable\n");
		cwd = "";
	}


	try {
		std::filesystem::path src = std::filesystem::path(cwd).parent_path().parent_path().append("res");
		std::filesystem::path dst = std::filesystem::path(cwd).append("res");
		logInfo("Setting up resources for exec directory \"%s\"", cwd.c_str());
		logInfo("Copying resources directory from \"%s\" to \"%s\"", src.string().c_str(), dst.string().c_str());
		std::filesystem::remove_all(dst);
		std::filesystem::copy(src, dst, std::filesystem::copy_options::recursive);
	} catch (std::exception exc) {
		logError("Failed to copy resources directory: %s", exc.what());
	}

	this->workingDirectory = cwd;
	this->resourceDirectory = "res";

	this->shaderDirectory = "res/shaders";
}


ResourceHandler::~ResourceHandler() {}

std::string ResourceHandler::formatFilePath(std::string file) const {
	std::string formatted(file);
	std::replace(formatted.begin(), formatted.end(), '/', FILE_SEPARATOR);
	std::replace(formatted.begin(), formatted.end(), '\\', FILE_SEPARATOR);
	return formatted;
}


bool ResourceHandler::loadFile(std::string file, std::string& dest, bool logError, bool formatted) const {

	if (!formatted) {
		file = this->formatFilePath(file);
	}

	std::ifstream fileStream(file.c_str(), std::ios::in);

	if (!fileStream.is_open()) {
		if (logError) {
			logWarn("Failed to load file: %s", file.c_str());
		}
		return false;
	}
	dest = std::string();

	while (!fileStream.eof()) {
		std::string temp = "";
		getline(fileStream, temp);
		dest.append(temp + "\n");
	}

	fileStream.close();
	return true;
}

bool ResourceHandler::loadFileAttemptPaths(std::vector<std::string> paths, std::string & dest, bool logError, bool formatted) const {
	if (paths.empty()) {
		if (logError) {
			logError("Could not load file with no specified paths to search");
		}
		return false;
	}

	paths = std::vector<std::string>(paths); // copy

	for (int i = 0; i < paths.size(); i++) {
		if (!formatted) {
			paths[i] = this->formatFilePath(paths[i]);
		}

		if (loadFile(paths[i], dest, false, true)) {
			return true;
		}
	}

	if (logError) {
		logWarn("Failed to load file %s", paths[0].c_str());

		for (int i = 1; i < paths.size(); i++) {
			logWarn("Searched in directory %s", paths[i].c_str());
		}
	}

	return false;
}

Shader* ResourceHandler::loadShader(uint32 type, std::string file) const {
	logInfo("Loading shader file %s", file.c_str());
	if (type != GL_VERTEX_SHADER
		&& type != GL_FRAGMENT_SHADER
		&& type != GL_GEOMETRY_SHADER
		&& type != GL_COMPUTE_SHADER
		&& type != GL_TESS_EVALUATION_SHADER
		&& type != GL_TESS_CONTROL_SHADER
		) {

		logWarn("The shader type passed to the loader was not valid or was not supported by this version. Shader file %s will not be loaded.", file.c_str());
		return nullptr;
	}

	int32 flag = GL_FALSE;
	int logLength;

	std::string fileRaw;
	std::vector<std::string> paths = {
		file,
		file + ".glsl",
		this->workingDirectory + "/" + file,
		this->workingDirectory + "/" + file + ".glsl",
		this->workingDirectory + "/" + this->resourceDirectory + "/" + file,
		this->workingDirectory + "/" + this->resourceDirectory + "/" + file + ".glsl",
		this->workingDirectory + "/" + this->shaderDirectory + "/" + file,
		this->workingDirectory + "/" + this->shaderDirectory + "/" + file + ".glsl",
	};

	if (!loadFileAttemptPaths(paths, fileRaw)) {
		logWarn("An error occurred while loading this shader, this may cause errors");
		return nullptr;
	}

	const char* glslSrc = fileRaw.c_str();

	logInfo("Compiling shader");
	uint32 shaderID = glCreateShader(type);
	glShaderSource(shaderID, 1, &glslSrc, nullptr);
	glCompileShader(shaderID);
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &flag);
	glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);
	char* shaderLog = new char[logLength];
	glGetShaderInfoLog(shaderID, logLength, nullptr, shaderLog);

	if (flag == GL_FALSE) { // error
		logError("Failed to compile shader: %s", shaderLog);
		glDeleteShader(shaderID);
	} else if (logLength > 0) { // warning or info about shader, not necessarily a failure.
		logInfo("%s", shaderLog);
	}

	return new Shader(type, shaderID, file, fileRaw);
}