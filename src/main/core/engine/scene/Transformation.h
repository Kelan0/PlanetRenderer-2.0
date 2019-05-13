#pragma once

#include "core/Core.h"

class Transformation
{
private:
	fvec3 translation;
	mat3 axisVectors;
	fvec3 axisScale;

public:
	Transformation(mat4 matrix = mat4(1.0));

	Transformation(fvec3 translation, quat orientation = quat(), fvec3 scale = fvec3(1.0));

	~Transformation();

	fvec3 getTranslation() const;

	Transformation* setTranslation(fvec3 translation);

	Transformation* setTranslation(double x, double y, double z);

	Transformation* translate(fvec3 translation, mat3 axis = mat3(1.0));

	Transformation* translate(double x, double y, double z, mat3 axis = mat3(1.0));

	quat getOrientation() const;

	Transformation* setOrientation(quat orientation, bool normalized = false);

	Transformation* rotate(quat orientation, bool normalized = false);

	Transformation* rotate(fvec3 axis, float angle, bool normalized = false);

	mat4 getAxisVectors() const;

	Transformation* setAxisVectors(mat3 axis = mat3(1.0), bool normalized = false);

	Transformation* setAxisVectors(fvec3 x, fvec3 y, fvec3 z, bool normalized = false);

	fvec3 getScale() const;

	Transformation* setScale(fvec3 scale);

	Transformation* setScale(double scale);

	Transformation* scale(fvec3 scale);

	Transformation* scale(double scale);

	Transformation* setFromMatrix(mat4 matrix, bool normalized = false);

	mat4 getMatrix() const;

	Transformation operator*(const Transformation& t) const;

	Transformation operator*=(const Transformation& t);
};

