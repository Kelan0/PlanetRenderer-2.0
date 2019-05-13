#include "BoundingVolume.h"

IntersectionType IntersectionTests::intersects(const AxisAlignedBB* aabb0, const AxisAlignedBB* aabb1) {
	if (aabb0 == NULL || aabb1 == NULL) {
		return NO_INTERSECTION;
	}

	dvec3 a0 = aabb0->getMin();
	dvec3 b0 = aabb0->getMax();
	dvec3 a1 = aabb1->getMin();
	dvec3 b1 = aabb1->getMax();

	if (a0.x > b1.x || b0.x < a1.x) return NO_INTERSECTION;
	if (a0.y > b1.y || b0.y < a1.y) return NO_INTERSECTION;
	if (a0.z > b1.z || b0.z < a1.z) return NO_INTERSECTION;

	return FULL_INTERSECTION; // TODO: partial intersection.
}

IntersectionType IntersectionTests::intersects(const AxisAlignedBB * aabb, const Frustum * frustum) {
	if (aabb == NULL || frustum == NULL) {
		return NO_INTERSECTION;
	}

	dvec3 a = aabb->getMin();
	dvec3 b = aabb->getMin();
	bool v0 = frustum->intersectsPoint(dvec3(a.x, a.y, a.z));
	bool v1 = frustum->intersectsPoint(dvec3(b.x, a.y, a.z));
	bool v2 = frustum->intersectsPoint(dvec3(b.x, b.y, a.z));
	bool v3 = frustum->intersectsPoint(dvec3(a.x, b.y, a.z));
	bool v4 = frustum->intersectsPoint(dvec3(a.x, a.y, b.z));
	bool v5 = frustum->intersectsPoint(dvec3(b.x, a.y, b.z));
	bool v6 = frustum->intersectsPoint(dvec3(b.x, b.y, b.z));
	bool v7 = frustum->intersectsPoint(dvec3(a.x, b.y, b.z));

	if (!v0 && !v1 && !v2 && !v3 && !v4 && !v5 && !v6 && !v7) {
		return NO_INTERSECTION;
	}
	if (v0 && v1 && v2 && v3 && v4 && v5 && v6 && v7) {
		return FULL_INTERSECTION;
	}

	return PARTIAL_INTERSECTION;
}

IntersectionType IntersectionTests::intersects(const Frustum * frustum0, const Frustum * frustum1) {
	//bool intersects = true;
	//
	//for (int i = 0; i < 6; i++) {
	//	Plane plane = frustum0->getPlane((FrustumPlane)i);
	//
	//	bool flag = false;
	//	for (int j = 0; j < 8; j++) {
	//		dvec3 corner = frustum1->getCorner((FrustumCorner)i);
	//
	//		flag |= getSignedPlaneDistance(corner, plane) > 0.0;
	//	}
	//
	//	intersects &= flag;
	//}
	//for (int i = 0; i < 6; i++) {
	//	Plane plane = frustum1->getPlane((FrustumPlane)i);
	//
	//	bool flag = false;
	//	for (int j = 0; j < 8; j++) {
	//		dvec3 corner = frustum0->getCorner((FrustumCorner)i);
	//
	//		flag |= getSignedPlaneDistance(corner, plane) > 0.0;
	//	}
	//
	//	intersects &= flag;
	//}
	//
	//return intersects ? FULL_INTERSECTION : NO_INTERSECTION;

	for (int i = 0; i < 12; i++) {
		if (frustum0->intersectsSegment(frustum1->getEdge((FrustumEdge)i))) {
			return FULL_INTERSECTION;
		}
		if (frustum1->intersectsSegment(frustum0->getEdge((FrustumEdge)i))) {
			return FULL_INTERSECTION;
		}
	}

	return NO_INTERSECTION;
}

IntersectionType IntersectionTests::intersects(const Ray ray, const Plane plane, double* distance) {
	dvec3 n = dvec3(plane);
	dvec3 o = ray.orig;
	dvec3 d = ray.dir;

	double pd = getSignedPlaneDistance(o, plane);
	if (fabs(pd) < EPS) { // ray originates on the surface of the plane. We can consider this as an intersection.
		if (distance != NULL) * distance = 0.0;
		return PARTIAL_INTERSECTION;
	}

	double nDotD = dot(n, d);

	if (fabs(nDotD) < EPS) { // n dot dir is very close to zero, meaning the ray is very close to parallel with this plane.
		if (distance != NULL) * distance = 0.0;
		return PARTIAL_INTERSECTION;
	}

	double rd = -pd / nDotD;
	if (distance != NULL) * distance = rd;

	if (rd < 0.0) {
		return NO_INTERSECTION;
	}

	return FULL_INTERSECTION;
}

bool IntersectionTests::raySphereIntersection(const Ray ray, const dvec3 sphereOrigin, const double sphereRadius, bool allowInside, double* distance) {
	double t0, t1;

	dvec3 dv = ray.orig - sphereOrigin;
	double a = length2(ray.dir);
	double b = 2.0 * dot(ray.dir, dv);
	double c = length2(dv) - sphereRadius * sphereRadius;


	double discr = b * b - 4.0 * a * c;
	if (discr < 0.0) {
		return NULL;
	} else if (discr < 1e-12) {
		t0 = t1 = -0.5 * b / a;
	} else {
		float q = (b > 0.0) ? -0.5 * (b + sqrt(discr)) : -0.5 * (b - sqrt(discr));
		t0 = q / a;
		t1 = c / q;
	}
	
	if (t0 > t1) {
		std::swap(t0, t1);
	}

	if (t0 < 0.0) {
		if (allowInside) {
			t0 = t1;
			if (t0 < 0.0) {
				return false;
			}
		} else {
			return false;
		}
	}

	if (distance != NULL) {
		*distance = t0;
	}
	return true;
}

bool IntersectionTests::axisProjectionSAT(dvec3 * ap, dvec3 * bp, int32 count, dvec3 axis) {

	if (count <= 0 || length2(axis) < EPS * EPS) {
		return false; // axis is invalid... 
	}

	double a0 = +INFINITY;
	double a1 = -INFINITY;
	double b0 = +INFINITY;
	double b1 = -INFINITY;
	double c0 = +INFINITY;
	double c1 = -INFINITY;

	for (int i = 0; i < count; i++) {
		double a = dot(ap[i], axis);
		double b = dot(bp[i], axis);

		a0 = glm::min(a0, a);
		a1 = glm::max(a1, a);
		b0 = glm::min(b0, b);
		b1 = glm::max(b1, b);
		c0 = glm::min(a0, b0);
		c1 = glm::max(a1, b1);
	}

	return (c1 - c0) < (a1 - a0 + b1 - b0); // true if there is an intersection along this axis.
}

double IntersectionTests::getSignedPlaneDistance(const dvec3 point, const Plane plane) {
	return plane.x* point.x + plane.y * point.y + plane.z * point.z + plane.w;
}
