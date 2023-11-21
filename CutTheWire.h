#pragma once
#include "Minigame.h"

class CutTheWire :
    public Minigame
{
	private:
		bool finished = false;
		std::array<bool, MAX_PLAYERS> winner = { true };
		std::array<bool, MAX_PLAYERS+1> cut = { false };
		uint_fast8_t whose_turn = 0;
		uint_fast8_t bomb = std::rand() % (MAX_PLAYERS + 1);
	public:
	void processMessage(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id) override;
	bool isFinished() override;
	std::array<bool, MAX_PLAYERS> getWinner() override;
};

