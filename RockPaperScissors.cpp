#include "RockPaperScissors.h"

void RockPaperScissors::processMessage(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	// ver se todos os jogadores ja jogaram
	// se sim, ver quem ganhou
	bool all_played = true;
	uint_fast8_t op = request->optional;
	if (op > 2) {
		response->msg_id = e_invalid_action;
		return;
	}
	hands[player_id] = (Hand)op;
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
	uint_fast16_t score = 1;
	for (uint_fast8_t i = 1; i < player_count; i++) {
		uint_fast8_t indv_score = (first - hands[i]) % 3;
		score += indv_score;
		winners[i] = indv_score == 2;
	}
	winners[0] = (score < (player_count + player_count / 2));
	if (winners[0])
	{
		// todos os outros jogadores perderam
		for (uint_fast8_t i = 1; i < player_count; i++)
		{
			winners[i] = false;
		}
	}
}

bool RockPaperScissors::isFinished() 
{
	return RockPaperScissors::finished;
}

std::array<bool, MAX_PLAYERS> RockPaperScissors::getWinner()
{
	return winners;
}
