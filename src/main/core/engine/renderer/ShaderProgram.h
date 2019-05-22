#pragma once

#include "core/Core.h"

class ShaderProgram;
class Shader;
struct FragmentDataLocation;
struct FragmentOutput;

//#define DOUBLE_PRECISION_UNIFORMS_ENABLED


#define DEFAULT_FRAGMENT_OUTPUTS FragmentOutput({	\
	FragmentDataLocation("outDiffuse", 0),			\
	FragmentDataLocation("outGlow", 1),				\
	FragmentDataLocation("outNormal", 2),			\
	FragmentDataLocation("outPosition", 3),			\
	FragmentDataLocation("outSpecularEmission", 4),	\
})

struct FragmentDataLocation {
	std::string name;
	uint32 index;

	FragmentDataLocation(std::string name, uint32 index):
		name(name), index(index) {}
};

struct FragmentOutput {
	std::vector<FragmentDataLocation> dataLocations;

	FragmentOutput(std::vector<FragmentDataLocation> dataLocations) :
		dataLocations(dataLocations) {}
};

class ShaderProgram
{
private:
	std::map<uint32, Shader*> shaders;
	std::unordered_map<std::string, int32> uniforms;
	std::unordered_map<std::string, int32> attributes;
	std::unordered_map<std::string, int32> dataLocations;
	//    std::vector<Light*> lights;
	uint32 programID;
	bool completed;

public:
	ShaderProgram(FragmentOutput fragmentOutput = DEFAULT_FRAGMENT_OUTPUTS);
	~ShaderProgram();

	void addShader(Shader* shader);

	void addShader(uint32 type, std::string file);

	void addAttribute(int32 index, std::string name);

	void addDataLocation(int32 index, std::string name);

	void setDataLocations(FragmentOutput locations);

	void completeProgram();

	//    void addLight(Light* light);

	//    void renderLights();

	void useProgram(bool use) const;

	uint32 getProgramID() const;

	bool isComplete() const;

	void setUniform(std::string uniform, float f);

	void setUniform(std::string uniform, float f, float f1);

	void setUniform(std::string uniform, float f, float f1, float f2);

	void setUniform(std::string uniform, float f, float f1, float f2, float f3);

	void setUniform(std::string uniform, double d);

	void setUniform(std::string uniform, double d, double d1);

	void setUniform(std::string uniform, double d, double d1, double d2);

	void setUniform(std::string uniform, double d, double d1, double d2, double d3);

	void setUniform(std::string uniform, int i);

	void setUniform(std::string uniform, int i, int i1);

	void setUniform(std::string uniform, int i, int i1, int i2);

	void setUniform(std::string uniform, int i, int i1, int i2, int i3);

	void setUniform(std::string uniform, bool b);

	void setUniform(std::string uniform, fvec2 v);

	void setUniform(std::string uniform, fvec3 v);

	void setUniform(std::string uniform, fvec4 v);

	void setUniform(std::string uniform, fmat2x2 m);

	void setUniform(std::string uniform, fmat2x3 m);

	void setUniform(std::string uniform, fmat2x4 m);

	void setUniform(std::string uniform, fmat3x2 m);

	void setUniform(std::string uniform, fmat3x3 m);

	void setUniform(std::string uniform, fmat3x4 m);

	void setUniform(std::string uniform, fmat4x2 m);

	void setUniform(std::string uniform, fmat4x3 m);

	void setUniform(std::string uniform, fmat4x4 m);

	void setUniform(std::string uniform, dvec2 v);

	void setUniform(std::string uniform, dvec3 v);

	void setUniform(std::string uniform, dvec4 v);

	void setUniform(std::string uniform, dmat2x2 m);

	void setUniform(std::string uniform, dmat2x3 m);

	void setUniform(std::string uniform, dmat2x4 m);

	void setUniform(std::string uniform, dmat3x2 m);

	void setUniform(std::string uniform, dmat3x3 m);

	void setUniform(std::string uniform, dmat3x4 m);

	void setUniform(std::string uniform, dmat4x2 m);

	void setUniform(std::string uniform, dmat4x3 m);

	void setUniform(std::string uniform, dmat4x4 m);

	//    void setUniform(std::string uniform, Light* light);

	//    void* getUniformValue(std::string uniform);

	int32 getUniform(std::string uniform);
};


class Shader
{
private:
	uint32 program;
	uint32 type;
	uint32 id;
	std::string file;
	std::string source;

public:
	Shader(uint32 type, uint32 id, std::string file, std::string source);

	~Shader();

	uint32 getPorgram() const;

	uint32 getType() const;

	uint32 getID() const;

	/**
	 * The file path of this shader
	 */
	std::string getFile() const;

	/**
	 * The source code for this shader
	 */
	std::string getSource() const;

	void attachTo(uint32 program);

	static std::string getShaderAsString(uint32 type);
};