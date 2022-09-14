#pragma once


#define TILE_WIDTH 16
#define TILE_HEIGHT 16

#define VIEW_WIN_X 640
#define VIEW_WIN_Y 480
#define DISP_AREA_X 640
#define DISP_AREA_Y 480

#define MUL_TILE_WIDTH(i) ((i) << 4)
#define MUL_TILE_HEIGHT(i) ((i) << 4)
#define DIV_TILE_WIDTH(i) ((i) >> 4)
#define DIV_TILE_HEIGHT(i) ((i) >> 4)
#define MOD_TILE_WIDTH(i) ((i) & 15)
#define MOD_TILE_HEIGHT(i) ((i) & 15)
#define MAX_WIDTH 1024
#define MAX_HEIGHT 256
#define TILEX_MASK 0xFF00
#define TILEX_SHIFT 8
#define TILEY_MASK 0x00FF

#define MAX_VIEWS 4
#define GRAVITY_G 2
#define POWERUPS_MOVE_SPEED 1

#define GRID_ELEMENT_WIDTH 4
#define GRID_ELEMENT_HEIGHT 4
#if TILE_WIDTH % GRID_ELEMENT_WIDTH != 0
#error "TILE_WIDTH % GRID_ELEMENT_WIDTH must be zero!"
#endif
#if TILE_HEIGHT % GRID_ELEMENT_HEIGHT != 0
#error "TILE_HEIGHT % GRID_ELEMENT_HEIGHT must be zero!"
#endif
#define GRID_BLOCK_COLUMNS (TILE_WIDTH / GRID_ELEMENT_WIDTH)
#define GRID_BLOCK_ROWS (TILE_HEIGHT / GRID_ELEMENT_HEIGHT)
#define GRID_ELEMENTS_PER_TILE (GRID_BLOCK_ROWS * GRID_BLOCK_COLUMNS)
#define GRID_MAX_HEIGHT (MAX_HEIGHT * GRID_BLOCK_ROWS)
#define GRID_MAX_WIDTH (MAX_WIDTH * GRID_BLOCK_COLUMNS)

#define GRID_THIN_AIR_MASK 0x0000 // element is ignored
#define GRID_LEFT_SOLID_MASK 0x0001 // bit 0
#define GRID_RIGHT_SOLID_MASK 0x0002 // bit 1
#define GRID_TOP_SOLID_MASK 0x0004 // bit 2
#define GRID_BOTTOM_SOLID_MASK 0x0008 // bit 3
#define GRID_GROUND_MASK 0x0010 // bit 4, keep objects top / bottom (gravity)
#define GRID_FLOATING_MASK 0x0020 // bit 5, keep objects anywhere inside (gravity)
#define GRID_EMPTY_TILE GRID_THIN_AIR_MASK
#define GRID_SOLID_TILE \
		(GRID_LEFT_SOLID_MASK | GRID_RIGHT_SOLID_MASK | GRID_TOP_SOLID_MASK | GRID_BOTTOM_SOLID_MASK)

#define MAX_PIXEL_WIDTH MUL_TILE_WIDTH(MAX_WIDTH)
#define MAX_PIXEL_HEIGHT MUL_TILE_HEIGHT(MAX_HEIGHT)
#define DIV_GRID_ELEMENT_WIDTH(i) ((i) >> 2)
#define DIV_GRID_ELEMENT_HEIGHT(i) ((i) >> 2)
#define MUL_GRID_ELEMENT_WIDTH(i) ((i) << 2)
#define MUL_GRID_ELEMENT_HEIGHT(i) ((i) << 2)

#define FRAME_DELAY 60