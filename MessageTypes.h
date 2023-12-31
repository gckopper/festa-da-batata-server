#ifndef MESSAGE_TYPES
#define MESSAGE_TYPES
#include <stdint.h>
#define ROOM_CODE_SIZE 8
#define SMALL_BUFLEN 9
#define BIG_BUFLEN 10

#define BOARD_WIDTH 8
#define BOARD_HEIGHT 5
#define BOARD_SIZE (BOARD_WIDTH * BOARD_HEIGHT)
#define NUM_TILES 28
#define BATATA_LOC 20
#define BATATA_LOCX (BATATA_LOC % BOARD_WIDTH)
#define BATATA_LOCY (BATATA_LOC / BOARD_WIDTH)
#define PRIZE_MINIGAME 8
#define MAX_PLAYERS 4

#define MAX_ROUNDS 5

enum Direction {
	up = 0,
	down = 1,
	left = 2,
	right = 3,
};

#define NOF_MINIGAMES 3

enum MinigamesEnum {
	rock_paper_scissors = 1,
	cut_the_wire = 2,
	coin_flip = 3,
};

enum Hand { rock = 0, paper = 1, scissors = 2, no = 3 };

#define NOF_BONUS 3

enum BatataBonus {
	coins = 0,
	steps = 1,
	emotes = 2,
};
// tamanho que a estrutura vai ter durante o transporte na rede
// pode, ou n�o, ser igual ao tamanho em memoria durante a execu��o
#define PODIUM_SIZE 12

struct PlayerPodium {
	unsigned char player_id;
	unsigned char batatas;
	unsigned char current_coins;
	// enum BatataBonus bonus_type; mas com um tamanho 
	// indepentende do compilador
	unsigned char bonus_type;
	unsigned long long bonus;
};

// mensagens acima de 127 s�o consideradas mensagens longas (com um 
// byte extra).
enum message_id {
	// to pronto pra jogar
	ready      = 0,

	// to pronto pra jogar
	unready    = 1,

	// pede pra rodar o dado
	roll_dice  = 2,

	// compra a batata
	buy_batata = 3,

	// join
	join       = 4,

	// leave
	leave      = 5,

	// MENSAGENS COM OPCIONAL
	// Pode ser usado pra escolher um resultado 
	// em uma interseccao ou em game pra move o
	// player
	move       = 128,

	// manda um emote irritante
	emote      = 129,

	// minigame
	minigame   = 130,

};

typedef struct ClientMessage {
	#ifdef SERVER 
	unsigned long long creator;
	#endif
	// has size ROOM_CODE_SIZE
	unsigned char* room_code;
	enum message_id msg_id;
	unsigned char optional;
} ClientMessage;

enum server_message_id {
	// menssagens de notificacao (0-63)
	// notifica que o jogo vai comecar
	n_game_start = 0,
	// notifica que � a vez de alguem
	n_turn_start = 1,
	// notifica que o jogo acabou
	n_game_end = 2,
	// player usou emote
	n_emote = 3,
	// player comprou batata
	n_buy_batata = 4,
	// player moveu
	n_move = 5,
	// player entrou na sala
	n_join = 6,
	// minigame
	n_minigame = 7,
	n_minigame_end = 8,

	// mensagens de resposta (64-127)
	// resposta tudo certo
	r_ok = 64,
	// resposta a um roll_dice
	// para onde andar
	// quantas moedas no caminho
	// total do dado
	r_roll_dice = 65,
	// resposta a um roll_dice com interseccao
	r_roll_dice_intersect = 66,
	// resposta a um roll_dice ou move onde
	// o player pode comprar batata
	r_can_buy_batata = 69,
	// resposta a um move
	r_move = 70,
	// confirma que o player entrou na sala e manda
	// quantos jogadores j� est�o na sala
	r_joined = 71,
	
	// mensagens de erro (128-255)
	// erro ao tentar join e sala cheia
	e_join = 128,
	// erro ao decodificar mensagem
	e_decode = 129,
	// nao e a vez do player
	e_not_your_turn = 130,
	// erro acao invalida
	e_invalid_action = 131,
	// erro sala nao existe
	e_room_not_found = 132,
	// erro sala cheia
	e_room_full = 133,
	// erro player nao esta na sala
	e_player_not_in_room = 134,
	// jogo ainda nao comecou
	e_game_not_started = 135,
	// erro ao tentar jogar fora do minigame
	e_not_in_minigame = 136,
	// jogo ja comecou
	e_game_already_started = 137,
};

typedef struct ServerMessage {
	enum server_message_id msg_id;
	unsigned char data_size;
	unsigned char* data;
} ServerMessage;

#endif
