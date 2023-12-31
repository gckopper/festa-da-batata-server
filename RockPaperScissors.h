#pragma once
#include "Minigame.h"

class RockPaperScissors :
    public Minigame
{
private:
	bool finished = false;
	std::array<Hand, MAX_PLAYERS> hands = { Hand::no };
	std::array<bool, MAX_PLAYERS> winners = {false};
public:
	void processMessage(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id) override;
	bool isFinished() override;
	std::array<bool, MAX_PLAYERS> getWinner() override;
};


