#pragma once

#include "core/Core.h"

typedef enum IntersectionType {
	NO_INTERSECTION = 0, // The two bounds are not intersecting
	PARTIAL_INTERSECTION = 1, // The two bounds are intersecting partially
	FULL_INTERSECTION = 2, // One of the bounds is fully enclosed by the other
} IntersectionType;

typedef enum FrustumCorner {
	FRUSTUM_LEFT_TOP_NEAR = 0,
	FRUSTUM_RIGHT_TOP_NEAR = 1,
	FRUSTUM_RIGHT_BOTTOM_NEAR = 2,
	FRUSTUM_LEFT_BOTTOM_NEAR = 3,
	FRUSTUM_LEFT_TOP_FAR = 4,
	FRUSTUM_RIGHT_TOP_FAR = 5,
	FRUSTUM_RIGHT_BOTTOM_FAR = 6,
	FRUSTUM_LEFT_BOTTOM_FAR = 7,
} FrustumCorner;

typedef enum FrustumEdge {
	FRUSTUM_LEFT_TOP = 0,
	FRUSTUM_RIGHT_TOP = 1,
	FRUSTUM_RIGHT_BOTTOM = 2,
	FRUSTUM_LEFT_BOTTOM = 3,
	FRUSTUM_LEFT_NEAR = 4,
	FRUSTUM_TOP_NEAR = 5,
	FRUSTUM_RIGHT_NEAR = 6,
	FRUSTUM_BOTTOM_NEAR = 7,
	FRUSTUM_LEFT_FAR = 8,
	FRUSTUM_TOP_FAR = 9,
	FRUSTUM_RIGHT_FAR = 10,
	FRUSTUM_BOTTOM_FAR = 11,
} FrustumEdge;

typedef enum FrustumPlane {
	FRUSTUM_LEFT = 0,
	FRUSTUM_RIGHT = 1,
	FRUSTUM_BOTTOM = 2,
	FRUSTUM_TOP = 3,
	FRUSTUM_NEAR = 4,
	FRUSTUM_FAR = 5
} FrustumPlane;

typedef dvec4 Plane;

struct Ray {
	dvec3 orig;
	dvec3 dir;
	dvec3 invDir;

	Ray(): orig(0.0), dir(0.0), invDir(0.0) {} // default constructor, produces an invalid ray though

	Ray(dvec3 orig, dvec3 dir, bool normalized = false) {
		this->orig = orig;
		this->dir = normalized ? dir : normalize(dir);
		this->invDir = 1.0 / this->dir;
	}

	Ray operator*(const dmat4& mat) {
		dvec3 nOrig = mat * dvec4(this->orig, 1.0);
		dvec3 nDir = mat * dvec4(this->dir, 0.0);
		return Ray(nOrig, nDir);
	}
};

struct LineSegment : public Ray {
	double length;

	LineSegment(): length(0.0), Ray() {}
	
	LineSegment(dvec3 v0, dvec3 v1): length(glm::length(v1 - v0)), Ray(v0, (v1 - v0) / this->length, true) {}

	LineSegment(dvec3 orig, dvec3 dir, double length): length(length), Ray(orig, dir) {}
	
	LineSegment operator*(const dmat4& mat) {
		dvec3 v0 = mat * dvec4(this->orig, 1.0);
		dvec3 v1 = mat * dvec4(this->orig + this->dir * this->length, 1.0);
		return LineSegment(v0, v1);
	}
};

class BoundingVolume {
public:
	BoundingVolume() {}

	virtual ~BoundingVolume() {}

	virtual dvec3 getCenter() const = 0;

	virtual dvec3 getClosestPoint(dvec3 point) const = 0;

	virtual double getDistance(dvec3 point) const = 0;

	virtual double getDistanceSq(dvec3 point) const = 0;

	virtual double getVolume() const = 0;

	virtual double getSurfaceArea() const = 0;

	virtual IntersectionType intersectsPoint(dvec3 point) const = 0;

	virtual IntersectionType intersectsRay(Ray ray, double* distance = NULL) const = 0;

	virtual IntersectionType intersectsSegment(LineSegment segment) const = 0;

	virtual IntersectionType intersectsPlane(Plane plane, bool reversed = false) const = 0;

	virtual IntersectionType intersectsBounds(BoundingVolume* bound) const = 0;
};

class AxisAlignedBB : public BoundingVolume {
private:
	dvec3 minBound;
	dvec3 maxBound;

public:
	AxisAlignedBB(dvec3 a, dvec3 b, bool minmax = true);

	AxisAlignedBB(std::vector<dvec3> pointCloud);

	void setMinMax(dvec3 minBound, dvec3 maxBound);

	void setCenterExtents(dvec3 center, dvec3 fullExtents);

	void setPointCloud(std::vector<dvec3> pointCloud);

	void expandToFit(dvec3 point);

	dvec3 getMin() const;

	dvec3 getMax() const;

	dvec3 getFullExtents() const;

	dvec3 getHalfExtents() const;

	dvec3 getCenter() const override;

	dvec3 getClosestPoint(dvec3 point) const override;

	double getDistance(dvec3 point) const override;

	double getDistanceSq(dvec3 point) const override;

	double getVolume() const override;

	double getSurfaceArea() const override;

	IntersectionType intersectsPoint(dvec3 point) const override;

	IntersectionType intersectsRay(Ray ray, double* distance = NULL) const override;

	IntersectionType intersectsSegment(LineSegment segment) const override;

	IntersectionType intersectsPlane(Plane plane, bool reversed = false) const override;

	IntersectionType intersectsBounds(BoundingVolume* bound) const override;
};

//class BoundingHull : public BoundingVolume {
//private:
//	std::vector<dvec3> points;
//
//public:
//	BoundingHull(std::vector<dvec3> points);
//
//	~BoundingHull();
//
//	AxisAlignedBB getEnclosingAABB() const;
//
//	//OrientedBB getEnclosingOBB() const;
//
//	void expandPoint(dvec3 point);
//
//	std::vector<dvec3> getHorizonRing(dvec3 point);
//
//	dvec3 getPoint(int32 index) const;
//
//	dvec3 getCenter() const override;
//
//	dvec3 getClosestPoint(dvec3 point) const override;
//
//	double getDistance(dvec3 point) const override;
//
//	double getDistanceSq(dvec3 point) const override;
//
//	double getVolume() const override;
//
//	double getSurfaceArea() const override;
//
//	IntersectionType intersectsPoint(dvec3 point) const override;
//
//	IntersectionType intersectsRay(Ray ray, double* distance = NULL) const override;
//
//  IntersectionType intersectsSegment(LineSegment segment) const override;
//
//	IntersectionType intersectsPlane(Plane plane, bool reversed = false) const override;
//
//	IntersectionType intersectsBounds(BoundingVolume* bound) const override;
//};

class Frustum : public BoundingVolume {
private:
	Plane planes[6];
	LineSegment edges[12];
	dvec3 corners[8];
	dvec3 center;
	double length;

	dvec3 tripplePlaneIntersection(const Plane p0, const Plane p1, const Plane p2);
public:
	Frustum(const Plane left, const Plane right, const Plane bottom, const Plane top, const Plane near, const Plane far);

	Frustum(const dvec3 ltn, const dvec3 rtn, const dvec3 rbn, const dvec3 lbn, const dvec3 ltf, const dvec3 rtf, const dvec3 rbf, const dvec3 lbf);

	Frustum(const dmat4 viewProjection);

	~Frustum();

	Frustum* set(const Plane left, const Plane right, const Plane bottom, const Plane top, const Plane near, const  Plane far);

	Frustum* set(const dvec3 ltn, const dvec3 rtn, const dvec3 rbn, const dvec3 lbn, const dvec3 ltf, const dvec3 rtf, const dvec3 rbf, const dvec3 lbf);
	
	Frustum* set(const dmat4 viewProjection);

	void renderDebug(bool lines) const;

	double getLength() const;

	Plane getPlane(FrustumPlane plane) const;

	LineSegment getEdge(FrustumEdge edge) const;

	dvec3 getCorner(FrustumCorner corner) const;

	dvec3 getCenter() const override;

	dvec3 getClosestPoint(dvec3 point) const override;

	double getDistance(dvec3 point) const override;

	double getDistanceSq(dvec3 point) const override;

	double getVolume() const override;

	double getSurfaceArea() const override;

	IntersectionType intersectsPoint(dvec3 point) const override;

	IntersectionType intersectsRay(Ray ray, double* distance = NULL) const override;

	IntersectionType intersectsSegment(LineSegment segment) const override;

	IntersectionType intersectsPlane(Plane plane, bool reversed = false) const override;

	IntersectionType intersectsBounds(BoundingVolume* bound) const override;
};

class BoundingOctal : public BoundingVolume {
private:
	dvec3 corners[8];

public:

};

namespace IntersectionTests {
	const double EPS = 1e-9;

	IntersectionType intersects(const AxisAlignedBB* aabb0, const AxisAlignedBB* aabb1);

	IntersectionType intersects(const AxisAlignedBB* aabb, const Frustum* frustum);

	IntersectionType intersects(const Frustum* frustum0, const Frustum* frustum1);

	IntersectionType intersects(const Ray ray, const Plane plane, double* distance = NULL);

	bool raySphereIntersection(const Ray ray, const dvec3 sphereOrigin, const double sphereRadius, bool allowInside = false, double* distance = NULL);

	bool axisProjectionSAT(dvec3* ap, dvec3* bp, int32 count, dvec3 axis);

	double getSignedPlaneDistance(const dvec3 point, const Plane plane);
}