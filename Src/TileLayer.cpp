#include "../Include/TileLayer.h"
#include "../Include/Bitmap.h"

//Rect viewWin = app::Rect{ 0, 0, VIEW_WIN_X, VIEW_WIN_Y };
//Rect displayArea = app::Rect{ 0, 0, DISP_AREA_X, DISP_AREA_Y };

TileLayer::TileLayer(Dim _rows, Dim _cols, Bitmap _tileSet) {
	grid = new GridLayer();
	viewWin = Rect{ 0, 0, VIEW_WIN_X, VIEW_WIN_Y };
	totalRows = _rows;
	totalColumns = _cols;
	tileSet = _tileSet;
}

TileLayer::~TileLayer() {
	delete grid;
}

extern Rect displayArea;

void TileLayer::Allocate(void) {
	map = new Index[totalRows * totalColumns];
	int x = displayArea.w + TILE_WIDTH;
	int y = displayArea.h + TILE_HEIGHT;
	dpyBuffer = BitmapCreate(x, y);
}

void TileLayer::InitCaching(int width, int height) {
	int size = width * height;
	divIndex = new Index[size];
	modIndex = new Index[size];

	for (int i = 0; i < size; ++i) {
		divIndex[i] = MUL_TILE_HEIGHT(i / width); //y
		modIndex[i] = MUL_TILE_WIDTH(i % width); //x
	}
}

const Point TileLayer::Pick(Dim x, Dim y) const
{
	return { DIV_TILE_WIDTH(x + viewWin.x), DIV_TILE_HEIGHT(y + viewWin.y) };
}

const Rect& TileLayer::GetViewWindow(void) const { 
	return viewWin; 
}

void TileLayer::SetViewWindow(const Rect& r)
{
	viewWin = r;
	dpyChanged = true;
}

Bitmap TileLayer::GetBitmap(void) const { return dpyBuffer; }

int TileLayer::GetPixelWidth(void) const { return viewWin.w; }

int TileLayer::GetPixelHeight(void) const { return viewWin.h; }

unsigned TileLayer::GetTileWidth(void) const { return DIV_TILE_WIDTH(viewWin.w); }

unsigned TileLayer::GetTileHeight(void) const { return DIV_TILE_HEIGHT(viewWin.h); }

unsigned TileLayer::GetMapWidth(void) {
	return widthInTiles;
}

unsigned TileLayer::GetMapHeight(void) {
	return heightInTiles;
}

void TileLayer::SetMapDims(unsigned width, unsigned height){
	widthInTiles = width;
	heightInTiles = height;
}

void TileLayer::Save(const std::string& path) const
{
	WriteText(std::ifstream(path)).close();
}

std::ifstream TileLayer::WriteText(::ifstream fp) const
{
	//fprintf(fp, "%s", TileLayer::ToString().c_str());
	return fp;
}

void TileLayer::SetTile(Dim col, Dim row, Index index) {
	//map[row][col] = index;
	map[row * totalColumns + col] = index;
}

Index TileLayer::GetTile(Dim row, Dim col) const{
	//return map[row][col];
	return map[row * totalColumns + col];
}

bool TileLayer::CanScrollHoriz(int dx) const{
	return viewWin.x >= -dx && (viewWin.x + viewWin.w + dx) <= GetMapPixelWidth();
}

bool TileLayer::CanScrollVert(int dy) const{
	return viewWin.y >= -dy && (viewWin.y + viewWin.h + dy) <= GetMapPixelHeight();
}

void TileLayer::Scroll(int dx, int dy) {
	viewWin.x += dx;
	viewWin.y += dy;
	dpyChanged = true;
}

void TileLayer::FilterScrollDistance(
	int viewStartCoord,// x  or y
	int viewSize,// w  or h
	int* d,// dx or dy
	int maxMapSize// w or h
) {
	auto val = *d + viewStartCoord;
	if (val < 0)
		*d = viewStartCoord;
	else if ((val + viewSize) >= maxMapSize)
		*d = maxMapSize - (viewStartCoord + viewSize);
}

void TileLayer::FilterScroll(int* dx, int* dy) {
	FilterScrollDistance(viewWin.x, viewWin.w, dx, GetMapPixelWidth());
	FilterScrollDistance(viewWin.y, viewWin.h, dy, GetMapPixelHeight());
}

void TileLayer::ScrollWithBoundsCheck(int dx, int dy) {
	FilterScroll(&dx, &dy);
	Scroll(dx, dy);
}

void TileLayer::ScrollWithBoundsCheck(int* dx, int* dy) {
	FilterScroll(dx, dy);
	Scroll(*dx, *dy);
}

Dim TileLayer::TileXc(Index index) {
	return modIndex[index];
}

Dim TileLayer::TileYc(Index index) {
	return divIndex[index];
}

extern unsigned int total_tiles;
void TileLayer::PutTile(Bitmap dest, Dim x, Dim y, Bitmap tiles, Index tile) {
	//blit or not to blit.
	//I dont like this if statmenent because it is excecuted a lot of times in every game loop, but couldn't think of something else
	if (tile != total_tiles)
		BitmapBlit(tiles, Rect{ TileXc(tile), TileYc(tile), TILE_WIDTH, TILE_HEIGHT }, dest, Point{ x, y });
}

void TileLayer::TileTerrainDisplay(Bitmap dest, const Rect& displayArea) {
	if (dpyChanged) {
		//reset the buffer
		BitmapClear(dpyBuffer, app::Make32(0, 0, 0, 0));

		auto startCol = DIV_TILE_WIDTH(viewWin.x);
		auto startRow = DIV_TILE_HEIGHT(viewWin.y);
		auto endCol = DIV_TILE_WIDTH(viewWin.x + viewWin.w - 1);
		auto endRow = DIV_TILE_HEIGHT(viewWin.y + viewWin.h - 1);
		dpyX = MOD_TILE_WIDTH(viewWin.x);
		dpyY = MOD_TILE_HEIGHT(viewWin.y);
		dpyChanged = false;
		for (Dim row = startRow; row <= endRow; ++row) {
			for (Dim col = startCol; col <= endCol; ++col) {
				if(GetTile(row, col) == 0 || GetTile(row, col) == 4 || GetTile(row, col) == 26)
					PutTile(dpyBuffer, MUL_TILE_WIDTH(col - startCol), MUL_TILE_HEIGHT(row - startRow), tileSet, total_tiles);
				else
					PutTile(dpyBuffer, MUL_TILE_WIDTH(col - startCol), MUL_TILE_HEIGHT(row - startRow), tileSet, GetTile(row, col));
			}
		}
	}

	BitmapBlit(dpyBuffer, { dpyX, dpyY, viewWin.w, viewWin.h }, dest, { displayArea.x, displayArea.y });
}

//-----------GRID LAYER-----------------

void TileLayer::insertSolid(Index id) {
	solids.insert(id);
}

bool TileLayer::IsTileIndexAssumedEmpty(Index index) {
	if (solids.find(index) != solids.end()) //if it is in the list, then its solid (not empty)
		return false;
	return true;
}

//views the map (in the csv) and adds in the Grid map if we are allowed to go through it
void TileLayer::ComputeTileGridBlocks1() {

	GridIndex* grid_start = grid->GetBuffer()[0][0];
	for (auto row = 0; row < MAX_HEIGHT; ++row) {
		GridIndex* tmp2 = grid_start;
		for (auto col = 0; col < MAX_WIDTH; ++col) {
			GridIndex* tmp = grid_start;
			for (auto k = 0; k < GRID_ELEMENT_HEIGHT; ++k) {
				memset(grid_start, IsTileIndexAssumedEmpty(GetTile(row, col)) ? GRID_EMPTY_TILE : GRID_SOLID_TILE, GRID_ELEMENT_WIDTH);
				grid_start += GRID_MAX_WIDTH;
			}
			grid_start = tmp + GRID_ELEMENT_WIDTH;
		}
		grid_start = tmp2 + GRID_MAX_WIDTH * 4;
	}
}

void TileLayer::UnsolidTileGridBlock(int _col, int _row) {
	grid->UnsolidTile(MUL_GRID_ELEMENT_WIDTH(_col), MUL_GRID_ELEMENT_HEIGHT(_row));
}

void TileLayer::SolidTileGridBlock(int _col, int _row) {
	grid->SolidTile(MUL_GRID_ELEMENT_WIDTH(_col), MUL_GRID_ELEMENT_HEIGHT(_row));
}

GridLayer* TileLayer::GetGrid(void) const{
	return grid;
}

CircularBackground::CircularBackground(Bitmap _tileset, std::string filename) {
	viewWin = Rect{ 0, 0, VIEW_WIN_X, VIEW_WIN_Y };

	int width_in_tiles = DIV_TILE_WIDTH(BitmapGetWidth(_tileset));

	InitBuffer(_tileset, filename, width_in_tiles);
}

void CircularBackground::InitBuffer(Bitmap tileset, std::string filename, int width) {
	string line, token, delimiter = ",";
	size_t pos = 0;
	ifstream csvFile(filename);
	int x = 0, y = 0;

	Bitmap tmp = BitmapCreate(MUL_TILE_WIDTH(MAX_WIDTH), MUL_TILE_HEIGHT(MAX_HEIGHT));

	if (csvFile.is_open()) {
		while (getline(csvFile, line)) {
			x = 0;
			while ((pos = line.find(delimiter)) != string::npos) {
				token = line.substr(0, pos);
				stringstream ss(token);
				int val;
				ss >> val;

				if (val != -1) {
					BitmapBlit(tileset, Rect{ MUL_TILE_WIDTH(val % width), MUL_TILE_WIDTH(val / width), TILE_WIDTH, TILE_HEIGHT }, tmp, Point{ MUL_TILE_WIDTH(x), MUL_TILE_HEIGHT(y) });
					x++;
				}
				line.erase(0, pos + delimiter.length());
			}
			stringstream ss(line);
			int val;
			ss >> val;
			BitmapBlit(tileset, Rect{ MUL_TILE_WIDTH(val % width), MUL_TILE_HEIGHT(val / width), TILE_WIDTH, TILE_HEIGHT }, tmp, Point{ MUL_TILE_WIDTH(x), MUL_TILE_HEIGHT(y) });
			y++;
		}
		csvFile.close();
	}
	//transfer the buffer to the right bitmap with the right dimensions;

	int pixels_width = MUL_TILE_WIDTH(x);
	int pixels_height = MUL_TILE_HEIGHT(y);

	bg = BitmapCreate(pixels_width, pixels_height);

	BitmapBlit(tmp, Rect{ 0, 0, pixels_width, pixels_height }, bg, Point{ 0, 0 });

	BitmapDestroy(tmp);
}

void CircularBackground::Scroll(int dx) {
	viewWin.x += dx;
	if (viewWin.x < 0) {
		while (viewWin.x < 0)
			viewWin.x = BitmapGetWidth(bg) + viewWin.x;
	}
	else
		if (viewWin.x >= BitmapGetWidth(bg)) {
			while(viewWin.x >= BitmapGetWidth(bg))
				viewWin.x = viewWin.x - BitmapGetWidth(bg);
		}
			
}

void CircularBackground::Display(Bitmap dest, int x, int y) const {
	auto bg_w = BitmapGetWidth(bg);
	auto w1 = std::min(bg_w - viewWin.x, viewWin.w);
	BitmapBlit(bg, { viewWin.x, viewWin.y, w1, viewWin.h }, dest, { x, y });
	if (w1 < viewWin.w) { // not whole view win fits
		auto w2 = viewWin.w - w1; // the remaining part
		BitmapBlit(bg, { 0, viewWin.y, w2, viewWin.h }, dest, { x + w1, y });
	}
}
