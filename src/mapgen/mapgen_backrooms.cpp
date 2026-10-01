// Copyright (C) 2026 Voltual
// 本程序是自由软件：你可以根据自由软件基金会发布的 GNU 通用公共许可证第3版
//（或任意更新的版本）的条款重新分发和/或修改它。
//本程序是基于希望它有用而分发的，但没有任何担保；甚至没有适销性或特定用途适用性的隐含担保。
// 有关更多细节，请参阅 GNU 通用公共许可证。
//
// 你应该已经收到了一份 GNU 通用公共许可证的副本
// 如果没有，请查阅 <http://www.gnu.org/licenses/>.

#include "mapgen_backrooms.h"
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

MapgenBackrooms::MapgenBackrooms(MapgenBackroomsParams *params, EmergeParams *emerge)
	: MapgenBasic(MAPGEN_BACKROOMS, params, emerge)
{
	spflags            = params->spflags;
	corridor_width     = params->corridor_width;
	wall_thickness     = params->wall_thickness;
	shelf_spacing      = params->shelf_spacing;
	shelf_height       = params->shelf_height;

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

	noise_wall_noise  = new Noise(&params->np_wall_noise, seed, csize.X, csize.Z);
	noise_shelf_noise = new Noise(&params->np_shelf_noise, seed + 999, csize.X, csize.Y + 2, csize.Z);

	MapgenBasic::np_cave1    = params->np_cave1;
	MapgenBasic::np_cave2    = params->np_cave2;
	MapgenBasic::np_cavern   = params->np_cavern;
	MapgenBasic::np_dungeons = params->np_dungeons;
}

MapgenBackrooms::~MapgenBackrooms()
{
	delete noise_filler_depth;
	delete noise_wall_noise;
	delete noise_shelf_noise;
}

MapgenBackroomsParams::MapgenBackroomsParams() :
	np_wall_noise   (0.0, 1.0,  v3f(120.0, 120.0, 120.0), 82341, 3, 0.5, 2.0),
	np_shelf_noise  (0.0, 1.0,  v3f(30.0,  30.0,  30.0),  95039, 3, 0.5, 2.0),
	np_filler_depth (0.0, 1.2,  v3f(150.0, 150.0, 150.0), 261,   3, 0.7, 2.0),
	np_cave1        (0.0, 12.0, v3f(61.0,  61.0,  61.0),  52534, 3, 0.5, 2.0),
	np_cave2        (0.0, 12.0, v3f(67.0,  67.0,  67.0),  10325, 3, 0.5, 2.0),
	np_cavern       (0.0, 1.0,  v3f(384.0, 128.0, 384.0), 723,   5, 0.63, 2.0),
	np_dungeons     (0.9, 0.5,  v3f(500.0, 500.0, 500.0), 0,     2, 0.8, 2.0)
{
}

void MapgenBackroomsParams::readParams(const Settings *settings)
{
	settings->getS16NoEx("mgbackrooms_corridor_width", corridor_width);
	settings->getS16NoEx("mgbackrooms_wall_thickness", wall_thickness);
	settings->getS16NoEx("mgbackrooms_shelf_spacing",  shelf_spacing);
	settings->getS16NoEx("mgbackrooms_shelf_height",   shelf_height);

	settings->getFloatNoEx("mgbackrooms_cave_width",         cave_width);
	settings->getS16NoEx("mgbackrooms_large_cave_depth",     large_cave_depth);
	settings->getU16NoEx("mgbackrooms_small_cave_num_min",   small_cave_num_min);
	settings->getU16NoEx("mgbackrooms_small_cave_num_max",   small_cave_num_max);
	settings->getU16NoEx("mgbackrooms_large_cave_num_min",   large_cave_num_min);
	settings->getU16NoEx("mgbackrooms_large_cave_num_max",   large_cave_num_max);
	settings->getFloatNoEx("mgbackrooms_large_cave_flooded", large_cave_flooded);
	settings->getS16NoEx("mgbackrooms_cavern_limit",         cavern_limit);
	settings->getS16NoEx("mgbackrooms_cavern_taper",         cavern_taper);
	settings->getFloatNoEx("mgbackrooms_cavern_threshold",   cavern_threshold);
	settings->getS16NoEx("mgbackrooms_dungeon_ymin",         dungeon_ymin);
	settings->getS16NoEx("mgbackrooms_dungeon_ymax",         dungeon_ymax);

	settings->getNoiseParams("mgbackrooms_np_wall_noise",   np_wall_noise);
	settings->getNoiseParams("mgbackrooms_np_shelf_noise",  np_shelf_noise);
	settings->getNoiseParams("mgbackrooms_np_filler_depth", np_filler_depth);
	settings->getNoiseParams("mgbackrooms_np_cave1",        np_cave1);
	settings->getNoiseParams("mgbackrooms_np_cave2",        np_cave2);
	settings->getNoiseParams("mgbackrooms_np_cavern",       np_cavern);
	settings->getNoiseParams("mgbackrooms_np_dungeons",     np_dungeons);
}

void MapgenBackroomsParams::writeParams(Settings *settings) const
{
	settings->setS16("mgbackrooms_corridor_width", corridor_width);
	settings->setS16("mgbackrooms_wall_thickness", wall_thickness);
	settings->setS16("mgbackrooms_shelf_spacing",  shelf_spacing);
	settings->setS16("mgbackrooms_shelf_height",   shelf_height);

	settings->setFloat("mgbackrooms_cave_width",         cave_width);
	settings->setS16("mgbackrooms_large_cave_depth",     large_cave_depth);
	settings->setU16("mgbackrooms_small_cave_num_min",   small_cave_num_min);
	settings->setU16("mgbackrooms_small_cave_num_max",   small_cave_num_max);
	settings->setU16("mgbackrooms_large_cave_num_min",   large_cave_num_min);
	settings->setU16("mgbackrooms_large_cave_num_max",   large_cave_num_max);
	settings->setFloat("mgbackrooms_large_cave_flooded", large_cave_flooded);
	settings->setS16("mgbackrooms_cavern_limit",         cavern_limit);
	settings->setS16("mgbackrooms_cavern_taper",         cavern_taper);
	settings->setFloat("mgbackrooms_cavern_threshold",   cavern_threshold);
	settings->setS16("mgbackrooms_dungeon_ymin",         dungeon_ymin);
	settings->setS16("mgbackrooms_dungeon_ymax",         dungeon_ymax);

	settings->setNoiseParams("mgbackrooms_np_wall_noise",   np_wall_noise);
	settings->setNoiseParams("mgbackrooms_np_shelf_noise",  np_shelf_noise);
	settings->setNoiseParams("mgbackrooms_np_filler_depth", np_filler_depth);
	settings->setNoiseParams("mgbackrooms_np_cave1",        np_cave1);
	settings->setNoiseParams("mgbackrooms_np_cave2",        np_cave2);
	settings->setNoiseParams("mgbackrooms_np_cavern",       np_cavern);
	settings->setNoiseParams("mgbackrooms_np_dungeons",     np_dungeons);
}

void MapgenBackroomsParams::setDefaultSettings(Settings *settings)
{
}

int MapgenBackrooms::getSpawnLevelAtPoint(v2s16 p)
{
	s16 period = corridor_width + wall_thickness;
	s16 mod_x = mymod(p.X, period);
	s16 mod_z = mymod(p.Y, period);

	bool is_wall = (mod_x < wall_thickness || mod_z < wall_thickness);
	if (is_wall) {
		return 16;
	}
	return 1;
}

void MapgenBackrooms::makeChunk(BlockMakeData *data)
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

s16 MapgenBackrooms::generateTerrain()
{
	MapNode n_air(CONTENT_AIR);
	MapNode n_stone(c_stone);
	MapNode n_water(c_water_source);

	s16 stone_surface_max_y = -MAX_MAP_GENERATION_LIMIT;

	noise_wall_noise->noiseMap2D(node_min.X, node_min.Z);
	noise_shelf_noise->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);

	u32 index3d = 0;
	const v3s32 &em = vm->m_area.getExtent();
	s16 period = corridor_width + wall_thickness;

	for (s16 z = node_min.Z; z <= node_max.Z; z++) {
		s16 mod_z = mymod(z, period);
		bool is_wall_z = (mod_z < wall_thickness);
		bool is_near_wall_z = (mod_z < wall_thickness + 4 || mod_z > period - 4);

		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
			u32 vi = vm->m_area.index(node_min.X, y, z);
			s16 mod_y = mymod(y, shelf_spacing);
			bool is_shelf_layer = (mod_y < shelf_height);

			for (s16 x = node_min.X; x <= node_max.X; x++, vi++, index3d++) {
				if (vm->m_data[vi].getContent() != CONTENT_IGNORE)
					continue;

				s16 mod_x = mymod(x, period);
				bool is_wall_x = (mod_x < wall_thickness);
				bool is_near_wall_x = (mod_x < wall_thickness + 4 || mod_x > period - 4);

				u32 index2d = (z - node_min.Z) * csize.X + (x - node_min.X);
				float wall_n = noise_wall_noise->result[index2d];
				float shelf_n = noise_shelf_noise->result[index3d];

				bool is_solid = false;

				if (is_wall_x || is_wall_z) {
					if (wall_n > -0.5f)
						is_solid = true;
				}

				if (is_shelf_layer && (is_near_wall_x || is_near_wall_z)) {
					if (shelf_n > -0.4f)
						is_solid = true;
				}

				if (is_solid) {
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

	return stone_surface_max_y;
}