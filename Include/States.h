#pragma once

enum spritestate_t {
	IDLE_STATE = 0, WALKING_STATE = 1, RUNNING_STATE = 2
};

enum spriteFormState_t {
	SMALL_MARIO = 0,
	SUPER_MARIO = 1,
	INVINCIBLE_MARIO_SMALL = 2,
	INVINCIBLE_MARIO_SUPER = 3,
	ENEMY = 4,
	SMASHED = 5,
	PIPE = 6,
	DELETE = 7,
	BRICK = 8,
	BLOCK = 9,
	EMPTY_BLOCK = 10,
	MOVED_BLOCK = 11,
	DELETE_BY_BLOCK = 12,
	COIN = 13
};
