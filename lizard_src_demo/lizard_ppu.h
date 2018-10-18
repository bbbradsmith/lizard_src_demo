#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

//
// PPU - simplified version of NES PPU
//

namespace PPU
{
	// low level PPU function
	void latch(uint16 address);
	void latch_at(uint8 x, uint8 y);
	void write(uint8 v);
	void write_vertical(uint8 v);
	void oam_dma(uint8* dma);

	// abstracted PPU function
	void scroll_x(int scroll); // horizontal scroll with vertical mirroring
	void split_x(int scroll_x, int split_y); // horiontal scroll split at Y row
	void overlay_y(int overlay, bool sprites); // overlays second nametable over bottom of screen
	void overlay_scroll(int fx, int sy); // x/y scroll at overlay split
	void overlay_scroll_x(int sx); // partial version of overlay_scroll
	void overlay_scroll_y(int sy); // partial version of overlay_scroll

	void init();

	// render output (returns 256*240 palette indices)
	const uint8* render();

	// clears framebuffer until next render
	void blank();

	// for options screen
	void push_overlay(); // saves current second nametable
	void pop_overlay(); // restores current second nametable
	void set_sprite_limit(bool t);

	// system functions for creating text on second nametable
	void meta_cls(); // clears nametable 1
	void meta_text(uint8 x, uint8 y, const char* text, uint8 attrib); // x>=32 goes on nametable 0
	void meta_palette(const uint8* p);

	// for debugging
	void cycle_debug();
	void cycle_debug_sprites();
	void cycle_debug_collide();
	void cycle_debug_box();
	void debug_off();
	void debug_cls(); // used by game tick
	void debug_cls2(); // used by main
	void debug_box(uint16 x0, uint8 y0, uint16 x1, uint8 y1); // used by game tick
	void debug_text(const char* msg, ...); // used by game tick
	void debug_text2(const char* msg, ...); // used by main
	void debug_overlay(uint32* buffer);
};

// end of file
