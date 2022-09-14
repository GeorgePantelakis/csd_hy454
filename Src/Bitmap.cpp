#include "../Include/app.h"
#include <allegro5/allegro.h>

using namespace app;

Bitmap BitmapLoad(const std::string& path) {
	return al_load_bitmap(path.c_str());
}

Bitmap BitmapCreate(unsigned short w, unsigned short h){
	return al_create_bitmap(w, h);
}

Bitmap SubBitmapCreate(Bitmap src, const Rect& from) {
	return al_create_sub_bitmap(src, from.x, from.y, from.w, from.h);
}

Bitmap BitmapCopy(Bitmap bmp) {
	return al_clone_bitmap(bmp);
}

Bitmap BitmapClear(Bitmap bmp, Color c) {
	Bitmap tmp = al_get_target_bitmap();
	al_set_target_bitmap(bmp);
	al_clear_to_color(c);
	al_set_target_bitmap(tmp);
	return bmp;
}

void BitmapDestroy(Bitmap bmp) {
	al_destroy_bitmap(bmp);
}

extern ALLEGRO_DISPLAY* display;
Bitmap BitmapGetScreen(void) {
	return al_get_backbuffer(display);
}

int BitmapGetWidth(Bitmap bmp) {
	return al_get_bitmap_width(bmp);
}

int BitmapGetHeight(Bitmap bmp) {
	return al_get_bitmap_height(bmp);
}

void BitmapBlit(Bitmap src,  const Rect& from, Bitmap dest, const Point& to) {
	Bitmap tile = al_create_sub_bitmap(src, from.x, from.y, from.w, from.h);
	al_set_target_bitmap(dest);
	al_draw_bitmap(tile, to.x, to.y, 0);
	al_destroy_bitmap(tile);
}

ALLEGRO_LOCKED_REGION* BitmapLock(Bitmap bmp) {
	return al_lock_bitmap(bmp, al_get_bitmap_format(bmp), ALLEGRO_LOCK_READWRITE);
}

void BitmapUnlock(Bitmap bmp) {
	al_unlock_bitmap(bmp);
}

//TODO
/*PixelMemory BitmapGetMemory(Bitmap bmp) {
	return (PixelMemory) bmp;
}

int BitmapGetLineOffset(Bitmap bmp) {
	//is this ok?
	return al_get_bitmap_width(bmp);
	//return 1;
}*/

extern ALLEGRO_DISPLAY* display;

int GetDepth() {
	return al_get_display_option(display, ALLEGRO_DEPTH_SIZE);
}

template<typename Tfunc>
void BitmapAccessPixels(Bitmap bmp, const Tfunc& f) {
	auto result = BitmapLock(bmp);
	assert(result);
	auto mem = result->data;
	for (int y = 0; y < BitmapGetHeight(bmp); ++y)
		for (int x = 0; x < BitmapGetWidth(bmp); ++x)
			f(mem[(x + y * BitmapGetWidth(bmp)) * 4]);
	/*auto offset = BitmapGetLineOffset(bmp);
	for (auto y = BitmapGetHeight(bmp); y--;) {
		auto buff = mem;
		for (auto x = BitmapGetWidth(bmp); x--;) {
			f(buff);
			buff += GetDepth();
		}
		mem += offset;
	}*/
	BitmapUnlock(bmp);
}
