#include "Game.h"

void MessageHandler::handleClient(SOCKET clientSocket)
{
	int iResult;
	unsigned char recvbuf[DEFAULT_BUFLEN];
	iResult = recv(clientSocket, (char*)recvbuf, DEFAULT_BUFLEN, 0);
	if (iResult > 0) {
		// processa todas as mensagens recebidas
		while (iResult >= SMALL_BUFLEN)
		{
			auto size = SMALL_BUFLEN;
			if (recvbuf[ROOM_CODE_SIZE] > 127 && iResult >= BIG_BUFLEN) {
				size = BIG_BUFLEN;
			}
			std::cout << "Tamanho da mensagem: " << size << std::endl;
			auto msg = std::make_unique<std::vector<unsigned char>>(recvbuf, recvbuf + size);
			std::unique_ptr<ClientMessage> decoded_msg = MessageHandler::decodeMessage(std::move(msg), clientSocket);
			auto response = std::make_unique<ServerMessage>();
			processMessage(std::move(decoded_msg), response);
			std::unique_ptr<std::vector<unsigned char>> encoded_msg = encodeMessage(std::move(response));
			int sendResult = send(clientSocket, (char*)encoded_msg->data(), (int)encoded_msg->size(), 0);
			if (sendResult == SOCKET_ERROR) {
				printf("Erro ao enviar mensagem: %d\n", WSAGetLastError());
				closesocket(clientSocket);
			}
			iResult -= size;
		}
		if (iResult != 0) {
			std::cout << "Mensagem com tamanho invalido de: " << iResult << " o resto sera descartado" << std::endl;
			auto response = std::make_unique<ServerMessage>();
			response->msg_id = e_decode;
			std::unique_ptr<std::vector<unsigned char>> encoded_msg = encodeMessage(std::move(response));
			int sendResult = send(clientSocket, (char*)encoded_msg->data(), (int)encoded_msg->size(), 0);
		}

	}
	else if (iResult == 0) {
		printf("Conex�o fechada pelo cliente\n");
		closesocket(clientSocket);
	}
	else {
		printf("Erro ao receber mensagem: %d\n", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		return;
	}
}

std::unique_ptr<ClientMessage> MessageHandler::decodeMessage(std::unique_ptr<std::vector<unsigned char>> msg, SOCKET creator)
{
	auto decoded_msg = std::make_unique<ClientMessage>();
	std::memcpy(decoded_msg->room_code, msg->data(), ROOM_CODE_SIZE);
	decoded_msg->msg_id = (message_id)msg->at(ROOM_CODE_SIZE);
	decoded_msg->creator = creator;
	if (msg->size() == 10){
		decoded_msg->optional = msg->at(ROOM_CODE_SIZE + 1);
	}
	return decoded_msg;
}

std::unique_ptr<std::vector<unsigned char>> MessageHandler::encodeMessage(std::unique_ptr<ServerMessage> msg)
{
	uint_fast8_t size = msg->data_size;
	std::cout << "tamanho da resposta: " << size + 0 << std::endl;
	auto res = std::make_unique<std::vector<unsigned char>>(size + 2);
	auto src = (unsigned char*)(res->data());
	std::memcpy(src, &msg->msg_id, 1);
	src[1] = size;
	if (size == 0)
	{
		return res;
	}
	std::memcpy(src + 2, &msg->data, size);
	std::free(msg->data);
	return res;
}

void MessageHandler::processMessage(std::unique_ptr<ClientMessage> msg, std::unique_ptr<ServerMessage>& response)
{
	std::shared_ptr<Room> room;
	auto room_code = std::string((char*)msg->room_code, ROOM_CODE_SIZE);
	auto search = rooms.find(room_code);
	std::cout << "Codigo da sala: " << room_code << std::endl;
	std::cout << "id da mensagem: " << msg->msg_id << std::endl;
	if (search == rooms.end()) {
		// se a sala nao existe, cria uma nova
		if (msg->msg_id != join) {
			std::cout << "Sala nao existe" << std::endl;
			response->msg_id = e_room_not_found;
			return;
		}
		room = std::make_shared<Room>();
		room->round = 0;
		room->players[0] = msg->creator;
		room->player_count = 1;
		room->name = std::string((char*)msg->room_code, ROOM_CODE_SIZE);
		for (uint_fast8_t i = 1; i < MAX_PLAYERS; i++)
		{
			room->players[i] = INVALID_SOCKET;
		}
		rooms.insert({ std::move(room_code), room });
		response->msg_id = r_ok;
		return;
	}
	room = search->second;

	std::cout << "Id da mensagem: " << msg->msg_id << std::endl;
	uint_fast8_t player_index = MAX_PLAYERS;
	for (uint_fast8_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (room->players[i] == msg->creator) {
			player_index = i;
			break;
		}
	}
	if (player_index == MAX_PLAYERS && msg->msg_id != join)
	{
		response->msg_id = e_player_not_in_room;
		return;
	}
	switch (msg->msg_id)
	{
	case ready:
		std::cout << "ready" << std::endl;
		MessageHandler::processReady(std::move(msg), response, room, player_index);
		break;
	case unready:
		std::cout << "unready" << std::endl;
		MessageHandler::processUnready(std::move(msg), response, room, player_index);
		break;
	case roll_dice:
		std::cout << "roll_dice" << std::endl;
		break;
	case buy_batata:
		std::cout << "buy_batata" << std::endl;
		break;
	case move:
		std::cout << "move" << std::endl;
		MessageHandler::processMove(std::move(msg), response, room, player_index);
		break;
	case emote:
		std::cout << "emote" << std::endl;
		MessageHandler::processEmote(std::move(msg), response, room, player_index);
		break;
	case join:
		std::cout << "join" << std::endl;
		MessageHandler::processJoin(std::move(msg), response, room, player_index);
		return;
	case minigame:
		std::cout << "minigame" << std::endl;
		if (!room->minigame) 	{
			response->msg_id = e_invalid_action;
			return;
		}
		room->minigame->processMessage(std::move(msg), response, room, player_index);
		if (room->minigame->isFinished())
		{
			room->isInMinigame = false;
			room->minigame.reset();
			room->whose_turn = 0;
			room->player_remaining_steps = 0;
			auto turn = std::make_unique<ServerMessage>();
			turn->msg_id = n_turn_start;
			MessageHandler::notifyPlayer(room, std::move(turn), room->whose_turn);
			uint_fast8_t winner = room->minigame->getWinner();
			auto end = std::make_unique<ServerMessage>();
			end->msg_id = n_minigame_end;
			end->data = (unsigned char*)std::calloc(1, sizeof(char));
			end->data_size = 1;
			end->data[0] = winner;
			if (winner != MAX_PLAYERS)
			{
				for (uint_fast8_t i = 1; i < room->player_count; i++)
				{
					room->coins[i] += PRIZE_MINIGAME;
				}
			}
			else {
				room->coins[winner] += PRIZE_MINIGAME;
			}
			notifyRoom(room, std::move(end), MAX_PLAYERS);
		}
		break;
	default:
		std::cout << "Mensagem invalida" << std::endl;
		response->msg_id = e_decode;
		return;
	}
}

void MessageHandler::notifyRoom(std::shared_ptr<Room> room, std::unique_ptr<ServerMessage> msg, uint_fast8_t player_id)
{
	std::unique_ptr<std::vector<unsigned char>> encoded_msg = encodeMessage(std::move(msg));
	bool got_error = false;
	for (uint_fast8_t i = 0; i < MAX_PLAYERS; i++)
	{
		SOCKET sock = room->players[i];
		if (sock == INVALID_SOCKET || i == player_id)
		{
			continue;
		}
		int sendResult = send(sock, (char*)encoded_msg->data(), (int)encoded_msg->size(), 0);
		if (sendResult == SOCKET_ERROR) {
			printf("Erro ao enviar mensagem: %d\n", WSAGetLastError());
			got_error = true;
			closesocket(sock);
			room->players[i] = INVALID_SOCKET;
			continue;
		}
	}
	if (got_error)
	{
		MessageHandler::endGame(room);
	}
}

void MessageHandler::notifyPlayer(std::shared_ptr<Room> room, std::unique_ptr<ServerMessage> msg, uint_fast8_t player_id)
{
	std::unique_ptr<std::vector<unsigned char>> encoded_msg = encodeMessage(std::move(msg));
	SOCKET sock = room->players[player_id];
	int sendResult = send(sock, (char*)encoded_msg->data(), (int)encoded_msg->size(), 0);
	if (sendResult == SOCKET_ERROR) {
		printf("Erro ao enviar mensagem: %d\n", WSAGetLastError());
		room->players[player_id] = INVALID_SOCKET;
		closesocket(sock);
		MessageHandler::endGame(room);
	}
}

void MessageHandler::processReady(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	// player ja esta pronto
	if (room->ready[player_id] || room->round != 0)
	{
		response->msg_id = e_invalid_action;
		return;
	}
	// marca o player como pronto
	room->ready[player_id] = true;
	// verifica se todos estao prontos
	for (uint_fast8_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (!room->ready[i])
		{
			response->msg_id = r_ok;
			return;
		}
	}
	// todos estao prontos, comeca o jogo
	bool everyone_ready = true;
	for (uint_fast8_t i = 0; i < room->player_count; i++)
	{
		if (!room->ready[i])
		{
			everyone_ready = false;
			break;
		}
	}
	response->msg_id = r_ok;
	if (everyone_ready)
	{
		MessageHandler::startGame(room);
	}
	return;
}

void MessageHandler::processUnready(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	// player nao esta pronto
	if (!room->ready[player_id] || room->round != 0)
	{
		response->msg_id = e_invalid_action;
		return;
	}
	// marca o player como nao pronto
	room->ready[player_id] = false;
	response->msg_id = r_ok;
	return;
}

void MessageHandler::processEmote(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	// notifica a sala
	auto emote = std::make_unique<ServerMessage>();
	emote->msg_id = n_emote;
	emote->data = (unsigned char*)std::calloc(1, sizeof(char));
	emote->data_size = 1;
	emote->data[0] = request->optional;
	notifyRoom(room, std::move(emote), player_id);
	response->msg_id = r_ok;
	return;
}

void MessageHandler::processJoin(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	// player ja esta na sala
	if (player_id != MAX_PLAYERS)
	{
		response->msg_id = e_invalid_action;
		return;
	}
	// sala cheia
	if (room->player_count == MAX_PLAYERS)
	{
		response->msg_id = e_room_full;
		return;
	}
	// jogo ja comecou
	if (room->round != 0)
	{
		response->msg_id = e_game_already_started;
		return;
	}
	// adiciona o player na sala
	room->players[room->player_count] = request->creator;
	room->player_count++;
	room->name = std::string((char*)request->room_code, ROOM_CODE_SIZE);
	response->msg_id = r_ok;
	// notifica a sala
	auto join = std::make_unique<ServerMessage>();
	join->msg_id = n_join;
	join->data = (unsigned char*)std::calloc(1, sizeof(char));
	join->data_size = 1;
	join->data[0] = player_id;
	notifyRoom(room, std::move(join), player_id);
	return;
}

void MessageHandler::processMove(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	// verifica se o jogo come�ou
	if (room->round == 0)
	{
		response->msg_id = e_game_not_started;
		return;
	}
	// verifica se e a vez do player
	if (room->whose_turn != player_id)
	{
		response->msg_id = e_not_your_turn;
		return;
	}
	uint_fast8_t r = room->player_remaining_steps;
	// verifica se o player ainda tem passos
	if (r == 0)
	{
		response->msg_id = e_invalid_action;
		return;
	}
	Direction d = (Direction)request->optional;
	if (board.intersectionDirection(room->position[player_id], room->direction[player_id]) != d)
	{
		response->msg_id = e_invalid_action;
		return;
	}
	response->data = (unsigned char*)std::calloc(2, sizeof(char));
	response->data_size = 2;
	response->msg_id = r_move;

	MessageHandler::executeMovements(std::move(request), response, room, player_id);

	return;
}

void MessageHandler::processRollDice(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	// verifica se o jogo come�ou
	if (room->round == 0)
	{
		response->msg_id = e_game_not_started;
		return;
	}
	// verifica se e a vez do player
	if (room->whose_turn != player_id)
	{
		response->msg_id = e_not_your_turn;
		return;
	}
	// verifica se o player ainda tem passos
	if (room->player_remaining_steps != 0)
	{
		response->msg_id = e_invalid_action;
		return;
	}
	// rola o dado
	room->player_remaining_steps = std::rand() % 8 + 1;
	response->data = (unsigned char*)std::calloc(3, sizeof(char));
	response->data_size = 3;
	response->data[2] = room->player_remaining_steps;
	response->msg_id = r_roll_dice;

	MessageHandler::executeMovements(std::move(request), response, room, player_id);
	return;
}

void MessageHandler::executeMovements(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	uint_fast8_t r = room->player_remaining_steps;
	Odio odio = board.executeMove(room->position[player_id], r, room->direction[player_id], room->coins[player_id] > 20);
	room->direction[player_id] = odio.direction;
	room->player_remaining_steps = odio.remaining;
	uint_fast8_t pos = odio.x + odio.y * BOARD_WIDTH;
	room->position[player_id] = pos;
	
	response->data[0] = pos;
	// calcula quantas moedas o player pegou
	uint_fast8_t coins = 0;
	for (uint_fast8_t i = 0; i < r; i++)
	{
		coins += std::rand() % 3;
	}
	response->data[1] = coins;
	// notifica a sala
	auto move = std::make_unique<ServerMessage>();
	move->msg_id = n_move;
	move->data = (unsigned char*)std::calloc(2, sizeof(char));
	move->data_size = 2;
	move->data[0] = pos;
	move->data[1] = coins;
	notifyRoom(room, std::move(move), player_id);
	// checa se o player pode comprar batata
	if (odio.canBuy)
	{
		response->msg_id = r_can_buy_batata;
		return;
	}
	if (odio.isIntersection)
	{
		response->msg_id = r_roll_dice_intersect;
		return;
	}
	// passa a vez
	room->whose_turn = (room->whose_turn + 1) % room->player_count;
	if (room->whose_turn == room->player_count)
	{
		room->round++;
		room->minigame = MinigamesFactory::createMinigame((MinigamesEnum)((std::rand() % 3) + 1));
		room->isInMinigame = true;
	}
	room->player_remaining_steps = 0;
	// notifica o proximo player
	auto turn = std::make_unique<ServerMessage>();
	turn->msg_id = n_turn_start;
	MessageHandler::notifyPlayer(room, std::move(turn), room->whose_turn);
}

void MessageHandler::processBuyBatata(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	// verifica se o jogo come�ou
	if (room->round == 0)
	{
		response->msg_id = e_game_not_started;
		return;
	}
	// verifica se e a vez do player
	if (room->whose_turn != player_id)
	{
		response->msg_id = e_not_your_turn;
		return;
	}
	// verifica se o player est� na posicao certa
	if (!board.isBatata(room->position[player_id]))
	{
		response->msg_id = e_invalid_action;
		return;
	}
	// verifica se o player tem moedas suficientes
	if (room->coins[player_id] < 20)
	{
		response->msg_id = e_invalid_action;
		return;
	}
	// compra a batata
	room->coins[player_id] -= 20;
	room->batatas[player_id]++;
	// notifica a sala
	auto buy = std::make_unique<ServerMessage>();
	buy->msg_id = n_buy_batata;
	buy->data = (unsigned char*)std::calloc(1, sizeof(char));
	buy->data_size = 1;
	buy->data[0] = player_id;
	notifyRoom(room, std::move(buy), player_id);
	// passa a vez
	room->whose_turn = (room->whose_turn + 1) % room->player_count;
	room->player_remaining_steps = 0;
	// notifica o proximo player
	auto turn = std::make_unique<ServerMessage>();
	turn->msg_id = n_turn_start;
	MessageHandler::notifyPlayer(room, std::move(turn), room->whose_turn);
	return;
}

void MessageHandler::startGame(std::shared_ptr<Room>& room)
{
	// notifica a sala
	auto start = std::make_unique<ServerMessage>();
	start->msg_id = n_game_start;
	notifyRoom(room, std::move(start), MAX_PLAYERS);
	// notifica o primeiro player
	auto turn = std::make_unique<ServerMessage>();
	turn->msg_id = n_turn_start;
	MessageHandler::notifyPlayer(room, std::move(turn), room->whose_turn);
	room->round++;
	return;
}

void MessageHandler::endGame(std::shared_ptr<Room>& room)
{
	bool have_valid_socket = true;
	for (uint_fast8_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (room->players[i] != INVALID_SOCKET)
		{
			have_valid_socket = false;
			break;
		}
	}
	if (have_valid_socket)
	{
		// notifica a sala
		auto end = std::make_unique<ServerMessage>();
		end->msg_id = n_game_end;
		end->data = (unsigned char*)std::calloc(1, sizeof(char));
		end->data_size = 1;
		end->data[0] = 1;
		notifyRoom(room, std::move(end), MAX_PLAYERS);
	}
	rooms.erase(room->name);
	return;
}