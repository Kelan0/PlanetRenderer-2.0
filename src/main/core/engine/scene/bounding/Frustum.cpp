#include "BoundingVolume.h"
#include "core/application/Application.h"
#include "core/engine/renderer/DebugRenderer.h"
#include "core/engine/geometry/MeshData.h"
#include "core/engine/scene/SceneGraph.h"

Frustum::Frustum(const Plane left, const Plane right, const Plane bottom, const Plane top, const Plane near, const Plane far) {
	this->set(left, right, bottom, top, near, far);
}

Frustum::Frustum(const dvec3 ltn, const dvec3 rtn, const dvec3 rbn, const dvec3 lbn, const dvec3 ltf, const dvec3 rtf, const dvec3 rbf, const dvec3 lbf) {
	this->set(ltn, rtn, rbn, lbn, ltf, rtf, rbf, lbf);
}

Frustum::Frustum(const dmat4 viewProjection) {
	this->set(viewProjection);
}

Frustum::~Frustum() {
	//delete[] this->planes;
}

Frustum* Frustum::set(const Plane left, const Plane right, const Plane bottom, const Plane top, const Plane near, const Plane far) {

	const dvec3 ltn = this->tripplePlaneIntersection(left, top, near);
	const dvec3 rtn = this->tripplePlaneIntersection(right, top, near);
	const dvec3 rbn = this->tripplePlaneIntersection(right, bottom, near);
	const dvec3 lbn = this->tripplePlaneIntersection(left, bottom, near);
	const dvec3 ltf = this->tripplePlaneIntersection(left, top, far);
	const dvec3 rtf = this->tripplePlaneIntersection(right, top, far);
	const dvec3 rbf = this->tripplePlaneIntersection(right, bottom, far);
	const dvec3 lbf = this->tripplePlaneIntersection(left, bottom, far);

	this->planes[FRUSTUM_LEFT] = left;
	this->planes[FRUSTUM_RIGHT] = right;
	this->planes[FRUSTUM_BOTTOM] = bottom;
	this->planes[FRUSTUM_TOP] = top;
	this->planes[FRUSTUM_NEAR] = near;
	this->planes[FRUSTUM_FAR] = far;

	this->edges[FRUSTUM_LEFT_TOP] = LineSegment(ltn, ltf); // near to far
	this->edges[FRUSTUM_RIGHT_TOP] = LineSegment(rtn, rtf); // near to far
	this->edges[FRUSTUM_RIGHT_BOTTOM] = LineSegment(rbn, rbf); // near to far
	this->edges[FRUSTUM_LEFT_BOTTOM] = LineSegment(lbn, lbf); // near to far
	this->edges[FRUSTUM_LEFT_NEAR] = LineSegment(lbn, ltn); // bottom to top
	this->edges[FRUSTUM_TOP_NEAR] = LineSegment(ltn, rtn); // left to right
	this->edges[FRUSTUM_RIGHT_NEAR] = LineSegment(rbn, rtn); // bottom to top
	this->edges[FRUSTUM_BOTTOM_NEAR] = LineSegment(lbn, rbn); // left to right
	this->edges[FRUSTUM_LEFT_FAR] = LineSegment(lbf, ltf); // bottom to top
	this->edges[FRUSTUM_TOP_FAR] = LineSegment(ltf, rtf); // left to right
	this->edges[FRUSTUM_RIGHT_FAR] = LineSegment(rbf, rtf); // bottom to top
	this->edges[FRUSTUM_BOTTOM_FAR] = LineSegment(lbf, rbf); // left to right

	this->corners[FRUSTUM_LEFT_TOP_NEAR] = ltn;
	this->corners[FRUSTUM_RIGHT_TOP_NEAR] = rtn;
	this->corners[FRUSTUM_RIGHT_BOTTOM_NEAR] = rbn;
	this->corners[FRUSTUM_LEFT_BOTTOM_NEAR] = lbn;
	this->corners[FRUSTUM_LEFT_TOP_FAR] = ltf;
	this->corners[FRUSTUM_RIGHT_TOP_FAR] = rtf;
	this->corners[FRUSTUM_RIGHT_BOTTOM_FAR] = rbf;
	this->corners[FRUSTUM_LEFT_BOTTOM_FAR] = lbf;

	this->length = fabs(far.w - near.w);// / glm::length(dvec3(far));

	return this;
}

Frustum * Frustum::set(const dvec3 ltn, const dvec3 rtn, const dvec3 rbn, const dvec3 lbn, const dvec3 ltf, const dvec3 rtf, const dvec3 rbf, const dvec3 lbf) {

	this->corners[FRUSTUM_LEFT_TOP_NEAR] = ltn;
	this->corners[FRUSTUM_RIGHT_TOP_NEAR] = rtn;
	this->corners[FRUSTUM_RIGHT_BOTTOM_NEAR] = rbn;
	this->corners[FRUSTUM_LEFT_BOTTOM_NEAR] = lbn;
	this->corners[FRUSTUM_LEFT_TOP_FAR] = ltf;
	this->corners[FRUSTUM_RIGHT_TOP_FAR] = rtf;
	this->corners[FRUSTUM_RIGHT_BOTTOM_FAR] = rbf;
	this->corners[FRUSTUM_LEFT_BOTTOM_FAR] = lbf;

	this->edges[FRUSTUM_LEFT_TOP] = LineSegment(ltn, ltf); // near to far
	this->edges[FRUSTUM_RIGHT_TOP] = LineSegment(rtn, rtf); // near to far
	this->edges[FRUSTUM_RIGHT_BOTTOM] = LineSegment(rbn, rbf); // near to far
	this->edges[FRUSTUM_LEFT_BOTTOM] = LineSegment(lbn, lbf); // near to far
	this->edges[FRUSTUM_LEFT_NEAR] = LineSegment(lbn, ltn); // bottom to top
	this->edges[FRUSTUM_TOP_NEAR] = LineSegment(ltn, rtn); // left to right
	this->edges[FRUSTUM_RIGHT_NEAR] = LineSegment(rbn, rtn); // bottom to top
	this->edges[FRUSTUM_BOTTOM_NEAR] = LineSegment(lbn, rbn); // left to right
	this->edges[FRUSTUM_LEFT_FAR] = LineSegment(lbf, ltf); // bottom to top
	this->edges[FRUSTUM_TOP_FAR] = LineSegment(ltf, rtf); // left to right
	this->edges[FRUSTUM_RIGHT_FAR] = LineSegment(rbf, rtf); // bottom to top
	this->edges[FRUSTUM_BOTTOM_FAR] = LineSegment(lbf, rbf); // left to right

	this->center = (ltn + rtn + rbn + lbn + ltf + rtf + rbf + lbf) / 8.0;
	dvec3 n;

	n = cross(ltf - ltn, lbf - ltn);
	n = normalize(n * glm::sign(dot(n, this->center - ltn)));
	this->planes[FRUSTUM_LEFT] = Plane(n, -dot(n, ltn));

	n = cross(rtf - rtn, rbf - rtn);
	n = normalize(n * glm::sign(dot(n, this->center - rtn)));
	this->planes[FRUSTUM_RIGHT] = Plane(n, -dot(n, rtn));

	n = cross(lbf - lbn, rbf - lbf);
	n = normalize(n * glm::sign(dot(n, this->center - lbn)));
	this->planes[FRUSTUM_BOTTOM] = Plane(n, -dot(n, lbn));

	n = cross(ltf - ltn, rtf - ltn);
	n = normalize(n * glm::sign(dot(n, this->center - ltn)));
	this->planes[FRUSTUM_TOP] = Plane(n, -dot(n, ltn));

	n = cross(rbn - lbn, ltn - lbn);
	n = normalize(n * glm::sign(dot(n, this->center - lbn)));
	this->planes[FRUSTUM_NEAR] = Plane(n, -dot(n, lbn));

	n = cross(rbf - lbf, ltf - lbf);
	n = normalize(n * glm::sign(dot(n, this->center - lbf)));
	this->planes[FRUSTUM_FAR] = Plane(n, -dot(n, lbf));

	//this->length = glm::length((ltn + rtn + rbn + lbn) * 0.25 - (ltf + rtf + rbf + lbf) * 0.25);
	this->length = fabs(this->planes[FRUSTUM_FAR].w - this->planes[FRUSTUM_NEAR].w);// / glm::length(dvec3(far));

	return this;
}

Frustum * Frustum::set(const dmat4 viewProjection) {

	const Plane left = Plane(
		viewProjection[0][3] + viewProjection[0][0],
		viewProjection[1][3] + viewProjection[1][0],
		viewProjection[2][3] + viewProjection[2][0],
		viewProjection[3][3] + viewProjection[3][0]
	);

	const Plane right = Plane(
		viewProjection[0][3] - viewProjection[0][0],
		viewProjection[1][3] - viewProjection[1][0],
		viewProjection[2][3] - viewProjection[2][0],
		viewProjection[3][3] - viewProjection[3][0]
	);
	const Plane bottom = Plane(
		viewProjection[0][3] + viewProjection[0][1],
		viewProjection[1][3] + viewProjection[1][1],
		viewProjection[2][3] + viewProjection[2][1],
		viewProjection[3][3] + viewProjection[3][1]
	);
	const Plane top = Plane(
		viewProjection[0][3] - viewProjection[0][1],
		viewProjection[1][3] - viewProjection[1][1],
		viewProjection[2][3] - viewProjection[2][1],
		viewProjection[3][3] - viewProjection[3][1]
	);
	const Plane near = Plane(
		viewProjection[0][3] + viewProjection[0][2],
		viewProjection[1][3] + viewProjection[1][2],
		viewProjection[2][3] + viewProjection[2][2],
		viewProjection[3][3] + viewProjection[3][2]
	);
	const Plane far = Plane(
		viewProjection[0][3] - viewProjection[0][2],
		viewProjection[1][3] - viewProjection[1][2],
		viewProjection[2][3] - viewProjection[2][2],
		viewProjection[3][3] - viewProjection[3][2]
	);

	const dvec3 ltn = this->tripplePlaneIntersection(left, top, near);
	const dvec3 rtn = this->tripplePlaneIntersection(right, top, near);
	const dvec3 rbn = this->tripplePlaneIntersection(right, bottom, near);
	const dvec3 lbn = this->tripplePlaneIntersection(left, bottom, near);
	const dvec3 ltf = this->tripplePlaneIntersection(left, top, far);
	const dvec3 rtf = this->tripplePlaneIntersection(right, top, far);
	const dvec3 rbf = this->tripplePlaneIntersection(right, bottom, far);
	const dvec3 lbf = this->tripplePlaneIntersection(left, bottom, far);


	this->planes[FRUSTUM_LEFT] = left;
	this->planes[FRUSTUM_RIGHT] = right;
	this->planes[FRUSTUM_BOTTOM] = bottom;
	this->planes[FRUSTUM_TOP] = top;
	this->planes[FRUSTUM_NEAR] = near;
	this->planes[FRUSTUM_FAR] = far;

	this->edges[FRUSTUM_LEFT_TOP] = LineSegment(ltn, ltf); // near to far
	this->edges[FRUSTUM_RIGHT_TOP] = LineSegment(rtn, rtf); // near to far
	this->edges[FRUSTUM_RIGHT_BOTTOM] = LineSegment(rbn, rbf); // near to far
	this->edges[FRUSTUM_LEFT_BOTTOM] = LineSegment(lbn, lbf); // near to far
	this->edges[FRUSTUM_LEFT_NEAR] = LineSegment(lbn, ltn); // bottom to top
	this->edges[FRUSTUM_TOP_NEAR] = LineSegment(ltn, rtn); // left to right
	this->edges[FRUSTUM_RIGHT_NEAR] = LineSegment(rbn, rtn); // bottom to top
	this->edges[FRUSTUM_BOTTOM_NEAR] = LineSegment(lbn, rbn); // left to right
	this->edges[FRUSTUM_LEFT_FAR] = LineSegment(lbf, ltf); // bottom to top
	this->edges[FRUSTUM_TOP_FAR] = LineSegment(ltf, rtf); // left to right
	this->edges[FRUSTUM_RIGHT_FAR] = LineSegment(rbf, rtf); // bottom to top
	this->edges[FRUSTUM_BOTTOM_FAR] = LineSegment(lbf, rbf); // left to right

	this->corners[FRUSTUM_LEFT_TOP_NEAR] = ltn;
	this->corners[FRUSTUM_RIGHT_TOP_NEAR] = rtn;
	this->corners[FRUSTUM_RIGHT_BOTTOM_NEAR] = rbn;
	this->corners[FRUSTUM_LEFT_BOTTOM_NEAR] = lbn;
	this->corners[FRUSTUM_LEFT_TOP_FAR] = ltf;
	this->corners[FRUSTUM_RIGHT_TOP_FAR] = rtf;
	this->corners[FRUSTUM_RIGHT_BOTTOM_FAR] = rbf;
	this->corners[FRUSTUM_LEFT_BOTTOM_FAR] = lbf;

	this->center = (ltn + rtn + rbn + lbn + ltf + rtf + rbf + lbf) * 0.125;
	//this->length = glm::length((ltn + rtn + rbn + lbn) * 0.25 - (ltf + rtf + rbf + lbf) * 0.25);
	this->length = fabs(far.w - near.w);// / glm::length(dvec3(far));

	return this;
}

void Frustum::renderDebug(bool lines) const {

	dvec3 v0 = this->getCorner(FRUSTUM_LEFT_TOP_NEAR);
	dvec3 v1 = this->getCorner(FRUSTUM_RIGHT_TOP_NEAR);
	dvec3 v2 = this->getCorner(FRUSTUM_RIGHT_BOTTOM_NEAR);
	dvec3 v3 = this->getCorner(FRUSTUM_LEFT_BOTTOM_NEAR);
	dvec3 v4 = this->getCorner(FRUSTUM_LEFT_TOP_FAR);
	dvec3 v5 = this->getCorner(FRUSTUM_RIGHT_TOP_FAR);
	dvec3 v6 = this->getCorner(FRUSTUM_RIGHT_BOTTOM_FAR);
	dvec3 v7 = this->getCorner(FRUSTUM_LEFT_BOTTOM_FAR);

	if (lines) {
		std::vector<Vertex> frustumCorners = {
			Vertex(v0, fvec3(0.0, 1.0, 0.0)),
			Vertex(v1, fvec3(0.0, 1.0, 0.0)),
			Vertex(v2, fvec3(0.0, 1.0, 0.0)),
			Vertex(v3, fvec3(0.0, 1.0, 0.0)),
			Vertex(v4, fvec3(0.0, 1.0, 0.0)),
			Vertex(v5, fvec3(0.0, 1.0, 0.0)),
			Vertex(v6, fvec3(0.0, 1.0, 0.0)),
			Vertex(v7, fvec3(0.0, 1.0, 0.0)),
		};

		std::vector<int32> lineIndices = {
			0, 3, 3, 7, 7, 4, 4, 0, // -x
			1, 2, 2, 6, 6, 5, 5, 1, // +x
			0, 1, 1, 5, 5, 4, 4, 0, // -y
			3, 2, 2, 6, 6, 7, 7, 3, // +y
			0, 1, 1, 2, 2, 3, 3, 0, // -z
			4, 5, 5, 6, 6, 7, 7, 4, // +z
		};
		
		DEBUG_RENDERER.begin(LINES);
		DEBUG_RENDERER.setLightingEnabled(false);
		DEBUG_RENDERER.render(frustumCorners, lineIndices);
		DEBUG_RENDERER.finish();
	} else {
		fvec3 n0 = fvec3(this->planes[FRUSTUM_LEFT]);
		fvec3 n1 = fvec3(this->planes[FRUSTUM_RIGHT]);
		fvec3 n2 = fvec3(this->planes[FRUSTUM_BOTTOM]);
		fvec3 n3 = fvec3(this->planes[FRUSTUM_TOP]);
		fvec3 n4 = fvec3(this->planes[FRUSTUM_NEAR]);
		fvec3 n5 = fvec3(this->planes[FRUSTUM_FAR]);

		std::vector<Vertex> frustumCorners = {
			Vertex(v0, fvec3(n0)), Vertex(v3, fvec3(n0)), Vertex(v7, fvec3(n0)), Vertex(v4, fvec3(n0)),
			Vertex(v1, fvec3(n1)), Vertex(v2, fvec3(n1)), Vertex(v6, fvec3(n1)), Vertex(v5, fvec3(n1)),
			Vertex(v0, fvec3(n2)), Vertex(v1, fvec3(n2)), Vertex(v5, fvec3(n2)), Vertex(v4, fvec3(n2)),
			Vertex(v3, fvec3(n3)), Vertex(v2, fvec3(n3)), Vertex(v6, fvec3(n3)), Vertex(v7, fvec3(n3)),
			Vertex(v0, fvec3(n4)), Vertex(v1, fvec3(n4)), Vertex(v2, fvec3(n4)), Vertex(v3, fvec3(n4)),
			Vertex(v4, fvec3(n5)), Vertex(v5, fvec3(n5)), Vertex(v6, fvec3(n5)), Vertex(v7, fvec3(n5)),
		};

		std::vector<int32> triIndices = {
			0 + 0 * 4, 1 + 0 * 4, 2 + 0 * 4, 0 + 0 * 4, 2 + 0 * 4, 3 + 0 * 4, // -x
			0 + 1 * 4, 1 + 1 * 4, 2 + 1 * 4, 0 + 1 * 4, 2 + 1 * 4, 3 + 1 * 4, // +x
			0 + 2 * 4, 1 + 2 * 4, 2 + 2 * 4, 0 + 2 * 4, 2 + 2 * 4, 3 + 2 * 4, // -y
			0 + 3 * 4, 1 + 3 * 4, 2 + 3 * 4, 0 + 3 * 4, 2 + 3 * 4, 3 + 3 * 4, // +y
			0 + 4 * 4, 1 + 4 * 4, 2 + 4 * 4, 0 + 4 * 4, 2 + 4 * 4, 3 + 4 * 4, // -z
			0 + 5 * 4, 1 + 5 * 4, 2 + 5 * 4, 0 + 5 * 4, 2 + 5 * 4, 3 + 5 * 4, // +z
		};

		DEBUG_RENDERER.begin(TRIANGLES);
		DEBUG_RENDERER.setLightingEnabled(true);
		DEBUG_RENDERER.render(frustumCorners, triIndices);
		DEBUG_RENDERER.finish();
	}
}

double Frustum::getLength() const {
	return this->length;
}

dvec3 Frustum::tripplePlaneIntersection(Plane p0, Plane p1, Plane p2) {
	// Fast triple-plane intersection point, assumes that the planes WILL intersect
	// at a single point, and none are parallel, or intersect along a line. For a
	// valid frustum this should be the case.

	dvec3 p0p1 = cross(dvec3(p0), dvec3(p1));
	dvec3 p1p2 = cross(dvec3(p1), dvec3(p2));
	dvec3 p2p0 = cross(dvec3(p2), dvec3(p0));
	dvec3 r = -p0.w * p1p2 - p1.w * p2p0 - p2.w * p0p1;
	return r * (1.0 / dot(dvec3(p0), p1p2));
}

Plane Frustum::getPlane(FrustumPlane plane) const {
	return this->planes[plane];
}

LineSegment Frustum::getEdge(FrustumEdge edge) const {
	return this->edges[edge];
}

dvec3 Frustum::getCorner(FrustumCorner corner) const {
	return this->corners[corner];
}

dvec3 Frustum::getCenter() const {
	return this->center;
}

dvec3 Frustum::getClosestPoint(dvec3 point) const {
	return dvec3(); // TODO
}

double Frustum::getDistance(dvec3 point) const {
	return glm::distance(point, this->getClosestPoint(point));
}

double Frustum::getDistanceSq(dvec3 point) const {
	return glm::distance2(point, this->getClosestPoint(point));
}

double Frustum::getVolume() const {
	return 0.0; // TODO
}

double Frustum::getSurfaceArea() const {
	return 0.0; // TODO
}

IntersectionType Frustum::intersectsPoint(dvec3 point) const {
	if (IntersectionTests::getSignedPlaneDistance(point, this->planes[FRUSTUM_LEFT]) < 0.0) return NO_INTERSECTION;
	if (IntersectionTests::getSignedPlaneDistance(point, this->planes[FRUSTUM_RIGHT]) < 0.0) return NO_INTERSECTION;
	if (IntersectionTests::getSignedPlaneDistance(point, this->planes[FRUSTUM_BOTTOM]) < 0.0) return NO_INTERSECTION;
	if (IntersectionTests::getSignedPlaneDistance(point, this->planes[FRUSTUM_TOP]) < 0.0) return NO_INTERSECTION;
	if (IntersectionTests::getSignedPlaneDistance(point, this->planes[FRUSTUM_NEAR]) < 0.0) return NO_INTERSECTION;
	if (IntersectionTests::getSignedPlaneDistance(point, this->planes[FRUSTUM_FAR]) < 0.0) return NO_INTERSECTION;

	return FULL_INTERSECTION;
}

IntersectionType Frustum::intersectsRay(Ray ray, double* distance) const {

	double dlr, drr, dbr, dtr, dnr, dfr;
	double dlo, dro, dbo, dto, dno, dfo;
	double d = INFINITY;

	IntersectionTests::intersects(ray, this->planes[FRUSTUM_LEFT], &dlr);
	dlo = IntersectionTests::getSignedPlaneDistance(ray.orig, this->planes[FRUSTUM_LEFT]);
	if (dlr < 0.0 && dlo < 0.0) return NO_INTERSECTION;
	if (dlr > 0.0) d = glm::min(d, dlr);

	IntersectionTests::intersects(ray, this->planes[FRUSTUM_RIGHT], &drr);
	dro = IntersectionTests::getSignedPlaneDistance(ray.orig, this->planes[FRUSTUM_RIGHT]);
	if (drr < 0.0 && dro < 0.0) return NO_INTERSECTION;
	if (drr > 0.0) d = glm::min(d, drr);

	IntersectionTests::intersects(ray, this->planes[FRUSTUM_BOTTOM], &dbr);
	dbo = IntersectionTests::getSignedPlaneDistance(ray.orig, this->planes[FRUSTUM_BOTTOM]);
	if (dbr < 0.0 && dbo < 0.0) return NO_INTERSECTION;
	if (dbr > 0.0) d = glm::min(d, dbr);

	IntersectionTests::intersects(ray, this->planes[FRUSTUM_TOP], &dtr);
	dto = IntersectionTests::getSignedPlaneDistance(ray.orig, this->planes[FRUSTUM_TOP]);
	if (dtr < 0.0 && dto < 0.0) return NO_INTERSECTION;
	if (dtr > 0.0) d = glm::min(d, dtr);

	IntersectionTests::intersects(ray, this->planes[FRUSTUM_NEAR], &dnr);
	dno = IntersectionTests::getSignedPlaneDistance(ray.orig, this->planes[FRUSTUM_NEAR]);
	if (dnr < 0.0 && dno < 0.0) return NO_INTERSECTION;
	if (dnr > 0.0) d = glm::min(d, dnr);

	IntersectionTests::intersects(ray, this->planes[FRUSTUM_FAR], &dfr);
	dfo = IntersectionTests::getSignedPlaneDistance(ray.orig, this->planes[FRUSTUM_FAR]);
	if (dfr < 0.0 && dfo < 0.0) return NO_INTERSECTION;
	if (dfr > 0.0) d = glm::min(d, dfr);

	// if ray origin is inside the frustum, return paerial intersection ?

	if (distance != NULL) * distance = d;
	return FULL_INTERSECTION;
}

IntersectionType Frustum::intersectsSegment(LineSegment segment) const {
	if (this->intersectsPoint(segment.orig) || this->intersectsPoint(segment.orig + segment.dir * segment.length)) {
		return FULL_INTERSECTION;
	}

	double distance;
	if (this->intersectsRay(segment, &distance)) {
		if (distance < segment.length) {
			return FULL_INTERSECTION;
		}
	}

	return NO_INTERSECTION;
}

IntersectionType Frustum::intersectsPlane(Plane plane, bool reversed) const {
	double v0 = IntersectionTests::getSignedPlaneDistance(this->corners[FRUSTUM_LEFT_TOP_NEAR], plane);
	double v1 = IntersectionTests::getSignedPlaneDistance(this->corners[FRUSTUM_RIGHT_TOP_NEAR], plane);
	double v2 = IntersectionTests::getSignedPlaneDistance(this->corners[FRUSTUM_RIGHT_BOTTOM_NEAR], plane);
	double v3 = IntersectionTests::getSignedPlaneDistance(this->corners[FRUSTUM_LEFT_BOTTOM_NEAR], plane);
	double v4 = IntersectionTests::getSignedPlaneDistance(this->corners[FRUSTUM_LEFT_TOP_FAR], plane);
	double v5 = IntersectionTests::getSignedPlaneDistance(this->corners[FRUSTUM_RIGHT_TOP_FAR], plane);
	double v6 = IntersectionTests::getSignedPlaneDistance(this->corners[FRUSTUM_RIGHT_BOTTOM_FAR], plane);
	double v7 = IntersectionTests::getSignedPlaneDistance(this->corners[FRUSTUM_LEFT_BOTTOM_FAR], plane);

	if (v0 <= 0.0 && v1 <= 0.0 && v2 <= 0.0 && v3 <= 0.0 && v4 <= 0.0 && v5 <= 0.0 && v6 <= 0.0 && v7 <= 0.0) {
		return NO_INTERSECTION;
	}
	if (v0 > 0.0 && v1 > 0.0 && v2 > 0.0 && v3 > 0.0 && v4 > 0.0 && v5 > 0.0 && v6 > 0.0 && v7 > 0.0) {
		return FULL_INTERSECTION;
	}

	return PARTIAL_INTERSECTION;
}

IntersectionType Frustum::intersectsBounds(BoundingVolume * bound) const {
	if (AxisAlignedBB * aabb = dynamic_cast<AxisAlignedBB*>(bound)) {
		return IntersectionTests::intersects(aabb, this);
	}

	if (Frustum * frustum = dynamic_cast<Frustum*>(bound)) {
		return IntersectionTests::intersects(this, frustum);
	}

	return NO_INTERSECTION;
}
