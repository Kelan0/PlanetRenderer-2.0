#include "ShaderProgram.h"
#include "core/application/Application.h"
#include "core/util/ResourceHandler.h"
#include <GL/glew.h>

ShaderProgram::ShaderProgram() :
	programID(0), completed(false) {}


ShaderProgram::~ShaderProgram() {
	for (int i = 0; i < this->shaders.size(); i++) {
		if (this->shaders[i] != NULL) {
			delete this->shaders[i];
		}
	}

	glDeleteProgram(this->programID);
}

void ShaderProgram::addShader(Shader* shader) {
	if (completed) {
		logError("Cannot add a GLSL shader object to this shader program, the program has already been linked");
		return;
	}

	if (shader == NULL) {
		logError("Failed to add GLSL shader object to program ID %d The shader was invalid or NULL", this->programID);
		return;
	}

	if (shaders.count(shader->getType()) != 0) {
		logError("This shader program already has a %s attached", Shader::getShaderAsString(shader->getType()).c_str());
		return;
	}

	shaders.insert(std::pair<uint32, Shader*>(shader->getType(), shader));
}

void ShaderProgram::addShader(uint32 type, std::string file) {
	addShader(RESOURCE_HANDLER.loadShader(type, file));
}

void ShaderProgram::addAttribute(int32 location, std::string attribute) {
	if (attribute.length() == 0 || attributes.count(attribute) != 0)
		return;

	attributes.insert(std::make_pair(attribute, location));
}

void ShaderProgram::completeProgram() {
	if (completed) {
		logError("Cannot complete shader program, the program has already been linked");
	}

	programID = glCreateProgram();
	logInfo("Attaching shaders");
	for (std::pair<uint32, Shader*> entry : shaders) {
		entry.second->attachTo(programID);
	}

	logInfo("Binding attribute locations");
	for (auto it = attributes.begin(); it != attributes.end(); ++it) {
		logInfo("Adding attribute \"%s\" at location %d", it->first.c_str(), it->second);
		glBindAttribLocation(programID, it->second, it->first.c_str());
	}

	int32 status = GL_FALSE;
	int logLength;

	glLinkProgram(programID);
	glGetProgramiv(programID, GL_LINK_STATUS, &status);
	glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLength);

	if (logLength > 0) {
		GLchar* programError = new GLchar[logLength];
		glGetProgramInfoLog(programID, logLength, nullptr, programError);
		if (status) {
			logWarn("Successfully linked shader program with warnings:\n%s", programError);
		}
		else {
			logError("Failed to link shader program:\n%s", programError);
		}
	}

	glValidateProgram(programID);
	glGetProgramiv(programID, GL_VALIDATE_STATUS, &status);
	glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLength);

	if (logLength > 0) {
		GLchar* programError = new GLchar[logLength];
		glGetProgramInfoLog(programID, logLength, nullptr, programError);
		if (status) {
			logWarn("Successfully validated shader program with warnings:\n%s", programError);
		}
		else {
			logError("Failed to validate shader program:\n%s", programError);
		}
	}

	completed = true;
}

//void ShaderProgram::addLight(Light* light) {
//    lights.push_back(light);
//}
//
//void ShaderProgram::renderLights() {
//    for (int i = 0; i < 10; i++) {
//        Light* light = nullptr;
//
//        if (i < lights.size()) {
//            light = lights[i];
//        }
//
//        if (light == nullptr) {
//            light = new Light(fvec3(), fvec3(), fvec3());
//        }
//
//        setUniform(std::string("lights[") + std::to_string(i) + "]", light);
//    }
//
//    lights.clear();
//}

void ShaderProgram::useProgram(bool use) const {
	if (use) {
		glUseProgram(programID);
	}
	else {
		glUseProgram(0);
	}
}

uint32 ShaderProgram::getProgramID() const {
	return programID;
}

bool ShaderProgram::isComplete() const {
	return completed;
}

void ShaderProgram::setUniform(std::string uniform, float f) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform1f(id, f);
	}
}

void ShaderProgram::setUniform(std::string uniform, float f, float f1) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform2f(id, f, f1);
	}
}

void ShaderProgram::setUniform(std::string uniform, float f, float f1, float f2) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform3f(id, f, f1, f2);
	}
}

void ShaderProgram::setUniform(std::string uniform, float f, float f1, float f2, float f3) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform4f(id, f, f1, f2, f3);
	}
}

void ShaderProgram::setUniform(std::string uniform, double d) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform1d(id, d);
	}
}

void ShaderProgram::setUniform(std::string uniform, double d, double d1) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform2d(id, d, d1);
	}
}

void ShaderProgram::setUniform(std::string uniform, double d, double d1, double d2) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform3d(id, d, d1, d2);
	}
}

void ShaderProgram::setUniform(std::string uniform, double d, double d1, double d2, double d3) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform4d(id, d, d1, d2, d3);
	}
}

void ShaderProgram::setUniform(std::string uniform, int i) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform1i(id, i);
	}
}

void ShaderProgram::setUniform(std::string uniform, int i, int i1) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform2i(id, i, i1);
	}
}

void ShaderProgram::setUniform(std::string uniform, int i, int i1, int i2) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform3i(id, i, i1, i2);
	}
}

void ShaderProgram::setUniform(std::string uniform, int i, int i1, int i2, int i3) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform4i(id, i, i1, i2, i3);
	}
}

void ShaderProgram::setUniform(std::string uniform, bool b) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniform1i(id, b);
	}
}

void ShaderProgram::setUniform(std::string uniform, fvec2 v) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		setUniform(uniform, v.x, v.y);
	}
}

void ShaderProgram::setUniform(std::string uniform, fvec3 v) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		setUniform(uniform, v.x, v.y, v.z);
	}
}

void ShaderProgram::setUniform(std::string uniform, fvec4 v) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		setUniform(uniform, v.x, v.y, v.z, v.w);
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat2x2 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix2fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat2x3 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix2x3fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat2x4 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix2x4fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat3x2 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix3x2fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat3x3 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix3fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat3x4 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix3x4fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat4x2 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix4x2fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat4x3 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix4x3fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, fmat4x4 m) {
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix4fv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
}

void ShaderProgram::setUniform(std::string uniform, dvec2 v) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		setUniform(uniform, v.x, v.y);
	}
#else
	this->setUniform(uniform, fvec2(v));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dvec3 v) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		setUniform(uniform, v.x, v.y, v.z);
	}
#else
	this->setUniform(uniform, fvec3(v));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dvec4 v) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		setUniform(uniform, v.x, v.y, v.z, v.w);
	}
#else
	this->setUniform(uniform, fvec4(v));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat2x2 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix2dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	this->setUniform(uniform, fmat2x2(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat2x3 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix2x3dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	this->setUniform(uniform, fmat2x3(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat2x4 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix2x4dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	this->setUniform(uniform, fmat2x4(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat3x2 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix3x2dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	this->setUniform(uniform, fmat3x2(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat3x3 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix3dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	this->setUniform(uniform, fmat3x3(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat3x4 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix3x4dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	this->setUniform(uniform, fmat3x4(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat4x2 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix4x2dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	this->setUniform(uniform, fmat4x2(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat4x3 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix4x3dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	this->setUniform(uniform, fmat4x3(m));
#endif
}

void ShaderProgram::setUniform(std::string uniform, dmat4x4 m) {
#ifdef DOUBLE_PRECISION_UNIFORMS_ENABLED
	int32 id = getUniform(uniform);

	if (id >= 0) {
		glUniformMatrix4dv(id, 1, GL_FALSE, glm::value_ptr(m));
	}
#else
	this->setUniform(uniform, fmat4x4(m));
#endif
}

//void ShaderProgram::setUniform(std::string uniform, Light* light) {
//    setUniform((char*)(std::string(uniform) + ".position").c_str(), light->getPosition());
//    setUniform((char*)(std::string(uniform) + ".colour").c_str(), light->getColour());
////    logInfo("%f, %f, %f", light->getColour().x, light->getColour().y, light->getColour().z);
//    setUniform((char*)(std::string(uniform) + ".attenuation").c_str(), light->getAttenuation());
//    setUniform((char*)(std::string(uniform) + ".direction").c_str(), light->getDirection());
//    setUniform((char*)(std::string(uniform) + ".innerRadius").c_str(), light->getInnerCone());
//    setUniform((char*)(std::string(uniform) + ".outerRadius").c_str(), light->getOuterCone());
//    setUniform((char*)(std::string(uniform) + ".type").c_str(), light->getType());
//}

int32 ShaderProgram::getUniform(std::string uniform) {
	int32 id = 0;
	if (uniforms.count(uniform) != 1) {
	    id = glGetUniformLocation(programID, uniform.c_str());
	
	    logInfo("Caching uniform loaction \"%s\" with id %d", uniform.c_str(), id);
	
	    uniforms.insert(std::make_pair(uniform, id));
	} else {
	    id = uniforms.at(uniform);
	}
	
	return id;
}


Shader::Shader(uint32 type, uint32 id, std::string file, std::string source) :
	program(0), id(id), type(type), file(file), source(source) {}

Shader::~Shader() {
	glDetachShader(this->program, this->id);
	glDeleteShader(this->id);
}

uint32 Shader::getPorgram() const {
	return this->program;
}

uint32 Shader::getType() const {
	return this->type;
}

uint32 Shader::getID() const {
	return this->id;
}

std::string Shader::getFile() const {
	return this->file;
}

std::string Shader::getSource() const {
	return this->source;
}

void Shader::attachTo(uint32 program) {
	this->program = program;
	glAttachShader(program, this->id);
}

std::string Shader::getShaderAsString(uint32 type) {
	if (type == GL_VERTEX_SHADER) return "GL_VERTEX_SHADER";
	if (type == GL_FRAGMENT_SHADER) return "GL_FRAGMENT_SHADER";
	if (type == GL_GEOMETRY_SHADER) return "GL_GEOMETRY_SHADER";
	if (type == GL_COMPUTE_SHADER) return "GL_COMPUTE_SHADER";
	if (type == GL_TESS_EVALUATION_SHADER) return "GL_TESS_EVALUATION_SHADER";
	if (type == GL_TESS_CONTROL_SHADER) return "GL_TESS_CONTROL_SHADER";

	return "INVALID_SHADER_TYPE";
}