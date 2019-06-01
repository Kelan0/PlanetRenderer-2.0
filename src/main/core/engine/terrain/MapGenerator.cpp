#include "MapGenerator.h"
#include "core/application/Application.h"
#include "core/engine/renderer/DebugRenderer.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/geometry/meshData.h"
#include "core/engine/terrain/TileSupplier.h"
#include "core/engine/terrain/Planet.h"
#include "core/util/Time.h"



MapGenerator::MapGenerator(Planet* planet, uint32 resolution) {
	this->planet = planet;
	this->resolution = resolution;
	this->renderDebugEdges = false;
	this->renderDebugSurface = false;

	uint64 a = Time::now();
	this->generateIcosohedron();
	uint64 b = Time::now();
	this->generateAirCurrents();
	uint64 c = Time::now();

	logInfo("Took %f ms to generate icosphere, %f ms to generate air currents", (b - a) / 1000000.0, (c - b) / 1000000.0);

	std::vector<Vertex> surfaceVertices;
	std::vector<Vertex> currentArrowVertices;

	std::vector<uint32> surfaceTriangleIndices;
	std::vector<uint32> surfaceLineIndices;
	std::vector<uint32> currentArrowIndices;

	fvec4* data = new fvec4[this->nodes.size()]();
	fvec3* points = new fvec3[this->nodes.size()]();

	for (int i = 0; i < this->nodes.size(); i++) points[i] = this->nodes[i].p;

	planet->getTileSupplier()->computePointData(this->nodes.size(), points, data);

	const int32 tgc = 6;
	fvec3 tg[tgc] = {
		fvec3(0.5F, 0.0F, 1.0F),
		fvec3(0.0F, 0.0F, 1.0F),
		fvec3(0.0F, 1.0F, 1.0F),
		fvec3(0.0F, 1.0F, 0.0F),
		fvec3(1.0F, 1.0F, 0.0F),
		fvec3(1.0F, 0.0F, 0.0F),
	};

	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode& n = this->nodes[i];

		if (data[i].w <= 0.0) {
			n.water = true;
		}
	}

	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode& n = this->nodes[i];

		float latitudeTemperature = 1.0 - abs(n.p.y);
		float altitudeTemperature = 1.0 - glm::max(0.0F, data[i].w);

		float latContrib = 0.7;
		float altContrib = 0.3;

		if (n.water) {
			altitudeTemperature *= 1.0 - glm::min(0.5F, abs(data[i].w));
		} else {
			altitudeTemperature = glm::min(altitudeTemperature * 1.3, 1.0);
		}

		float temperature = latitudeTemperature * latContrib + altitudeTemperature * altContrib;
		
		float f = temperature * (tgc - 1);
		int32 i0 = glm::clamp<int32>(f - 1.0F, 0, tgc - 1);
		int32 i1 = glm::clamp<int32>(f - 0.0F, 0, tgc - 1);
		int32 i2 = glm::clamp<int32>(f + 1.0F, 0, tgc - 1);
		int32 i3 = glm::clamp<int32>(f + 2.0F, 0, tgc - 1);
		
		fvec3 colour = glm::catmullRom(tg[i0], tg[i1], tg[i2], tg[i3], glm::fract(f));

		surfaceVertices.push_back(Vertex(n.p * planet->getRadius(), fvec3(0.0), fvec2(0.0), colour));

		// current arrows
		
		if (true || dot(n.windVector, n.windVector) > 1e-12) {

			double magnitude = length(n.windVector);
			dvec3 direction = n.windVector / magnitude;
			dvec3 side = normalize(cross(direction, n.p));
			int32 baseIndex = currentArrowVertices.size();
			currentArrowVertices.push_back(Vertex(n.p * planet->getRadius() + side * 10.0 * magnitude));
			currentArrowVertices.push_back(Vertex(n.p * planet->getRadius() - side * 10.0 * magnitude));
			currentArrowVertices.push_back(Vertex(n.p * planet->getRadius() + direction * 80.0 * magnitude));

			currentArrowIndices.push_back(baseIndex + 0);
			currentArrowIndices.push_back(baseIndex + 1);
			currentArrowIndices.push_back(baseIndex + 2);
		}
	}

	for (int i = 0; i < this->edges.size(); i++) {
		MapEdge e = this->edges[i];
		surfaceLineIndices.push_back(e.n[0]);
		surfaceLineIndices.push_back(e.n[1]);
	}

	for (int i = 0; i < this->faces.size(); i++) {
		MapFace f = this->faces[i];
		surfaceTriangleIndices.push_back(f.n[0]);
		surfaceTriangleIndices.push_back(f.n[1]);
		surfaceTriangleIndices.push_back(f.n[2]);
	}

	VertexLayout vertexLayout = VertexLayout(44, {
		VertexAttribute(0, 3, 0),
		VertexAttribute(1, 3, 12),
		VertexAttribute(2, 2, 24),
		VertexAttribute(3, 3, 32)
		}, [](Vertex v) -> std::vector<float> {
			return std::vector<float> {
				float(v.position.x), float(v.position.y), float(v.position.z),
					float(v.normal.x), float(v.normal.y), float(v.normal.z),
					float(v.texture.x), float(v.texture.y),
					float(v.colour.r), float(v.colour.g), float(v.colour.b)
			};
		}
	);

	MeshData* triangleMeshData = new MeshData(surfaceVertices, surfaceTriangleIndices, vertexLayout);
	this->debugTriangleMesh = new GLMesh(triangleMeshData, vertexLayout);
	delete triangleMeshData;

	MeshData* lineMeshData = new MeshData(surfaceVertices, surfaceLineIndices, vertexLayout);
	this->debugLineMesh = new GLMesh(lineMeshData, vertexLayout);
	this->debugLineMesh->setPrimitive(LINES);

	MeshData* currentArrowMeshData = new MeshData(currentArrowVertices, currentArrowIndices, vertexLayout);
	this->debugCurrentArrowMesh = new GLMesh(currentArrowMeshData, vertexLayout);
	delete currentArrowMeshData;

	delete lineMeshData;
}

MapGenerator::~MapGenerator() {

}

void MapGenerator::generateIcosohedron() {
	const double a = 0.525731112119133606;
	const double b = 0.850650808352039932;
	const double c = 0.0;


	std::vector<MapNode> icoNodes = {
		MapNode(dvec3(c, +b, +a)), MapNode(dvec3(c, +b, -a)), MapNode(dvec3(c, -b, +a)), MapNode(dvec3(c, -b, -a)),
		MapNode(dvec3(+a, c, +b)), MapNode(dvec3(-a, c, +b)), MapNode(dvec3(+a, c, -b)), MapNode(dvec3(-a, c, -b)),
		MapNode(dvec3(+b, +a, c)), MapNode(dvec3(+b, -a, c)), MapNode(dvec3(-b, +a, c)), MapNode(dvec3(-b, -a, c)),
	};

	std::vector<MapEdge> icoEdges = {
		MapEdge({ 0, 1, }), MapEdge({ 0, 4, }), MapEdge({ 0, 5, }),
		MapEdge({ 0, 8, }), MapEdge({ 0, 10, }), MapEdge({ 1, 6, }),
		MapEdge({ 1, 7, }), MapEdge({ 1, 8, }), MapEdge({ 1, 10, }),
		MapEdge({ 2, 3, }), MapEdge({ 2, 4, }), MapEdge({ 2, 5, }),
		MapEdge({ 2, 9, }), MapEdge({ 2, 11, }), MapEdge({ 3, 6, }),
		MapEdge({ 3, 7, }), MapEdge({ 3, 9, }), MapEdge({ 3, 11, }),
		MapEdge({ 4, 5, }), MapEdge({ 4, 8, }), MapEdge({ 4, 9, }),
		MapEdge({ 5, 10, }), MapEdge({ 5, 11, }), MapEdge({ 6, 7, }),
		MapEdge({ 6, 8, }), MapEdge({ 6, 9, }), MapEdge({ 7, 10, }),
		MapEdge({ 7, 11, }), MapEdge({ 8, 9, }), MapEdge({ 10, 11, }),
	};

	std::vector<MapFace> icoFaces = {
		MapFace({ 0, 1, 8 }, { 0, 7, 3 }),
		MapFace({ 0, 4, 5 }, { 1, 18, 2 }),
		MapFace({ 0, 5, 10 }, { 2, 21, 4 }),
		MapFace({ 0, 8, 4 }, { 3, 19, 1 }),
		MapFace({ 0, 10, 1 }, { 4, 8, 0 }),
		MapFace({ 1, 6, 8 }, { 5, 24, 7 }),
		MapFace({ 1, 7, 6 }, { 6, 23, 5 }),
		MapFace({ 1, 10, 7 }, { 8, 26, 6 }),
		MapFace({ 2, 3, 11 }, { 9, 17, 13 }),
		MapFace({ 2, 4, 9 }, { 10, 20, 12 }),
		MapFace({ 2, 5, 4 }, { 11, 18, 10 }),
		MapFace({ 2, 9, 3 }, { 12, 16, 9 }),
		MapFace({ 2, 11, 5 }, { 13, 22, 11 }),
		MapFace({ 3, 6, 7 }, { 14, 23, 15 }),
		MapFace({ 3, 7, 11 }, { 15, 27, 17 }),
		MapFace({ 3, 9, 6 }, { 16, 25, 14 }),
		MapFace({ 4, 8, 9 }, { 19, 28, 20 }),
		MapFace({ 5, 11, 10 }, { 22, 29, 21 }),
		MapFace({ 6, 9, 8 }, { 25, 28, 24 }),
		MapFace({ 7, 10, 11 }, { 26, 29, 27 }),
	};

	int i, j, k, s, t;

	for (i = 0; i < icoEdges.size(); i++) {
		for (j = 0; j < icoEdges[i].n.size(); j++) {
			icoNodes[j].e.push_back(i);
		}
	}

	for (i = 0; i < icoFaces.size(); i++) {
		for (j = 0; j < icoFaces[i].n.size(); j++) {
			icoNodes[j].f.push_back(i);
		}
	}

	for (i = 0; i < icoFaces.size(); i++) {
		for (j = 0; j < icoFaces[i].e.size(); j++) {
			icoEdges[j].f.push_back(i);
		}
	}

	std::vector<MapNode> nodes;
	std::vector<MapEdge> edges;
	std::vector<MapFace> faces;
	std::vector<int32> faceNodes;
	std::vector<int32> faceEdges0;
	std::vector<int32> faceEdges1;
	std::vector<int32> faceEdges2;

	for (i = 0; i < icoNodes.size(); i++) {
		nodes.push_back(MapNode(icoNodes[i].p));
	}

	for (i = 0; i < icoEdges.size(); i++) {
		MapEdge& edge = icoEdges[i];

		MapNode& n0 = icoNodes[edge.n[0]];
		MapNode& n1 = icoNodes[edge.n[1]];

		nodes[edge.n[0]].e.push_back(edges.size());
		int32 pi = edge.n[0];

		for (j = 1; j < this->resolution; j++) {
			int32 ei = edges.size();
			int32 ni = nodes.size();
			edge.se.push_back(ei);
			edge.sn.push_back(ni);
			edges.push_back(MapEdge({ pi, ni }));
			pi = ni;
			nodes.push_back(MapNode(slerp(n0.p, n1.p, double(j) / this->resolution), { ei, ei + 1 }));
		}

		edge.se.push_back(edges.size());
		nodes[edge.n[1]].e.push_back(edges.size());
		edges.push_back(MapEdge({ pi, edge.n[1] }));
	}

	for (i = 0; i < icoFaces.size(); i++) {
		MapFace& face = icoFaces[i];
		MapEdge& e0 = icoEdges[face.e[0]];
		MapEdge& e1 = icoEdges[face.e[1]];
		MapEdge& e2 = icoEdges[face.e[2]];
		dvec3 p0 = icoNodes[face.n[0]].p;
		dvec3 p1 = icoNodes[face.n[1]].p;
		dvec3 p2 = icoNodes[face.n[2]].p;

		dvec3 d = p1 - p0;


		bool reverseEdge0, reverseEdge1, reverseEdge2;

		reverseEdge0 = face.n[0] != e0.n[0];
		auto getEdgeNode0 = [&](int32 k) { return e0.sn[reverseEdge0 ? (this->resolution - 2 - k) : k]; };
		reverseEdge1 = face.n[1] != e1.n[0];
		auto getEdgeNode1 = [&](int32 k) { return e1.sn[reverseEdge1 ? (this->resolution - 2 - k) : k]; };
		reverseEdge2 = face.n[0] != e2.n[0];
		auto getEdgeNode2 = [&](int32 k) { return e2.sn[reverseEdge2 ? (this->resolution - 2 - k) : k]; };

		faceNodes.clear();
		faceNodes.push_back(face.n[0]);

		for (j = 0; j < e0.sn.size(); j++) {
			faceNodes.push_back(getEdgeNode0(j));
		}

		faceNodes.push_back(face.n[1]);

		for (s = 1; s < this->resolution; s++) {
			faceNodes.push_back(getEdgeNode2(s - 1));
			dvec3 p0 = nodes[getEdgeNode2(s - 1)].p;
			dvec3 p1 = nodes[getEdgeNode1(s - 1)].p;
			for (t = 1; t < this->resolution - s; t++) {
				faceNodes.push_back(nodes.size());
				nodes.push_back(MapNode(slerp(p0, p1, double(t) / (this->resolution - s))));
			}
			faceNodes.push_back(getEdgeNode1(s - 1));
		}

		faceNodes.push_back(face.n[2]);

		reverseEdge0 = face.n[0] != e0.n[0];
		auto getEdgeEdge0 = [&](int32 k) { return e0.se[reverseEdge0 ? (this->resolution - 1 - k) : k];  };
		reverseEdge1 = face.n[1] != e1.n[0];
		auto getEdgeEdge1 = [&](int32 k) { return e1.se[reverseEdge1 ? (this->resolution - 1 - k) : k];  };
		reverseEdge2 = face.n[0] != e2.n[0];
		auto getEdgeEdge2 = [&](int32 k) { return e2.se[reverseEdge2 ? (this->resolution - 1 - k) : k];  };

		int32 nodeIndex, edgeIndex;

		faceEdges0.clear();
		for (j = 0; j < this->resolution; ++j) {
			faceEdges0.push_back(getEdgeEdge0(j));
		}
		nodeIndex = this->resolution + 1;
		for (s = 1; s < this->resolution; s++) {
			for (t = 0; t < this->resolution - s; t++) {
				faceEdges0.push_back(edges.size());
				MapEdge edge = MapEdge({ faceNodes[nodeIndex], faceNodes[nodeIndex + 1] });
				nodes[edge.n[0]].e.push_back(edges.size());
				nodes[edge.n[1]].e.push_back(edges.size());
				edges.push_back(edge);
				nodeIndex++;
			}
			nodeIndex++;
		}

		faceEdges1.clear();
		nodeIndex = 1;
		for (s = 0; s < this->resolution; s++) {
			for (t = 1; t < this->resolution - s; t++) {
				faceEdges1.push_back(edges.size());
				MapEdge edge = MapEdge({ faceNodes[nodeIndex], faceNodes[nodeIndex + this->resolution - s] });
				nodes[edge.n[0]].e.push_back(edges.size());
				nodes[edge.n[1]].e.push_back(edges.size());
				edges.push_back(edge);
				nodeIndex++;
			}
			faceEdges1.push_back(getEdgeEdge1(s));
			nodeIndex += 2;
		}

		faceEdges2.clear();
		nodeIndex = 1;
		for (s = 0; s < this->resolution; s++) {
			faceEdges2.push_back(getEdgeEdge2(s));
			for (t = 1; t < this->resolution - s; t++) {
				faceEdges2.push_back(edges.size());
				MapEdge edge = MapEdge({ faceNodes[nodeIndex], faceNodes[nodeIndex + this->resolution - s + 1] });
				nodes[edge.n[0]].e.push_back(edges.size());
				nodes[edge.n[1]].e.push_back(edges.size());
				edges.push_back(edge);
				nodeIndex++;
			}
			nodeIndex += 2;
		}

		nodeIndex = 0;
		edgeIndex = 0;
		for (s = 0; s < this->resolution; s++) {
			for (t = 1; t < this->resolution - s + 1; t++) {
				MapFace subFace = MapFace(
					{ faceNodes[nodeIndex], faceNodes[nodeIndex + 1], faceNodes[nodeIndex + this->resolution - s + 1] },
					{ faceEdges0[edgeIndex], faceEdges1[edgeIndex], faceEdges2[edgeIndex] }
				);

				nodes[subFace.n[0]].f.push_back(faces.size());
				nodes[subFace.n[1]].f.push_back(faces.size());
				nodes[subFace.n[2]].f.push_back(faces.size());
				edges[subFace.e[0]].f.push_back(faces.size());
				edges[subFace.e[1]].f.push_back(faces.size());
				edges[subFace.e[2]].f.push_back(faces.size());
				faces.push_back(subFace);
				nodeIndex++;
				edgeIndex++;
			}
			nodeIndex++;
		}

		nodeIndex = 1;
		edgeIndex = 0;
		for (s = 1; s < this->resolution; s++) {
			for (t = 1; t < this->resolution - s + 1; t++) {
				MapFace subFace = MapFace(
					{ faceNodes[nodeIndex], faceNodes[nodeIndex + this->resolution - s + 2], faceNodes[nodeIndex + this->resolution - s + 1] },
					{ faceEdges2[edgeIndex + 1], faceEdges0[edgeIndex + this->resolution - s + 1], faceEdges1[edgeIndex] }
				);

				nodes[subFace.n[0]].f.push_back(faces.size());
				nodes[subFace.n[1]].f.push_back(faces.size());
				nodes[subFace.n[2]].f.push_back(faces.size());
				edges[subFace.e[0]].f.push_back(faces.size());
				edges[subFace.e[1]].f.push_back(faces.size());
				edges[subFace.e[2]].f.push_back(faces.size());
				faces.push_back(subFace);
				nodeIndex++;
				edgeIndex++;
			}
			nodeIndex += 2;
			edgeIndex += 1;
		}
	}

	this->nodes = nodes;
	this->edges = edges;
	this->faces = faces;
}

void MapGenerator::generateAirCurrents() {
	struct CircularCurrent {
		dvec3 p;
		double s;
		double r;
	};

	const int32 currentCount = 20;
	CircularCurrent currents[currentCount];

	for (int i = 0; i < currentCount; i++) {
		CircularCurrent c;
		c.p = glm::sphericalRand(1.0);
		c.s = (rand() % 2 == 0 ? +1.0 : -1.0) * drand(2.2, 25.4);
		c.r = drand(0.1, 0.3) * PI;

		currents[i] = c;
	}

	float maxa = -INFINITY, mina = +INFINITY;
	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode& n = this->nodes[i];

		n.windVector = fvec3(0.0);
		float w = 0.0;
		for (int j = 0; j < currentCount; j++) {
			CircularCurrent c = currents[j];

			float angle = glm::angle(c.p, n.p);
			maxa = glm::max(maxa, angle);
			mina = glm::min(mina, angle);

			if (angle < c.r) {
				double dist = angle / c.r;
				double weight = 1.0 - dist;
				double strength = c.s * weight * dist;
				n.windVector += cross(c.p, n.p) * strength;
				w += 1.0F;
			}
		}

		if (w > 0.0) {
			n.windVector /= w;
		}
	}

	logInfo("Max angle = %f, min angle = %f", maxa, mina);
}

void MapGenerator::render(double partialTicks, double dt) {
	if (this->renderDebugSurface) {
		DEBUG_RENDERER.setLightingEnabled(false);
		DEBUG_RENDERER.setColour(fvec4(1.0F, 1.0F, 1.0F, 1.0F));
		DEBUG_RENDERER.renderMesh(this->debugTriangleMesh);
		DEBUG_RENDERER.renderMesh(this->debugCurrentArrowMesh);
	}

	if (this->renderDebugEdges) {
		DEBUG_RENDERER.setLineSize(3.0F);
		//DEBUG_RENDERER.setColour(fvec4(0.25F, 0.25F, 0.25F, 1.0F));
		DEBUG_RENDERER.renderMesh(this->debugLineMesh);
	}
}

void MapGenerator::setRenderDebugEdges(bool renderDebugEdges) {
	this->renderDebugEdges = renderDebugEdges;
}

bool MapGenerator::doRenderDebugEdges() const {
	return this->renderDebugEdges;
}

void MapGenerator::setRenderDebugSurface(bool renderDebugSurface) {
	this->renderDebugSurface = renderDebugSurface;
}

bool MapGenerator::doRenderDebugSurface() const {
	return this->renderDebugSurface;
}
