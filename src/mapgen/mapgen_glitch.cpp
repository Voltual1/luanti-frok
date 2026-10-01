// Copyright (C) 2026 Voltual
// 本程序是自由软件：你可以根据自由软件基金会发布的 GNU 通用公共许可证第3版
//（或任意更新的版本）的条款重新分发和/或修改它。
//本程序是基于希望它有用而分发的，但没有任何担保；甚至没有适销性或特定用途适用性的隐含担保。
// 有关更多细节，请参阅 GNU 通用公共许可证。
//
// 你应该已经收到了一份 GNU 通用公共许可证的副本
// 如果没有，请查阅 <http://www.gnu.org/licenses/>.

#include "mapgen_glitch.h"
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
#include <cmath>

MapgenGlitch::MapgenGlitch(MapgenGlitchParams *params, EmergeParams *emerge)
	: MapgenBasic(MAPGEN_GLITCH, params, emerge)
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

	np_terrain_base    = params->np_terrain_base;

	noise_filler_depth = new Noise(&params->np_filler_depth, seed, csize.X, csize.Z);

	MapgenBasic::np_cave1    = params->np_cave1;
	MapgenBasic::np_cave2    = params->np_cave2;
	MapgenBasic::np_cavern   = params->np_cavern;
	MapgenBasic::np_dungeons = params->np_dungeons;
}

MapgenGlitch::~MapgenGlitch()
{
	delete noise_filler_depth;
}

MapgenGlitchParams::MapgenGlitchParams() :
	np_terrain_base (4.0,  35.0, v3f(250, 250, 250), 82341, 5, 0.6, 2.0),
	np_filler_depth (0.0,  1.2,  v3f(150, 150, 150), 261,   3, 0.7, 2.0),
	np_cave1        (0.0,  12.0, v3f(61,  61,  61),  52534, 3, 0.5, 2.0),
	np_cave2        (0.0,  12.0, v3f(67,  67,  67),  10325, 3, 0.5, 2.0),
	np_cavern       (0.0,  1.0,  v3f(384, 128, 384), 723,   5, 0.63, 2.0),
	np_dungeons     (0.9,  0.5,  v3f(500, 500, 500), 0,     2, 0.8, 2.0)
{
}

void MapgenGlitchParams::readParams(const Settings *settings)
{
	settings->getFloatNoEx("mgglitch_cave_width",         cave_width);
	settings->getS16NoEx("mgglitch_large_cave_depth",     large_cave_depth);
	settings->getU16NoEx("mgglitch_small_cave_num_min",   small_cave_num_min);
	settings->getU16NoEx("mgglitch_small_cave_num_max",   small_cave_num_max);
	settings->getU16NoEx("mgglitch_large_cave_num_min",   large_cave_num_min);
	settings->getU16NoEx("mgglitch_large_cave_num_max",   large_cave_num_max);
	settings->getFloatNoEx("mgglitch_large_cave_flooded", large_cave_flooded);
	settings->getS16NoEx("mgglitch_cavern_limit",         cavern_limit);
	settings->getS16NoEx("mgglitch_cavern_taper",         cavern_taper);
	settings->getFloatNoEx("mgglitch_cavern_threshold",   cavern_threshold);
	settings->getS16NoEx("mgglitch_dungeon_ymin",         dungeon_ymin);
	settings->getS16NoEx("mgglitch_dungeon_ymax",         dungeon_ymax);

	settings->getNoiseParams("mgglitch_np_terrain_base", np_terrain_base);
	settings->getNoiseParams("mgglitch_np_filler_depth", np_filler_depth);
	settings->getNoiseParams("mgglitch_np_cave1",        np_cave1);
	settings->getNoiseParams("mgglitch_np_cave2",        np_cave2);
	settings->getNoiseParams("mgglitch_np_cavern",       np_cavern);
	settings->getNoiseParams("mgglitch_np_dungeons",     np_dungeons);
}

void MapgenGlitchParams::writeParams(Settings *settings) const
{
	settings->setFloat("mgglitch_cave_width",         cave_width);
	settings->setS16("mgglitch_large_cave_depth",     large_cave_depth);
	settings->setU16("mgglitch_small_cave_num_min",   small_cave_num_min);
	settings->setU16("mgglitch_small_cave_num_max",   small_cave_num_max);
	settings->setU16("mgglitch_large_cave_num_min",   large_cave_num_min);
	settings->setU16("mgglitch_large_cave_num_max",   large_cave_num_max);
	settings->setFloat("mgglitch_large_cave_flooded", large_cave_flooded);
	settings->setS16("mgglitch_cavern_limit",         cavern_limit);
	settings->setS16("mgglitch_cavern_taper",         cavern_taper);
	settings->setFloat("mgglitch_cavern_threshold",   cavern_threshold);
	settings->setS16("mgglitch_dungeon_ymin",         dungeon_ymin);
	settings->setS16("mgglitch_dungeon_ymax",         dungeon_ymax);

	settings->setNoiseParams("mgglitch_np_terrain_base", np_terrain_base);
	settings->setNoiseParams("mgglitch_np_filler_depth", np_filler_depth);
	settings->setNoiseParams("mgglitch_np_cave1",        np_cave1);
	settings->setNoiseParams("mgglitch_np_cave2",        np_cave2);
	settings->setNoiseParams("mgglitch_np_cavern",       np_cavern);
	settings->setNoiseParams("mgglitch_np_dungeons",     np_dungeons);
}

void MapgenGlitchParams::setDefaultSettings(Settings *settings)
{
}

int MapgenGlitch::getSpawnLevelAtPoint(v2s16 p)
{
	return 4;
}

void MapgenGlitch::makeChunk(BlockMakeData *data)
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

	// 计算三维 Chunk 坐标
	v3s16 chunk_pos(node_min.X / csize.X, node_min.Y / csize.Y, node_min.Z / csize.Z);

	// 原点区块 (0,0,0) 使用地图原生种子，其它区块根据区块坐标进行确定性哈希偏移
	s32 chunk_seed;
	if (chunk_pos.X == 0 && chunk_pos.Y == 0 && chunk_pos.Z == 0) {
		chunk_seed = seed;
	} else {
		chunk_seed = (s32)getBlockSeed2(chunk_pos, seed);
	}

	blockseed = getBlockSeed2(full_node_min, chunk_seed);

	s16 stone_surface_max_y = generateTerrainForChunk(chunk_seed);

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

s16 MapgenGlitch::generateTerrainForChunk(s32 chunk_seed)
{
	MapNode n_air(CONTENT_AIR);
	MapNode n_stone(c_stone);
	MapNode n_water(c_water_source);

	Noise noise_terrain(&np_terrain_base, chunk_seed, csize.X, csize.Z);
	noise_terrain.noiseMap2D(node_min.X, node_min.Z);

	s16 stone_surface_max_y = -MAX_MAP_GENERATION_LIMIT;
	u32 index = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		s16 surface_y = (s16)noise_terrain.result[index];

		u32 vi = vm->m_area.index(x, node_min.Y - 1, z);
		const v3s32 &em = vm->m_area.getExtent();

		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
			if (vm->m_data[vi].getContent() == CONTENT_IGNORE) {
				if (y <= surface_y) {
					vm->m_data[vi] = n_stone;
					if (y > stone_surface_max_y)
						stone_surface_max_y = y;
				} else if (y <= water_level) {
					vm->m_data[vi] = n_water;
				} else {
					vm->m_data[vi] = n_air;
				}
			}
			VoxelArea::add_y(em, vi, 1);
		}
	}

	if (stone_surface_max_y < node_min.Y)
	stone_surface_max_y = node_min.Y;

return stone_surface_max_y;
}