#include "Transformation.h"


Transformation::Transformation(mat4 matrix):
	translation(fvec3()), axisVectors(mat3(1.0F)), axisScale(fvec3(1.0F)) {

	this->setFromMatrix(matrix);
}

Transformation::Transformation(fvec3 translation, quat orientation, fvec3 scale) :
	translation(fvec3()), axisVectors(mat3(1.0F)), axisScale(fvec3(1.0F)) {

	this->setTranslation(translation);
	this->setOrientation(orientation);
	this->setScale(scale);
}

Transformation::~Transformation() {}

fvec3 Transformation::getTranslation() const {
	return this->translation;
}

Transformation* Transformation::setTranslation(fvec3 translation) {
	this->translation = translation;
	return this;
}

Transformation* Transformation::setTranslation(double x, double y, double z) {
	this->setTranslation(vec3(x, y, z));
	return this;
}

Transformation* Transformation::translate(fvec3 translation, mat3 axis) {
	this->translation += axis[0] * translation.x + axis[1] * translation.y + axis[2] * translation.z;
	return this;
}

Transformation* Transformation::translate(double x, double y, double z, mat3 axis) {
	this->translation += axis[0] * float(x) + axis[1] * float(y) + axis[2] * float(z);
	return this;
}

quat Transformation::getOrientation() const {
	return quat_cast(this->axisVectors);
}

Transformation* Transformation::setOrientation(quat orientation, bool normalized) {
	if (!normalized) {
		this->setAxisVectors(mat3_cast(normalize(orientation)), true);
	} else {
		this->setAxisVectors(mat3_cast(orientation), true);
	}
	return this;
}

Transformation* Transformation::rotate(quat orientation, bool normalized) {
	if (!normalized) {
		this->setOrientation(normalize(orientation) * this->getOrientation(), true);
	} else {
		this->setOrientation(orientation * this->getOrientation(), true);
	}
	return this;
}

Transformation* Transformation::rotate(fvec3 axis, float angle, bool normalized) {
	if (!normalized) {
		this->rotate(glm::angleAxis(angle, normalize(axis)), true);
	} else {
		this->rotate(glm::angleAxis(angle, axis), true);
	}
	return this;
}

mat4 Transformation::getAxisVectors() const {
	return this->axisVectors;
}

Transformation* Transformation::setAxisVectors(mat3 axis, bool normalized) {
	if (!normalized) {
		this->axisVectors[0] = normalize(axis[0]);
		this->axisVectors[1] = normalize(axis[1]);
		this->axisVectors[2] = normalize(axis[2]);
	} else {
		this->axisVectors[0] = axis[0];
		this->axisVectors[1] = axis[1];
		this->axisVectors[2] = axis[2];
	}
	return this;
}

Transformation* Transformation::setAxisVectors(fvec3 x, fvec3 y, fvec3 z, bool normalized) {
	this->setAxisVectors(mat3(x, y, z), normalized);
	return this;
}

fvec3 Transformation::getScale() const {
	return this->axisScale;
}

Transformation* Transformation::setScale(fvec3 scale) {
	this->axisScale = scale;
	return this;
}

Transformation* Transformation::setScale(double scale) {
	this->axisScale = vec3(scale);
	return this;
}

Transformation* Transformation::scale(fvec3 scale) {
	this->axisScale *= scale;
	return this;
}

Transformation* Transformation::scale(double scale) {
	this->axisScale *= scale;
	return this;
}

Transformation* Transformation::setFromMatrix(mat4 matrix, bool normalized) {
	this->translation = vec3(matrix[3]);
	this->axisVectors = mat3(matrix);

	if (!normalized) {
		double lx = length(this->axisVectors[0]);
		double ly = length(this->axisVectors[1]);
		double lz = length(this->axisVectors[2]);

		this->axisVectors[0] /= lx;
		this->axisVectors[1] /= ly;
		this->axisVectors[2] /= lz;

		this->axisScale = vec3(lx, ly, lz);
	} else {
		this->axisScale = fvec3(1.0);
	}
	return this;
}

mat4 Transformation::getMatrix() const {
	mat4 m;
	m[0] = vec4(this->axisVectors[0] * this->axisScale.x, 0.0);
	m[1] = vec4(this->axisVectors[1] * this->axisScale.y, 0.0);
	m[2] = vec4(this->axisVectors[2] * this->axisScale.z, 0.0);
	m[3] = vec4(this->translation, 1.0);

	return m;
}

Transformation Transformation::operator*(const Transformation& t) const {
	return Transformation(t.getMatrix() * this->getMatrix());
}

Transformation Transformation::operator*=(const Transformation& t) {
	this->setFromMatrix(t.getMatrix() * this->getMatrix());
	return *this;
}