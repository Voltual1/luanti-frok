// Copyright (C) 2026 Voltual
// 本程序是自由软件：你可以根据自由软件基金会发布的 GNU 通用公共许可证第3版
//（或任意更新的版本）的条款重新分发和/或修改它。
//本程序是基于希望它有用而分发的，但没有任何担保；甚至没有适销性或特定用途适用性的隐含担保。
// 有关更多细节，请参阅 GNU 通用公共许可证。
//
// 你应该已经收到了一份 GNU 通用公共许可证的副本
// 如果没有，请查阅 <http://www.gnu.org/licenses/>.

#include "mapgen_arg.h"
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

MapgenARG::MapgenARG(MapgenARGParams *params, EmergeParams *emerge)
	: MapgenBasic(MAPGEN_ARG, params, emerge)
{
	spflags            = params->spflags;
	web_thickness      = params->web_thickness;
	web_thickness_fine = params->web_thickness_fine;

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

	noise_web1 = new Noise(&params->np_web1, seed, csize.X, csize.Y + 2, csize.Z);
	noise_web2 = new Noise(&params->np_web2, seed + 1337, csize.X, csize.Y + 2, csize.Z);
	noise_web3 = new Noise(&params->np_web3, seed + 2026, csize.X, csize.Y + 2, csize.Z);
	noise_web4 = new Noise(&params->np_web4, seed + 4044, csize.X, csize.Y + 2, csize.Z);

	MapgenBasic::np_cave1    = params->np_cave1;
	MapgenBasic::np_cave2    = params->np_cave2;
	MapgenBasic::np_cavern   = params->np_cavern;
	MapgenBasic::np_dungeons = params->np_dungeons;
}

MapgenARG::~MapgenARG()
{
	delete noise_filler_depth;
	delete noise_web1;
	delete noise_web2;
	delete noise_web3;
	delete noise_web4;
}

MapgenARGParams::MapgenARGParams() :
	np_web1         (0.0, 1.0, v3f(42.0, 42.0, 42.0), 983240, 4, 0.55, 2.0),
	np_web2         (0.0, 1.0, v3f(42.0, 42.0, 42.0), 432109, 4, 0.55, 2.0),
	np_web3         (0.0, 1.0, v3f(20.0, 20.0, 20.0), 876543, 3, 0.50, 2.0),
	np_web4         (0.0, 1.0, v3f(20.0, 20.0, 20.0), 123456, 3, 0.50, 2.0),
	np_filler_depth (0.0, 1.2, v3f(150.0, 150.0, 150.0), 261, 3, 0.7,  2.0),
	np_cave1        (0.0, 12.0, v3f(61.0, 61.0, 61.0), 52534, 3, 0.5,  2.0),
	np_cave2        (0.0, 12.0, v3f(67.0, 67.0, 67.0), 10325, 3, 0.5,  2.0),
	np_cavern       (0.0, 1.0, v3f(384.0, 128.0, 384.0), 723, 5, 0.63, 2.0),
	np_dungeons     (0.9, 0.5, v3f(500.0, 500.0, 500.0), 0, 2, 0.8,  2.0)
{
}

void MapgenARGParams::readParams(const Settings *settings)
{
	settings->getFloatNoEx("mgarg_web_thickness",      web_thickness);
	settings->getFloatNoEx("mgarg_web_thickness_fine", web_thickness_fine);

	settings->getFloatNoEx("mgarg_cave_width",         cave_width);
	settings->getS16NoEx("mgarg_large_cave_depth",     large_cave_depth);
	settings->getU16NoEx("mgarg_small_cave_num_min",   small_cave_num_min);
	settings->getU16NoEx("mgarg_small_cave_num_max",   small_cave_num_max);
	settings->getU16NoEx("mgarg_large_cave_num_min",   large_cave_num_min);
	settings->getU16NoEx("mgarg_large_cave_num_max",   large_cave_num_max);
	settings->getFloatNoEx("mgarg_large_cave_flooded", large_cave_flooded);
	settings->getS16NoEx("mgarg_cavern_limit",         cavern_limit);
	settings->getS16NoEx("mgarg_cavern_taper",         cavern_taper);
	settings->getFloatNoEx("mgarg_cavern_threshold",   cavern_threshold);
	settings->getS16NoEx("mgarg_dungeon_ymin",         dungeon_ymin);
	settings->getS16NoEx("mgarg_dungeon_ymax",         dungeon_ymax);

	settings->getNoiseParams("mgarg_np_web1",         np_web1);
	settings->getNoiseParams("mgarg_np_web2",         np_web2);
	settings->getNoiseParams("mgarg_np_web3",         np_web3);
	settings->getNoiseParams("mgarg_np_web4",         np_web4);
	settings->getNoiseParams("mgarg_np_filler_depth", np_filler_depth);
	settings->getNoiseParams("mgarg_np_cave1",        np_cave1);
	settings->getNoiseParams("mgarg_np_cave2",        np_cave2);
	settings->getNoiseParams("mgarg_np_cavern",       np_cavern);
	settings->getNoiseParams("mgarg_np_dungeons",     np_dungeons);
}

void MapgenARGParams::writeParams(Settings *settings) const
{
	settings->setFloat("mgarg_web_thickness",      web_thickness);
	settings->setFloat("mgarg_web_thickness_fine", web_thickness_fine);

	settings->setFloat("mgarg_cave_width",         cave_width);
	settings->setS16("mgarg_large_cave_depth",     large_cave_depth);
	settings->setU16("mgarg_small_cave_num_min",   small_cave_num_min);
	settings->setU16("mgarg_small_cave_num_max",   small_cave_num_max);
	settings->setU16("mgarg_large_cave_num_min",   large_cave_num_min);
	settings->setU16("mgarg_large_cave_num_max",   large_cave_num_max);
	settings->setFloat("mgarg_large_cave_flooded", large_cave_flooded);
	settings->setS16("mgarg_cavern_limit",         cavern_limit);
	settings->setS16("mgarg_cavern_taper",         cavern_taper);
	settings->setFloat("mgarg_cavern_threshold",   cavern_threshold);
	settings->setS16("mgarg_dungeon_ymin",         dungeon_ymin);
	settings->setS16("mgarg_dungeon_ymax",         dungeon_ymax);

	settings->setNoiseParams("mgarg_np_web1",         np_web1);
	settings->setNoiseParams("mgarg_np_web2",         np_web2);
	settings->setNoiseParams("mgarg_np_web3",         np_web3);
	settings->setNoiseParams("mgarg_np_web4",         np_web4);
	settings->setNoiseParams("mgarg_np_filler_depth", np_filler_depth);
	settings->setNoiseParams("mgarg_np_cave1",        np_cave1);
	settings->setNoiseParams("mgarg_np_cave2",        np_cave2);
	settings->setNoiseParams("mgarg_np_cavern",       np_cavern);
	settings->setNoiseParams("mgarg_np_dungeons",     np_dungeons);
}

void MapgenARGParams::setDefaultSettings(Settings *settings)
{
}

int MapgenARG::getSpawnLevelAtPoint(v2s16 p)
{
	s16 max_spawn_y = 120;
	for (s16 y = max_spawn_y; y >= -60; y--) {
		float n1 = NoiseFractal3D(&noise_web1->np, p.X, y, p.Y, seed);
		float n2 = NoiseFractal3D(&noise_web2->np, p.X, y, p.Y, seed + 1337);
		float dist_main = std::sqrt(n1 * n1 + n2 * n2);

		float n3 = NoiseFractal3D(&noise_web3->np, p.X, y, p.Y, seed + 2026);
		float n4 = NoiseFractal3D(&noise_web4->np, p.X, y, p.Y, seed + 4044);
		float dist_fine = std::sqrt(n3 * n3 + n4 * n4);

		if (dist_main < web_thickness || dist_fine < web_thickness_fine) {
			return y + 2;
		}
	}
	return MAX_MAP_GENERATION_LIMIT;
}

void MapgenARG::makeChunk(BlockMakeData *data)
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

int MapgenARG::generateTerrain()
{
	u32 index = 0;
	int stone_surface_max_y = -MAX_MAP_GENERATION_LIMIT;

	noise_web1->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);
	noise_web2->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);
	noise_web3->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);
	noise_web4->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);

	for (s16 z = node_min.Z; z <= node_max.Z; z++) {
		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
			u32 vi = vm->m_area.index(node_min.X, y, z);
			for (s16 x = node_min.X; x <= node_max.X; x++, vi++, index++) {
				if (vm->m_data[vi].getContent() != CONTENT_IGNORE)
					continue;

				float n1 = noise_web1->result[index];
				float n2 = noise_web2->result[index];
				float dist_main = std::sqrt(n1 * n1 + n2 * n2);

				float n3 = noise_web3->result[index];
				float n4 = noise_web4->result[index];
				float dist_fine = std::sqrt(n3 * n3 + n4 * n4);

				if (dist_main < web_thickness || dist_fine < web_thickness_fine) {
					vm->m_data[vi] = MapNode(c_stone);
					if (y > stone_surface_max_y)
						stone_surface_max_y = y;
				} else {
					vm->m_data[vi] = MapNode(CONTENT_AIR);
				}
			}
		}
	}

	return stone_surface_max_y;
}