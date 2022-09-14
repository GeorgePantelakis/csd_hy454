#include "../Include/GridLayer.h"
#include <set>
#include <iostream>

//using namespace app;
//GridMap grid;
//static TileColorsHolder emptyTileColors;

GridLayer::GridLayer() {
	//do nothing
}


void GridLayer::Allocate(void) {
	//grid = new GridIndex[total = totalRows * totalColumns];
	memset(grid, GRID_EMPTY_TILE, GRID_MAX_HEIGHT * GRID_MAX_WIDTH);
}


bool GridLayer::SolidOnRight(const Rect& r) {
	int dx = 1; // right 1 pixel

	FilterGridMotionRight(r, &dx);

	return dx == 0; 
}

bool GridLayer::SolidOnLeft(const Rect& r) {
	int dx = -1; // left 1 pixel
	FilterGridMotionLeft(r, &dx);

	return dx == 0;
}

bool GridLayer::IsOnSolidGround(const Rect& r,spritestate_t state) { // will need later for gravity
	int dy = 1; // down 1 pixel
	if (state == RUNNING_STATE)
		FilterGridMotionDown(Rect{ r.x - 1, r.y, r.w + 2, r.h }, &dy); //if running, then if i can stand a pixle left or right dont fall
	else
		FilterGridMotionDown(r, &dy);

	return dy == 0; // if true IS attached to solid ground
}

void GridLayer::UnsolidTile(int col, int row) {
	GridIndex* grid_start = &(grid[row][col]);
	for (auto k = 0; k < GRID_ELEMENT_HEIGHT; ++k) {
		memset(grid_start, GRID_EMPTY_TILE, GRID_ELEMENT_WIDTH);
		grid_start += GRID_MAX_WIDTH;
	}
}

void GridLayer::SolidTile(int col, int row) {
	GridIndex* grid_start = &(grid[row][col]);
	for (auto k = 0; k < GRID_ELEMENT_HEIGHT; ++k) {
		memset(grid_start, GRID_SOLID_TILE, GRID_ELEMENT_WIDTH);
		grid_start += GRID_MAX_WIDTH;
	}
}

GridMap* GridLayer::GetBuffer(void) { return &grid; }

const GridMap* GridLayer::GetBuffer(void) const { return &grid; }


void SetGridTile(GridMap* m, Dim col, Dim row, GridIndex index) {
	(*m)[row][col] = index;
}

GridIndex GetGridTile(const GridMap* m, Dim col, Dim row) {
	//return(*m)[MUL_GRID_ELEMENT_HEIGHT(row)][MUL_GRID_ELEMENT_WIDTH(col)];
	return(*m)[row][col];
}

void SetSolidGridTile(GridMap* m, Dim col, Dim row) {
	SetGridTile(m, col, row, GRID_SOLID_TILE);
}

void SetEmptyGridTile(GridMap* m, Dim col, Dim row) {
	SetGridTile(m, col, row, GRID_EMPTY_TILE);
}

void SetGridTileFlags(GridMap* m, Dim col, Dim row, GridIndex flags) {
	SetGridTile(m, col, row, flags);
}

void SetGridTileTopSolidOnly(GridMap* m, Dim col, Dim row) {
	SetGridTileFlags(m, row, col, GRID_TOP_SOLID_MASK);
}

bool CanPassGridTile(GridMap* m, Dim col, Dim row, GridIndex flags) {
	//cout << (int)GetGridTile(m, row, col) << endl;
	return (GetGridTile(m, col, row) & flags) == 0;
}

void GridLayer::FilterGridMotion(const Rect& r, int* dx, int* dy){
	//assert(abs(*dx) <= GRID_ELEMENT_WIDTH && abs(*dy) <= GRID_ELEMENT_HEIGHT);

	// try horizontal move
	if (*dx < 0)
		FilterGridMotionLeft(r, dx);
	else if (*dx > 0)
		FilterGridMotionRight(r, dx);

	// try vertical move
	if (*dy < 0)
		FilterGridMotionUp(r, dy);
	else if (*dy > 0)
		FilterGridMotionDown(r, dy);
}

void GridLayer::FilterGridMotionLeft(const Rect& r, int* dx){
	auto x1_next = r.x + *dx;
	if (x1_next < 0)
		*dx = -r.x;
	else {
		auto newCol = DIV_GRID_ELEMENT_WIDTH(x1_next);
		auto currCol = DIV_GRID_ELEMENT_WIDTH(r.x);
		if (newCol != currCol) {
			assert(newCol < currCol); // we really move left
			auto startRow = DIV_GRID_ELEMENT_HEIGHT(r.y);
			auto endRow = DIV_GRID_ELEMENT_HEIGHT(r.y + r.h - 1);
			for (auto row = startRow; row <= endRow; ++row) {
				if (!CanPassGridTile(&grid, newCol, row, GRID_RIGHT_SOLID_MASK)) {
					*dx = MUL_GRID_ELEMENT_WIDTH(currCol) - r.x;
					break;
				}
			}
		}
	}
}

void GridLayer::FilterGridMotionRight(const Rect& r, int* dx) {
	auto x2 = r.x + r.w - 1;
	auto x2_next = x2 + *dx;
	if (x2_next >= MAX_PIXEL_WIDTH)
		*dx = MAX_PIXEL_WIDTH - x2;
	else {
		auto newCol = DIV_GRID_ELEMENT_WIDTH(x2_next);
		auto currCol = DIV_GRID_ELEMENT_WIDTH(x2);
		if (newCol != currCol) {
			assert(newCol > currCol); // we really move right
			auto startRow = DIV_GRID_ELEMENT_HEIGHT(r.y);
			auto endRow = DIV_GRID_ELEMENT_HEIGHT(r.y + r.h - 1);
			for (auto row = startRow; row <= endRow; ++row)
				if (!CanPassGridTile(&grid, newCol, row, GRID_LEFT_SOLID_MASK)) {
					*dx = MUL_GRID_ELEMENT_WIDTH(newCol) - (x2 + 1);
					break;
				}
		}
	}
}

void GridLayer::FilterGridMotionUp(const Rect& r, int* dy) {
	auto y1_next = r.y + *dy;
	if (y1_next < 0)
		*dy = -r.y;
	else {
		auto newRow = DIV_GRID_ELEMENT_HEIGHT(y1_next);
		auto currRow = DIV_GRID_ELEMENT_HEIGHT(r.y);
		if (newRow != currRow) {
			assert(newRow < currRow); // we really move up
			auto startCol = DIV_GRID_ELEMENT_WIDTH(r.x);
			auto endCol = DIV_GRID_ELEMENT_WIDTH(r.x + r.w - 1);
			for (auto col = startCol; col <= endCol; ++col) {
				for (auto row = currRow; row >= newRow; --row) {
					if (!CanPassGridTile(&grid, col, row, GRID_TOP_SOLID_MASK)) {
						*dy = MUL_GRID_ELEMENT_WIDTH(currRow) - r.y;
						break;
					}
				}
				
			}
		}
	}
}

void GridLayer::FilterGridMotionDown(const Rect& r, int* dy) {
	auto y2 = r.y + r.h - 1;
	auto y2_next = y2 + *dy;
	if (y2_next >= MAX_PIXEL_HEIGHT)
		*dy = MAX_PIXEL_HEIGHT - y2;
	else {
		auto newRow = DIV_GRID_ELEMENT_HEIGHT(y2_next);
		auto currRow = DIV_GRID_ELEMENT_HEIGHT(y2);
		if (newRow != currRow) {
			assert(newRow > currRow); // we really move down
			auto startCol = DIV_GRID_ELEMENT_WIDTH(r.x);
			auto endCol = DIV_GRID_ELEMENT_WIDTH(r.x + r.w - 1);
			for (auto col = startCol; col <= endCol; ++col)
				for (auto row = currRow; row <= newRow; ++row) {
					if (!CanPassGridTile(&grid, col, row, GRID_BOTTOM_SOLID_MASK)) {
						*dy = MUL_GRID_ELEMENT_HEIGHT(row) - (y2 + 1);
						break;
					}
				}
		}
	}
}

/*void GridLayer::ComputeTileGridBlocks1(class TileLayer *tilelayer) {
	GridIndex* grid_start = &grid[0][0];
	for (auto row = 0; row < MAX_HEIGHT; ++row) {
		GridIndex* tmp2 = grid_start;
		for (auto col = 0; col < MAX_WIDTH; ++col) {
			GridIndex* tmp = grid_start;
			for (auto k = 0; k < GRID_ELEMENT_HEIGHT; ++k) {
				memset(grid_start, IsTileIndexAssumedEmpty(tilelayer->GetTile(row, col)) ? GRID_EMPTY_TILE : GRID_SOLID_TILE, GRID_ELEMENT_WIDTH);
				grid_start += GRID_MAX_WIDTH;
			}
			grid_start = tmp + GRID_ELEMENT_WIDTH;
			//memset(grid, IsTileIndexAssumedEmpty(GetTile(map, row, col)) ? GRID_EMPTY_TILE : GRID_SOLID_TILE, GRID_ELEMENTS_PER_TILE);
			//grid += GRID_ELEMENTS_PER_TILE;
		}
		//grid += GRID_MAX_WIDTH * 3;
		grid_start = tmp2 + GRID_MAX_WIDTH * 4;
	}
}

bool GridLayer::IsTileColorEmpty(Color c) {
	return emptyTileColors.In(c);
}

void ComputeTileGridBlocks2(const TileMap* map, GridIndex* grid, Bitmap tileSet, Color transColor, unsigned char solidThreshold) {
	auto tileElem = BitmapCreate(TILE_WIDTH, TILE_HEIGHT);
	auto gridElem = BitmapCreate(GRID_ELEMENT_WIDTH, GRID_ELEMENT_HEIGHT);
	for (auto row = 0; row < MAX_HEIGHT; ++row)
		for (auto col = 0; col < MAX_WIDTH; ++col) {
			auto index = GetTile(map, col, row);
			PutTile(tileElem, 0, 0, tileSet, index);
			if (IsTileIndexAssumedEmpty(index)) {
				emptyTileColors.Insert(tileElem, index);// assume tile colors to be empty
				memset(grid, GRID_EMPTY_TILE, GRID_ELEMENTS_PER_TILE);
				grid += GRID_ELEMENTS_PER_TILE;
			}
			else
				ComputeGridBlock(grid, index, tileElem, gridElem, tileSet, transColor, solidThreshold);
		}
	BitmapDestroy(tileElem);
	BitmapDestroy(gridElem);
}

void ComputeGridBlock(GridIndex*& grid, Index index, Bitmap tileElem, Bitmap gridElem, Bitmap tileSet, Color transColor, unsigned char solidThreshold) {
	for (auto i = 0; i < GRID_ELEMENTS_PER_TILE; ++i) {
		auto x = i % GRID_BLOCK_ROWS;
		auto y = i / GRID_BLOCK_ROWS;
		BitmapBlit(tileElem, { x * GRID_ELEMENT_WIDTH, y * GRID_ELEMENT_HEIGHT, GRID_ELEMENT_WIDTH, GRID_ELEMENT_HEIGHT }, gridElem, { 0, 0 });
		auto isEmpty = ComputeIsGridIndexEmpty(tileSet, transColor, solidThreshold);
		*grid++ = isEmpty ? GRID_EMPTY_TILE : GRID_SOLID_TILE;
	}
}

bool GridLayer::ComputeIsGridIndexEmpty(Bitmap gridElement, Color transColor, unsigned char solidThreshold) {
	auto n = 0;
	BitmapAccessPixels(gridElement, [transColor, &n](unsigned char* mem) {
		auto c = GetPixel32(mem);
		if (c.r != transColor.r && c.g != transColor.g && c.b != transColor.b && !IsTileColorEmpty(c))
			++n;
		});
	return n <= solidThreshold;
}
*/