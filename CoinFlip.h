#pragma once
#include "Minigame.h"

class CoinFlip:
	public Minigame
{
private:
	bool finished = false;
	std::array<bool, MAX_PLAYERS> winner = { false };
	std::array<bool, MAX_PLAYERS> hasPlayed = { false };
	bool coin = std::rand() % 2;
public:
	void processMessage(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id) override;
	bool isFinished() override;
	std::array<bool, MAX_PLAYERS> getWinner() override;
};

