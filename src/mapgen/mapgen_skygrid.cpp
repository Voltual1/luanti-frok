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
#include "util/numeric.h"

MapgenSkygrid::MapgenSkygrid(MapgenSkygridParams *params, EmergeParams *emerge)
	: Mapgen(MAPGEN_SKYGRID, params, emerge)
{
	grid_spacing = params->grid_spacing;
	if (grid_spacing < 2)
		grid_spacing = 2;
}

MapgenSkygridParams::MapgenSkygridParams()
{
}

void MapgenSkygridParams::readParams(const Settings *settings)
{
	settings->getS16NoEx("mgskygrid_grid_spacing", grid_spacing);
}

void MapgenSkygridParams::writeParams(Settings *settings) const
{
	settings->setS16("mgskygrid_grid_spacing", grid_spacing);
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
	v3s16 node_min = blockpos_min * MAP_BLOCKSIZE;
	v3s16 node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	blockseed = getBlockSeed2(node_min, seed);

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 y = node_min.Y; y <= node_max.Y; y++) {
		u32 vi = vm->m_area.index(node_min.X, y, z);
		for (s16 x = node_min.X; x <= node_max.X; x++, vi++) {
			if (vm->m_data[vi].getContent() != CONTENT_IGNORE)
				continue;

			if ((x % grid_spacing == 0) && (y % grid_spacing == 0) && (z % grid_spacing == 0)) {
				u32 rand_val = getBlockSeed2(v3s16(x, y, z), seed);
				content_t selected_c = m_possible_contents[rand_val % m_possible_contents.size()];
				vm->m_data[vi] = MapNode(selected_c);
			} else {
				vm->m_data[vi] = MapNode(CONTENT_AIR);
			}
		}
	}

	updateLiquid(&data->transforming_liquid, node_min, node_max);

	if (flags & MG_LIGHT) {
		calcLighting(node_min, node_max, node_min, node_max);
	}

	this->generating = false;
}