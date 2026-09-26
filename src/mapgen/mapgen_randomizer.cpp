// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 Voltual
// Inspired by Randomizer Mod: https://content.luanti.org/packages/NO11/randomizer/

#include "mapgen_randomizer.h"
#include "emerge.h"
#include "mapnode.h"
#include "nodedef.h"
#include "voxel.h"
#include "util/numeric.h"
#include <vector>

void MapgenRandomizerParams::readParams(const Settings *settings)
{
	MapgenV7Params::readParams(settings);
}

void MapgenRandomizerParams::writeParams(Settings *settings) const
{
	MapgenV7Params::writeParams(settings);
}

void MapgenRandomizerParams::setDefaultSettings(Settings *settings)
{
	MapgenV7Params::setDefaultSettings(settings);
}

MapgenRandomizer::MapgenRandomizer(MapgenRandomizerParams *params, EmergeParams *emerge)
	: MapgenV7(params, emerge)
{
}

void MapgenRandomizer::randomizeNodes()
{
	if (!vm || !ndef)
		return;

	std::vector<content_t> replace_candidates;
	const u32 num_nodes = ndef->size();
	for (content_t id = 0; id < num_nodes; ++id) {
		const ContentFeatures &f = ndef->get(id);
		if (f.name.empty() || f.name == "ignore" || f.name == "air" || f.name == "unknown")
			continue;

		// 排除非普通画法、落下方块、隐藏方块、告示牌以及需要构建回调/元数据的方块
		if (f.drawtype == NDT_NORMAL &&
				f.getGroup("not_in_creative_inventory") != 1 &&
				f.getGroup("falling_node") != 1 &&
				f.getGroup("sign") != 1 &&
				!f.has_on_construct) {
			replace_candidates.push_back(id);
		}
	}

	if (replace_candidates.empty())
		return;

	for (s16 z = node_min.Z; z <= node_max.Z; z++) {
		for (s16 y = node_min.Y; y <= node_max.Y; y++) {
			u32 vi = vm->m_area.index(node_min.X, y, z);
			for (s16 x = node_min.X; x <= node_max.X; x++) {
				content_t c = vm->m_data[vi].getContent();
				if (c != CONTENT_AIR && c != CONTENT_IGNORE && c != CONTENT_UNKNOWN) {
					const ContentFeatures &f = ndef->get(c);
					// 仅随机化天然地形方块（is_ground_content），避开告示牌、基岩及建筑结构
					if (f.is_ground_content &&
							f.drawtype == NDT_NORMAL &&
							f.getGroup("bedrock") != 1 &&
							f.getGroup("sign") != 1 &&
							f.drawtype != NDT_LIQUID &&
							f.drawtype != NDT_FLOWINGLIQUID) {
						u32 rng = getBlockSeed2(v3s16(x, y, z), seed);
						content_t new_c = replace_candidates[rng % replace_candidates.size()];
						vm->m_data[vi].setContent(new_c);
					}
				}
				vi++;
			}
		}
	}
}

void MapgenRandomizer::makeChunk(BlockMakeData *data)
{
	MapgenV7::makeChunk(data);

	this->generating = true;
	randomizeNodes();
	this->generating = false;
}