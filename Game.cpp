#include "Game.h"

void MessageHandler::handleClient(SOCKET clientSocket)
{
	int iResult;
	unsigned char recvbuf[DEFAULT_BUFLEN] = {0};
	iResult = recv(clientSocket, (char*)recvbuf, DEFAULT_BUFLEN, 0);
	if (iResult > 0) {
		// processa todas as mensagens recebidas
		while (iResult >= SMALL_BUFLEN)
		{
			auto size = SMALL_BUFLEN;
			if (recvbuf[ROOM_CODE_SIZE] > 127 && iResult >= BIG_BUFLEN) {
				size = BIG_BUFLEN;
			}
			LOG("Tamanho da mensagem: " << size)
			auto msg = std::make_unique<std::vector<unsigned char>>(recvbuf, recvbuf + size);
			std::unique_ptr<ClientMessage> decoded_msg = MessageHandler::decodeMessage(std::move(msg), clientSocket);
			processMessage(std::move(decoded_msg));
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
		printf("Conexão fechada pelo cliente\n");
		closesocket(clientSocket);
	}
	else {
		printf("Erro ao receber mensagem: %d\n", WSAGetLastError());
		closesocket(clientSocket);
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

	auto res = std::make_unique<std::vector<unsigned char>>(size + 2);
	unsigned char* src = res->data();
	src[0] = (uint8_t)msg->msg_id;
	src[1] = size;
	if (size == 0)
	{
		return res;
	}
	std::memcpy(src + 2, (unsigned char*)(msg->data), size);

	delete msg->data;
	return res;
}

void MessageHandler::processMessage(std::unique_ptr<ClientMessage> msg)
{
	std::shared_ptr<Room> room;
	auto room_code = std::string((char*)msg->room_code, ROOM_CODE_SIZE);
	auto search = rooms.find(room_code);
	if (search == rooms.end()) {
		auto response = std::make_unique<ServerMessage>();
		if (msg->msg_id != join) {

			response->msg_id = e_room_not_found;
			MessageHandler::notifySocket(msg->creator, std::move(response));
			closesocket(msg->creator);
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
		response->msg_id = r_ok;
		rooms.insert({ std::move(room_code), room });
		notifyPlayer(room, std::move(response), 0);
		return;
	}
	room = search->second;

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
		auto response = std::make_unique<ServerMessage>();
		response->msg_id = e_player_not_in_room;
		MessageHandler::notifySocket(msg->creator, std::move(response));
		closesocket(msg->creator);
		return;
	}
	switch (msg->msg_id)
	{
	case ready:
		LOG("ready");
		MessageHandler::processReady(std::move(msg), room, player_index);
		break;
	case unready:
		LOG("unready");
		MessageHandler::processUnready(std::move(msg), room, player_index);
		break;
	case roll_dice:
		LOG("roll_dice");
		MessageHandler::processRollDice(std::move(msg), room, player_index);
		break;
	case buy_batata:
		LOG("buy_batata");
		MessageHandler::processBuyBatata(std::move(msg), room, player_index);
		break;
	case move:
		LOG("move");
		MessageHandler::processMove(std::move(msg), room, player_index);
		break;
	case emote:
		LOG("emote");
		MessageHandler::processEmote(std::move(msg), room, player_index);
		break;
	case join:
		LOG("join");
		MessageHandler::processJoin(std::move(msg), room, player_index);
		return;
	case minigame:
		LOG("minigame");
		MessageHandler::processMinigame(std::move(msg), room, player_index);
		break;
	default:
		std::cout << "Mensagem invalida" << std::endl;
		auto response = std::make_unique<ServerMessage>();
		response->msg_id = e_decode;
		notifyPlayer(room, std::move(response), player_index);
		return;
	}
}

void MessageHandler::notifyRoom(std::shared_ptr<Room> room, std::unique_ptr<ServerMessage> msg, uint_fast8_t player_id)
{
	std::unique_ptr<std::vector<unsigned char>> encoded_msg = encodeMessage(std::move(msg));
	bool got_error = false;
	for (uint_fast8_t i = 0; i < room->player_count; i++)
	{
		SOCKET sock = room->players[i];
		if (i == player_id)
		{
			continue;
		}
		if (sock == INVALID_SOCKET)
		{
			LOG("Socket invalido em um notify room, terminando o jogo")
			got_error = true;
			break;
		}
		int sendResult = send(sock, (char*)encoded_msg->data(), (int)encoded_msg->size(), 0);
		if (sendResult == SOCKET_ERROR) {
			printf("Erro ao enviar mensagem: %d\n", WSAGetLastError());
			got_error = true;
			closesocket(sock);
			room->players[i] = INVALID_SOCKET;
			break;
		}
	}
	if (got_error)
	{
		LOG("Terminando o jogo depois de erros em notify room")
		MessageHandler::endGame(room);
	}
}

void MessageHandler::notifyPlayer(std::shared_ptr<Room> room, std::unique_ptr<ServerMessage> msg, uint_fast8_t player_id)
{
	SOCKET sock = room->players[player_id];
	bool r = MessageHandler::notifySocket(sock, std::move(msg));
	if (r)
	{
		room->players[player_id] = INVALID_SOCKET;
		LOG("Terminando o jogo depois de erros em notify player")
		MessageHandler::endGame(room);
	}
}

bool MessageHandler::notifySocket(SOCKET sock, std::unique_ptr<ServerMessage> msg)
{
	std::unique_ptr<std::vector<unsigned char>> encoded_msg = encodeMessage(std::move(msg));
	int sendResult = send(sock, (char*)encoded_msg->data(), (int)encoded_msg->size(), 0);
	if (sendResult == SOCKET_ERROR) {
		printf("Erro ao enviar mensagem: %d\n", WSAGetLastError());
		closesocket(sock);
		return true;
	}
	return false;
}

void MessageHandler::processReady(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	auto response = std::make_unique<ServerMessage>();
	// player ja esta pronto
	if (room->ready[player_id] || room->round != 0)
	{
		response->msg_id = e_invalid_action;
		notifyPlayer(room, std::move(response), player_id);
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
			notifyPlayer(room, std::move(response), player_id);
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
	notifyPlayer(room, std::move(response), player_id);
	if (everyone_ready)
	{
		MessageHandler::startGame(room);
	}
	return;
}

void MessageHandler::processUnready(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	auto response = std::make_unique<ServerMessage>();
	// player nao esta pronto
	if (!room->ready[player_id] || room->round != 0)
	{
		response->msg_id = e_invalid_action;
		notifyPlayer(room, std::move(response), player_id);
		return;
	}
	// marca o player como nao pronto
	room->ready[player_id] = false;
	response->msg_id = r_ok;
	notifyPlayer(room, std::move(response), player_id);
	return;
}

void MessageHandler::processEmote(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	auto response = std::make_unique<ServerMessage>();
	// notifica a sala
	auto emote = std::make_unique<ServerMessage>();
	emote->msg_id = n_emote;
	emote->data = (unsigned char*)std::calloc(1, sizeof(char));
	emote->data_size = 1;
	emote->data[0] = request->optional;
	notifyRoom(room, std::move(emote), player_id);
	room->stat_emotes[player_id]++;
	response->msg_id = r_ok;
	notifyPlayer(room, std::move(response), player_id);
	return;
}

void MessageHandler::processJoin(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	auto response = std::make_unique<ServerMessage>();
	LOG("JOIN")
	// player ja esta na sala
	if (player_id != MAX_PLAYERS)
	{
		response->msg_id = e_invalid_action;
		MessageHandler::notifySocket(request->creator, std::move(response));
		closesocket(request->creator);
		return;
	}
	// sala cheia
	if (room->player_count == MAX_PLAYERS)
	{
		response->msg_id = e_room_full;
		MessageHandler::notifySocket(request->creator, std::move(response));
		closesocket(request->creator);
		return;
	}
	// jogo ja comecou
	if (room->round != 0)
	{
		response->msg_id = e_game_already_started;
		MessageHandler::notifySocket(request->creator, std::move(response));
		closesocket(request->creator);
		return;
	}
	// adiciona o player na sala
	room->players[room->player_count] = request->creator;
	player_id = room->player_count++;
	room->name = std::string((char*)request->room_code, ROOM_CODE_SIZE);

	response->msg_id = r_ok;
	response->data = (unsigned char*)std::calloc(1, sizeof(char));
	response->data_size = 1;
	response->data[0] = room->player_count;

	LOG("Player id: " << (int)player_id);
	notifyPlayer(room, std::move(response), player_id);

	// notifica a sala
	auto join = std::make_unique<ServerMessage>();
	join->msg_id = n_join;
	join->data = (unsigned char*)std::calloc(1, sizeof(char));
	join->data_size = 1;
	join->data[0] = player_id;
	notifyRoom(room, std::move(join), player_id);
	return;
}

void MessageHandler::processMove(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	auto response = std::make_unique<ServerMessage>();
	// verifica se o jogo começou
	if (room->round == 0)
	{
		response->msg_id = e_game_not_started;
		notifyPlayer(room, std::move(response), player_id);
		return;
	}
	// verifica se e a vez do player
	if (room->whose_turn != player_id)
	{
		response->msg_id = e_not_your_turn;
		notifyPlayer(room, std::move(response), player_id);
		return;
	}
	uint_fast8_t r = room->player_remaining_steps;
	// verifica se o player ainda tem passos
	if (r == 0)
	{
		response->msg_id = e_invalid_action;
		notifyPlayer(room, std::move(response), player_id);
		return;
	}
	uint_fast8_t op = request->optional;
	if (op > 3)
	{
		response->msg_id = e_invalid_action;
		notifyPlayer(room, std::move(response), player_id);
		return;
	}
	Direction d = (Direction)op;
	if (board.intersectionDirection(room->position[player_id], room->direction[player_id]) != d)
	{
		response->msg_id = e_invalid_action;
		notifyPlayer(room, std::move(response), player_id);
		return;
	}
	room->direction[player_id] = d;
	response->data = (unsigned char*)std::calloc(2, sizeof(char));
	response->data_size = 2;
	response->msg_id = r_move;
	MessageHandler::executeMovements(std::move(request), response, room, player_id);

	return;
}

void MessageHandler::processRollDice(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	auto response = std::make_unique<ServerMessage>();
	// verifica se o jogo começou
	if (room->round == 0)
	{
		response->msg_id = e_game_not_started;
		notifyPlayer(room, std::move(response), player_id);
		return;
	}
	// verifica se e a vez do player
	if (room->whose_turn != player_id)
	{
		response->msg_id = e_not_your_turn;
		notifyPlayer(room, std::move(response), player_id);
		return;
	}
	// verifica se o player ainda tem passos
	if (room->player_remaining_steps != 0)
	{
		response->msg_id = e_invalid_action;
		notifyPlayer(room, std::move(response), player_id);
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
	LOG("PASSOS: " << (int)r << " - " << (int)odio.remaining);
	// se isso der underflow tem um bug em board.cpp
	room->stat_steps[player_id] += r - odio.remaining;
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
	room->coins[player_id] += coins;
	room->stat_coins[player_id] += coins;
	
	// checa se o player pode comprar batata
	if (odio.canBuy)
	{
		response->msg_id = r_can_buy_batata;
	}
	if (odio.isIntersection)
	{
		LOG("PQ!!!" << (int)room->player_remaining_steps + 0);
		response->msg_id = r_roll_dice_intersect;
	}
	notifyPlayer(room, std::move(response), player_id);
	// notifica a sala
	auto move = std::make_unique<ServerMessage>();
	move->msg_id = n_move;
	move->data = (unsigned char*)std::calloc(2, sizeof(char));
	move->data_size = 2;
	move->data[0] = pos;
	move->data[1] = coins;
	notifyRoom(room, std::move(move), player_id);
	if (odio.isIntersection || odio.canBuy)
	{
		return;
	}
	// passa a vez
	room->player_remaining_steps = 0;
	room->whose_turn = (room->whose_turn + 1) % room->player_count;
	if (room->whose_turn == 0)
	{
		room->round++;
		room->minigame = MinigamesFactory::createMinigame((MinigamesEnum)((std::rand() % 3) + 1));
	}
	// notifica o proximo player
	auto turn = std::make_unique<ServerMessage>();
	turn->msg_id = n_turn_start;
	MessageHandler::notifyPlayer(room, std::move(turn), room->whose_turn);
}

void MessageHandler::processBuyBatata(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	auto response = std::make_unique<ServerMessage>();
	// verifica se o jogo começou
	if (room->round == 0)
	{
		response->msg_id = e_game_not_started;
		notifyPlayer(room, std::move(response), player_id);
		return;
	}
	// verifica se e a vez do player
	if (room->whose_turn != player_id)
	{
		response->msg_id = e_not_your_turn;
		notifyPlayer(room, std::move(response), player_id);
		return;
	}
	// verifica se o player está na posicao certa
	if (!board.isBatata(room->position[player_id]))
	{
		response->msg_id = e_invalid_action;
		notifyPlayer(room, std::move(response), player_id);
		return;
	}
	// verifica se o player tem moedas suficientes
	if (room->coins[player_id] < 20)
	{
		response->msg_id = e_invalid_action;
		notifyPlayer(room, std::move(response), player_id);
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
	response->msg_id = r_ok;
	response->data = (unsigned char*)std::calloc(2, sizeof(char));
	MessageHandler::executeMovements(std::move(request), response, room, player_id);
	return;
}

void MessageHandler::processMinigame(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id)
{
	auto response = std::make_unique<ServerMessage>();
	if (!room->minigame) {
		response->msg_id = e_not_in_minigame;
		notifyPlayer(room, std::move(response), player_id);
		return;
	}
	response->msg_id = r_ok;
	room->minigame->processMessage(std::move(request), response, room, player_id);
	notifyPlayer(room, std::move(response), player_id);
	if (!room->minigame->isFinished())
	{
		return;
	}
	room->minigame.reset();
	room->whose_turn = 0;
	room->player_remaining_steps = 0;
	
	std::array<bool, MAX_PLAYERS> winner = room->minigame->getWinner();
	auto end = std::make_unique<ServerMessage>();
	// o tamanho do vetor de vencedores e o numero de bytes necessarios para armazenar todos os vencedores
	// o valor não dara overflow pois o numero diminui nas duas primeiras operacoes
	// player_count de 0 seria um bug em outro lugar e divisao por 8 nao da underflow
	// como o numero é dividido por 8 antes de ser somado 1, o valor nao dara overflow
	uint_fast8_t bytes_needed = (room->player_count - 1) / 8 + 1;
	end->data = (unsigned char*)std::calloc(bytes_needed, sizeof(char));
	end->data_size = bytes_needed;
	end->msg_id = n_minigame_end;

	for (uint_fast8_t i = 0; i < room->player_count; i++)
	{
		if (winner[i])
		{
			end->data[i / 8] |= 1 << (i % 8);
			room->coins[i] += PRIZE_MINIGAME;
			room->stat_coins[i] += PRIZE_MINIGAME;
		}
	}
	notifyRoom(room, std::move(end), MAX_PLAYERS);
}

void MessageHandler::startGame(std::shared_ptr<Room>& room)
{
	// notifica a sala
	auto start = std::make_unique<ServerMessage>();
	start->msg_id = n_game_start;
	start->data = (unsigned char*)std::calloc(1, sizeof(char));
	start->data_size = 1;
	start->data[0] = room->player_count;
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
	LOG("ENDING THE GAME IN: " << room->name)
	bool have_valid_socket = false;
	bool have_invalid_socket = false;
	for (uint_fast8_t i = 0; i < room->player_count; i++)
	{
		LOG("Player " << (int)i << " batata: " << (int)room->batatas[i]);
		LOG("Player " << (int)i << " coins: " << (int)room->coins[i]);
		LOG("Player " << (int)i << " position: " << (int)room->position[i]);
		if (room->players[i] == INVALID_SOCKET)
		{
			have_invalid_socket = true;
			continue;
		}
		have_valid_socket = true;
	}
	if (have_valid_socket)
	{
		// notifica a sala
		uint_fast8_t player_count = room->player_count;
		auto batata_bonus = BatataBonus::emotes; //(BatataBonus)(std::rand() % NOF_BONUS);
		auto podium = std::vector<std::pair<uint_fast8_t, uint_fast64_t>>(player_count);
		for (uint_fast8_t i = 0; i < player_count; i++)
		{
			uint_fast64_t v;
			switch (batata_bonus)
			{
			case coins:
				v = room->stat_coins[i];
				break;
			case steps:
				v = room->stat_steps[i];
				break;
			case emotes:
				v = room->stat_emotes[i];
				break;
			default:
				LOG("Batata bonus selecionado aleatoriamente é inválido");
				break;
			}
			podium[i] = std::make_pair(i, v);
		}
		std::sort(podium.begin(), podium.end(), [](std::pair<uint_fast8_t, uint_fast64_t> a, std::pair<uint_fast8_t, uint_fast64_t> b) {
			return a.second > b.second;
		});
		unsigned char* buf = (unsigned char*)std::calloc(player_count, PODIUM_SIZE);
		if (buf == NULL)
		{
			std::cerr << "CALLOC FALHOU ENTRANDO EM PANICO!!!" << std::endl;
			WSACleanup();
			exit(1);
		}
		for (uint_fast8_t i = 0; i < player_count; i++)
		{
			uint_fast8_t player_id = podium.at(i).first;
			buf[i * PODIUM_SIZE] = player_id;
			buf[i * PODIUM_SIZE + 1] = room->batatas[player_id];
			buf[i * PODIUM_SIZE + 2] = (unsigned char)batata_bonus;
			std::memcpy(buf + 3 + (i * PODIUM_SIZE), &room->stat_coins[player_id], sizeof(unsigned long long));
			std::memcpy(buf + 3 + sizeof(unsigned long long) + (i * PODIUM_SIZE), &room->stat_steps[player_id], sizeof(unsigned long long));
			std::memcpy(buf + 3 + (2*sizeof(unsigned long long)) + (i * PODIUM_SIZE), &room->stat_emotes[player_id], sizeof(unsigned long long));
		}
		for (uint_fast8_t i = 0; i < player_count; i++)
		{
			SOCKET sock = room->players[i];
			if (sock == INVALID_SOCKET)
			{
				continue;
			}
			auto end = std::make_unique<ServerMessage>();
			end->msg_id = n_game_end;
			end->data = (unsigned char*)std::calloc(player_count, PODIUM_SIZE);
			end->data_size = player_count * PODIUM_SIZE;
			std::memcpy(end->data, buf, player_count * PODIUM_SIZE);
			MessageHandler::notifySocket(sock, std::move(end));
			if (have_invalid_socket)
			{
				room->players[i] = INVALID_SOCKET;
				closesocket(sock);
			}
		}
		delete buf;
	}
	rooms.erase(room->name);
	return;
}
