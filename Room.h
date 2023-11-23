#pragma once
#include <array>
#include <memory>
#include <string>
#include "Minigame.h"

extern "C" {
#include "MessageTypes.h"
#include <WinSock2.h>
}

// forward declaration
class Minigame;

struct Room {
	std::string name;
	uint_fast8_t round = 0;
	std::array<SOCKET, MAX_PLAYERS> players = { INVALID_SOCKET };
	std::array<bool, MAX_PLAYERS> ready = { false };
	std::array<uint_fast8_t, MAX_PLAYERS> coins = { 0 };
	std::array<uint_fast8_t, MAX_PLAYERS> batatas = { 0 };
	std::array<Direction, MAX_PLAYERS> direction = { Direction::up };
	std::array<uint_fast8_t, MAX_PLAYERS> position = { 0 };
	std::array<uint_fast64_t, MAX_PLAYERS> stat_emotes = { 0 };
	std::array<uint_fast64_t, MAX_PLAYERS> stat_steps = { 0 };
	std::array<uint_fast64_t, MAX_PLAYERS> stat_coins = { 0 };

	uint_fast8_t player_count = 0;
	uint_fast8_t whose_turn = 0;
	uint_fast8_t player_remaining_steps = 0;
	std::unique_ptr<Minigame> minigame;
};