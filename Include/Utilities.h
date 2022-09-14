#pragma once
#include <string>
#include <vector>
#include <math.h>
#include <sstream>
#include <iostream>

using namespace std;

struct game_params {
	unsigned initial_player_lifes = 3;
	unsigned invinsible_mario_duration = 30;
	unsigned coins_to_get_life = 100;
	unsigned koopa_troopa_stun_duration = 5;
	unsigned enemy_speed = 1;
	unsigned enemy_shell_speed = 3;
	unsigned goomba_points = 100;
	unsigned green_koopa_troopa_points = 100;
	unsigned red_koopa_troopa_points = 100;
	unsigned piranha_plant_points = 100;
	unsigned powerups_points = 1000;
	unsigned mario_max_speed = 4;
	unsigned coin_points = 200;
};

vector <string> splitString(string text, string delimiter);
int customRound(double number);
string standarizeSize(string text, int size);

struct pointShow {
    int points;
    int x;
    int y;
    int final_y;
};