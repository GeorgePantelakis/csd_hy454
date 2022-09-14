#include "../Include/GravityHandler.h"
#include <sstream>
#include <stdio.h>
#include <iostream>

void GravityHandler::Reset(void) {
	is_Falling = false;
}

void GravityHandler::Check(const Rect& r) {
	if (gravityAddicted) {
		if (onSolidGround(r)) {
			if (is_Falling) {
				is_Falling = false;
				//onStopFalling();
			}
		} else if(!is_Falling) {
			is_Falling = true;
			fallingTimer = 0;
			//onStartFalling();
		}
	}
}

bool GravityHandler::isOnSolidGround(const Rect& r) {
	return onSolidGround(r);
}

bool GravityHandler::isFalling() {
	return is_Falling;

}

void GravityHandler::setGravityAddicted(bool val) {
	gravityAddicted = val;
}

void GravityHandler::SetFalling(bool v) {
	is_Falling = v;
}

int GravityHandler::getFallingTimer(void) {
	return fallingTimer;
}

void GravityHandler::increaseFallingTimer(void) {
	fallingTimer++;
}

void GravityHandler::checkFallingDistance(int& dy, Rect r) {
	int new_dy = 0;

	while (!onSolidGround(r)) {
		new_dy++;
		r.y++;
		if (new_dy >= dy) {
			break;
		}
	}

	dy = new_dy;
}