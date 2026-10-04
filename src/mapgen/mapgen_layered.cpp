// Copyright (C) 2026 Voltual
// 本程序是自由软件：你可以根据自由软件基金会发布的 GNU 通用公共许可证第3版
//（或任意更新的版本）的条款重新分发和/或修改它。
//本程序是基于希望它有用而分发的，但没有任何担保；甚至没有适销性或特定用途适用性的隐含担保。
// 有关更多细节，请参阅 GNU 通用公共许可证。
//
// 你应该已经收到了一份 GNU 通用公共许可证的副本
// 如果没有，请查阅 <http://www.gnu.org/licenses/>.

#include "mapgen_layered.h"
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

MapgenLayered::MapgenLayered(MapgenLayeredParams *params, EmergeParams *emerge)
	: MapgenBasic(MAPGEN_LAYERED, params, emerge)
{
	spflags            = params->spflags;
	chunk_group_size   = params->chunk_group_size;
	overworld_y_min    = params->overworld_y_min;
	band1_y_max        = params->band1_y_max;
	band2_y_max        = params->band2_y_max;
	band3_y_max        = params->band3_y_max;
	band4_y_max        = params->band4_y_max;

	grid_spacing       = params->grid_spacing;
	web_thickness      = params->web_thickness;
	web_thickness_fine = params->web_thickness_fine;

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

	noise_far1       = new Noise(&params->np_far1, seed, csize.X, csize.Y + 2, csize.Z);
	noise_far2       = new Noise(&params->np_far2, seed + 101, csize.X, csize.Y + 2, csize.Z);
	noise_far_select = new Noise(&params->np_far_select, seed + 202, csize.X, csize.Y + 2, csize.Z);

	noise_web1 = new Noise(&params->np_web1, seed + 303, csize.X, csize.Y + 2, csize.Z);
	noise_web2 = new Noise(&params->np_web2, seed + 404, csize.X, csize.Y + 2, csize.Z);
	noise_web3 = new Noise(&params->np_web3, seed + 505, csize.X, csize.Y + 2, csize.Z);
	noise_web4 = new Noise(&params->np_web4, seed + 606, csize.X, csize.Y + 2, csize.Z);

	noise_wall_noise  = new Noise(&params->np_wall_noise, seed + 707, csize.X, csize.Z);
	noise_shelf_noise = new Noise(&params->np_shelf_noise, seed + 808, csize.X, csize.Y + 2, csize.Z);

	np_glitch_terrain_base = params->np_glitch_terrain_base;

	MapgenBasic::np_cave1    = params->np_cave1;
	MapgenBasic::np_cave2    = params->np_cave2;
	MapgenBasic::np_cavern   = params->np_cavern;
	MapgenBasic::np_dungeons = params->np_dungeons;
}

MapgenLayered::~MapgenLayered()
{
	delete noise_filler_depth;
	delete noise_far1;
	delete noise_far2;
	delete noise_far_select;
	delete noise_web1;
	delete noise_web2;
	delete noise_web3;
	delete noise_web4;
	delete noise_wall_noise;
	delete noise_shelf_noise;
}

MapgenLayeredParams::MapgenLayeredParams() :
	np_far1                (0.0, 30.0, v3f(120.0, 80.0, 120.0), 82341, 4, 0.55, 2.0),
	np_far2                (0.0, 20.0, v3f(60.0,  40.0, 60.0),  95039, 3, 0.50, 2.0),
	np_far_select          (0.0, 1.0,  v3f(200.0, 200.0, 200.0), 4213,  3, 0.5, 2.0),
	np_web1                (0.0, 1.0,  v3f(42.0, 42.0, 42.0),   983240, 4, 0.55, 2.0),
	np_web2                (0.0, 1.0,  v3f(42.0, 42.0, 42.0),   432109, 4, 0.55, 2.0),
	np_web3                (0.0, 1.0,  v3f(20.0, 20.0, 20.0),   876543, 3, 0.50, 2.0),
	np_web4                (0.0, 1.0,  v3f(20.0, 20.0, 20.0),   123456, 3, 0.50, 2.0),
	np_wall_noise          (0.0, 1.0,  v3f(120.0, 120.0, 120.0), 82341, 3, 0.5, 2.0),
	np_shelf_noise         (0.0, 1.0,  v3f(30.0,  30.0,  30.0),  95039, 3, 0.5, 2.0),
	np_glitch_terrain_base (4.0, 35.0, v3f(250.0, 250.0, 250.0), 82341, 5, 0.6, 2.0),
	np_filler_depth        (0.0, 1.2,  v3f(150.0, 150.0, 150.0), 261,   3, 0.7,  2.0),
	np_cave1               (0.0, 12.0, v3f(61.0, 61.0, 61.0),   52534,  3, 0.5,  2.0),
	np_cave2               (0.0, 12.0, v3f(67.0, 67.0, 67.0),   10325,  3, 0.5,  2.0),
	np_cavern              (0.0, 1.0,  v3f(384.0, 128.0, 384.0), 723,   5, 0.63, 2.0),
	np_dungeons            (0.9, 0.5,  v3f(500.0, 500.0, 500.0), 0,     2, 0.8,  2.0)
{
	chunk_group_size = 4;
	overworld_y_min  = -64;
	band1_y_max      = 300;
	band2_y_max      = 1000;
	band3_y_max      = 1800;
	band4_y_max      = 3000;
}

void MapgenLayeredParams::readParams(const Settings *settings)
{
	settings->getS16NoEx("mglayered_chunk_group_size", chunk_group_size);
	settings->getS16NoEx("mglayered_overworld_y_min",  overworld_y_min);
	settings->getS16NoEx("mglayered_band1_y_max",      band1_y_max);
	settings->getS16NoEx("mglayered_band2_y_max",      band2_y_max);
	settings->getS16NoEx("mglayered_band3_y_max",      band3_y_max);
	settings->getS16NoEx("mglayered_band4_y_max",      band4_y_max);
	settings->getS16NoEx("mglayered_grid_spacing",      grid_spacing);
	settings->getFloatNoEx("mglayered_web_thickness",      web_thickness);
	settings->getFloatNoEx("mglayered_web_thickness_fine", web_thickness_fine);

	settings->getNoiseParams("mglayered_np_far1",                 np_far1);
	settings->getNoiseParams("mglayered_np_far2",                 np_far2);
	settings->getNoiseParams("mglayered_np_far_select",           np_far_select);
	settings->getNoiseParams("mglayered_np_web1",                 np_web1);
	settings->getNoiseParams("mglayered_np_web2",                 np_web2);
	settings->getNoiseParams("mglayered_np_web3",                 np_web3);
	settings->getNoiseParams("mglayered_np_web4",                 np_web4);
	settings->getNoiseParams("mglayered_np_wall_noise",           np_wall_noise);
	settings->getNoiseParams("mglayered_np_shelf_noise",          np_shelf_noise);
	settings->getNoiseParams("mglayered_np_glitch_terrain_base", np_glitch_terrain_base);
	settings->getNoiseParams("mglayered_np_filler_depth",        np_filler_depth);
}

void MapgenLayeredParams::writeParams(Settings *settings) const
{
	settings->setS16("mglayered_chunk_group_size", chunk_group_size);
	settings->setS16("mglayered_overworld_y_min",  overworld_y_min);
	settings->setS16("mglayered_band1_y_max",      band1_y_max);
	settings->setS16("mglayered_band2_y_max",      band2_y_max);
	settings->setS16("mglayered_band3_y_max",      band3_y_max);
	settings->setS16("mglayered_band4_y_max",      band4_y_max);
	settings->setS16("mglayered_grid_spacing",      grid_spacing);
	settings->setFloat("mglayered_web_thickness",      web_thickness);
	settings->setFloat("mglayered_web_thickness_fine", web_thickness_fine);

	settings->setNoiseParams("mglayered_np_far1",                 np_far1);
	settings->setNoiseParams("mglayered_np_far2",                 np_far2);
	settings->setNoiseParams("mglayered_np_far_select",           np_far_select);
	settings->setNoiseParams("mglayered_np_web1",                 np_web1);
	settings->setNoiseParams("mglayered_np_web2",                 np_web2);
	settings->setNoiseParams("mglayered_np_web3",                 np_web3);
	settings->setNoiseParams("mglayered_np_web4",                 np_web4);
	settings->setNoiseParams("mglayered_np_wall_noise",           np_wall_noise);
	settings->setNoiseParams("mglayered_np_shelf_noise",          np_shelf_noise);
	settings->setNoiseParams("mglayered_np_glitch_terrain_base", np_glitch_terrain_base);
	settings->setNoiseParams("mglayered_np_filler_depth",        np_filler_depth);
}

void MapgenLayeredParams::setDefaultSettings(Settings *settings)
{
}

void MapgenLayered::initPossibleContents()
{
	if (!m_possible_contents.empty())
		return;

	u32 sz = ndef->size();
	for (content_t c = 0; c < sz; c++) {
		if (c == CONTENT_AIR || c == CONTENT_IGNORE || c == CONTENT_UNKNOWN)
			continue;

		const ContentFeatures &f = ndef->get(c);
		if (f.name.empty() || f.drawtype == NDT_AIRLIKE || f.drawtype == NDT_SIGNLIKE)
			continue;

		if (f.getGroup("not_in_creative_inventory") != 0 || f.name.find("sign") != std::string::npos)
			continue;

		m_possible_contents.push_back(c);
	}

	if (m_possible_contents.empty()) {
		m_possible_contents.push_back(c_stone);
	}
}

int MapgenLayered::getSpawnLevelAtPoint(v2s16 p)
{
	return 10;
}

void MapgenLayered::getGroupLayerOrder(s32 group_hash, LayerType layers_out[LAYER_COUNT])
{
	for (int i = 0; i < LAYER_COUNT; i++)
		layers_out[i] = (LayerType)i;

	PcgRandom pr(group_hash);
	for (int i = LAYER_COUNT - 1; i > 0; i--) {
		int j = pr.range(0, i);
		std::swap(layers_out[i], layers_out[j]);
	}
}

void MapgenLayered::makeChunk(BlockMakeData *data)
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

	// 仅主世界及高空层才运行生物群落与装饰，下界/末地区间完全放行
	if (node_max.Y >= overworld_y_min) {
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
	}

	updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);

	if (flags & MG_LIGHT) {
		calcLighting(node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0),
			full_node_min, full_node_max);
	}

	this->generating = false;
}

s16 MapgenLayered::generateTerrain()
{
	MapNode n_air(CONTENT_AIR);
	MapNode n_stone(c_stone);
	MapNode n_water(c_water_source);

	s16 stone_surface_max_y = node_min.Y;

	// 计算当前地图区块归属的 2D 区块组 (Chunk Group)
	s16 chunk_x = std::floor((float)node_min.X / csize.X);
	s16 chunk_z = std::floor((float)node_min.Z / csize.Z);
	s16 group_x = std::floor((float)chunk_x / chunk_group_size);
	s16 group_z = std::floor((float)chunk_z / chunk_group_size);

	s32 group_hash = (s32)getBlockSeed2(v3s16(group_x, 0, group_z), seed);

	// 针对当前区块组随机洗牌 5 层千层饼的垂直堆叠顺序
	LayerType layer_order[LAYER_COUNT];
	getGroupLayerOrder(group_hash, layer_order);

	// 检查当前 Chunk 中参与的图层类型，只计算必要的 Noise
	bool has_farlands  = false;
	bool has_arg       = false;
	bool has_backrooms = false;
	bool has_glitch    = false;

	if (node_max.Y >= overworld_y_min) {
		for (s16 y = node_min.Y; y <= node_max.Y; y += MAP_BLOCKSIZE) {
			LayerType ly;
			if (y < band1_y_max)       ly = layer_order[0];
			else if (y < band2_y_max) ly = layer_order[1];
			else if (y < band3_y_max) ly = layer_order[2];
			else if (y < band4_y_max) ly = layer_order[3];
			else                      ly = layer_order[4];

			if (ly == LAYER_FARLANDS)  has_farlands = true;
			if (ly == LAYER_ARG)       has_arg = true;
			if (ly == LAYER_BACKROOMS) has_backrooms = true;
			if (ly == LAYER_GLITCH)    has_glitch = true;
		}
	}

	if (has_farlands) {
		noise_far1->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);
		noise_far2->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);
		noise_far_select->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);
	}

	if (has_arg) {
		noise_web1->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);
		noise_web2->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);
		noise_web3->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);
		noise_web4->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);
	}

	if (has_backrooms) {
		noise_wall_noise->noiseMap2D(node_min.X, node_min.Z);
		noise_shelf_noise->noiseMap3D(node_min.X, node_min.Y - 1, node_min.Z);
	}

	std::unique_ptr<Noise> noise_glitch_terrain;
	if (has_glitch) {
		v3s16 chunk_pos(chunk_x, node_min.Y / csize.Y, chunk_z);
		s32 chunk_seed = (chunk_pos.X == 0 && chunk_pos.Y == 0 && chunk_pos.Z == 0) ?
			seed : (s32)getBlockSeed2(chunk_pos, seed);
		noise_glitch_terrain = std::make_unique<Noise>(&np_glitch_terrain_base, chunk_seed, csize.X, csize.Z);
		noise_glitch_terrain->noiseMap2D(node_min.X, node_min.Z);
	}

	u32 index = 0;
	s16 period = corridor_width + wall_thickness;
	const v3s32 &em = vm->m_area.getExtent();

	for (s16 z = node_min.Z; z <= node_max.Z; z++) {
		s16 mod_z = mymod(z, period);
		bool is_wall_z = (mod_z < wall_thickness);
		bool is_near_wall_z = (mod_z < wall_thickness + 4 || mod_z > period - 4);

		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
			u32 vi = vm->m_area.index(node_min.X, y, z);

			// 【完全放行下界与末地】Y < overworld_y_min (-64)
			if (y < overworld_y_min) {
				continue;
			}

			// 确定当前高度 $Y$ 对应当前区块组洗牌后的奇观图层类型
			LayerType cur_layer;
			if (y < band1_y_max)       cur_layer = layer_order[0];
			else if (y < band2_y_max) cur_layer = layer_order[1];
			else if (y < band3_y_max) cur_layer = layer_order[2];
			else if (y < band4_y_max) cur_layer = layer_order[3];
			else                      cur_layer = layer_order[4];

			s16 mod_y = mymod(y, shelf_spacing);
			bool is_shelf_layer = (mod_y < shelf_height);

			for (s16 x = node_min.X; x <= node_max.X; x++, vi++, index++) {
				if (vm->m_data[vi].getContent() != CONTENT_IGNORE)
					continue;

				switch (cur_layer) {
				case LAYER_SKYGRID: {
					if ((x % grid_spacing == 0) && (y % grid_spacing == 0) && (z % grid_spacing == 0)) {
						u32 rand_val = getBlockSeed2(v3s16(x, y, z), seed);
						content_t selected_c = m_possible_contents[rand_val % m_possible_contents.size()];
						vm->m_data[vi] = MapNode(selected_c);
						if (y > stone_surface_max_y)
							stone_surface_max_y = y;
					} else {
						vm->m_data[vi] = n_air;
					}
					break;
				}

				case LAYER_GLITCH: {
					u32 index2d = (z - node_min.Z) * csize.X + (x - node_min.X);
					s16 surface_y = (s16)noise_glitch_terrain->result[index2d];
					if (y <= surface_y) {
						vm->m_data[vi] = n_stone;
						if (y > stone_surface_max_y)
							stone_surface_max_y = y;
					} else {
						vm->m_data[vi] = n_air;
					}
					break;
				}

				case LAYER_BACKROOMS: {
					s16 mod_x = mymod(x, period);
					bool is_wall_x = (mod_x < wall_thickness);
					bool is_near_wall_x = (mod_x < wall_thickness + 4 || mod_x > period - 4);

					u32 index2d = (z - node_min.Z) * csize.X + (x - node_min.X);
					float wall_n = noise_wall_noise->result[index2d];
					float shelf_n = noise_shelf_noise->result[index];

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
					} else {
						vm->m_data[vi] = n_air;
					}
					break;
				}

				case LAYER_ARG: {
					float w1 = noise_web1->result[index];
					float w2 = noise_web2->result[index];
					float dist_main = std::sqrt(w1 * w1 + w2 * w2);

					float w3 = noise_web3->result[index];
					float w4 = noise_web4->result[index];
					float dist_fine = std::sqrt(w3 * w3 + w4 * w4);

					if (dist_main < web_thickness || dist_fine < web_thickness_fine) {
						vm->m_data[vi] = n_stone;
						if (y > stone_surface_max_y)
							stone_surface_max_y = y;
					} else {
						vm->m_data[vi] = n_air;
					}
					break;
				}

				case LAYER_FARLANDS:
				default: {
					s16 mod_y_far = mymod(y, 8);
					float terrace_boost = (mod_y_far < 2) ? 0.25f : 0.0f;

					float n1       = noise_far1->result[index];
					float n2       = noise_far2->result[index];
					float n_select = rangelim(noise_far_select->result[index], 0.0f, 1.0f);

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
					break;
				}
				}
			}
		}
	}

	if (stone_surface_max_y < node_min.Y)
		stone_surface_max_y = node_min.Y;

	return stone_surface_max_y;
}