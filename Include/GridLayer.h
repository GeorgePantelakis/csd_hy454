#pragma once
#include "Defines.h"
#include "Typedefs.h"
#include "Shapes.h"
#include "States.h"


struct RGB {
	RGBValue r, g, b;
};
struct RGBA : public RGB {
	RGBValue a;
};

class GridLayer {
private:
	GridMap grid;
	void Allocate(void);

	void FilterGridMotionDown(const Rect& r, int* dy);
	void FilterGridMotionLeft(const Rect& r, int* dy);
	void FilterGridMotionRight(const Rect& r, int* dy);
	void FilterGridMotionUp(const Rect& r, int* dy);

	bool IsTileColorEmpty(Color c);
	bool ComputeIsGridIndexEmpty(Bitmap gridElement, Color transColor, unsigned char solidThreshold);
public:
	GridLayer();
	void FilterGridMotion(const Rect& r, int* dx, int* dy);
	bool IsOnSolidGround(const Rect& r, spritestate_t state);
	bool SolidOnRight(const Rect& r);
	bool SolidOnLeft(const Rect& r);

	void UnsolidTile(int col, int row);
	void SolidTile(int col, int row);

	GridMap* GetBuffer(void);
	const GridMap* GetBuffer(void) const;
	//GridLayer(unsigned rows, unsigned cols);
};

void SetGridTile(GridMap* m, Dim col, Dim row, GridIndex index);

GridIndex GetGridTile(const GridMap* m, Dim col, Dim row);

void SetSolidGridTile(GridMap* m, Dim col, Dim row);

void SetEmptyGridTile(GridMap* m, Dim col, Dim row);

void SetGridTileFlags(GridMap* m, Dim col, Dim row, GridIndex flags);

void SetGridTileTopSolidOnly(GridMap* m, Dim col, Dim row);

bool CanPassGridTile(GridMap* m, Dim col, Dim row, GridIndex flags);