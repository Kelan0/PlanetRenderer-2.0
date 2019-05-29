# PlanetRenderer-2.0
Remaking my original planet rendering engine, but much improved.

The original engine was a first attempt, and many things were done wrong, or done in a very limiting fashion. For example, there was no way of actually providing colouring or height information for anything more than a per-vertex resolution, and there was no way of providing normal maps to terrain chunks to add extra fine details. Furthermore, the terrain loading was quite slow and caused pretty bad frame rate issues when the player moved too quickly, since all of the terrain geometry was generated on the CPU, and was not very parallelizable. In order to overcome these issues, it would require a major overhaul of how the engine works at a fundamental level. It was a learning experience though, and I was able to apply what I have learned to remaking a new and improved engine.

## Features
### Chunked Level-of-Detail
Chunked Level-of-Detail is a really good LOD option for use on spherical worlds, as it lends itself really nicely to use in quadtrees. A planet is fundamentally made out of a spherified cube, where each of the six cube faces correspond to a quadtree. The quadtree is subdivided depending on the distance to the viewer, where if the viewer is closer than a certain threshold, the node gets divided into four children. The quadtrees have a maximum tree depth of 24, and each subdivision creates nodes with half the edge length of the parent node, meaning that for an earth sized planet (12,742 kilometers diameter), the smallest possible node has an edge length of 12,742/(2^24) kilometers, or about 76cm. This is going to be a slightly smaller value when the planets crvature is accounted for. Furthermore, each node gets rendered with a quad mesh, generally with 8 or 16 equidistant vertices along the edges, so the smallest geometric detail is ~4cm. This can be even smaller with more tessellation.

### GPU based terrain generation
The previous attempt at rendering an entirely procedural planet had all of the terrain generation stuff happening on the CPU. This was nice and easy to get working, but it could not very well parallelize the task of generating chunks, since there were only 8 CPU threads to work with. This ultimately meant that terrain loading was slow, both in the sense that terrain chunks took a noticable fraction of a second to actually appear, and in the sense that when terrain was being generated, it would cause very noticable frame spikes. This engine took on the major task of implementing all of the terrain generation code GPU-side, using OpenGL compute shaders. A texture is generated for each terrain tile, with height and normal information.

The terrain TileSupplier class is responsible for all of the terrain loading. It maintains a cache of currently loaded and previously loaded terrain tiles, and tries to keep generated tiles in memory for as long as possible to avoid the costly regeneration of existing tiles. If a new tile is needed, it will try to use an unallocated tile slot, or if it can't, it will choose the least-recently-used tile to evict from the cache and overwrite. The TileSupplier is also responsible for asynchronously reading the tile texture data back from GPU memory, since this data is required CPU-side for things like bounding box generation.

## Screenshots of progress so far

![Screenshot 0](https://i.imgur.com/zHGE68a.png)
![Screenshot 1](https://cdn.discordapp.com/attachments/169182393784205313/582511484580528138/unknown.png)
![Screenshot 2](https://cdn.discordapp.com/attachments/169182393784205313/582526166389227520/unknown.png)
![Screenshot 3](https://cdn.discordapp.com/attachments/169182393784205313/582524682352197632/unknown.png)
![Screenshot 4](https://i.imgur.com/TvsG6IS.jpg)
