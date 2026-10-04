// Copyright (C) 2026 Voltual
// 本程序是自由软件：你可以根据自由软件基金会发布的 GNU 通用公共许可证第3版
//（或任意更新的版本）的条款重新分发和/或修改它。
//本程序是基于希望它有用而分发的，但没有任何担保；甚至没有适销性或特定用途适用性的隐含担保。
// 有关更多细节，请参阅 GNU 通用公共许可证。
//
// 你应该已经收到了一份 GNU 通用公共许可证的副本
// 如果没有，请查阅 <http://www.gnu.org/licenses/>.

#pragma once

#include "mapgen.h"

enum LayerType {
	LAYER_FARLANDS,
	LAYER_ARG,
	LAYER_BACKROOMS,
	LAYER_GLITCH,
	LAYER_SKYGRID,
	LAYER_COUNT
};

struct MapgenLayeredParams : public MapgenParams
{
	s16 chunk_group_size = 4;
	s16 overworld_y_min  = -64;
	s16 band1_y_max      = 300;
	s16 band2_y_max      = 1000;
	s16 band3_y_max      = 1800;
	s16 band4_y_max      = 3000;

	s16 grid_spacing = 4;

	float web_thickness      = 0.42f;
	float web_thickness_fine = 0.32f;

	s16 corridor_width = 24;
	s16 wall_thickness = 10;
	s16 shelf_spacing  = 16;
	s16 shelf_height   = 3;

	float cave_width = 0.09f;
	s16 large_cave_depth = -33;
	u16 small_cave_num_min = 0;
	u16 small_cave_num_max = 0;
	u16 large_cave_num_min = 0;
	u16 large_cave_num_max = 2;
	float large_cave_flooded = 0.5f;
	s16 cavern_limit = -256;
	s16 cavern_taper = 256;
	float cavern_threshold = 0.7f;
	s16 dungeon_ymin = -31000;
	s16 dungeon_ymax = 31000;

	NoiseParams np_far1;
	NoiseParams np_far2;
	NoiseParams np_far_select;
	NoiseParams np_web1;
	NoiseParams np_web2;
	NoiseParams np_web3;
	NoiseParams np_web4;
	NoiseParams np_wall_noise;
	NoiseParams np_shelf_noise;
	NoiseParams np_glitch_terrain_base;
	NoiseParams np_filler_depth;
	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_cavern;
	NoiseParams np_dungeons;

	MapgenLayeredParams();
	~MapgenLayeredParams() = default;

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
	void setDefaultSettings(Settings *settings);
};

class MapgenLayered : public MapgenBasic
{
public:
	MapgenLayered(MapgenLayeredParams *params, EmergeParams *emerge);
	~MapgenLayered();

	virtual MapgenType getType() const { return MAPGEN_LAYERED; }

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);
	s16 generateTerrain();

private:
	s16 chunk_group_size;
	s16 overworld_y_min;
	s16 band1_y_max;
	s16 band2_y_max;
	s16 band3_y_max;
	s16 band4_y_max;

	s16 grid_spacing;
	float web_thickness;
	float web_thickness_fine;

	s16 corridor_width;
	s16 wall_thickness;
	s16 shelf_spacing;
	s16 shelf_height;

	Noise *noise_far1 = nullptr;
	Noise *noise_far2 = nullptr;
	Noise *noise_far_select = nullptr;

	Noise *noise_web1 = nullptr;
	Noise *noise_web2 = nullptr;
	Noise *noise_web3 = nullptr;
	Noise *noise_web4 = nullptr;

	Noise *noise_wall_noise = nullptr;
	Noise *noise_shelf_noise = nullptr;

	NoiseParams np_glitch_terrain_base;

	std::vector<content_t> m_possible_contents;
	void initPossibleContents();
	void getGroupLayerOrder(s32 group_hash, LayerType layers_out[LAYER_COUNT]);
};