#pragma once

#include <bitset>
#include <array>
#include "MessageTypes.h"
#include "Logger.h"
struct Odio {
	uint_fast8_t x;
	uint_fast8_t y;
	Direction direction;
	bool isIntersection;
	uint_fast8_t remaining;
	bool canBuy;
};

class Board
{
	private:
		const std::bitset<BOARD_SIZE> board =
					std::bitset<BOARD_SIZE>(
						0b1111'1111'1000'0001'1111'1111'1000'0001'1111'1111
					);
	public:
		bool isIntersection(uint_fast8_t x, uint_fast8_t y);
		bool isIntersection(uint_fast8_t pos);
		bool isValid(uint_fast8_t x, uint_fast8_t y);
		bool isValid(uint_fast8_t pos);
		Direction turnCorner(uint_fast8_t x, uint_fast8_t y, Direction current);
		Direction turnCorner(uint_fast8_t pos, Direction current);
		bool isValidintersectionDirection(uint_fast8_t x, uint_fast8_t y, Direction d);
		bool isValidintersectionDirection(uint_fast8_t pos, Direction d);
		bool isBatata(uint_fast8_t pos);
		Odio executeMove(uint_fast8_t pos, uint_fast8_t distance, Direction current, bool can_buy);
};

