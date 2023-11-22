#include "Board.h"

bool Board::isIntersection(uint_fast8_t x, uint_fast8_t y)
{
	return (x == 0 || x == 8) && y == 2;
}

bool Board::isIntersection(uint_fast8_t pos)
{
	return pos == 16 || pos == 23;
}

bool Board::isValid(uint_fast8_t x, uint_fast8_t y)
{
	return Board::isValid(x + y * BOARD_WIDTH);
}

bool Board::isValid(uint_fast8_t pos)
{
	return pos < BOARD_SIZE && board[pos] != 0;
}

Direction Board::turnCorner(uint_fast8_t x, uint_fast8_t y, Direction current)
{
	if (current == up || current == down)
	{
		if (Board::isValid(x + 1, y)) {
			return right;
		}
		return left;
	}
	if (Board::isValid(x, y + 1)) {
		return up;
	}
	return down;
}

Direction Board::turnCorner(uint_fast8_t pos, Direction current)
{
	return Board::turnCorner(pos % BOARD_WIDTH, pos / BOARD_WIDTH, current);
}

bool Board::isValidintersectionDirection(uint_fast8_t x, uint_fast8_t y, Direction d)
{
	switch (d)
	{
		case up:
			return Board::isValid(x, y + 1);
		case down:
			return Board::isValid(x, y - 1);
		case left:
			return Board::isValid(x - 1, y);
		case right:
			return Board::isValid(x + 1, y);
	}
	return false;
}

bool Board::isValidintersectionDirection(uint_fast8_t pos, Direction d)
{
	return Board::isValidintersectionDirection(pos % BOARD_WIDTH, pos / BOARD_WIDTH, d);
}

bool Board::isBatata(uint_fast8_t pos)
{
	return pos == BATATA_LOC;
}

// distancia a andar que deve ser menor que o tamanho do tabuleiro
// caso o tabuleiro seja muito grande pode dar overflow
// muito grande é (maior lado + tamanho) > 255
Odio Board::executeMove(uint_fast8_t pos, uint_fast8_t distance, Direction current, bool can_buy)
{
	uint_fast8_t x = pos % BOARD_WIDTH;
	uint_fast8_t y = pos / BOARD_WIDTH;
	uint_fast8_t nx = pos % BOARD_WIDTH;
	uint_fast8_t ny = pos / BOARD_WIDTH;
	uint_fast8_t remaining = 0;
	bool willIntersect = false;
	bool willIntersectX = x == 0 || x == (BOARD_WIDTH-1);
	bool willIntersectY = y == 2;

	switch (current)
	{
	case up:
		ny = y + distance;
		willIntersect = willIntersectX && ny >= 2 && y < 2;
		if (ny >= BOARD_HEIGHT)
		{
			remaining = ny - BOARD_HEIGHT;
			ny = BOARD_HEIGHT - 1;
		}
		if (willIntersect)
		{
			remaining = (distance + y - 2);
			ny = 2;
		}
		break;
	case down:
		ny = y - distance;
		// ny vai dar underflow
		if (distance > y)
		{
			remaining = distance - y;
			ny = 0;
		}
		willIntersect = willIntersectX && ny <= 2 && y > 2;
		if (willIntersect)
		{
			remaining = (y - 2);
			ny = 2;
		}
		break;
	case left:
		nx = x - distance;
		// nx vai dar underflow
		if (distance > x)
		{
			willIntersect = willIntersectY;
			remaining = distance - x;
			nx = 0;
		}
		if (can_buy && willIntersectY && x >= BATATA_LOCX && nx < BATATA_LOCX)
		{
			willIntersect = true;
			remaining = x - 4;
			nx = 4;
			break;
		}
		break;
	case right:
		nx = x + distance;
		if (nx >= BOARD_WIDTH)
		{
			willIntersect = willIntersectY;
			remaining = nx - BOARD_WIDTH;
			nx = BOARD_WIDTH - 1;
		}
		if (can_buy && willIntersectY && x <= BATATA_LOCX && nx > BATATA_LOCX)
		{
			willIntersect = true;
			remaining = nx - 4;
			nx = 4;
			break;
		}
		break;
	}
	if (willIntersect)
	{
		return { nx, 2, Direction::up, true, remaining, can_buy};
	}
	if (remaining > 0)
	{
		return Board::executeMove(nx+ny* BOARD_WIDTH, remaining, Board::turnCorner(nx, ny, current), can_buy);
	}
	return { nx, ny, current, false, 0 , false};
}

