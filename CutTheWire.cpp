#include "CutTheWire.h"

void CutTheWire::processMessage(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
    if (player_id != whose_turn) {
        response->msg_id = e_not_your_turn;
		return;
    }
    // pega o fio que o jogador cortou
    uint_fast8_t wire = request->optional;
    if (wire > cut.size()) {
		response->msg_id = e_invalid_action;
		return;
	}
    // verifica se o fio ja foi cortado
    if (cut[wire]) {
		response->msg_id = e_invalid_action;
        return;
    }
    // corta o fio
    cut[wire] = true;
    // verifica se o fio cortado era o certo
    if (wire == bomb) {
		// o jogador perdeu
		winner[player_id] = false;
		finished = true;
		return;
	}
	// passa a vez para o proximo jogador
	whose_turn = (whose_turn + 1) % room->player_count;
	// responde com mensagem ok
	response->msg_id = r_ok;

}

bool CutTheWire::isFinished()
{
    return finished;
}

std::array<bool, MAX_PLAYERS> CutTheWire::getWinner()
{
    return winner;
}
