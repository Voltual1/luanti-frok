// Copyright (C) 2026 Voltual
// 本程序是自由软件：你可以根据自由软件基金会发布的 GNU 通用公共许可证第3版
//（或任意更新的版本）的条款重新分发和/或修改它。
//本程序是基于希望它有用而分发的，但没有任何担保；甚至没有适销性或特定用途适用性的隐含担保。
// 有关更多细节，请参阅 GNU 通用公共许可证。
//
// 你应该已经收到了一份 GNU 通用公共许可证的副本
// 如果没有，请查阅 <http://www.gnu.org/licenses/>.

#include "mapgen_farlands.h"
#include "voxel.h"
#include "noise.h"
#include "mapnode.h"
#include "map.h"
#include "nodedef.h"
#include "voxelalgorithms.h"
#include "settings.h"
#include "emerge.h"
#include "dungeongen.h"
#include "cavegen.h"
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "util/numeric.h"
#include <cmath>

static inline s16 mymod(s16 a, s16 b)
{
	s16 r = a % b;
	return r < 0 ? r + b : r;
}

MapgenFarlands::MapgenFarlands(MapgenFarlandsParams *params, EmergeParams *emerge)
	: MapgenBasic(MAPGEN_FARLANDS, params, emerge)
{
	spflags            = params->spflags;
	cave_width         = params->cave_width;
	large_cave_depth   = params->large_cave_depth;
	small_cave_num_min = params->small_cave_num_min;
	small_cave_num_max = params->small_cave_num_max;
	large_cave_num_min = params->large_cave_num_min;
	large_cave_num_max = params->large_cave_num_max;
	large_cave_flooded = params->large_cave_flooded;
	cavern_limit       = params->cavern_limit;
	cavern_taper       = params->cavern_taper;
	cavern_threshold   = params->cavern_threshold;
	dungeon_ymin       = params->dungeon_ymin;
	dungeon_ymax       = params->dungeon_ymax;

	noise_filler_depth = new Noise(&params->np_filler_depth, seed, csize.X, csize.Z);

	noise_far1       = new Noise(&params->np_far1, seed, csize.X, csize.Y + 2, csize.Z);
	noise_far2       = new Noise(&params->np_far2, seed + 101, csize.X, csize.Y + 2, csize.Z);
	noise_far_select = new Noise(&params->np_far_select, seed + 202, csize.X, csize.Y + 2, csize.Z);

	MapgenBasic::np_cave1    = params->np_cave1;
	MapgenBasic::np_cave2    = params->np_cave2;
	MapgenBasic::np_cavern   = params->np_cavern;
	MapgenBasic::np_dungeons = params->np_dungeons;
}

MapgenFarlands::~MapgenFarlands()
{
	delete noise_filler_depth;
	delete noise_far1;
	delete noise_far2;
	delete noise_far_select;
}

MapgenFarlandsParams::MapgenFarlandsParams() :
	np_far1         (0.0, 30.0, v3f(120.0, 80.0, 120.0), 82341, 4, 0.55, 2.0),
	np_far2         (0.0, 20.0, v3f(60.0,  40.0, 60.0),  95039, 3, 0.50, 2.0),
	np_far_select   (0.0, 1.0,  v3f(200.0, 200.0, 200.0), 4213,  3, 0.50, 2.0),
	np_filler_depth (0.0, 1.2,  v3f(150.0, 150.0, 150.0), 261,   3, 0.7,  2.0),
	np_cave1        (0.0, 12.0, v3f(61.0,  61.0,  61.0),  52534, 3, 0.5,  2.0),
	np_cave2        (0.0, 12.0, v3f(67.0,  67.0,  67.0),  10325, 3, 0.5,  2.0),
	np_cavern       (0.0, 1.0,  v3f(384.0, 128.0, 384.0), 723,   5, 0.63, 2.0),
	np_dungeons     (0.9, 0.5,  v3f(500.0, 500.0, 500.0), 0,     2, 0.8,  2.0)
{
}

void MapgenFarlandsParams::readParams(const Settings *settings)
{
	settings->getFloatNoEx("mgfarlands_cave_width",         cave_width);
	settings->getS16NoEx("mgfarlands_large_cave_depth",     large_cave_depth);
	settings->getU16NoEx("mgfarlands_small_cave_num_min",   small_cave_num_min);
	settings->getU16NoEx("mgfarlands_small_cave_num_max",   small_cave_num_max);
	settings->getU16NoEx("mgfarlands_large_cave_num_min",   large_cave_num_min);
	settings->getU16NoEx("mgfarlands_large_cave_num_max",   large_cave_num_max);
	settings->getFloatNoEx("mgfarlands_large_cave_flooded", large_cave_flooded);
	settings->getS16NoEx("mgfarlands_cavern_limit",         cavern_limit);
	settings->getS16NoEx("mgfarlands_cavern_taper",         cavern_taper);
	settings->getFloatNoEx("mgfarlands_cavern_threshold",   cavern_threshold);
	settings->getS16NoEx("mgfarlands_dungeon_ymin",         dungeon_ymin);
	settings->getS16NoEx("mgfarlands_dungeon_ymax",         dungeon_ymax);

	settings->getNoiseParams("mgfarlands_np_far1",         np_far1);
	settings->getNoiseParams("mgfarlands_np_far2",         np_far2);
	settings->getNoiseParams("mgfarlands_np_far_select",   np_far_select);
	settings->getNoiseParams("mgfarlands_np_filler_depth", np_filler_depth);
	settings->getNoiseParams("mgfarlands_np_cave1",        np_cave1);
	settings->getNoiseParams("mgfarlands_np_cave2",        np_cave2);
	settings->getNoiseParams("mgfarlands_np_cavern",       np_cavern);
	settings->getNoiseParams("mgfarlands_np_dungeons",     np_dungeons);
}

void MapgenFarlandsParams::writeParams(Settings *settings) const
{
	settings->setFloat("mgfarlands_cave_width",         cave_width);
	settings->setS16("mgfarlands_large_cave_depth",     large_cave_depth);
	settings->setU16("mgfarlands_small_cave_num_min",   small_cave_num_min);
	settings->setU16("mgfarlands_small_cave_num_max",   small_cave_num_max);
	settings->setU16("mgfarlands_large_cave_num_min",   large_cave_num_min);
	settings->setU16("mgfarlands_large_cave_num_max",   large_cave_num_max);
	settings->setFloat("mgfarlands_large_cave_flooded", large_cave_flooded);
	settings->setS16("mgfarlands_cavern_limit",         cavern_limit);
	settings->setS16("mgfarlands_cavern_taper",         cavern_taper);
	settings->setFloat("mgfarlands_cavern_threshold",   cavern_threshold);
	settings->setS16("mgfarlands_dungeon_ymin",         dungeon_ymin);
	settings->setS16("mgfarlands_dungeon_ymax",         dungeon_ymax);

	settings->setNoiseParams("mgfarlands_np_far1",         np_far1);
	settings->setNoiseParams("mgfarlands_np_far2",         np_far2);
	settings->setNoiseParams("mgfarlands_np_far_select",   np_far_select);
	settings->setNoiseParams("mgfarlands_np_filler_depth", np_filler_depth);
	settings->setNoiseParams("mgfarlands_np_cave1",        np_cave1);
	settings->setNoiseParams("mgfarlands_np_cave2",        np_cave2);
	settings->setNoiseParams("mgfarlands_np_cavern",       np_cavern);
	settings->setNoiseParams("mgfarlands_np_dungeons",     np_dungeons);
}

void MapgenFarlandsParams::setDefaultSettings(Settings *settings)
{
}

int MapgenFarlands::getSpawnLevelAtPoint(v2s16 p)
{
	for (s16 y = 60; y >= -60; y--) {
		float n1 = NoiseFractal3D(&noise_far1->np, p.X, y, p.Y, seed);
		float n2 = NoiseFractal3D(&noise_far2->np, p.X, y, p.Y, seed + 101);
		float n_select = rangelim(NoiseFractal3D(&noise_far_select->np, p.X, y, p.Y, seed + 202), 0.0f, 1.0f);

		float density = n1 + n_select * (n2 - n1) - (y - water_level) * 0.15f;

		if (density > 0.0f)
			return y + 2;
	}

	return MAX_MAP_GENERATION_LIMIT;
}

void MapgenFarlands::makeChunk(BlockMakeData *data)
{
	assert(data->vmanip);
	assert(data->nodedef);

	this->generating = true;
	this->vm   = data->vmanip;
	this->ndef = data->nodedef;

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;
	node_min = blockpos_min * MAP_BLOCKSIZE;
	node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);
	full_node_min = (blockpos_min - 1) * MAP_BLOCKSIZE;
	full_node_max = (blockpos_max + 2) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	blockseed = getBlockSeed2(full_node_min, seed);

	s16 stone_surface_max_y = generateTerrain();

	updateHeightmap(node_min, node_max);

	if (biomegen) {
		biomegen->calcBiomeNoise(node_min);
		if (flags & MG_BIOMES) {
			generateBiomes();
		} else {
			biomegen->getBiomes(heightmap, node_min);
		}
	}

	if (flags & MG_CAVES) {
		generateCavesNoiseIntersection(stone_surface_max_y);
		generateCavesRandomWalk(stone_surface_max_y, large_cave_depth);
	}

	if (flags & MG_ORES)
		m_emerge->oremgr->placeAllOres(this, blockseed, node_min, node_max);

	if (flags & MG_DUNGEONS)
		generateDungeons(stone_surface_max_y);

	if (flags & MG_DECORATIONS)
		m_emerge->decomgr->placeAllDecos(this, blockseed, node_min, node_max);

	if (flags & MG_BIOMES)
		dustTopNodes();

	updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);

	if (flags & MG_LIGHT) {
		calcLighting(node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0),
			full_node_min, full_node_max);
	}

	this->generating = false;
}

s16 MapgenFarlands::generateTerrain()
{
	MapNode n_air(CONTENT_AIR);
	MapNode n_stone(c_stone);
	MapNode n_water(c_water_source);

	s16 stone_surface_max_y = node_min.Y;

	noise_far1->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);
	noise_far2->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);
	noise_far_select->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);

	u32 index = 0;
	const v3s32 &em = vm->m_area.getExtent();

	for (s16 z = node_min.Z; z <= node_max.Z; z++) {
		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
			u32 vi = vm->m_area.index(node_min.X, y, z);

			// 每隔 8 个高度在壁面上延伸出平整栈道与阶梯
			s16 mod_y = mymod(y, 8);
			float terrace_boost = (mod_y < 2) ? 0.25f : 0.0f;

			for (s16 x = node_min.X; x <= node_max.X; x++, vi++, index++) {
				if (vm->m_data[vi].getContent() != CONTENT_IGNORE)
					continue;

				float n1       = noise_far1->result[index];
				float n2       = noise_far2->result[index];
				float n_select = rangelim(noise_far_select->result[index], 0.0f, 1.0f);

				// 自然 3D 山体与大峡谷长廊密度场，带有台阶栈道增益
				float base_density = n1 + n_select * (n2 - n1) - (y - water_level) * 0.15f;
				float density      = base_density + terrace_boost;

				if (density > 0.0f) {
					vm->m_data[vi] = n_stone;
					if (y > stone_surface_max_y)
						stone_surface_max_y = y;
				} else if (y <= water_level) {
					vm->m_data[vi] = n_water;
				} else {
					vm->m_data[vi] = n_air;
				}
			}
		}
	}

	if (stone_surface_max_y < node_min.Y)
		stone_surface_max_y = node_min.Y;

	return stone_surface_max_y;
}