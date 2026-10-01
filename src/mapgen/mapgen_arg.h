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

struct MapgenARGParams : public MapgenParams
{
	float web_thickness = 0.42f;
	float web_thickness_fine = 0.32f;
	s16 dungeon_ymin = -31000;
	s16 dungeon_ymax = 31000;

	NoiseParams np_web1;
	NoiseParams np_web2;
	NoiseParams np_web3;
	NoiseParams np_web4;
	NoiseParams np_filler_depth;
	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_cavern;
	NoiseParams np_dungeons;

	MapgenARGParams();
	~MapgenARGParams() = default;

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
	void setDefaultSettings(Settings *settings);
};

class MapgenARG : public MapgenBasic
{
public:
	MapgenARG(MapgenARGParams *params, EmergeParams *emerge);
	~MapgenARG();

	virtual MapgenType getType() const { return MAPGEN_ARG; }

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);
	int generateTerrain();

private:
	float web_thickness;
	float web_thickness_fine;
	Noise *noise_web1 = nullptr;
	Noise *noise_web2 = nullptr;
	Noise *noise_web3 = nullptr;
	Noise *noise_web4 = nullptr;
};