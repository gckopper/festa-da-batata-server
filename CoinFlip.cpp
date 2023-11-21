#include "CoinFlip.h"

void CoinFlip::processMessage(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	bool op = request->optional;
	if (op == coin)
	{
		winner[player_id] = true;
	}
	hasPlayed[player_id] = true;
	bool allPlayed = true;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (!hasPlayed[i])
		{
			allPlayed = false;
			break;
		}
	}
	if (allPlayed)
	{
		finished = true;
	}
}

bool CoinFlip::isFinished()
{
	return finished;
}

std::array<bool, MAX_PLAYERS> CoinFlip::getWinner()
{
	return winner;
}
