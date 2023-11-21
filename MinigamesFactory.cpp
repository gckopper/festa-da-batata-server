#include "MinigamesFactory.h"

std::unique_ptr<Minigame> MinigamesFactory::createMinigame(MinigamesEnum e)
{
	switch (e)
	{
	case rock_paper_scissors:
		return std::make_unique<RockPaperScissors>();
		break;
	case cut_the_wire:
		return std::make_unique<CutTheWire>();
		break;
	case coin_flip:
		break;
	default:
		break;
	}
	return nullptr;
}
