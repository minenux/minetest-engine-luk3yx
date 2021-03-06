/*
Minetest
Copyright (C) 2014-2016 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2015-2017 paramat

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "mg_ore.h"
#include "mapgen.h"
#include "noise.h"
#include "map.h"
#include "log.h"
#include <algorithm>


FlagDesc flagdesc_ore[] = {
	{"absheight",                 OREFLAG_ABSHEIGHT},
	{"puff_cliffs",               OREFLAG_PUFF_CLIFFS},
	{"puff_additive_composition", OREFLAG_PUFF_ADDITIVE},
	{NULL,                        0}
};


///////////////////////////////////////////////////////////////////////////////


OreManager::OreManager(IGameDef *gamedef) :
	ObjDefManager(gamedef, OBJDEF_ORE)
{
}


size_t OreManager::placeAllOres(Mapgen *mg, u32 blockseed,
	v3s16 nmin, v3s16 nmax, s16 ore_zero_level)
{
	size_t nplaced = 0;

	for (size_t i = 0; i != m_objects.size(); i++) {
		Ore *ore = (Ore *)m_objects[i];
		if (!ore)
			continue;

		nplaced += ore->placeOre(mg, blockseed, nmin, nmax, ore_zero_level);
		blockseed++;
	}

	return nplaced;
}


void OreManager::clear()
{
	for (size_t i = 0; i < m_objects.size(); i++) {
		Ore *ore = (Ore *)m_objects[i];
		delete ore;
	}
	m_objects.clear();
}


///////////////////////////////////////////////////////////////////////////////


Ore::Ore()
{
	flags = 0;
	noise = NULL;
}


Ore::~Ore()
{
	delete noise;
}


void Ore::resolveNodeNames()
{
	getIdFromNrBacklog(&c_ore, "", CONTENT_AIR);
	getIdsFromNrBacklog(&c_wherein);
}


size_t Ore::placeOre(Mapgen *mg, u32 blockseed,
	v3s16 nmin, v3s16 nmax, s16 ore_zero_level)
{
	// Ore y_min / y_max is displaced by ore_zero_level or remains unchanged.
	// Any ore with a limit at +-MAX_MAP_GENERATION_LIMIT is considered to have
	// that limit at +-infinity, so we do not alter that limit.
	s32 y_min_disp = (y_min <= -MAX_MAP_GENERATION_LIMIT) ?
		-MAX_MAP_GENERATION_LIMIT : y_min + ore_zero_level;

	s32 y_max_disp = (y_max >= MAX_MAP_GENERATION_LIMIT) ?
		MAX_MAP_GENERATION_LIMIT : y_max + ore_zero_level;

	if (nmin.Y > y_max_disp || nmax.Y < y_min_disp)
		return 0;

	int actual_ymin = MYMAX(nmin.Y, y_min_disp);
	int actual_ymax = MYMIN(nmax.Y, y_max_disp);
	if (clust_size >= actual_ymax - actual_ymin + 1)
		return 0;

	nmin.Y = actual_ymin;
	nmax.Y = actual_ymax;
	generate(mg->vm, mg->seed, blockseed, nmin, nmax, mg->biomemap);

	return 1;
}


///////////////////////////////////////////////////////////////////////////////


void OreScatter::generate(MMVManip *vm, int mapseed, u32 blockseed,
	v3s16 nmin, v3s16 nmax, u8 *biomemap)
{
	PcgRandom pr(blockseed);
	MapNode n_ore(c_ore, 0, ore_param2);

	u32 sizex  = (nmax.X - nmin.X + 1);
	u32 volume = (nmax.X - nmin.X + 1) *
				 (nmax.Y - nmin.Y + 1) *
				 (nmax.Z - nmin.Z + 1);
	u32 csize     = clust_size;
	u32 cvolume    = csize * csize * csize;
	u32 nclusters = volume / clust_scarcity;

	for (u32 i = 0; i != nclusters; i++) {
		int x0 = pr.range(nmin.X, nmax.X - csize + 1);
		int y0 = pr.range(nmin.Y, nmax.Y - csize + 1);
		int z0 = pr.range(nmin.Z, nmax.Z - csize + 1);

		if ((flags & OREFLAG_USE_NOISE) &&
			(NoisePerlin3D(&np, x0, y0, z0, mapseed) < nthresh))
			continue;

		if (biomemap && !biomes.empty()) {
			u32 index = sizex * (z0 - nmin.Z) + (x0 - nmin.X);
			UNORDERED_SET<u8>::iterator it = biomes.find(biomemap[index]);
			if (it == biomes.end())
				continue;
		}

		for (u32 z1 = 0; z1 != csize; z1++)
		for (u32 y1 = 0; y1 != csize; y1++)
		for (u32 x1 = 0; x1 != csize; x1++) {
			if (pr.range(1, cvolume) > clust_num_ores)
				continue;

			u32 i = vm->m_area.index(x0 + x1, y0 + y1, z0 + z1);
			if (!CONTAINS(c_wherein, vm->m_data[i].getContent()))
				continue;

			vm->m_data[i] = n_ore;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////


void OreSheet::generate(MMVManip *vm, int mapseed, u32 blockseed,
	v3s16 nmin, v3s16 nmax, u8 *biomemap)
{
	PcgRandom pr(blockseed + 4234);
	MapNode n_ore(c_ore, 0, ore_param2);

	u16 max_height = column_height_max;
	int y_start_min = nmin.Y + max_height;
	int y_start_max = nmax.Y - max_height;

	int y_start = y_start_min < y_start_max ?
		pr.range(y_start_min, y_start_max) :
		(y_start_min + y_start_max) / 2;

	if (!noise) {
		int sx = nmax.X - nmin.X + 1;
		int sz = nmax.Z - nmin.Z + 1;
		noise = new Noise(&np, 0, sx, sz);
	}
	noise->seed = mapseed + y_start;
	noise->perlinMap2D(nmin.X, nmin.Z);

	size_t index = 0;
	for (int z = nmin.Z; z <= nmax.Z; z++)
	for (int x = nmin.X; x <= nmax.X; x++, index++) {
		float noiseval = noise->result[index];
		if (noiseval < nthresh)
			continue;

		if (biomemap && !biomes.empty()) {
			UNORDERED_SET<u8>::iterator it = biomes.find(biomemap[index]);
			if (it == biomes.end())
				continue;
		}

		u16 height = pr.range(column_height_min, column_height_max);
		int ymidpoint = y_start + noiseval;
		int y0 = MYMAX(nmin.Y, ymidpoint - height * (1 - column_midpoint_factor));
		int y1 = MYMIN(nmax.Y, y0 + height - 1);

		for (int y = y0; y <= y1; y++) {
			u32 i = vm->m_area.index(x, y, z);
			if (!vm->m_area.contains(i))
				continue;
			if (!CONTAINS(c_wherein, vm->m_data[i].getContent()))
				continue;

			vm->m_data[i] = n_ore;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////

OrePuff::OrePuff() :
	Ore()
{
	noise_puff_top    = NULL;
	noise_puff_bottom = NULL;
}


OrePuff::~OrePuff()
{
	delete noise_puff_top;
	delete noise_puff_bottom;
}


void OrePuff::generate(MMVManip *vm, int mapseed, u32 blockseed,
	v3s16 nmin, v3s16 nmax, u8 *biomemap)
{
	PcgRandom pr(blockseed + 4234);
	MapNode n_ore(c_ore, 0, ore_param2);

	int y_start = pr.range(nmin.Y, nmax.Y);

	if (!noise) {
		int sx = nmax.X - nmin.X + 1;
		int sz = nmax.Z - nmin.Z + 1;
		noise = new Noise(&np, 0, sx, sz);
		noise_puff_top = new Noise(&np_puff_top, 0, sx, sz);
		noise_puff_bottom = new Noise(&np_puff_bottom, 0, sx, sz);
	}

	noise->seed = mapseed + y_start;
	noise->perlinMap2D(nmin.X, nmin.Z);
	bool noise_generated = false;

	size_t index = 0;
	for (int z = nmin.Z; z <= nmax.Z; z++)
	for (int x = nmin.X; x <= nmax.X; x++, index++) {
		float noiseval = noise->result[index];
		if (noiseval < nthresh)
			continue;

		if (biomemap && !biomes.empty()) {
			UNORDERED_SET<u8>::iterator it = biomes.find(biomemap[index]);
			if (it == biomes.end())
				continue;
		}

		if (!noise_generated) {
			noise_generated = true;
			noise_puff_top->perlinMap2D(nmin.X, nmin.Z);
			noise_puff_bottom->perlinMap2D(nmin.X, nmin.Z);
		}

		float ntop    = noise_puff_top->result[index];
		float nbottom = noise_puff_bottom->result[index];

		if (!(flags & OREFLAG_PUFF_CLIFFS)) {
			float ndiff = noiseval - nthresh;
			if (ndiff < 1.0f) {
				ntop *= ndiff;
				nbottom *= ndiff;
			}
		}

		int ymid = y_start;
		int y0 = ymid - nbottom;
		int y1 = ymid + ntop;

		if ((flags & OREFLAG_PUFF_ADDITIVE) && (y0 > y1))
			SWAP(int, y0, y1);

		for (int y = y0; y <= y1; y++) {
			u32 i = vm->m_area.index(x, y, z);
			if (!vm->m_area.contains(i))
				continue;
			if (!CONTAINS(c_wherein, vm->m_data[i].getContent()))
				continue;

			vm->m_data[i] = n_ore;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////


void OreBlob::generate(MMVManip *vm, int mapseed, u32 blockseed,
	v3s16 nmin, v3s16 nmax, u8 *biomemap)
{
	PcgRandom pr(blockseed + 2404);
	MapNode n_ore(c_ore, 0, ore_param2);

	u32 sizex  = (nmax.X - nmin.X + 1);
	u32 volume = (nmax.X - nmin.X + 1) *
				 (nmax.Y - nmin.Y + 1) *
				 (nmax.Z - nmin.Z + 1);
	u32 csize  = clust_size;
	u32 nblobs = volume / clust_scarcity;

	if (!noise)
		noise = new Noise(&np, mapseed, csize, csize, csize);

	for (u32 i = 0; i != nblobs; i++) {
		int x0 = pr.range(nmin.X, nmax.X - csize + 1);
		int y0 = pr.range(nmin.Y, nmax.Y - csize + 1);
		int z0 = pr.range(nmin.Z, nmax.Z - csize + 1);

		if (biomemap && !biomes.empty()) {
			u32 bmapidx = sizex * (z0 - nmin.Z) + (x0 - nmin.X);
			UNORDERED_SET<u8>::iterator it = biomes.find(biomemap[bmapidx]);
			if (it == biomes.end())
				continue;
		}

		bool noise_generated = false;
		noise->seed = blockseed + i;

		size_t index = 0;
		for (u32 z1 = 0; z1 != csize; z1++)
		for (u32 y1 = 0; y1 != csize; y1++)
		for (u32 x1 = 0; x1 != csize; x1++, index++) {
			u32 i = vm->m_area.index(x0 + x1, y0 + y1, z0 + z1);
			if (!CONTAINS(c_wherein, vm->m_data[i].getContent()))
				continue;

			// Lazily generate noise only if there's a chance of ore being placed
			// This simple optimization makes calls 6x faster on average
			if (!noise_generated) {
				noise_generated = true;
				noise->perlinMap3D(x0, y0, z0);
			}

			float noiseval = noise->result[index];

			float xdist = (s32)x1 - (s32)csize / 2;
			float ydist = (s32)y1 - (s32)csize / 2;
			float zdist = (s32)z1 - (s32)csize / 2;

			noiseval -= (sqrt(xdist * xdist + ydist * ydist + zdist * zdist) / csize);

			if (noiseval < nthresh)
				continue;

			vm->m_data[i] = n_ore;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////

OreVein::OreVein() :
	Ore()
{
	noise2 = NULL;
}


OreVein::~OreVein()
{
	delete noise2;
}


void OreVein::generate(MMVManip *vm, int mapseed, u32 blockseed,
	v3s16 nmin, v3s16 nmax, u8 *biomemap)
{
	PcgRandom pr(blockseed + 520);
	MapNode n_ore(c_ore, 0, ore_param2);

	u32 sizex = (nmax.X - nmin.X + 1);

	if (!noise) {
		int sx = nmax.X - nmin.X + 1;
		int sy = nmax.Y - nmin.Y + 1;
		int sz = nmax.Z - nmin.Z + 1;
		noise  = new Noise(&np, mapseed, sx, sy, sz);
		noise2 = new Noise(&np, mapseed + 436, sx, sy, sz);
	}
	bool noise_generated = false;

	size_t index = 0;
	for (int z = nmin.Z; z <= nmax.Z; z++)
	for (int y = nmin.Y; y <= nmax.Y; y++)
	for (int x = nmin.X; x <= nmax.X; x++, index++) {
		u32 i = vm->m_area.index(x, y, z);
		if (!vm->m_area.contains(i))
			continue;
		if (!CONTAINS(c_wherein, vm->m_data[i].getContent()))
			continue;

		if (biomemap && !biomes.empty()) {
			u32 bmapidx = sizex * (z - nmin.Z) + (x - nmin.X);
			UNORDERED_SET<u8>::iterator it = biomes.find(biomemap[bmapidx]);
			if (it == biomes.end())
				continue;
		}

		// Same lazy generation optimization as in OreBlob
		if (!noise_generated) {
			noise_generated = true;
			noise->perlinMap3D(nmin.X, nmin.Y, nmin.Z);
			noise2->perlinMap3D(nmin.X, nmin.Y, nmin.Z);
		}

		// randval ranges from -1..1
		float randval   = (float)pr.next() / (pr.RANDOM_RANGE / 2) - 1.f;
		float noiseval  = contour(noise->result[index]);
		float noiseval2 = contour(noise2->result[index]);
		if (noiseval * noiseval2 + randval * random_factor < nthresh)
			continue;

		vm->m_data[i] = n_ore;
	}
}
