#include "MapGenerator.h"
#include "core/application/Application.h"
#include "core/engine/renderer/DebugRenderer.h"
#include "core/engine/renderer/GLMesh.h"
#include "core/engine/renderer/ShaderProgram.h"
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

	//a = Time::now();
	//this->generateIcosohedron();
	//b = Time::now();
	//logInfo("Took %f ms to generate map geometry with %d nodes", (b - a) / 1000000.0, this->nodes.size());
	//
	//a = Time::now();
	//this->generateAirCurrents();
	//b = Time::now();
	//logInfo("Took %f ms to generate air currents", (b - a) / 1000000.0);
	//
	//a = Time::now();
	//this->initializeHeat();
	//b = Time::now();
	//logInfo("Took %f ms to initialize node temperature", (b - a) / 1000000.0);
	//
	//a = Time::now();
	//this->initializeMoisture();
	//b = Time::now();
	//logInfo("Took %f ms to initialize node moisture", (b - a) / 1000000.0);
	//
	//a = Time::now();
	//this->initializeBiomes();
	//b = Time::now();
	//logInfo("Took %f ms to initialize node biomes", (b - a) / 1000000.0);
	//
	//a = Time::now();
	//this->generateDebugMeshes();
	//b = Time::now();
	//logInfo("Took %f ms to generate debug geometry", (b - a) / 1000000.0);

}

MapGenerator::~MapGenerator() {

}

void MapGenerator::generateIcosohedron() {
	const double a = 0.525731112119133606;
	const double b = 0.850650808352039932;
	const double c = 0.0;

	std::vector<MapNode*> icoNodes = {
		new MapNode(fvec3(c, +b, +a)), new MapNode(fvec3(c, +b, -a)), new MapNode(fvec3(c, -b, +a)), new MapNode(fvec3(c, -b, -a)),
		new MapNode(fvec3(+a, c, +b)), new MapNode(fvec3(-a, c, +b)), new MapNode(fvec3(+a, c, -b)), new MapNode(fvec3(-a, c, -b)),
		new MapNode(fvec3(+b, +a, c)), new MapNode(fvec3(+b, -a, c)), new MapNode(fvec3(-b, +a, c)), new MapNode(fvec3(-b, -a, c)),
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
		fvec3 p0 = icoNodes[face->n[0]]->p;
		fvec3 p1 = icoNodes[face->n[1]]->p;
		fvec3 p2 = icoNodes[face->n[2]]->p;

		fvec3 d = p1 - p0;


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
			fvec3 p0 = nodes[getEdgeNode2(s - 1)]->p;
			fvec3 p1 = nodes[getEdgeNode1(s - 1)]->p;
			for (t = 1; t < this->resolution - s; t++) {
				faceNodes.push_back(nodes.size());
				nodes.push_back(new MapNode(slerp(p0, p1, float(t) / (this->resolution - s))));
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
		n->index = i;

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
		fvec3 p;
		float s;
		float r;
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

	float maxWindStrength = 0.0;

	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode* n = this->nodes[i];

		fvec3 w = fvec3(0.0);

		float outflow = 0.0;
		float weight = 0.0;

		for (int j = 0; j < currentCount; j++) {
			CircularCurrent c = currents[j];

			float angle = glm::angle(c.p, n->p);

			if (angle < c.r) {
				float dist = angle / c.r;
				float weight = 1.0 - dist;
				float strength = c.s * weight * dist;
				w += normalize(cross(c.p, n->p)) * strength;
				weight += 1.0F;
			}
		}

		if (weight > 0.0) {
			w /= weight;
		}

		n->windStrength = length(w);
		n->windVector = (n->windStrength > 1e-8) ? (w / n->windStrength) : fvec3(0.0);

		maxWindStrength = glm::max(maxWindStrength, n->windStrength);

		n->c.resize(n->e.size());
		for (int j = 0; j < n->e.size(); j++) {
			MapEdge* e = this->edges[n->e[j]];
			MapNode* m = this->nodes[(n == this->nodes[e->n[0]]) ? e->n[1] : e->n[0]];

			fvec3 v = normalize(m->p - n->p);
			float d = dot(v, w);

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

	float totalHeat = 0.0;
	std::vector<MapNode*> activeNodes;

	uint64 a, b;
	a = Time::now();
	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode* n = this->nodes[i];

		float latitudeTemperature = 1.0 - abs(n->p.y);
		float altitudeTemperature = 1.0 - glm::max(0.0F, n->heightmapData.w);

		float latContrib = 0.7 * (latitudeTemperature * 0.23 + 0.77);
		float altContrib = 1.0 - latContrib;

		n->heatAbsorbsion = 0.08F * n->area / glm::clamp(n->normalizedWindStrength, 0.1F, 1.0F);

		if (n->water) {
			altitudeTemperature *= 1.0 - glm::min(0.9F, abs(n->heightmapData.w));
		} else {
			altitudeTemperature = glm::min(altitudeTemperature * 1.3, 1.0);
			n->heatAbsorbsion *= 1.5;
		}

		n->temperature = 0.0;
		n->airHeat = n->area * (latitudeTemperature * latContrib + altitudeTemperature * altContrib);
		n->nextHeat = 0.0;
		totalHeat += n->airHeat;

		activeNodes.push_back(n);
	}
	b = Time::now();

	logInfo("Generated %f heat for %d nodes in %f ms", totalHeat, activeNodes.size(), (b - a) / 1000000.0);

	// 6400 ms single threaded

	float remainingHeat = totalHeat;
	int32 heatIterations = 0;
	do {
		float consumedHeat = 0.0;
		float lossRate = 0.02;

		const int count = activeNodes.size();
		const int workerCount = glm::min(count, 8);

		std::vector<MapNode*> nextActiveNodes;
		nextActiveNodes.reserve(count);

		uint64 timeLocked = 0;

		std::mutex lock;
		std::thread workers[8];

		for (int threadId = 0; threadId < workerCount; threadId++) {
			workers[threadId] = std::thread([&](int threadId) {
				int workerSize = (int)glm::ceil(count / 8.0);
				int startIndex = glm::min(workerSize * (threadId + 0), count);
				int endIndex = glm::min(workerSize * (threadId + 1), count);

				std::vector<MapNode*> localNextActiveNodes;
				localNextActiveNodes.reserve(workerSize);

				float localConsumedHeat = 0.0;

				for (int i = startIndex; i < endIndex; i++) {
					MapNode* n = activeNodes[i];

					if (n->airHeat <= 0.0) {
						continue;
					}

					float change = glm::max(0.0F, glm::min(n->airHeat, n->heatAbsorbsion * (1.0F - n->temperature / n->area)));
					n->temperature += change;
					localConsumedHeat += change;
					change = glm::min(n->airHeat, change + (n->area * (n->temperature / n->area) * lossRate));

					float movingHeat = n->airHeat - change;
					n->airHeat = 0.0;

					for (int j = 0; j < n->e.size(); j++) {
						if (n->c[j] > 0.0) {
							MapEdge* e = this->edges[n->e[j]];
							MapNode* m = this->nodes[(n == this->nodes[e->n[0]]) ? e->n[1] : e->n[0]];

							m->nextHeat += movingHeat * n->c[j];
							localNextActiveNodes.push_back(m);
						}
					}
				}

				std::sort(localNextActiveNodes.begin(), localNextActiveNodes.end());
				localNextActiveNodes.erase(std::unique(localNextActiveNodes.begin(), localNextActiveNodes.end()), localNextActiveNodes.end());

				uint64 a0 = Time::now();
				lock.lock();
				consumedHeat += localConsumedHeat;
				nextActiveNodes.insert(nextActiveNodes.end(), localNextActiveNodes.begin(), localNextActiveNodes.end());
				uint64 b0 = Time::now();

				timeLocked += b0 - a0;
				lock.unlock();
			}, threadId);
		}

		for (int threadId = 0; threadId < workerCount; threadId++) {
			workers[threadId].join();
		}

		std::sort(nextActiveNodes.begin(), nextActiveNodes.end());
		nextActiveNodes.erase(std::unique(nextActiveNodes.begin(), nextActiveNodes.end()), nextActiveNodes.end());

		remainingHeat -= consumedHeat;
	
		logInfo("consumedHeat %f / %f (%f%%), remainingHeat %f over %d active nodes, %d next active, spent %f ms locked", 
			consumedHeat / 1000.0, totalHeat / 1000.0, consumedHeat / totalHeat * 100.0, remainingHeat / 1000.0, activeNodes.size(), nextActiveNodes.size(), timeLocked / 1000000.0);
	
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
	float totalMoisture = 0.0;
	std::vector<MapNode*> activeNodes;

	uint64 a, b;

	a = Time::now();
	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode* n = this->nodes[i];

		n->moisture = 0.0;
		n->nextMoisture = 0.0;
		n->moistureAbsorbsion = (0.0058F * (1.0F + (1.0F - glm::clamp(n->temperature, 0.0F, 1.0F)))) * n->area / glm::clamp(n->normalizedWindStrength, 0.1F, 1.0F);

		if (n->water) {
			n->airMoisture = n->area * glm::clamp(0.5 + n->temperature * 0.5, 0.0, 1.0); // Hotter water evaporates more moisture.
			n->maxMoisture = n->area * 0.25;
		} else {
			n->airMoisture = 0.0;
			float h = glm::clamp(n->heightmapData.w, 0.0F, 1.0F);
			n->maxMoisture = n->area * (h * 0.25 + 0.25);
			n->moistureAbsorbsion *= 1.0 + h * 0.5; // Higher altitudes encourage more precipitation
		}

		totalMoisture += n->airMoisture;

		activeNodes.push_back(n);
	}
	b = Time::now();

	logInfo("Generated %f moisture for %d nodes in %f ms", totalMoisture, activeNodes.size(), (b - a) / 1000000.0);

	float remainingMoisture = totalMoisture;
	int32 moistureIterations = 0;


	// Single threaded, 13,000 ms in 99 iterations
	// Multithreaded, 2,500 ms in 99 iterations
	do {
		float consumedMoisture = 0.0;
		float lossRate = 0.02;

		const int count = activeNodes.size();
		const int workerCount = glm::min(count, 8);

		std::vector<MapNode*> nextActiveNodes;
		nextActiveNodes.reserve(count);

		uint64 timeLocked = 0;

		std::mutex lock;
		std::thread workers[8];

		for (int threadId = 0; threadId < workerCount; threadId++) {
			workers[threadId] = std::thread([&](int threadId) {
				int workerSize = (int)glm::ceil(count / 8.0);
				int startIndex = glm::min(workerSize * (threadId + 0), count);
				int endIndex = glm::min(workerSize * (threadId + 1), count);

				std::vector<MapNode*> localNextActiveNodes;
				localNextActiveNodes.reserve(workerSize);

				float localConsumedMoisture = 0.0;

				for (int i = startIndex; i < endIndex; i++) {
					MapNode* n = activeNodes[i];

					if (n->airMoisture <= 0.0) {
						continue;
					}

					float change = glm::max(0.0F, glm::min(n->airMoisture, n->moistureAbsorbsion * (1.0F - n->moisture / n->maxMoisture)));
					n->moisture += change;
					localConsumedMoisture += change;
					change = glm::min(n->airMoisture, change + (n->area * (n->moisture / n->maxMoisture) * lossRate));

					float movingMoisture = n->airMoisture - change;
					n->airMoisture = 0.0;

					for (int j = 0; j < n->e.size(); j++) {
						if (n->c[j] > 0.0) {
							MapEdge* e = this->edges[n->e[j]];
							MapNode* m = this->nodes[(n == this->nodes[e->n[0]]) ? e->n[1] : e->n[0]];

							m->nextMoisture += movingMoisture * n->c[j];
							localNextActiveNodes.push_back(m);
						}
					}
				}

				std::sort(localNextActiveNodes.begin(), localNextActiveNodes.end());
				localNextActiveNodes.erase(std::unique(localNextActiveNodes.begin(), localNextActiveNodes.end()), localNextActiveNodes.end());
				
				uint64 a0 = Time::now();
				lock.lock();
				consumedMoisture += localConsumedMoisture;
				nextActiveNodes.insert(nextActiveNodes.end(), localNextActiveNodes.begin(), localNextActiveNodes.end());
				uint64 b0 = Time::now();

				timeLocked += b0 - a0;
				lock.unlock();
			}, threadId);
		}

		for (int threadId = 0; threadId < workerCount; threadId++) {
			workers[threadId].join();
		}

		remainingMoisture -= consumedMoisture;

		logInfo("consumedMoisture %f / %f (%f%%), remainingMoisture %f over %d active nodes, %d next active, spent %f ms locked",
			consumedMoisture / 1000.0, totalMoisture / 1000.0, consumedMoisture / totalMoisture * 100.0, remainingMoisture / 1000.0, activeNodes.size(), nextActiveNodes.size(), timeLocked / 1000000.0);

		std::sort(nextActiveNodes.begin(), nextActiveNodes.end());
		nextActiveNodes.erase(std::unique(nextActiveNodes.begin(), nextActiveNodes.end()), nextActiveNodes.end());

		activeNodes = nextActiveNodes;// std::vector<MapNode*>(nextActiveNodes.begin(), nextActiveNodes.end());

		for (int i = 0; i < activeNodes.size(); i++) {
			MapNode* n = activeNodes[i];
			n->airMoisture = n->nextMoisture;
			n->nextMoisture = 0.0;
		}

		moistureIterations++;
		if (remainingMoisture <= 0.0 || consumedMoisture < 1e-5 || moistureIterations > 200) {
			break;
		}
	} while (true);

	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode* n = this->nodes[i];
		n->moisture = (n->moisture + n->airMoisture) / n->maxMoisture;
	}

	logInfo("Simulated moisture wind distribution in %d iterations", moistureIterations);
}

void MapGenerator::initializeBiomes() {
	this->lifeZones.push_back(new LifeZone("Tropical Desert",				5.0 / 6.0, 6.0 / 6.0, 0.0 / 8.0, 1.0 / 8.0, fvec3(1.000, 1.000, 0.502))); // FFFF80, 
	this->lifeZones.push_back(new LifeZone("Tropical Desert Scrub",			5.0 / 6.0, 6.0 / 6.0, 1.0 / 8.0, 2.0 / 8.0, fvec3(0.878, 1.000, 0.502))); // E0FF80, 
	this->lifeZones.push_back(new LifeZone("Tropical Thorn Woodland",		5.0 / 6.0, 6.0 / 6.0, 2.0 / 8.0, 3.0 / 8.0, fvec3(0.753, 1.000, 0.502))); // C0FF80, 
	this->lifeZones.push_back(new LifeZone("Tropical Very Dry Forest",		5.0 / 6.0, 6.0 / 6.0, 3.0 / 8.0, 4.0 / 8.0, fvec3(0.627, 1.000, 0.502))); // A0FF80, 
	this->lifeZones.push_back(new LifeZone("Tropical Dry Forest",			5.0 / 6.0, 6.0 / 6.0, 4.0 / 8.0, 5.0 / 8.0, fvec3(0.502, 1.000, 0.402))); // 80FF80, 
	this->lifeZones.push_back(new LifeZone("Tropical Moist Forest",			5.0 / 6.0, 6.0 / 6.0, 5.0 / 8.0, 6.0 / 8.0, fvec3(0.376, 1.000, 0.252))); // 60FF80, 
	this->lifeZones.push_back(new LifeZone("Tropical Wet Forest",			5.0 / 6.0, 6.0 / 6.0, 6.0 / 8.0, 7.0 / 8.0, fvec3(0.251, 1.000, 0.275))); // 40FF90, 
	this->lifeZones.push_back(new LifeZone("Tropical Rain Forest",			5.0 / 6.0, 6.0 / 6.0, 7.0 / 8.0, 8.0 / 8.0, fvec3(0.125, 1.000, 0.317))); // 20FFA0, 

	this->lifeZones.push_back(new LifeZone("Subtropical Desert",			4.5 / 6.0, 5.0 / 6.0, 0.0 / 7.0, 1.0 / 7.0, fvec3(0.941, 0.941, 0.502))); // F0F080, 
	this->lifeZones.push_back(new LifeZone("Subtropical Desert Scrub",		4.5 / 6.0, 5.0 / 6.0, 1.0 / 7.0, 2.0 / 7.0, fvec3(0.816, 0.941, 0.502))); // D0F080, 
	this->lifeZones.push_back(new LifeZone("Subtropical Thorn Woodland",	4.5 / 6.0, 5.0 / 6.0, 2.0 / 7.0, 3.0 / 7.0, fvec3(0.753, 1.000, 0.502))); // C0FF80, 
	this->lifeZones.push_back(new LifeZone("Subtropical Dry Forest",		4.5 / 6.0, 5.0 / 6.0, 3.0 / 7.0, 4.0 / 7.0, fvec3(0.502, 0.941, 0.502))); // 80F080, 
	this->lifeZones.push_back(new LifeZone("Subtropical Moist Forest",		4.5 / 6.0, 5.0 / 6.0, 4.0 / 7.0, 5.0 / 7.0, fvec3(0.376, 0.941, 0.502))); // 60F080, 
	this->lifeZones.push_back(new LifeZone("Subtropical Wet Forest",		4.5 / 6.0, 5.0 / 6.0, 5.0 / 7.0, 6.0 / 7.0, fvec3(0.251, 0.941, 0.565))); // 40F090, 
	this->lifeZones.push_back(new LifeZone("Subtropical Rain Forest",		4.5 / 6.0, 5.0 / 6.0, 6.0 / 7.0, 7.0 / 7.0, fvec3(0.125, 0.941, 0.690))); // 20F0B0, 

	this->lifeZones.push_back(new LifeZone("Warm Temperate Desert",			4.0 / 6.0, 4.5 / 6.0, 0.0 / 7.0, 1.0 / 7.0, fvec3(0.878, 0.878, 0.502))); // E0E080, 
	this->lifeZones.push_back(new LifeZone("Warm Temperate Desert Scrub",	4.0 / 6.0, 4.5 / 6.0, 1.0 / 7.0, 2.0 / 7.0, fvec3(0.753, 0.878, 0.502))); // C0E080, 
	this->lifeZones.push_back(new LifeZone("Warm Temperate Thorn Scrub",	4.0 / 6.0, 4.5 / 6.0, 2.0 / 7.0, 3.0 / 7.0, fvec3(0.502, 0.878, 0.502))); // 80E080, 
	this->lifeZones.push_back(new LifeZone("Warm Temperate Dry Forest",		4.0 / 6.0, 4.5 / 6.0, 3.0 / 7.0, 4.0 / 7.0, fvec3(0.502, 0.878, 0.502))); // 80e080, 
	this->lifeZones.push_back(new LifeZone("Warm Temperate Moist Forest",	4.0 / 6.0, 4.5 / 6.0, 4.0 / 7.0, 5.0 / 7.0, fvec3(0.376, 0.878, 0.502))); // 60E080, 
	this->lifeZones.push_back(new LifeZone("Warm Temperate Wet Forest",		4.0 / 6.0, 4.5 / 6.0, 5.0 / 7.0, 6.0 / 7.0, fvec3(0.251, 0.878, 0.565))); // 40e090, 
	this->lifeZones.push_back(new LifeZone("Warm Temperate Rain Forest",	4.0 / 6.0, 4.5 / 6.0, 6.0 / 7.0, 7.0 / 7.0, fvec3(0.125, 0.878, 0.753))); // 20E0C0, 

	this->lifeZones.push_back(new LifeZone("Cool Temperate Desert",			3.0 / 6.0, 4.0 / 6.0, 0.0 / 6.0, 1.0 / 6.0, fvec3(0.753, 0.753, 0.502))); // C0C080, 
	this->lifeZones.push_back(new LifeZone("Cool Temperate Desert Scrub",	3.0 / 6.0, 4.0 / 6.0, 1.0 / 6.0, 2.0 / 6.0, fvec3(0.627, 0.753, 0.502))); // A0C080, 
	this->lifeZones.push_back(new LifeZone("Cool Temperate Steppe",			3.0 / 6.0, 4.0 / 6.0, 2.0 / 6.0, 3.0 / 6.0, fvec3(0.502, 0.753, 0.502))); // 80C080, 
	this->lifeZones.push_back(new LifeZone("Cool Temperate Moist Forest",	3.0 / 6.0, 4.0 / 6.0, 3.0 / 6.0, 4.0 / 6.0, fvec3(0.376, 0.753, 0.502))); // 60C080, 
	this->lifeZones.push_back(new LifeZone("Cool Temperate Wet Forest",		3.0 / 6.0, 4.0 / 6.0, 4.0 / 6.0, 5.0 / 6.0, fvec3(0.251, 0.753, 0.565))); // 40C090, 
	this->lifeZones.push_back(new LifeZone("Cool Temperate Rain Forest",	3.0 / 6.0, 4.0 / 6.0, 5.0 / 6.0, 6.0 / 6.0, fvec3(0.125, 0.753, 0.753))); // 20C0C0, 

	this->lifeZones.push_back(new LifeZone("Boreal Desert",					2.0 / 6.0, 3.0 / 6.0, 0.0 / 5.0, 1.0 / 5.0, fvec3(0.627, 0.627, 0.502))); // A0A080, 
	this->lifeZones.push_back(new LifeZone("Boreal Dry Scrub",				2.0 / 6.0, 3.0 / 6.0, 1.0 / 5.0, 2.0 / 5.0, fvec3(0.502, 0.627, 0.502))); // 80A080, 
	this->lifeZones.push_back(new LifeZone("Boreal Moist Forest",			2.0 / 6.0, 3.0 / 6.0, 2.0 / 5.0, 3.0 / 5.0, fvec3(0.376, 0.627, 0.502))); // 60A080, 
	this->lifeZones.push_back(new LifeZone("Boreal Wet Forest",				2.0 / 6.0, 3.0 / 6.0, 3.0 / 5.0, 4.0 / 5.0, fvec3(0.251, 0.627, 0.565))); // 40A090, 
	this->lifeZones.push_back(new LifeZone("Boreal Rain Forest",			2.0 / 6.0, 3.0 / 6.0, 4.0 / 5.0, 5.0 / 5.0, fvec3(0.125, 0.627, 0.753))); // 20A0C0, 

	this->lifeZones.push_back(new LifeZone("Subpolar Dry Tundra",			1.0 / 6.0, 2.0 / 6.0, 0.0 / 4.0, 1.0 / 4.0, fvec3(0.502, 0.502, 0.502))); // 808080, 
	this->lifeZones.push_back(new LifeZone("Subpolar Moist Tundra",			1.0 / 6.0, 2.0 / 6.0, 1.0 / 4.0, 2.0 / 4.0, fvec3(0.376, 0.502, 0.502))); // 608080, 
	this->lifeZones.push_back(new LifeZone("Subpolar Wet Tundra",			1.0 / 6.0, 2.0 / 6.0, 2.0 / 4.0, 3.0 / 4.0, fvec3(0.251, 0.502, 0.502))); // 408080, 
	this->lifeZones.push_back(new LifeZone("Subpolar Rain Tundra",			1.0 / 6.0, 2.0 / 6.0, 3.0 / 4.0, 4.0 / 4.0, fvec3(0.125, 0.502, 0.753))); // 2080C0, 

	this->lifeZones.push_back(new LifeZone("Polar Desert",					0.0 / 6.0, 1.0 / 6.0, 0.0 / 3.0, 1.0 / 3.0, fvec3(0.753, 0.753, 0.753))); // C0C0C0, 
	this->lifeZones.push_back(new LifeZone("Polar Ice",						0.0 / 6.0, 1.0 / 6.0, 1.0 / 3.0, 2.0 / 3.0, fvec3(1.000, 1.000, 1.000))); // FFFFFF, 
	this->lifeZones.push_back(new LifeZone("Polar Ice",						0.0 / 6.0, 1.0 / 6.0, 2.0 / 3.0, 3.0 / 3.0, fvec3(1.000, 1.000, 1.000))); // FFFFFF, 

	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode* n = this->nodes[i];
		float temperature = glm::clamp(n->temperature * 1.7F - 0.7F, 0.0F, 1.0F);
		float moisture = glm::clamp(n->moisture, 0.0F, 1.0F);

		if (n->water) {
			if (temperature <= 0.0) {
				n->lifeZone = this->lifeZones[this->lifeZones.size() - 1];
			} else {
				n->lifeZone = NULL;
			}
		} else {

			for (int j = 0; j < this->lifeZones.size(); j++) {
				LifeZone* zone = this->lifeZones[j];

				if (temperature < zone->minTemperature) continue;
				if (temperature > zone->maxTemperature) continue;
				if (moisture < zone->minMoisture) continue;
				if (moisture > zone->maxMoisture) continue;

				n->lifeZone = zone;
			}
		}
	}
}

void MapGenerator::generateDebugMeshes() {
	std::vector<Vertex> surfaceVertices; surfaceVertices.reserve(this->nodes.size() * 1);
	std::vector<Vertex> currentArrowVertices; currentArrowVertices.reserve(this->nodes.size() * 3);

	std::vector<uint32> surfaceTriangleIndices; surfaceTriangleIndices.reserve(this->faces.size() * 3);
	std::vector<uint32> surfaceLineIndices; surfaceLineIndices.reserve(this->edges.size() * 2);
	std::vector<uint32> currentTriangleIndices; currentTriangleIndices.reserve(this->nodes.size() * 3);
	std::vector<uint32> currentLineIndices; currentLineIndices.reserve(this->nodes.size() * 6);

	const int32 tgc = 6;
	fvec3 tg[tgc] = {
		fvec3(0.5F, 0.0F, 1.0F),
		fvec3(0.0F, 0.0F, 1.0F),
		fvec3(0.0F, 1.0F, 1.0F),
		fvec3(0.0F, 1.0F, 0.0F),
		fvec3(1.0F, 1.0F, 0.0F),
		fvec3(1.0F, 0.0F, 0.0F),
	};

	uint64 a, b, c;

	a = Time::now();
	for (int i = 0; i < this->nodes.size(); i++) {
		MapNode* n = this->nodes[i];

		//float f = n->moisture * (tgc - 1);
		//int32 i0 = glm::clamp<int32>(f - 1.0F, 0, tgc - 1);
		//int32 i1 = glm::clamp<int32>(f - 0.0F, 0, tgc - 1);
		//int32 i2 = glm::clamp<int32>(f + 1.0F, 0, tgc - 1);
		//int32 i3 = glm::clamp<int32>(f + 2.0F, 0, tgc - 1);

		//fvec3 colour = glm::catmullRom(tg[i0], tg[i1], tg[i2], tg[i3], glm::fract(f));
		fvec3 colour = n->lifeZone != NULL ? n->lifeZone->colour : fvec3(0.2, 0.3, 0.9);// fvec3(n->temperature, n->moisture, 0.0);

		surfaceVertices.push_back(Vertex(n->p * float(planet->getRadius()), fvec3(0.0), fvec2(0.0), colour));

		// current arrows

		if (true || dot(n->windVector, n->windVector) > 1e-12) {

			float magnitude = n->windStrength;
			fvec3 direction = n->windVector;
			fvec3 side = normalize(cross(direction, n->p));
			int32 baseIndex = currentArrowVertices.size();
			float size = glm::min(sqrt(magnitude), 8.0F);
			currentArrowVertices.push_back(Vertex(n->p * float(planet->getRadius()) * 1.0001F + side * 3.0F * size));
			currentArrowVertices.push_back(Vertex(n->p * float(planet->getRadius()) * 1.0001F - side * 3.0F * size));
			currentArrowVertices.push_back(Vertex(n->p * float(planet->getRadius()) * 1.0001F + direction * 20.0F * size));

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
	b = Time::now();
	logInfo("Took %f ms to create debug mesh vertices", (b - a) / 1000000.0);

	a = Time::now();
	for (int i = 0; i < this->edges.size(); i++) {
		MapEdge* e = this->edges[i];
		surfaceLineIndices.push_back(e->n[0]);
		surfaceLineIndices.push_back(e->n[1]);
	}
	b = Time::now();
	logInfo("Took %f ms to create debug mesh edge indices", (b - a) / 1000000.0);

	a = Time::now();
	for (int i = 0; i < this->faces.size(); i++) {
		MapFace* f = this->faces[i];
		surfaceTriangleIndices.push_back(f->n[0]);
		surfaceTriangleIndices.push_back(f->n[1]);
		surfaceTriangleIndices.push_back(f->n[2]);
	}
	b = Time::now();
	logInfo("Took %f ms to create debug mesh face indices", (b - a) / 1000000.0);

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

	a = Time::now();
	MeshData* surfaceTriangleMeshData = new MeshData(surfaceVertices, surfaceTriangleIndices, vertexLayout);
	b = Time::now();
	this->debugSurfaceTriangleMesh = new GLMesh(surfaceTriangleMeshData, vertexLayout);
	c = Time::now();
	logInfo("Took %f ms to create debug triangle mesh data, %f ms to upload to video memory", (b - a) / 1000000.0, (c - b) / 1000000.0);

	a = Time::now();
	MeshData* surfaceLineMeshData = new MeshData(surfaceVertices, surfaceLineIndices, vertexLayout);
	b = Time::now();
	this->debugSurfaceLineMesh = new GLMesh(surfaceLineMeshData, vertexLayout);
	this->debugSurfaceLineMesh->setPrimitive(LINES);
	c = Time::now();
	logInfo("Took %f ms to create debug surface line mesh data, %f ms to upload to video memory", (b - a) / 1000000.0, (c - b) / 1000000.0);

	a = Time::now();
	MeshData* currentTriangleMeshData = new MeshData(currentArrowVertices, currentTriangleIndices, vertexLayout);
	b = Time::now();
	this->debugCurrentTriangleMesh = new GLMesh(currentTriangleMeshData, vertexLayout);
	c = Time::now();
	logInfo("Took %f ms to create debug wind current triangle mesh data, %f ms to upload to video memory", (b - a) / 1000000.0, (c - b) / 1000000.0);

	a = Time::now();
	MeshData* currentLineMeshData = new MeshData(currentArrowVertices, currentLineIndices, vertexLayout);
	b = Time::now();
	this->debugCurrentLineMesh = new GLMesh(currentLineMeshData, vertexLayout);
	this->debugCurrentLineMesh->setPrimitive(LINES);
	c = Time::now();
	logInfo("Took %f ms to create debug wind current line mesh data, %f ms to upload to video memory", (b - a) / 1000000.0, (c - b) / 1000000.0);

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

		if (this->debugClosestWalkMesh != NULL && this->debugClosestWalk.size() > 1) {
			DEBUG_RENDERER.setLineSize(5.0F);
			DEBUG_RENDERER.setColour(fvec4(0.1F, 0.1F, 0.1F, 1.0F));
			DEBUG_RENDERER.renderMesh(this->debugClosestWalkMesh);
		}
	}

	if (this->renderDebugCurrents) {
		DEBUG_RENDERER.setLightingEnabled(false);
		//DEBUG_RENDERER.setColour(fvec4(1.0F, 1.0F, 1.0F, 1.0F));
		//DEBUG_RENDERER.renderMesh(this->debugCurrentTriangleMesh);

		DEBUG_RENDERER.setLineSize(1.0F);
		DEBUG_RENDERER.setColour(fvec4(0.25F, 0.25F, 0.25F, 1.0F));
		//DEBUG_RENDERER.renderMesh(this->debugCurrentLineMesh);
		DEBUG_RENDERER.renderMesh(this->debugSurfaceLineMesh);
	}
}

MapNode* MapGenerator::getClosestMapNode(dvec3 point, int startPoint) {
	// choose a random point on the sphere, and walk in the direction of the point to find, until no direction yields a closer vertex.
	// This should avoid iterating every single vertex.

	int32 currIndex = (startPoint < 0 || startPoint >= this->nodes.size()) ? (rand() % this->nodes.size()) : startPoint; // random start point if none is specified
	MapNode* currNode = this->nodes[currIndex];
	double currDist = glm::distance2(dvec3(currNode->p), point);

	while (true) {
		if (currNode == NULL) {
			break;
		}

		bool flag = false;

		for (int i = 0; i < currNode->e.size(); i++) {
			MapEdge* edge = this->edges[currNode->e[i]];
			int32 neighbourIndex = (currNode == this->nodes[edge->n[0]]) ? edge->n[1] : edge->n[0];
			MapNode* neighbourNode = this->nodes[neighbourIndex];

			double neighbourDist = glm::distance2(dvec3(neighbourNode->p), point);

			if (neighbourDist < currDist) {
				currDist = neighbourDist;
				currNode = neighbourNode;
				currIndex = neighbourIndex;
				flag = true;
			}
		}

		if (!flag) {
			break; // No closer nodes were found. This is the closest.
		}
	}

	return currNode;
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

void MapGenerator::setDebugSurfaceRenderMode(int renderMode) {
	this->debugSurfaceRenderMode = renderMode % 3;
}

int MapGenerator::getDebugSurfaceRenderMode() const {
	return this->debugSurfaceRenderMode;
}
