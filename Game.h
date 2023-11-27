#pragma once
#define SERVER
#include <iostream>
#include <unordered_map>
#include <array>
#include "Board.h"
#include <cstdlib> 
#include "MinigamesFactory.h"
#include "Room.h"
#include "Logger.h"
#include <algorithm>

extern "C" {
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include "MessageTypes.h"
}

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_BUFLEN 512

struct Room;

class MessageHandler
{
private:
	std::unordered_map<std::string, std::shared_ptr<Room>> rooms;
	std::unordered_map<SOCKET, std::shared_ptr<Room>> sock_to_rooms;
	Board board;
public:

	void handleClient(SOCKET clientSocket);
	std::unique_ptr<ClientMessage> decodeMessage(std::unique_ptr<std::vector<unsigned char>> msg, SOCKET creator);
	std::unique_ptr<std::vector<unsigned char>> encodeMessage(std::unique_ptr<ServerMessage> msg);
	void processMessage(std::unique_ptr<ClientMessage> request);

	void notifyRoom(std::shared_ptr<Room> room, std::unique_ptr<ServerMessage> msg, uint_fast8_t player_id);
	void notifyPlayer(std::shared_ptr<Room> room, std::unique_ptr<ServerMessage> msg, uint_fast8_t player_id);
	bool notifySocket(SOCKET sock, std::unique_ptr<ServerMessage> msg);


	void processReady(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id);
	void processUnready(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id);
	void processEmote(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room> &room, uint_fast8_t player_id);
	void processJoin(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id);
	void processMove(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id);
	void processRollDice(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id);
	void executeMovements(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id);
	void processBuyBatata(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id);
	void processMinigame(std::unique_ptr<ClientMessage> request, std::shared_ptr<Room>& room, uint_fast8_t player_id);

	void startGame(std::shared_ptr<Room>& room);
	void endGame(std::shared_ptr<Room> &room);
	void deleteRoomAndEndGame(SOCKET sock);
};

