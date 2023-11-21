#pragma once
#include <array>
#include <memory>
#include <string>
#include "Minigame.h"
#include "Game.h"

extern "C" {
#include "MessageTypes.h"
}

// forward declaration
class Minigame;
class Game;

struct Room {
	std::string name;
	uint_fast8_t round = 0;
	std::array<SOCKET, MAX_PLAYERS> players = { INVALID_SOCKET };
	std::array<bool, MAX_PLAYERS> ready = { false };
	std::array<uint_fast8_t, MAX_PLAYERS> coins = { 0 };
	std::array<uint_fast8_t, MAX_PLAYERS> batatas = { 0 };
	std::array<Direction, MAX_PLAYERS> direction = { up };
	std::array<uint_fast8_t, MAX_PLAYERS> position = { 0 };
	uint_fast8_t player_count = 0;
	uint_fast8_t whose_turn = 0;
	uint_fast8_t player_remaining_steps = 0;
	bool isInMinigame = false;
	std::unique_ptr<Minigame> minigame;
};