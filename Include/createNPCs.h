#include "Sprite.h"

namespace app {

	//create enemies
	Sprite* create_enemy_goomba(int x, int y);
	Sprite* create_enemy_green_koopa_troopa(int x, int y);
	Sprite* create_enemy_red_koopa_troopa(int x, int y);
	Sprite* create_enemy_piranha_plant(int x, int y);

	//create powerups
	void create_super_mushroom(int x, int y);
	void create_1UP_mushroom(int x, int y, Game* game);
	void create_starman(int x, int y);

	//create blocks
	void create_brick_sprite(int x, int y);
	void create_block_sprite(int, int, Game*);
	void create_coin_sprite(int x, int y, Game* game);
	void MoveScene(int new_screen_x, int new_screen_y, int new_mario_x, int new_mario_y);
	Sprite* LoadPipeCollision(Sprite* mario, string pipes);
}