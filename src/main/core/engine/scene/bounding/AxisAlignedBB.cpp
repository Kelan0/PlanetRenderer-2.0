#include "BoundingVolume.h"

AxisAlignedBB::AxisAlignedBB(dvec3 a, dvec3 b, bool minmax) {
	if (minmax) {
		this->setMinMax(a, b);
	} else {
		this->setCenterExtents(a, b);
	}
}

AxisAlignedBB::AxisAlignedBB(std::vector<dvec3> pointCloud) {
	this->setPointCloud(pointCloud);
}

void AxisAlignedBB::setMinMax(dvec3 minBound, dvec3 maxBound) {
	this->minBound = glm::min(minBound, maxBound);
	this->maxBound = glm::max(minBound, maxBound);
}

void AxisAlignedBB::setCenterExtents(dvec3 center, dvec3 fullExtents) {
	this->setMinMax(center - fullExtents * 0.5, center + fullExtents * 0.5);
}

void AxisAlignedBB::setPointCloud(std::vector<dvec3> pointCloud) {
	assert(!pointCloud.empty());

	this->minBound = dvec3(+INFINITY);
	this->maxBound = dvec3(-INFINITY);

	for (int i = 0; i < pointCloud.size(); i++) {
		this->expandToFit(pointCloud[i]);
	}
}

void AxisAlignedBB::expandToFit(dvec3 point) {
	assert(!isnan(point.x + point.y + point.z) && isfinite(point.x + point.y + point.z));

	this->minBound = glm::min(minBound, point);
	this->maxBound = glm::max(maxBound, point);
}

dvec3 AxisAlignedBB::getMin() const {
	return this->minBound;
}

dvec3 AxisAlignedBB::getMax() const {
	return this->maxBound;
}

dvec3 AxisAlignedBB::getFullExtents() const {
	return this->maxBound - this->minBound;
}

dvec3 AxisAlignedBB::getHalfExtents() const {
	return (this->maxBound - this->minBound) * 0.5;
}

dvec3 AxisAlignedBB::getCenter() const {
	return (this->maxBound + this->minBound) * 0.5;
}

dvec3 AxisAlignedBB::getClosestPoint(dvec3 point) const {
	return glm::clamp(point, this->minBound, this->maxBound);
}

double AxisAlignedBB::getDistance(dvec3 point) const {
	return glm::distance(point, this->getClosestPoint(point));
}

double AxisAlignedBB::getDistanceSq(dvec3 point) const {
	return glm::distance2(point, this->getClosestPoint(point));
}

double AxisAlignedBB::getVolume() const {
	dvec3 extents = this->getFullExtents();
	return extents.x * extents.y * extents.z;
}

double AxisAlignedBB::getSurfaceArea() const {
	dvec3 extents = this->getFullExtents();
	return (extents.x * extents.y + extents.y * extents.z + extents.z * extents.x) * 2.0;
}

IntersectionType AxisAlignedBB::intersectsPoint(dvec3 point) const {
	if (point.x < minBound.x || point.x > maxBound.x) return NO_INTERSECTION;
	if (point.y < minBound.y || point.y > maxBound.y) return NO_INTERSECTION;
	if (point.z < minBound.z || point.z > maxBound.z) return NO_INTERSECTION;

	return FULL_INTERSECTION;
}

IntersectionType AxisAlignedBB::intersectsRay(Ray ray, double* distance) const {

	double t1 = (this->minBound.x - ray.orig.x) * ray.invDir.x;
	double t2 = (this->maxBound.x - ray.orig.x) * ray.invDir.x;
	double t3 = (this->minBound.y - ray.orig.y) * ray.invDir.y;
	double t4 = (this->maxBound.y - ray.orig.y) * ray.invDir.y;
	double t5 = (this->minBound.z - ray.orig.z) * ray.invDir.z;
	double t6 = (this->maxBound.z - ray.orig.z) * ray.invDir.z;

	double tmin = glm::max(glm::max(glm::min(t1, t2), glm::min(t3, t4)), glm::min(t5, t6));
	double tmax = glm::min(glm::min(glm::max(t1, t2), glm::max(t3, t4)), glm::max(t5, t6));

	if (tmax < 0) {
		if (distance != NULL) *distance = tmax;
		return NO_INTERSECTION;
	}

	if (tmin > tmax) {
		if (distance != NULL) *distance = tmax;
		return NO_INTERSECTION;
	}

	if (distance != NULL) *distance = tmin;
	return FULL_INTERSECTION;
}

IntersectionType AxisAlignedBB::intersectsSegment(LineSegment segment) const {
	dvec3 dir = segment.dir;
	double distance;

	if (this->intersectsRay(segment, &distance)) {
		if (distance < segment.length) {
			return FULL_INTERSECTION;
		}
	}

	return NO_INTERSECTION;
}

IntersectionType AxisAlignedBB::intersectsPlane(Plane plane, bool reversed) const {
	double x0 = minBound.x * plane.x;
	double x1 = maxBound.x * plane.x;
	double y0 = minBound.y * plane.y;
	double y1 = maxBound.y * plane.y;
	double z0 = minBound.z * plane.z;
	double z1 = maxBound.z * plane.z;

	// dot(v, p.xyz) + p.w,   where v = a corner vertex of the box and p = the plane.
	double p1 = x0 + y0 + z0 + plane.w;
	double p2 = x1 + y0 + z0 + plane.w;
	double p3 = x1 + y1 + z0 + plane.w;
	double p4 = x0 + y1 + z0 + plane.w;
	double p5 = x0 + y0 + z1 + plane.w;
	double p6 = x1 + y0 + z1 + plane.w;
	double p7 = x1 + y1 + z1 + plane.w;
	double p8 = x0 + y1 + z1 + plane.w;

	if (p1 <= 0 && p2 <= 0 && p3 <= 0 && p4 <= 0 && p5 <= 0 && p6 <= 0 && p7 <= 0 && p8 <= 0) {
		return reversed ? FULL_INTERSECTION : NO_INTERSECTION;
	}
	if (p1 > 0 && p2 > 0 && p3 > 0 && p4 > 0 && p5 > 0 && p6 > 0 && p7 > 0 && p8 > 0) {
		return reversed ? NO_INTERSECTION : FULL_INTERSECTION;
	}

	return PARTIAL_INTERSECTION;
}

IntersectionType AxisAlignedBB::intersectsBounds(BoundingVolume * bound) const {

	if (AxisAlignedBB* aabb = dynamic_cast<AxisAlignedBB*>(bound)) {
		return IntersectionTests::intersects(this, aabb);
	}

	if (Frustum* frustum = dynamic_cast<Frustum*>(bound)) {
		return IntersectionTests::intersects(this, frustum);
	}

	return NO_INTERSECTION;
}
