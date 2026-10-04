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

struct MapgenBackroomsParams : public MapgenParams
{
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

	NoiseParams np_wall_noise;
	NoiseParams np_shelf_noise;
	NoiseParams np_filler_depth;
	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_cavern;
	NoiseParams np_dungeons;

	MapgenBackroomsParams();
	~MapgenBackroomsParams() = default;

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
	void setDefaultSettings(Settings *settings);
};

class MapgenBackrooms : public MapgenBasic
{
public:
	MapgenBackrooms(MapgenBackroomsParams *params, EmergeParams *emerge);
	~MapgenBackrooms();

	virtual MapgenType getType() const { return MAPGEN_BACKROOMS; }

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);
	s16 generateTerrain();

private:
	s16 corridor_width;
	s16 wall_thickness;
	s16 shelf_spacing;
	s16 shelf_height;

	Noise *noise_wall_noise = nullptr;
	Noise *noise_shelf_noise = nullptr;
};