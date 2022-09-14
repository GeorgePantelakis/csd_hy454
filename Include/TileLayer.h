#pragma once
#include "Typedefs.h"
#include "Defines.h"
#include <string>
#include <set>
#include "GridLayer.h"

class GridLayer;

class TileLayer {
	private:
		Index* map = nullptr;
		GridLayer* grid = nullptr;
		Dim totalRows = 0, totalColumns = 0;
		Dim widthInTiles = 0, heightInTiles = 0;
		Bitmap tileSet = nullptr;
		Rect viewWin;
		Bitmap dpyBuffer = nullptr;
		bool dpyChanged = true;
		Dim dpyX = 0, dpyY = 0;
		std::set<Index> solids; // holds the ids of the solid tiles

		/*Pre caching*/
		Index* divIndex = nullptr;
		Index* modIndex = nullptr;
		Dim TileXc(Index index);
		Dim TileYc(Index index);

		void FilterScrollDistance(
			int viewStartCoord,// x  or y
			int viewSize,// w  or h
			int* d,// dx or dy
			int maxMapSize// w or h
		);
		void FilterScroll(int* dx, int* dy);
		void PutTile(Bitmap dest, Dim x, Dim y, Bitmap tiles, Index tile);
		bool IsTileIndexAssumedEmpty(Index index);
	public:
		void Allocate(void);
		void InitCaching(int width, int height);

		void insertSolid(Index id);
		void SetTile(Dim col, Dim row, Index index);
		Index GetTile(Dim col, Dim row) const;
		void ComputeTileGridBlocks1(void);
		void UnsolidTileGridBlock(int col, int row);
		void SolidTileGridBlock(int col, int row);
		GridLayer* GetGrid(void) const;
		const Point Pick(Dim x, Dim y) const;

		const Rect& GetViewWindow(void) const;
		void SetViewWindow(const Rect& r);

		void TileTerrainDisplay(Bitmap dest, const Rect& displayArea);
		Bitmap GetBitmap(void) const;
		int GetPixelWidth(void) const;
		int GetPixelHeight(void) const;
		unsigned GetTileWidth(void) const;
		unsigned GetTileHeight(void) const;

		unsigned GetMapWidth(void);
		unsigned GetMapHeight(void);
		void SetMapDims(unsigned, unsigned);

		void Scroll(int dx, int dy);
		bool CanScrollHoriz(int dx) const;
		bool CanScrollVert(int dy) const;
		void ScrollWithBoundsCheck(int dx, int dy);
		void ScrollWithBoundsCheck(int *dx, int *dy);

		auto ToString(void) const -> const std::string; // unparse
		bool FromString(const std::string&); // parse
		void Save(const std::string& path) const;
		 
		bool Load(const std::string& path);
		std::ifstream WriteText(std::ifstream fp) const;
		bool ReadText(FILE* fp); // TODO: carefull generic parsing
		TileLayer(Dim _rows, Dim _cols, Bitmap _tileSet);
		~TileLayer(); // cleanup here with care!
};

class CircularBackground { // horizontal stripe
	private:
		Rect viewWin;
		Bitmap bg = nullptr; //buffer
		void InitBuffer(Bitmap tileset, std::string filename, int width);
	public:
		void Scroll(int dx);
		void Display(Bitmap dest, int x, int y) const;
		CircularBackground(Bitmap tileset, std::string filename);
};