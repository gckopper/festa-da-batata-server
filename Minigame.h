#pragma once
#include "Room.h"
extern "C" {
#include "MessageTypes.h"
}
// forward declaration
struct Room;

class Minigame
{
public:
	virtual void processMessage(std::unique_ptr<ClientMessage> request, std::unique_ptr<ServerMessage>& response, std::shared_ptr<Room>& room, uint_fast8_t player_id) = 0;
	virtual bool isFinished() = 0;
	virtual uint_fast8_t getWinner() = 0;
};