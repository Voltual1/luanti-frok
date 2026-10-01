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

struct MapgenSkygridParams : public MapgenParams
{
	s16 grid_spacing = 4;

	MapgenSkygridParams();
	~MapgenSkygridParams() = default;

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
	void setDefaultSettings(Settings *settings);
};

class MapgenSkygrid : public Mapgen
{
public:
	MapgenSkygrid(MapgenSkygridParams *params, EmergeParams *emerge);
	~MapgenSkygrid() = default;

	virtual MapgenType getType() const { return MAPGEN_SKYGRID; }

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);

private:
	s16 grid_spacing;
	std::vector<content_t> m_possible_contents;

	void initPossibleContents();
};