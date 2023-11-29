#pragma once
#include "Minigame.h"
#include "RockPaperScissors.h"
#include "CutTheWire.h"
#include "CoinFlip.h"
class MinigamesFactory
{
public:
	static std::unique_ptr<Minigame> createMinigame(MinigamesEnum);
};

