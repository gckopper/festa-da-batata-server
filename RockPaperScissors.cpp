#include "RockPaperScissors.h"

void RockPaperScissors::processMessage(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	if (!room->isInMinigame)
	{
		response->msg_id = e_not_in_minigame;
		return;
	}
	// ver se todos os jogadores ja jogaram
	// se sim, ver quem ganhou
	bool all_played = true;
	hands[player_id] = (Hand)request->optional;
	uint_fast8_t player_count = room->player_count;
	for (uint_fast8_t i = 0; i < player_count; i++)
	{
		if (hands[i] == Hand::no)
		{
			all_played = false;
			break;
		}
	}
	if (!all_played)
	{
		// responder com mensagem ok
		response->msg_id = r_ok;
		return;
	}
	Hand first = hands[0];
	uint_fast16_t score = 0;
	for (uint_fast8_t i = 1; i < player_count; i++) {
		score += (first - hands[i] + 3) % 3;
	}
	// responder com mensagem de resultado
	winner = (score > (player_count + player_count/2)) ? MAX_PLAYERS : 0;
}

bool RockPaperScissors::isFinished() 
{
	return RockPaperScissors::finished;
}

uint_fast8_t RockPaperScissors::getWinner()
{
	return winner;
}
