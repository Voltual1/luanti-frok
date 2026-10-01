// Copyright (C) 2026 Voltual
// 本程序是自由软件：你可以根据自由软件基金会发布的 GNU 通用公共许可证第3版
//（或任意更新的版本）的条款重新分发和/或修改它。
//本程序是基于希望它有用而分发的，但没有任何担保；甚至没有适销性或特定用途适用性的隐含担保。
// 有关更多细节，请参阅 GNU 通用公共许可证。
//
// 你应该已经收到了一份 GNU 通用公共许可证的副本
// 如果没有，请查阅 <http://www.gnu.org/licenses/>.

#include "mapgen_skygrid.h"
#include "voxel.h"
#include "mapnode.h"
#include "map.h"
#include "nodedef.h"
#include "emerge.h"
#include "settings.h"
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "util/numeric.h"

MapgenSkygrid::MapgenSkygrid(MapgenSkygridParams *params, EmergeParams *emerge)
	: MapgenBasic(MAPGEN_SKYGRID, params, emerge)
{
	spflags            = params->spflags;
	grid_spacing       = params->grid_spacing;
	if (grid_spacing < 2)
		grid_spacing = 2;

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

	MapgenBasic::np_cave1    = params->np_cave1;
	MapgenBasic::np_cave2    = params->np_cave2;
	MapgenBasic::np_cavern   = params->np_cavern;
	MapgenBasic::np_dungeons = params->np_dungeons;
}

MapgenSkygrid::~MapgenSkygrid()
{
	delete noise_filler_depth;
}

MapgenSkygridParams::MapgenSkygridParams() :
	np_filler_depth (0.0, 1.2, v3f(150.0, 150.0, 150.0), 261, 3, 0.7,  2.0),
	np_cave1        (0.0, 12.0, v3f(61.0, 61.0, 61.0), 52534, 3, 0.5,  2.0),
	np_cave2        (0.0, 12.0, v3f(67.0, 67.0, 67.0), 10325, 3, 0.5,  2.0),
	np_cavern       (0.0, 1.0, v3f(384.0, 128.0, 384.0), 723, 5, 0.63, 2.0),
	np_dungeons     (0.9, 0.5, v3f(500.0, 500.0, 500.0), 0, 2, 0.8,  2.0)
{
}

void MapgenSkygridParams::readParams(const Settings *settings)
{
	settings->getS16NoEx("mgskygrid_grid_spacing",       grid_spacing);
	settings->getFloatNoEx("mgskygrid_cave_width",         cave_width);
	settings->getS16NoEx("mgskygrid_large_cave_depth",     large_cave_depth);
	settings->getU16NoEx("mgskygrid_small_cave_num_min",   small_cave_num_min);
	settings->getU16NoEx("mgskygrid_small_cave_num_max",   small_cave_num_max);
	settings->getU16NoEx("mgskygrid_large_cave_num_min",   large_cave_num_min);
	settings->getU16NoEx("mgskygrid_large_cave_num_max",   large_cave_num_max);
	settings->getFloatNoEx("mgskygrid_large_cave_flooded", large_cave_flooded);
	settings->getS16NoEx("mgskygrid_cavern_limit",         cavern_limit);
	settings->getS16NoEx("mgskygrid_cavern_taper",         cavern_taper);
	settings->getFloatNoEx("mgskygrid_cavern_threshold",   cavern_threshold);
	settings->getS16NoEx("mgskygrid_dungeon_ymin",         dungeon_ymin);
	settings->getS16NoEx("mgskygrid_dungeon_ymax",         dungeon_ymax);

	settings->getNoiseParams("mgskygrid_np_filler_depth", np_filler_depth);
	settings->getNoiseParams("mgskygrid_np_cave1",        np_cave1);
	settings->getNoiseParams("mgskygrid_np_cave2",        np_cave2);
	settings->getNoiseParams("mgskygrid_np_cavern",       np_cavern);
	settings->getNoiseParams("mgskygrid_np_dungeons",     np_dungeons);
}

void MapgenSkygridParams::writeParams(Settings *settings) const
{
	settings->setS16("mgskygrid_grid_spacing",       grid_spacing);
	settings->setFloat("mgskygrid_cave_width",         cave_width);
	settings->setS16("mgskygrid_large_cave_depth",     large_cave_depth);
	settings->setU16("mgskygrid_small_cave_num_min",   small_cave_num_min);
	settings->setU16("mgskygrid_small_cave_num_max",   small_cave_num_max);
	settings->setU16("mgskygrid_large_cave_num_min",   large_cave_num_min);
	settings->setU16("mgskygrid_large_cave_num_max",   large_cave_num_max);
	settings->setFloat("mgskygrid_large_cave_flooded", large_cave_flooded);
	settings->setS16("mgskygrid_cavern_limit",         cavern_limit);
	settings->setS16("mgskygrid_cavern_taper",         cavern_taper);
	settings->setFloat("mgskygrid_cavern_threshold",   cavern_threshold);
	settings->setS16("mgskygrid_dungeon_ymin",         dungeon_ymin);
	settings->setS16("mgskygrid_dungeon_ymax",         dungeon_ymax);

	settings->setNoiseParams("mgskygrid_np_filler_depth", np_filler_depth);
	settings->setNoiseParams("mgskygrid_np_cave1",        np_cave1);
	settings->setNoiseParams("mgskygrid_np_cave2",        np_cave2);
	settings->setNoiseParams("mgskygrid_np_cavern",       np_cavern);
	settings->setNoiseParams("mgskygrid_np_dungeons",     np_dungeons);
}

void MapgenSkygridParams::setDefaultSettings(Settings *settings)
{
}

void MapgenSkygrid::initPossibleContents()
{
	if (!m_possible_contents.empty())
		return;

	u32 sz = ndef->size();
	for (content_t c = 0; c < sz; c++) {
		if (c == CONTENT_AIR || c == CONTENT_IGNORE || c == CONTENT_UNKNOWN)
			continue;

		const ContentFeatures &f = ndef->get(c);
		if (f.name.empty())
			continue;

		if (f.drawtype != NDT_AIRLIKE) {
			m_possible_contents.push_back(c);
		}
	}

	if (m_possible_contents.empty()) {
		m_possible_contents.push_back(ndef->getId("mapgen_stone"));
	}
}

int MapgenSkygrid::getSpawnLevelAtPoint(v2s16 p)
{
	s16 gx = (p.X / grid_spacing) * grid_spacing;
	s16 gz = (p.Y / grid_spacing) * grid_spacing;
	if (gx != p.X || gz != p.Y)
		return MAX_MAP_GENERATION_LIMIT;

	for (s16 gy = 16; gy >= -16; gy -= grid_spacing) {
		return gy + 1;
	}

	return 0;
}

void MapgenSkygrid::makeChunk(BlockMakeData *data)
{
	assert(data->vmanip);
	assert(data->nodedef);

	this->generating = true;
	this->vm   = data->vmanip;
	this->ndef = data->nodedef;

	initPossibleContents();

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
		biomegen->getBiomes(heightmap, node_min);
	}

	if (flags & MG_ORES)
		m_emerge->oremgr->placeAllOres(this, blockseed, node_min, node_max);

	if (flags & MG_DECORATIONS)
		m_emerge->decomgr->placeAllDecos(this, blockseed, node_min, node_max);

	updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);

	if (flags & MG_LIGHT) {
		calcLighting(node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0),
			full_node_min, full_node_max);
	}

	this->generating = false;
}

s16 MapgenSkygrid::generateTerrain()
{
	s16 stone_surface_max_y = -MAX_MAP_GENERATION_LIMIT;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
		u32 vi = vm->m_area.index(node_min.X, y, z);
		for (s16 x = node_min.X; x <= node_max.X; x++, vi++) {
			if (vm->m_data[vi].getContent() != CONTENT_IGNORE)
				continue;

			if ((x % grid_spacing == 0) && (y % grid_spacing == 0) && (z % grid_spacing == 0)) {
				u32 rand_val = getBlockSeed2(v3s16(x, y, z), seed);
				content_t selected_c = m_possible_contents[rand_val % m_possible_contents.size()];
				vm->m_data[vi] = MapNode(selected_c);
				if (y > stone_surface_max_y)
					stone_surface_max_y = y;
			} else {
				vm->m_data[vi] = MapNode(CONTENT_AIR);
			}
		}
	}

	return stone_surface_max_y;
}