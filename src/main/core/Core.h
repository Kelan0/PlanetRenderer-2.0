#pragma once

#include <iostream>

#include <sstream>
#include <fstream>

#include <functional>
#include <vector>
#include <queue>
#include <stack>
#include <set>
#include <map>
#include <filesystem>
#include <unordered_map>
#include <string>
#include <math.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>

typedef signed long long int64;
typedef signed int int32;
typedef signed short int16;
typedef signed char int8;

typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

typedef float float32;
typedef double float64;

typedef uint64 TimePoint;

typedef glm::vec<1, int32, glm::highp>     ivec1;
typedef glm::vec<2, int32, glm::highp>     ivec2;
typedef glm::vec<3, int32, glm::highp>     ivec3;
typedef glm::vec<4, int32, glm::highp>     ivec4;

typedef glm::vec<1, uint32, glm::highp>    uvec1;
typedef glm::vec<2, uint32, glm::highp>    uvec2;
typedef glm::vec<3, uint32, glm::highp>    uvec3;
typedef glm::vec<4, uint32, glm::highp>    uvec4;
										   
typedef glm::qua<float, glm::highp>        fquat;
typedef glm::qua<double, glm::highp>       dquat;
										   
typedef glm::vec<1, float, glm::highp>     fvec1;
typedef glm::vec<2, float, glm::highp>     fvec2;
typedef glm::vec<3, float, glm::highp>     fvec3;
typedef glm::vec<4, float, glm::highp>     fvec4;
typedef glm::mat<2, 2, float, glm::highp>  fmat2;
typedef glm::mat<3, 3, float, glm::highp>  fmat3;
typedef glm::mat<4, 4, float, glm::highp>  fmat4;
typedef	glm::mat<2, 2, float, glm::highp>  fmat2x2;
typedef	glm::mat<3, 2, float, glm::highp>  fmat3x2;
typedef	glm::mat<4, 2, float, glm::highp>  fmat4x2;
typedef	glm::mat<2, 3, float, glm::highp>  fmat2x3;
typedef	glm::mat<3, 3, float, glm::highp>  fmat3x3;
typedef	glm::mat<4, 3, float, glm::highp>  fmat4x3;
typedef	glm::mat<2, 4, float, glm::highp>  fmat2x4;
typedef	glm::mat<3, 4, float, glm::highp>  fmat3x4;
typedef	glm::mat<4, 4, float, glm::highp>  fmat4x4;

typedef glm::vec<1, double, glm::highp>    dvec1;
typedef glm::vec<2, double, glm::highp>    dvec2;
typedef glm::vec<3, double, glm::highp>    dvec3;
typedef glm::vec<4, double, glm::highp>    dvec4;
typedef glm::mat<2, 2, double, glm::highp> dmat2;
typedef glm::mat<3, 3, double, glm::highp> dmat3;
typedef glm::mat<4, 4, double, glm::highp> dmat4;
typedef	glm::mat<2, 2, double, glm::highp> dmat2x2;
typedef	glm::mat<3, 2, double, glm::highp> dmat3x2;
typedef	glm::mat<4, 2, double, glm::highp> dmat4x2;
typedef	glm::mat<2, 3, double, glm::highp> dmat2x3;
typedef	glm::mat<3, 3, double, glm::highp> dmat3x3;
typedef	glm::mat<4, 3, double, glm::highp> dmat4x3;
typedef	glm::mat<2, 4, double, glm::highp> dmat2x4;
typedef	glm::mat<3, 4, double, glm::highp> dmat3x4;
typedef	glm::mat<4, 4, double, glm::highp> dmat4x4;

// Default, single precision floating point vectors.
										   
typedef fquat   quat;

typedef fvec1   vec1;
typedef fvec2   vec2;
typedef fvec3   vec3;
typedef fvec4   vec4;
			    
typedef fmat2   mat2;
typedef fmat3   mat3;
typedef fmat4   mat4;
typedef fmat2x2 mat2x2;
typedef fmat3x2 mat3x2;
typedef fmat4x2 mat4x2;
typedef fmat2x3 mat2x3;
typedef fmat3x3 mat3x3;
typedef fmat4x3 mat4x3;
typedef fmat2x4 mat2x4;
typedef fmat3x4 mat3x4;
typedef fmat4x4 mat4x4;

#define TWO_PI glm::two_pi<double>()
#define PI glm::pi<double>()
#define HALF_PI glm::half_pi<double>()
#define QUARTER_PI glm::quarter_pi<double>()

#define ROOT_TWO glm::root_two<double>()
