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
	this->renderDebugCurrents = false;
	this->renderDebugSurface = false;

	uint64 a, b;

	a = Time::now();
	this->generateIcosohedron();
	b = Time::now();
	logInfo("Took %f ms to generate map geometry with %d nodes", (b - a) / 1000000.0, this->nodes.size());

	a = Time::now();
	this->generateAirCurrents();
	b = Time::now();
	logInfo("Took %f ms to generate air currents", (b - a) / 1000000.0);

	a = Time::now();
	this->initializeHeat();
	b = Time::now();
	logInfo("Took %f ms to initialize node temperature", (b - a) / 1000000.0);

	a = Time::now();
	this->initializeMoisture();
	b = Time::now();
	logInfo("Took %f ms to initialize node moisture", (b - a) / 1000000.0);

	a = Time::now();
	this->generateDebugMeshes();
	b = Time::now();
	logInfo("Took %f ms to generate debug geometry", (b - a) / 1000000.0);

}

MapGenerator::~MapGenerator() {

}

void MapGenerator::generateIcosohedron() {
	const double a = 0.525731112119133606;
	const double b = 0.850650808352039932;
	const double c = 0.0;


	std::vector<MapNode*> icoNodes = {
		new MapNode(dvec3(c, +b, +a)), new MapNode(dvec3(c, +b, -a)), new MapNode(dvec3(c, -b, +a)), new MapNode(dvec3(c, -b, -a)),
		new MapNode(dvec3(+a, c, +b)), new MapNode(dvec3(-a, c, +b)), new MapNode(dvec3(+a, c, -b)), new MapNode(dvec3(-a, c, -b)),
		new MapNode(dvec3(+b, +a, c)), new MapNode(dvec3(+b, -a, c)), new MapNode(dvec3(-b, +a, c)), new MapNode(dvec3(-b, -a, c)),
	};

	std::vector<MapEdge*> icoEdges = {
		new MapEdge({ 0, 1, }), new MapEdge({ 0, 4, }), new MapEdge({ 0, 5, }),
		new MapEdge({ 0, 8, }), new MapEdge({ 0, 10, }), new  MapEdge({ 1, 6, }),
		new MapEdge({ 1, 7, }), new MapEdge({ 1, 8, }), new MapEdge({ 1, 10, }),
		new MapEdge({ 2, 3, }), new MapEdge({ 2, 4, }), new MapEdge({ 2, 5, }),
		new MapEdge({ 2, 9, }), new MapEdge({ 2, 11, }), new  MapEdge({ 3, 6, }),
		new MapEdge({ 3, 7, }), new MapEdge({ 3, 9, }), new MapEdge({ 3, 11, }),
		new MapEdge({ 4, 5, }), new MapEdge({ 4, 8, }), new MapEdge({ 4, 9, }),
		new MapEdge({ 5, 10, }), new  MapEdge({ 5, 11, }), new MapEdge({ 6, 7, }),
		new MapEdge({ 6, 8, }), new MapEdge({ 6, 9, }), new MapEdge({ 7, 10, }),
		new MapEdge({ 7, 11, }), new  MapEdge({ 8, 9, }), new MapEdge({ 10, 11, }),
	};

	std::vector<MapFace*> icoFaces = {
		new MapFace({ 0, 1, 8 }, { 0, 7, 3 }),
		new MapFace({ 0, 4, 5 }, { 1, 18, 2 }),
		new MapFace({ 0, 5, 10 }, { 2, 21, 4 }),
		new MapFace({ 0, 8, 4 }, { 3, 19, 1 }),
		new MapFace({ 0, 10, 1 }, { 4, 8, 0 }),
		new MapFace({ 1, 6, 8 }, { 5, 24, 7 }),
		new MapFace({ 1, 7, 6 }, { 6, 23, 5 }),
		new MapFace({ 1, 10, 7 }, { 8, 26, 6 }),
		new MapFace({ 2, 3, 11 }, { 9, 17, 13 }),
		new MapFace({ 2, 4, 9 }, { 10, 20, 12 }),
		new MapFace({ 2, 5, 4 }, { 11, 18, 10 }),
		new MapFace({ 2, 9, 3 }, { 12, 16, 9 }),
		new MapFace({ 2, 11, 5 }, { 13, 22, 11 }),
		new MapFace({ 3, 6, 7 }, { 14, 23, 15 }),
		new MapFace({ 3, 7, 11 }, { 15, 27, 17 }),
		new MapFace({ 3, 9, 6 }, { 16, 25, 14 }),
		new MapFace({ 4, 8, 9 }, { 19, 28, 20 }),
		new MapFace({ 5, 11, 10 }, { 22, 29, 21 }),
		new MapFace({ 6, 9, 8 }, { 25, 28, 24 }),
		new MapFace({ 7, 10, 11 }, { 26, 29, 27 }),
	};

	int i, j, k, s, t;

	for (i = 0; i < icoEdges.size(); i++) {
		for (j = 0; j < icoEdges[i]->n.size(); j++) {
			icoNodes[j]->e.push_back(i);
		}
	}

	for (i = 0; i < icoFaces.size(); i++) {
		for (j = 0; j < icoFaces[i]->n.size(); j++) {
			icoNodes[j]->f.push_back(i);
		}
	}

	for (i = 0; i < icoFaces.size(); i++) {
		for (j = 0; j < icoFaces[i]->e.size(); j++) {
			icoEdges[j]->f.push_back(i);
		}
	}

	std::vector<MapNode*> nodes;
	std::vector<MapEdge*> edges;
	std::vector<MapFace*> faces;
	std::vector<int32> faceNodes;
	std::vector<int32> faceEdges0;
	std::vector<int32> faceEdges1;
	std::vector<int32> faceEdges2;

	for (i = 0; i < icoNodes.size(); i++) {
		nodes.push_back(new MapNode(icoNodes[i]->p));
	}

	for (i = 0; i < icoEdges.size(); i++) {
		MapEdge* edge = icoEdges[i];

		MapNode* n0 = icoNodes[edge->n[0]];
		MapNode* n1 = icoNodes[edge->n[1]];

		nodes[edge->n[0]]->e.push_back(edges.size());
		int32 pi = edge->n[0];

		for (j = 1; j < this->resolution; j++) {
			int32 ei = edges.size();
			int32 ni = nodes.size();
			edge->se.push_back(ei);
			edge->sn.push_back(ni);
			edges.push_back(new MapEdge({ pi, ni }));
			pi = ni;
			nodes.push_back(new MapNode(slerp(n0->p, n1->p, double(j) / this->resolution), { ei, ei + 1 }));
		}

		edge->se.push_back(edges.size());
		nodes[edge->n[1]]->e.push_back(edges.size());
		edges.push_back(new MapEdge({ pi, edge->n[1] }));
	}

	for (i = 0; i < icoFaces.size(); i++) {
		MapFace* face = icoFaces[i];
		MapEdge* e0 = icoEdges[face->e[0]];
		MapEdge* e1 = icoEdges[face->e[1]];
		MapEdge* e2 = icoEdges[face->e[2]];
		dvec3 p0 = icoNodes[face->n[0]]->p;
		dvec3 p1 = icoNodes[face->n[1]]->p;
		dvec3 p2 = icoNodes[face->n[2]]->p;

		dvec3 d = p1 - p0;


		bool reverseEdge0, reverseEdge1, reverseEdge2;

		reverseEdge0 = face->n[0] != e0->n[0];
		auto getEdgeNode0 = [&](int32 k) { return e0->sn[reverseEdge0 ? (this->resolution - 2 - k) : k]; };
		reverseEdge1 = face->n[1] != e1->n[0];
		auto getEdgeNode1 = [&](int32 k) { return e1->sn[reverseEdge1 ? (this->resolution - 2 - k) : k]; };
		reverseEdge2 = face->n[0] != e2->n[0];
		auto getEdgeNode2 = [&](int32 k) { return e2->sn[reverseEdge2 ? (this->resolution - 2 - k) : k]; };

		faceNodes.clear();
		faceNodes.push_back(face->n[0]);

		for (j = 0; j < e0->sn.size(); j++) {
			faceNodes.push_back(getEdgeNode0(j));
		}

		faceNodes.push_back(face->n[1]);

		for (s = 1; s < this->resolution; s++) {
			faceNodes.push_back(getEdgeNode2(s - 1));
			dvec3 p0 = nodes[getEdgeNode2(s - 1)]->p;
			dvec3 p1 = nodes[getEdgeNode1(s - 1)]->p;
			for (t = 1; t < this->resolution - s; t++) {
				faceNodes.push_back(nodes.size());
				nodes.push_back(new MapNode(slerp(p0, p1, double(t) / (this->resolution - s))));
			}
			faceNodes.push_back(getEdgeNode1(s - 1));
		}

		faceNodes.push_back(face->n[2]);

		reverseEdge0 = face->n[0] != e0->n[0];
		auto getEdgeEdge0 = [&](int32 k) { return e0->se[reverseEdge0 ? (this->resolution - 1 - k) : k];  };
		reverseEdge1 = face->n[1] != e1->n[0];
		auto getEdgeEdge1 = [&](int32 k) { return e1->se[reverseEdge1 ? (this->resolution - 1 - k) : k];  };
		reverseEdge2 = face->n[0] != e2->n[0];
		auto getEdgeEdge2 = [&](int32 k) { return e2->se[reverseEdge2 ? (this->resolution - 1 - k) : k];  };

		int32 nodeIndex, edgeIndex;

		faceEdges0.clear();
		for (j = 0; j < this->resolution; ++j) {
			faceEdges0.push_back(getEdgeEdge0(j));
		}
		nodeIndex = this->resolution + 1;
		for (s = 1; s < this->resolution; s++) {
			for (t = 0; t < this->resolution - s; t++) {
				faceEdges0.push_back(edges.size());
				MapEdge* edge = new MapEdge({ faceNodes[nodeIndex], faceNodes[nodeIndex + 1] });
				nodes[edge->n[0]]->e.push_back(edges.size());
				nodes[edge->n[1]]->e.push_back(edges.size());
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
				MapEdge* edge = new MapEdge({ faceNodes[nodeIndex], faceNodes[nodeIndex + this->resolution - s] });
				nodes[edge->n[0]]->e.push_back(edges.size());
				nodes[edge->n[1]]->e.push_back(edges.size());
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
				MapEdge* edge = new MapEdge({ faceNodes[nodeIndex], faceNodes[nodeIndex + this->resolution - s + 1] });
				nodes[edge->n[0]]->e.push_back(edges.size());
				nodes[edge->n[1]]->e.push_back(edges.size());
				edges.push_back(edge);
				nodeIndex++;
			}
			nodeIndex += 2;
		}

		nodeIndex = 0;
		edgeIndex = 0;
		for (s = 0; s < this->resolution; s++) {
			for (t = 1; t < this->resolution - s + 1; t++) {
				MapFace* subFace = new MapFace(
					{ faceNodes[nodeIndex], faceNodes[nodeIndex + 1], faceNodes[nodeIndex + this->resolution - s + 1] },
					{ faceEdges0[edgeIndex], faceEdges1[edgeIndex], faceEdges2[edgeIndex] }
				);

				nodes[subFace->n[0]]->f.push_back(faces.size());
				nodes[subFace->n[1]]->f.push_back(faces.size());
				nodes[subFace->n[2]]->f.push_back(faces.size());
				edges[subFace->e[0]]->f.push_back(faces.size());
				edges[subFace->e[1]]->f.push_back(faces.size());
				edges[subFace->e[2]]->f.push_back(faces.size());
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
				MapFace* subFace = new MapFace(
					{ faceNodes[nodeIndex], faceNodes[nodeIndex + this->resolution - s + 2], faceNodes[nodeIndex + this->resolution - s + 1] },
					{ faceEdges2[edgeIndex + 1], faceEdges0[edgeIndex + this->resolution - s + 1], faceEdges1[edgeIndex] }
				);

				nodes[subFace->n[0]]->f.push_back(faces.size());
				nodes[subFace->n[1]]->f.push_back(faces.size());
				nodes[subFace->n[2]]->f.push_back(faces.size());
				edges[subFace->e[0]]->f.push_back(faces.size());
				edges[subFace->e[1]]->f.push_back(faces.size());
				edges[subFace->e[2]]->f.push_back(faces.size());
				faces.push_back(subFace);
				nodeIndex++;
				edgeIndex++;
			}
			nodeIndex += 2;
			edgeIndex += 1;
		}
	}

	fvec4* data = new fvec4[nodes.size()]();
	fvec3* points = new fvec3[nodes.size()]();

	for (int i = 0; i < nodes.size(); i++) points[i] = nodes[i]->p;

	planet->getTileSupplier()->computePointData(nodes.size(), points, data);

	for (int i = 0; i < nodes.size(); i++) {
		MapNode* n = nodes[i];

		// Area = planet surface area / number of nodes. Fairly crude approximation.
		n->area = (4.0 * PI * this->planet->getRadius() * this->planet->getRadius()) / nodes.size();
		n->heightmapData = fvec4(data[i]);
		if (data[i].w <= 0.0) {
			n->water = true;
		}
	}

	//delete[] data;
	//delete[] points;

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

	const int32 currentCount = 60;
	CircularCurrent currents[currentCount];

	for (int i = 0; i < currentCount; i++) {
		CircularCurrent c;
		c.p = glm::sphericalRand(1.0);
		c.s = (rand() % 2 == 0 ? +1.0 : -1.0) * drand(2.0, 80.0);
		c.r = drand(0.1, 0.3) * PI;

		currents[i] = c;
	}

	double maxWindStrength = 0.0;

	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode* n = this->nodes[i];

		dvec3 w = dvec3(0.0);

		double outflow = 0.0;
		double weight = 0.0;

		for (int j = 0; j < currentCount; j++) {
			CircularCurrent c = currents[j];

			float angle = glm::angle(c.p, n->p);

			if (angle < c.r) {
				double dist = angle / c.r;
				double weight = 1.0 - dist;
				double strength = c.s * weight * dist;
				w += normalize(cross(c.p, n->p)) * strength;
				weight += 1.0F;
			}
		}

		if (weight > 0.0) {
			w /= weight;
		}

		n->windStrength = length(w);
		n->windVector = (n->windStrength > 1e-8) ? (w / n->windStrength) : dvec3(0.0);

		maxWindStrength = glm::max(maxWindStrength, n->windStrength);

		n->c.resize(n->e.size());
		for (int j = 0; j < n->e.size(); j++) {
			MapEdge* e = this->edges[n->e[j]];
			MapNode* m = this->nodes[(n == this->nodes[e->n[0]]) ? e->n[1] : e->n[0]];

			dvec3 v = normalize(m->p - n->p);
			double d = dot(v, w);

			if (d > 0.0) {
				n->c[j] = d;
				outflow += d;
			} else {
				n->c[j] = 0.0;
			}
		}

		if (outflow > 0.0) {
			for (int j = 0; j < n->e.size(); j++) {
				n->c[j] /= outflow;
			}
		}
	}

	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode* n = this->nodes[i];

		n->normalizedWindStrength = n->windStrength / maxWindStrength;
	}
}

void MapGenerator::initializeHeat() {

	double totalHeat = 0.0;
	std::vector<MapNode*> activeNodes;

	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode* n = this->nodes[i];

		double latitudeTemperature = 1.0 - abs(n->p.y);
		double altitudeTemperature = 1.0 - glm::max(0.0F, n->heightmapData.w);

		double latContrib = 0.7;
		double altContrib = 0.3;

		n->heatAbsorbsion = 0.2 * n->area / glm::clamp(n->normalizedWindStrength, 0.1, 1.0);

		if (n->water) {
			altitudeTemperature *= 1.0 - glm::min(0.5F, abs(n->heightmapData.w));
		}
		else {
			altitudeTemperature = glm::min(altitudeTemperature * 1.3, 1.0);
			n->heatAbsorbsion *= 1.5;
		}

		n->temperature = 0.0;
		n->airHeat = n->area * (latitudeTemperature * latContrib + altitudeTemperature * altContrib);
		n->nextHeat = 0.0;
		totalHeat += n->airHeat;

		activeNodes.push_back(n);
	}

	logInfo("Generated %f heat for %d nodes", totalHeat, activeNodes.size());

	double remainingHeat = totalHeat;
	int32 heatIterations = 0;
	do {
		double consumedHeat = 0.0;
	
		double lossRate = 0.02;
	
		std::set<MapNode*> nextActiveNodes;
		for (int i = 0; i < activeNodes.size(); i++) {
			MapNode* n = activeNodes[i];
	
			if (n->airHeat <= 0.0) {
				continue;
			}
	
			double change = glm::max(0.0, glm::min(n->airHeat, n->heatAbsorbsion * (1.0 - n->temperature / n->area)));
			n->temperature += change;
			consumedHeat += change;
			change = glm::min(n->airHeat, change + (n->area * (n->temperature / n->area) * lossRate));
	
			double movingHeat = n->airHeat - change;
			n->airHeat = 0.0;
	
			for (int j = 0; j < n->e.size(); j++) {
				if (n->c[j] > 0.0) {
					MapEdge* e = this->edges[n->e[j]];
					MapNode* m = this->nodes[(n == this->nodes[e->n[0]]) ? e->n[1] : e->n[0]];
	
					m->nextHeat += movingHeat * n->c[j];
					nextActiveNodes.insert(m);
				}
			}
		}
		remainingHeat -= consumedHeat;
	
		logInfo("consumedHeat %f / %f (%f%%), remainingHeat %f over %d active nodes, %d next active", 
			consumedHeat / 1000.0, totalHeat / 1000.0, consumedHeat / totalHeat * 100.0, remainingHeat / 1000.0, activeNodes.size(), nextActiveNodes.size());
	
		activeNodes = std::vector<MapNode*>(nextActiveNodes.begin(), nextActiveNodes.end());
		
		for (int i = 0; i < activeNodes.size(); i++) {
			MapNode* n = activeNodes[i];
			n->airHeat = n->nextHeat;
			n->nextHeat = 0.0;
		}
	
		heatIterations++;
		if (remainingHeat <= 0.0 || consumedHeat < 1e-5 || heatIterations > 25) {// || consumedHeat < 1e-5) {
			break;
		}
	} while (true);

	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode* n = this->nodes[i];
		n->temperature = (n->temperature + n->airHeat) / n->area;
	}
	
	logInfo("Simulated heat wind distribution in %d iterations", heatIterations);
}

void MapGenerator::initializeMoisture() {
	double totalMoisture = 0.0;
	std::vector<MapNode*> activeNodes;

	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode* n = this->nodes[i];

		n->moisture = 0.0;
		n->nextMoisture = 0.0;
		n->moistureAbsorbsion = (0.0075 * (1.0 + (1.0 - glm::clamp(n->temperature, 0.0, 1.0)))) * n->area / glm::clamp(n->normalizedWindStrength, 0.1, 1.0);

		if (n->water) {
			n->airMoisture = n->area * glm::clamp(0.5 + n->temperature * 0.5, 0.0, 1.0); // Hotter water evaporates more moisture.
			n->maxMoisture = n->area * 0.25;
		} else {
			n->airMoisture = 0.0;
			double h = glm::clamp(n->heightmapData.w, 0.0F, 1.0F);
			n->maxMoisture = n->area * (h * 0.25 + 0.25);
			n->moistureAbsorbsion *= 1.0 + h * 0.5; // Higher altitudes encourage more precipitation
		}

		totalMoisture += n->airMoisture;

		activeNodes.push_back(n);
	}

	logInfo("Generated %f moisture for %d nodes", totalMoisture, activeNodes.size());

	double remainingMoisture = totalMoisture;
	int32 moistureIterations = 0;
	do {
		double consumedMoisture = 0.0;

		double lossRate = 0.02;

		std::set<MapNode*> nextActiveNodes;
		for (int i = 0; i < activeNodes.size(); i++) {
			MapNode* n = activeNodes[i];

			if (n->airMoisture <= 0.0) {
				continue;
			}

			double change = glm::max(0.0, glm::min(n->airMoisture, n->moistureAbsorbsion * (1.0 - n->moisture / n->maxMoisture)));
			n->moisture += change;
			consumedMoisture += change;
			change = glm::min(n->airMoisture, change + (n->area * (n->moisture / n->maxMoisture) * lossRate));

			double movingMoisture = n->airMoisture - change;
			n->airMoisture = 0.0;

			for (int j = 0; j < n->e.size(); j++) {
				if (n->c[j] > 0.0) {
					MapEdge* e = this->edges[n->e[j]];
					MapNode* m = this->nodes[(n == this->nodes[e->n[0]]) ? e->n[1] : e->n[0]];

					m->nextMoisture += movingMoisture * n->c[j];
					nextActiveNodes.insert(m);
				}
			}
		}
		remainingMoisture -= consumedMoisture;

		logInfo("consumedMoisture %f / %f (%f%%), remainingMoisture %f over %d active nodes, %d next active",
			consumedMoisture / 1000.0, totalMoisture / 1000.0, consumedMoisture / totalMoisture * 100.0, remainingMoisture / 1000.0, activeNodes.size(), nextActiveNodes.size());

		activeNodes = std::vector<MapNode*>(nextActiveNodes.begin(), nextActiveNodes.end());

		for (int i = 0; i < activeNodes.size(); i++) {
			MapNode* n = activeNodes[i];
			n->airMoisture = n->nextMoisture;
			n->nextMoisture = 0.0;
		}

		moistureIterations++;
		if (remainingMoisture <= 0.0 || consumedMoisture < 1e-5 || moistureIterations > 100) {
			break;
		}
	} while (true);

	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode* n = this->nodes[i];
		n->moisture = (n->moisture + n->airMoisture) / n->maxMoisture;
	}

	logInfo("Simulated moisture wind distribution in %d iterations", moistureIterations);
}

void MapGenerator::generateDebugMeshes() {
	std::vector<Vertex> surfaceVertices;
	std::vector<Vertex> currentArrowVertices;

	std::vector<uint32> surfaceTriangleIndices;
	std::vector<uint32> surfaceLineIndices;
	std::vector<uint32> currentTriangleIndices;
	std::vector<uint32> currentLineIndices;

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
		MapNode* n = this->nodes[i];

		//float f = n->moisture * (tgc - 1);
		//int32 i0 = glm::clamp<int32>(f - 1.0F, 0, tgc - 1);
		//int32 i1 = glm::clamp<int32>(f - 0.0F, 0, tgc - 1);
		//int32 i2 = glm::clamp<int32>(f + 1.0F, 0, tgc - 1);
		//int32 i3 = glm::clamp<int32>(f + 2.0F, 0, tgc - 1);

		//fvec3 colour = glm::catmullRom(tg[i0], tg[i1], tg[i2], tg[i3], glm::fract(f));
		fvec3 colour = fvec3(n->temperature, n->moisture, 0.0);

		surfaceVertices.push_back(Vertex(n->p * planet->getRadius(), fvec3(0.0), fvec2(0.0), colour));

		// current arrows

		if (true || dot(n->windVector, n->windVector) > 1e-12) {

			double magnitude = n->windStrength;
			dvec3 direction = n->windVector;
			dvec3 side = normalize(cross(direction, n->p));
			int32 baseIndex = currentArrowVertices.size();
			double size = glm::min(sqrt(magnitude), 8.0);
			currentArrowVertices.push_back(Vertex(n->p * planet->getRadius() * 1.0001 + side * 3.0 * size));
			currentArrowVertices.push_back(Vertex(n->p * planet->getRadius() * 1.0001 - side * 3.0 * size));
			currentArrowVertices.push_back(Vertex(n->p * planet->getRadius() * 1.0001 + direction * 20.0 * size));

			currentTriangleIndices.push_back(baseIndex + 0);
			currentTriangleIndices.push_back(baseIndex + 1);
			currentTriangleIndices.push_back(baseIndex + 2);

			currentLineIndices.push_back(baseIndex + 0);
			currentLineIndices.push_back(baseIndex + 1);
			currentLineIndices.push_back(baseIndex + 1);
			currentLineIndices.push_back(baseIndex + 2);
			currentLineIndices.push_back(baseIndex + 2);
			currentLineIndices.push_back(baseIndex + 0);
		}
	}

	for (int i = 0; i < this->edges.size(); i++) {
		MapEdge* e = this->edges[i];
		surfaceLineIndices.push_back(e->n[0]);
		surfaceLineIndices.push_back(e->n[1]);
	}

	for (int i = 0; i < this->faces.size(); i++) {
		MapFace* f = this->faces[i];
		surfaceTriangleIndices.push_back(f->n[0]);
		surfaceTriangleIndices.push_back(f->n[1]);
		surfaceTriangleIndices.push_back(f->n[2]);
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

	MeshData* surfaceTriangleMeshData = new MeshData(surfaceVertices, surfaceTriangleIndices, vertexLayout);
	this->debugSurfaceTriangleMesh = new GLMesh(surfaceTriangleMeshData, vertexLayout);

	MeshData* surfaceLineMeshData = new MeshData(surfaceVertices, surfaceLineIndices, vertexLayout);
	this->debugSurfaceLineMesh = new GLMesh(surfaceLineMeshData, vertexLayout);
	this->debugSurfaceLineMesh->setPrimitive(LINES);

	MeshData* currentTriangleMeshData = new MeshData(currentArrowVertices, currentTriangleIndices, vertexLayout);
	this->debugCurrentTriangleMesh = new GLMesh(currentTriangleMeshData, vertexLayout);

	MeshData* currentLineMeshData = new MeshData(currentArrowVertices, currentLineIndices, vertexLayout);
	this->debugCurrentLineMesh = new GLMesh(currentLineMeshData, vertexLayout);
	this->debugCurrentLineMesh->setPrimitive(LINES);

	delete surfaceTriangleMeshData;
	delete surfaceLineMeshData;
	delete currentTriangleMeshData;
	delete currentLineMeshData;
}

void MapGenerator::render(double partialTicks, double dt) {
	if (this->renderDebugSurface) {
		DEBUG_RENDERER.setLightingEnabled(false);
		DEBUG_RENDERER.setColour(fvec4(1.0F, 1.0F, 1.0F, 1.0F));
		DEBUG_RENDERER.renderMesh(this->debugSurfaceTriangleMesh);
	}

	if (this->renderDebugCurrents) {
		DEBUG_RENDERER.setLightingEnabled(false);
		DEBUG_RENDERER.setColour(fvec4(1.0F, 1.0F, 1.0F, 1.0F));
		DEBUG_RENDERER.renderMesh(this->debugCurrentTriangleMesh);

		DEBUG_RENDERER.setLineSize(1.0F);
		DEBUG_RENDERER.setColour(fvec4(0.25F, 0.25F, 0.25F, 1.0F));
		DEBUG_RENDERER.renderMesh(this->debugCurrentLineMesh);
	}
}

void MapGenerator::setRenderDebugCurrents(bool renderDebugCurrents) {
	this->renderDebugCurrents = renderDebugCurrents;
}

bool MapGenerator::doRenderDebugCurrents() const {
	return this->renderDebugCurrents;
}

void MapGenerator::setRenderDebugSurface(bool renderDebugSurface) {
	this->renderDebugSurface = renderDebugSurface;
}

bool MapGenerator::doRenderDebugSurface() const {
	return this->renderDebugSurface;
}
