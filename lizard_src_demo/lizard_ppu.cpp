// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include "lizard_game.h"
#include "lizard_ppu.h"
#include "lizard_text.h"
#include "enums.h"

namespace PPU
{

#define SCANLINE_LIMIT 1

//
// internal data
//

static uint8 frame_buffer[256 * 240];
static uint8 bg_buffer[ELEMENTS_OF(frame_buffer)];
static uint16 vaddr;

static uint8 chr[512][8][8];
static uint8 nmt[30][64];
static uint8 att[30][64];
static uint8 oam[256];
static uint8 pal[32];

static bool fx_overlay_sprites;
static unsigned int fx_overlay;
static unsigned int fx_scroll;
static unsigned int fx_overlay_scroll_x;
static unsigned int fx_overlay_scroll_y;
static unsigned int fx_split_scroll_x;
static unsigned int fx_split_y;

static uint8 fx_store_nmt[30][32];
static uint8 fx_store_att[30][32];
static uint8 fx_store_chr[256][8][8];
static uint8 fx_store_pal[32];
static bool fx_store_overlay_sprites;
static unsigned int fx_store_overlay;
static unsigned int fx_store_scroll;
static unsigned int fx_store_overlay_scroll_x;
static unsigned int fx_store_overlay_scroll_y;
static unsigned int fx_store_split_scroll_x;
static unsigned int fx_store_split_y;


#if SCANLINE_LIMIT
static bool sprite_scanline[64][8];
static bool sprite_limit;
#endif

const int DEBUG_BOX_MAX = 32;
const int DEBUG_TEXT_MAX = 1024;
static unsigned int debug_box_count;
static uint16 debug_box_x0[DEBUG_BOX_MAX];
static uint8  debug_box_y0[DEBUG_BOX_MAX];
static uint16 debug_box_x1[DEBUG_BOX_MAX];
static uint8  debug_box_y1[DEBUG_BOX_MAX];
static uint8  debug_text_buffer[DEBUG_TEXT_MAX];
static uint8  debug_text2_buffer[DEBUG_TEXT_MAX];
static unsigned int debug_text_count;
static unsigned int debug_text2_count;

enum PPUDebugEnum {
	DEBUG_OFF = 0,
	DEBUG_PAL0,
	DEBUG_PAL1,
	DEBUG_PAL2,
	DEBUG_PAL3,
	DEBUG_MAX
};

static int debug;
static bool debug_sprites;
static bool debug_box_on;
static bool debug_collide_on;

//
// internal
//

void write_(uint8 v)
{
	switch (vaddr & 0x3000)
	{
		case 0x0000:
		case 0x1000:
			{
				unsigned int tile = (vaddr & 0x1FFF) >> 4; // 16 bytes per CHR tile
				unsigned int row = vaddr & 0x7;
				if ((vaddr & 0x8) == 0) // low bit plane
				{
					for (int x=0; x<8; ++x)
					{
						chr[tile][row][x] = (chr[tile][row][x] & 2) | ((v & 0x80) ? 1 : 0);
						v <<= 1;
					}
				}
				else // high bit plane
				{
					for (int x=0; x<8; ++x)
					{
						chr[tile][row][x] = (chr[tile][row][x] & 1) | ((v & 0x80) ? 2 : 0);
						v <<= 1;
					}
				}
			}
			break;
		case 0x2000:
			{
				unsigned int table = (vaddr & 0x400) >> 5; // nametable select (add 32 if second nametable)
				unsigned int naddr = (vaddr & 0x3FF); // horizontal mirroring
				unsigned int row = (naddr >> 5);
				if (row < 30)
				{
					unsigned int col = (naddr & 0x1F) + table;
					nmt[row][col] = v;
				}
				else // attribute
				{
					unsigned int aaddr = naddr - (32*30);
					unsigned int row = (aaddr >> 3) << 2;
					unsigned int col = ((aaddr & 0x7) << 2) + table;
					att[row+0][col+0] = v & 0x3;
					att[row+0][col+1] = v & 0x3;
					att[row+1][col+0] = v & 0x3;
					att[row+1][col+1] = v & 0x3;
					v >>= 2;
					att[row+0][col+2] = v & 0x3;
					att[row+0][col+3] = v & 0x3;
					att[row+1][col+2] = v & 0x3;
					att[row+1][col+3] = v & 0x3;
					if (row <= 26)
					{
						v >>= 2;
						att[row+2][col+0] = v & 0x3;
						att[row+2][col+1] = v & 0x3;
						att[row+3][col+0] = v & 0x3;
						att[row+3][col+1] = v & 0x3;
						v >>= 2;
						att[row+2][col+2] = v & 0x3;
						att[row+2][col+3] = v & 0x3;
						att[row+3][col+2] = v & 0x3;
						att[row+3][col+3] = v & 0x3;
					}
				}
			}
			break;
		case 0x3000:
			if ((vaddr & 0x0FFF) >= 0x0F00)
			{
				int paddr = vaddr & 0x1F;
				v = v & 63;

				pal[paddr] = v;

				if ((paddr & 0x03) == 0)
				{
					if (v == 0x0F) v = 0x0D; // fakes a unique black for the background for sprite masking

					// the NES mirrors the 0 entries in pairs $3F00/$3F10, $3F04/$3F14, etc.
					pal[paddr & 0x0F] = v;
					pal[paddr | 0x10] = v;

					// the NES stores 4 unique 0-entry values, but it only ever renders using $00.
					// since this program never tries to read the values back, it simplifies
					// the rendering implementation to pretend the unread entries follow $00.
					for (int i=1; i<8; ++i)
					{
						pal[(4*i)] = pal[0];
					}
				}
			}
			else // nametable mirror
			{
				unsigned int temp = vaddr;
				vaddr = vaddr - 0x1000;
				write_(v);
				vaddr = temp;
			}
			break;
	}
}

static void render_background(unsigned int xoff, unsigned int yoff, unsigned int split)
{
	unsigned int x_low_start  = xoff & 7;
	unsigned int x_high_start = (xoff >> 3) & 63;
	unsigned int y_low  = yoff & 7;
	unsigned int y_high = (yoff >> 3);

	if (y_high >= 30) y_high = 0;

	for (unsigned int ry=split; ry<240; ++ry)
	{
		if (ry == fx_split_y)
		{
			xoff = fx_split_scroll_x;
			x_low_start  = xoff & 7;
			x_high_start = (xoff >> 3) & 63;
		}

		unsigned int x_low  = x_low_start;
		unsigned int x_high = x_high_start;
		uint8 tile = nmt[y_high][x_high];
		uint8 tatt = att[y_high][x_high] << 2;

		for (unsigned int rx=0; rx<256; ++rx)
		{
			uint8 p = chr[tile][y_low][x_low];
			frame_buffer[(ry<<8)+rx] = pal[tatt+p];

			++x_low;
			if (x_low >= 8)
			{
				x_low = 0;
				x_high = (x_high+1) & 63;
				tile = nmt[y_high][x_high];
				tatt = att[y_high][x_high] << 2;
			}
		}
		++y_low;
		if (y_low >= 8)
		{
			y_low = 0;
			++y_high;
			if (y_high >= 30) y_high = 0;
			tile = nmt[y_high][x_high];
			tatt = att[y_high][x_high] << 2;
		}
	}
}

static void render_sprite(uint8 x, uint8 y, uint8 tile, uint8 tpal, bool flip_x, bool flip_y, bool bg
	#if SCANLINE_LIMIT
		, int sprite_index
	#endif
	)
{
	#if !SCANLINE_LIMIT
		#define sprite_scanline[sprite_index][sy] true
	#endif

	NES_ASSERT(y < 240, "render_sprite out of range!");

	const int xw = ((x+8) <= 256) ? 8 : (256 - x);
	const int yw = ((y+8) <= 240) ? 8 : (240 - y);
	const int x_start = flip_x ? 7 : 0;
	const int y_start = flip_y ? 7 : 0;
	const int x_inc = flip_x ? -1 : 1;
	const int y_inc = flip_y ? -1 : 1;

	tpal = (tpal << 2) + 16;

	if (!bg)
	{
		int ty = y_start;
		for (int sy=0; sy<yw; ++sy)
		{
			if (sprite_scanline[sprite_index][sy])
			{
				int tx = x_start;
				for (int sx=0; sx<xw; ++sx)
				{
					uint8 p = chr[tile+256][ty][tx];
					if (p)
					{
						const int index = ((y+sy)<<8)+(x+sx);
						frame_buffer[index] = pal[tpal+p];
					}
					tx += x_inc;
				}
			}
			ty += y_inc;
		}
	}
	else
	{
		int ty = y_start;
		for (int sy=0; sy<yw; ++sy)
		{
			if (sprite_scanline[sprite_index][sy])
			{
				int tx = x_start;
				for (int sx=0; sx<xw; ++sx)
				{
					uint8 p = chr[tile+256][ty][tx];
					const int index = ((y+sy)<<8)+(x+sx);

					// This is not completely correct test for background pixel, but good enough for this game:
					// if pal[0] is equal to other background palette entries, they would be mistaken for empty background).
					// This doesn't come up in Lizard; background priority sprites are rare, and this case is avoided.
					if (p)
					{
						if (bg_buffer[index] == pal[0])
						{
							// sprite not behind background
							frame_buffer[index] = pal[tpal+p];
						}
						else
						{
							// background masks the foreground
							frame_buffer[index] = bg_buffer[index];
						}
					}
					tx += x_inc;
				}
			}
			ty += y_inc;
		}
	}
}

static void render_sprites()
{
	// store a copy of the background before drawing sprites,
	// for background priority sprite masking.
	::memcpy(bg_buffer, frame_buffer, sizeof(frame_buffer));

	#if SCANLINE_LIMIT
	if (sprite_limit)
	{
		for (int i=0; i<64; ++i)
		{
			const uint8 y = oam[(i<<2)+0];
			if (y >= 239) continue; // skip offscreen sprites

			for (int scanline=0; scanline<8; ++scanline)
			{
				const int ty = int(y) + scanline;
				int overlap = 0;
				for (int j=0; j<i; ++j)
				{
					int oy = int(oam[(j<<2)+0]);
					if ((ty >= oy) && (ty < (oy+8)))
					{
						++overlap;
						if (overlap >= 8)
						{
							break;
						}
					}
				}
				sprite_scanline[i][scanline] = (overlap < 8);
			}
		}
	}
	else
	{
		for (int i=0; i<64; ++i)
		{
			for (int scanline=0; scanline<8; ++scanline)
			{
				sprite_scanline[i][scanline] = true;
			}
		}
	}
	#endif

	// sprites are rendered in backward order so low indexed sprites appear on top
	for (int s=63; s>=0; --s)
	{
		uint8 y    = oam[(s<<2)+0];
		uint8 tile = oam[(s<<2)+1];
		uint8 att  = oam[(s<<2)+2];
		uint8 x    = oam[(s<<2)+3];

		if (y < 239)
		{
			render_sprite(x,y+1,tile,att & 0x03, (att & 0x40) != 0, (att & 0x80) != 0, (att & 0x20) != 0
				#if SCANLINE_LIMIT
					,s
				#endif
				);
		}
	}
}

//
// public interface
//

void latch(uint16 address)
{
	vaddr = address;
}

void latch_at(uint8 x, uint8 y)
{
	latch(0x2000 + (32*y) + x);
}

void write(uint8 v)
{
	write_(v);
	++vaddr;
}

void write_vertical(uint8 v)
{
	write_(v);
	vaddr += 32;
}

void oam_dma(uint8* dma)
{
	memcpy(oam,dma,256);
}

void scroll_x(int scroll)
{
	fx_scroll = scroll & 0x1FF;
}

void split_x(int scroll_x, int split_y)
{
	// does a horizontal scroll split at the specified Y line

	fx_split_scroll_x = scroll_x & 0x1FF;
	fx_split_y = split_y;
}

void overlay_y(int overlay, bool sprites)
{
	// equivalent to:
	//   waiting for sprite 0 hit at overlay line
	//   turn off sprite rendering
	//   switch to nametable 2 with scroll 0,0

	fx_overlay = overlay;
	fx_overlay_sprites = sprites;
}

void overlay_scroll(int sx, int sy)
{
	overlay_scroll_x(sx);
	overlay_scroll_y(sy);
}

void overlay_scroll_x(int sx)
{
	fx_overlay_scroll_x = sx & 0x1FF;
}

void overlay_scroll_y(int sy)
{
	fx_overlay_scroll_y = sy & 0xFF;
}

void init()
{
	memset(frame_buffer,0,sizeof(frame_buffer));
	vaddr = 0;
	memset(chr,0,sizeof(chr));
	memset(nmt,0,sizeof(nmt));
	memset(att,0,sizeof(att));
	memset(oam,0xFF,sizeof(oam));
	memset(pal,0x0F,sizeof(pal));
	fx_overlay = 240;
	fx_overlay_sprites = false;
	fx_scroll = 0;
	fx_overlay_scroll_x = 0;
	fx_overlay_scroll_y = 0;
	memset(fx_store_nmt,0,sizeof(fx_store_nmt));
	memset(fx_store_att,0,sizeof(fx_store_att));
	debug = DEBUG_OFF;
	debug_sprites = false;
	debug_collide_on = false;
	debug_box_on = false;
	debug_box_count = 0;
	debug_text_count = 0;
	debug_text2_count = 0;
}

const uint8* render()
{
	// render top of background
	render_background(fx_scroll, 0, 0);

	// render sprites if blocked by overlay
	if (!fx_overlay_sprites) render_sprites();

	// render overlay
	render_background(fx_overlay_scroll_x, fx_overlay_scroll_y, fx_overlay);

	// render sprites if not blocked by overlay
	if (fx_overlay_sprites) render_sprites();

	// debug
	if (debug != DEBUG_OFF)
	{
		//memset(frame_buffer,0,sizeof(frame_buffer));
		switch (debug)
		{
		default:
			break;
		case DEBUG_PAL0:
		case DEBUG_PAL1:
		case DEBUG_PAL2:
		case DEBUG_PAL3:
			const int pal_select = (debug - DEBUG_PAL0) * 4;
			for (int y=0;y<128;++y)
			for (int x=0;x<256;++x)
			{
				int tile = ((x & 127)/8) + ((y/8)*16) + ((x & 128)?256:0);
				frame_buffer[(y*256)+x] = pal[pal_select + ((x&128)?16:0) + chr[tile][y&7][x&7]];

				//const uint8 DPAL[4] = { 0x0F, 0x16, 0x11, 0x30 }; // black red blue white
				//frame_buffer[(y*256)+x] = DPAL[chr[tile][y&7][x&7]];
			}
			for (int p=0;p<32;++p)
			{
				for (int y=0;y<8;++y)
				for (int x=0;x<8;++x)
				{
					frame_buffer[((y+128)*256)+(x+(p*8))] = pal[p];
				}
			}
			break;
		}
	}
	
	return frame_buffer;
}

void push_overlay()
{
	for (int y=0; y<30; ++y)
	for (int x=0; x<32; ++x)
	{
		fx_store_nmt[y][x] = nmt[y][x+32];
		fx_store_att[y][x] = att[y][x+32];
	}

	for (int c=0; c<256; ++c)
	for (int y=0; y<8; ++y)
	for (int x=0; x<8; ++x)
	{
		fx_store_chr[c][y][x] = chr[c][y][x];
	}

	for (int p=0; p<32; ++p)
	{
		fx_store_pal[p] = pal[p];
	}

	fx_store_overlay_sprites   = fx_overlay_sprites;
	fx_store_overlay           = fx_overlay;
	fx_store_scroll            = fx_scroll;
	fx_store_overlay_scroll_x  = fx_overlay_scroll_x;
	fx_store_overlay_scroll_y  = fx_overlay_scroll_y;
	fx_store_split_scroll_x    = fx_split_scroll_x;
	fx_store_split_y           = fx_split_y;
}

void pop_overlay()
{
	for (int y=0; y<30; ++y)
	for (int x=0; x<32; ++x)
	{
		nmt[y][x+32] = fx_store_nmt[y][x];
		att[y][x+32] = fx_store_att[y][x];
	}

	for (int c=0; c<256; ++c)
	for (int y=0; y<8; ++y)
	for (int x=0; x<8; ++x)
	{
		chr[c][y][x] = fx_store_chr[c][y][x];
	}

	for (int p=0; p<32; ++p)
	{
		pal[p] = fx_store_pal[p];
	}

	fx_overlay_sprites   = fx_store_overlay_sprites;
	fx_overlay           = fx_store_overlay;
	fx_scroll            = fx_store_scroll;
	fx_overlay_scroll_x  = fx_store_overlay_scroll_x;
	fx_overlay_scroll_y  = fx_store_overlay_scroll_y;
	fx_split_scroll_x    = fx_store_split_scroll_x;
	fx_split_y           = fx_store_split_y;
}

void set_sprite_limit(bool t)
{
	#if SCANLINE_LIMIT
		sprite_limit = t;
	#endif
}

void meta_cls()
{
	char space = Game::text_convert(' ');
	for (int y=0;y<30;++y)
	for (int x=0;x<32;++x)
	{
		nmt[y][x+32] = space;
		att[y][x+32] = 0x00;
	}
}

void meta_text(uint8 x, uint8 y, const char* text, uint8 attrib)
{
	if (y >= 30) return;
	attrib = attrib & 0x3;
	x = (x + 32) & 63;
	while (*text != 0)
	{
		char c = *text;
		c = Game::text_convert(c);
		nmt[y][x] = c;
		att[y][x] = attrib;
		++text;
		++x;
		if (x >= 64 || x == 32) return;
	}
}

void meta_palette(const uint8 *p)
{
	memcpy(pal,p,16);
}

void cycle_debug()
{
	debug = debug + 1;
	if (debug >= DEBUG_MAX) debug = DEBUG_OFF;
}

void cycle_debug_sprites()
{
	debug_sprites = !debug_sprites;
}

void cycle_debug_collide()
{
	debug_collide_on = !debug_collide_on;
}

void cycle_debug_box()
{
	debug_box_on = !debug_box_on;
}

void debug_off()
{
	debug = DEBUG_OFF;
}

void debug_cls()
{
	debug_box_count = 0;
	debug_text_count = 0;
}

void debug_cls2()
{
	debug_text2_count = 0;
}

void debug_box(uint16 x0, uint8 y0, uint16 x1, uint8 y1)
{
	if (debug_box_count >= DEBUG_BOX_MAX) return;

	debug_box_x0[debug_box_count] = x0;
	debug_box_y0[debug_box_count] = y0;
	debug_box_x1[debug_box_count] = x1;
	debug_box_y1[debug_box_count] = y1;
	
	++debug_box_count;
}

void debug_text(const char* msg, ...)
{
	if (debug_text_count >= DEBUG_TEXT_MAX) return;
	if (debug_text_count != 0)
	{
		debug_text_buffer[debug_text_count] = '\n';
		++debug_text_count;
	}
	
	static char msg_out[DEBUG_TEXT_MAX];

	va_list args;
	va_start( args, msg );
	vsnprintf( msg_out, sizeof(msg_out), msg, args );

	const char* c = msg_out;
	while (*c && debug_text_count < DEBUG_TEXT_MAX)
	{
		debug_text_buffer[debug_text_count] = Game::text_convert(*c);
		++debug_text_count;
		++c;
	}
}

void debug_text2(const char* msg, ...)
{
	if (debug_text2_count >= DEBUG_TEXT_MAX) return;
	if (debug_text2_count != 0)
	{
		debug_text2_buffer[debug_text2_count] = '\n';
		++debug_text2_count;
	}
	
	static char msg_out[DEBUG_TEXT_MAX];

	va_list args;
	va_start( args, msg );
	vsnprintf( msg_out, sizeof(msg_out), msg, args );

	const char* c = msg_out;
	while (*c && debug_text2_count < DEBUG_TEXT_MAX)
	{
		debug_text2_buffer[debug_text2_count] = Game::text_convert(*c);
		++debug_text2_count;
		++c;
	}
}

unsigned int debug_overlay_scroll = 0;

inline void debug_overlay_box(uint32* buffer, uint16 x0, uint8 y0, uint16 x1, uint8 y1, uint32 color)
{
	int dx0 = x0 - debug_overlay_scroll;
	int dx1 = x1 - debug_overlay_scroll;
	if (dx0 > 255) return;
	if (dx1 < 0  ) return;		
	if (dx0 < 0  ) dx0 = 0;
	if (dx1 > 255) dx1 = 255;

	for (uint8 dy = y0; dy <= y1; ++dy)
	{
		if (dy >= 240) return;
		uint32* row = buffer + (dy * 256);

		for (int dx = dx0; dx <= dx1; ++dx)
		{
			row[dx] ^= color;
		}
	}
}

void debug_overlay(uint32* buffer)
{
	debug_overlay_scroll = fx_scroll;
	if (Game::h.dog_type[Game::RIVER_SLOT] == Game::DOG_RIVER || Game::h.dog_type[0] == Game::DOG_FROB)
	{
		debug_overlay_scroll = 0;
	}

	// draw collision boxes as XOR FF
	if (debug_collide_on)
	{
		for (int y=0; y<30; ++y)
		for (int x=0; x<64; ++x)
		{
			const uint16 x8 = x * 8;
			const uint8  y8 = y * 8;
			if (!Game::collide_tile(x8,y8)) continue;
			debug_overlay_box(buffer,x8,y8,x8+7,y8+7,0x00FFFFFF);
		}

		for (int b=0; b<4; ++b)
		{
			debug_overlay_box(buffer,Game::h_blocker_x0[b],Game::h.blocker_y0[b],Game::h_blocker_x1[b],Game::h.blocker_y1[b],0x00FF0000);
		}
	}

	// draw debug boxes as XOR
	if (debug_box_on)
	{
		for (unsigned int i=0; i<debug_box_count; ++i)
		{
			//unsigned int color = 0x00FFFFFF;
			// use random color
			uint32 color = 0x003F3F3F | ( rand() ^ (rand() << 16) );
			debug_overlay_box(buffer,debug_box_x0[i],debug_box_y0[i],debug_box_x1[i],debug_box_y1[i],color);
		}
	}

	// draw sprites as XOR
	if (debug_sprites)
	{
		for (unsigned int i=0; i<64; ++i)
		{
			//unsigned int color = 0x00FFFFFF;
			// use random color
			uint32 color = 0x003F3F3F | ( rand() ^ (rand() << 16) );

			uint8 sy = oam[(i*4)+0] + 1;
			uint8 sx = oam[(i*4)+3];
			if (sy == 0) continue;
			uint16 dsx = uint16(sx) + debug_overlay_scroll;
			debug_overlay_box(buffer,dsx+0,sy+0,dsx+7,sy+0,color);
			debug_overlay_box(buffer,dsx+0,sy+7,dsx+7,sy+7,color);
			debug_overlay_box(buffer,dsx+0,sy+0,dsx+0,sy+7,color);
			debug_overlay_box(buffer,dsx+7,sy+0,dsx+7,sy+7,color);
		}
	}

	// draw text
	uint8 tx = 3;
	uint8 ty = 3;

	for (unsigned int i=0; i<debug_text2_count; ++i)
	{
		const uint8 c = debug_text2_buffer[i];
		if (c == '\n')
		{
			tx = 3;
			ty += 8;
			continue;
		}

		uint16 p = tx + (ty * 256);
		for (int y=0; y<8; ++y)
		{
			for (int x=0; x<8; ++x)
			{
				if (chr[c][y][x])
				{
					buffer[(p+x)&0xFFFF] = 0x00FFFF00;
				}
			}
			p += 256;
		}
		tx += 8;
	}
	if (debug_text2_count > 0)
	{
		tx = 3;
		ty += 8;
	}

	for (unsigned int i=0; i<debug_text_count; ++i)
	{
		const uint8 c = debug_text_buffer[i];
		if (c == '\n')
		{
			tx = 3;
			ty += 8;
			continue;
		}

		uint16 p = tx + (ty * 256);
		for (int y=0; y<8; ++y)
		{
			for (int x=0; x<8; ++x)
			{
				if (chr[c][y][x])
				{
					buffer[(p+x)&0xFFFF] = 0x00FFFF00;
				}
			}
			p += 256;
		}
		tx += 8;
	}
}

}; // namespace PPU

// end of file
