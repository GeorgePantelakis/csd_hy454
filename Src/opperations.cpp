#include "../Include/app.h"

bool operator<(const Color left, const Color right) {
	return left.r < right.r && left.g < right.g && left.b < right.b && left.a < right.a;
}