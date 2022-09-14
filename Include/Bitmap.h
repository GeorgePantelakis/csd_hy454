#pragma once

#include "app.h"
#include <allegro5/allegro.h>

using namespace app;

//PixelMemory BitmapGetMemory(Bitmap bmp);
//int BitmapGetLineOffset(Bitmap bmp);

Bitmap BitmapLoad(const std::string& path);

Bitmap BitmapCreate(unsigned short w, unsigned short h);

Bitmap SubBitmapCreate(Bitmap src, const Rect& from);

Bitmap BitmapCopy(Bitmap bmp);

Bitmap BitmapClear(Bitmap bmp, Color c);

void BitmapDestroy(Bitmap bmp);

Bitmap BitmapGetScreen(void);

int BitmapGetWidth(Bitmap bmp);

int BitmapGetHeight(Bitmap bmp);

void BitmapBlit(Bitmap src, const Rect& from, Bitmap dest, const Point& to);

ALLEGRO_LOCKED_REGION* BitmapLock(Bitmap bmp);

void BitmapUnlock(Bitmap bmp);

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

int GetDepth();

template<typename Tfunc>
void BitmapAccessPixels(Bitmap bmp, const Tfunc& f);
