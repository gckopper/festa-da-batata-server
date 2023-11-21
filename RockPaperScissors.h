#pragma once
#include "Room.h"

class RockPaperScissors :
    public Minigame
{
private:
	bool finished = false;
	std::array<Hand, MAX_PLAYERS> hands = { Hand::no };
	uint_fast8_t winner = MAX_PLAYERS;
public:
	void processMessage(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id) override;
	bool isFinished() override;
	uint_fast8_t getWinner() override;
};


