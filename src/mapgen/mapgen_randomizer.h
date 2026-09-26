// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 Voltual
// Inspired by Randomizer Mod: https://content.luanti.org/packages/NO11/randomizer/

#pragma once

#include "mapgen_v7.h"

struct MapgenRandomizerParams : public MapgenV7Params {
	MapgenRandomizerParams() = default;
	~MapgenRandomizerParams() = default;

	void readParams(const Settings *settings) override;
	void writeParams(Settings *settings) const override;
	void setDefaultSettings(Settings *settings) override;
};

class MapgenRandomizer : public MapgenV7 {
public:
	MapgenRandomizer(MapgenRandomizerParams *params, EmergeParams *emerge);
	~MapgenRandomizer() override = default;

	MapgenType getType() const override { return MAPGEN_RANDOMIZER; }

	void makeChunk(BlockMakeData *data) override;

private:
	void randomizeNodes();
};