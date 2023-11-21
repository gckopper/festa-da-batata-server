#pragma once
#include "Minigame.h"
#include "RockPaperScissors.h"
class MinigamesFactory
{
public:
	static std::unique_ptr<Minigame> createMinigame(MinigamesEnum);
};

