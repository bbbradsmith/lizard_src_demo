// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "lizard_game.h"
#include "lizard_text.h"
#include "enums.h"

#include <vector>
#include <algorithm>

using namespace Game;

// font set
//
//     ASCII                tileset
//     $0123456789ABCDEF    $0123456789ABCDEF
//      ----------------     ----------------
// $4X  @ABCDEFGHIJKLMNO  =  @ABCDEFGHIJKLMNO
// $5X  PQRSTUVWXYZ[\]^_  =  PQRTSUVWXYZ!/.:'
// $6X  `abcdefghijklmno  =  0123456789?,**-*
// $7X  pqrstuvwxyz{|}~   =   ***************
//
// $24  $  =  newline

// $B0-FF specify characters directly (after & with $7F)
//   $FD-FF start

// 0x80-AF special characters, usable only on particular screens
// void boss (cat)
//   $80-$9C a japanese sentence
// info, super moose, options
//   $A0 up
//   $A1 down
//   $A2 left
//   $A3 right
//   $A4 B
//   $A5 A
//   $A6-A8 select
//   $A9 option selector
// ending:
//   $AA bullet point
// start:
//   $AB left
//   $AC right

const char ascii_table[129] =
	"llllllllllllllll"
	"llllllllllllllll"
	"p[_llll_llllkn]\\"
	"`abcdefghi^^lnlj"
	"@ABCDEFGHIJKLMNO"
	"PQRSTUVWXYZl\\llp"
	"_ABCDEFGHIJKLMNO"
	"PQRSTUVWXYZlllnl";

const uint8 special_table[48] =
{
	0x0D, // $80 シ
	0x0E, // $81 ル
	0x0F, // $82 エ
	0x1D, // $83 ッ
	0x1E, // $84 ト
	0x1F, // $85 や
	0x2A, // $86 か
	0x2B, // $87 け
	0x2C, // $88 ゛
	0x2D, // $89 く
	0x2E, // $8A め
	0x2F, // $8B い
	0x3B, // $8C を
	0x3C, // $8D み
	0x3D, // $8E て
	0x3E, // $8F る
	0x89, // $90 。
	0x8A, // $91 も
	0x96, // $92 う
	0x9A, // $93 ん
	0xA9, // $94 ごく (1 of 2)
	0xAA, // $95 ごく (2 of 2)
	0xB9, // $96 の
	0xBA, // $97 じ
	0xBB, // $98 ゆ
	0xBC, // $99 だん (1 of 2)
	0xBD, // $9A だん (2 of 2)
	0xBE, // $9B は
	0xBF, // $9C な
	0x6C, // $9D
	0x6C, // $9E
	0x6C, // $9F
	0xDD, // $A0 up
	0xED, // $A1 down
	0xEE, // $A2 left
	0xEF, // $A3 right
	0xDE, // $A4 B
	0xDF, // $A5 A
	0xFD, // $A6 select (1 of 3)
	0xFE, // $A7 select (2 of 3)
	0xFF, // $A8 select (3 of 3)
	0xCF, // $A9 option selector
	0x27, // $AA bullet point (ending)
	0x0E, // $AB left (start)
	0x0F, // $AC right (start)
	0x6C, // $AD
	0x6C, // $AE
	0x6C, // $AF
};

namespace Game
{

extern const char* const text_table_default[TEXT_COUNT_META];
extern const uint8 text_glyphs_default[];

// localization / translation language selection

// Each language has a unique ID
// 

struct Language {
	uint8 id;
	const char* name;
	const char* table[TEXT_COUNT_META];
	const uint8* glyphs;
	bool operator<(const Language& x) const { return string_less(name,x.name); }
};

static unsigned int language = 0;
static unsigned int language_id = 0;
std::vector<Language> languages;

const char* const * text_table = text_table_default;
const uint8* text_glyphs = text_glyphs_default;

static void text_language(unsigned int l)
{
	if (l < languages.size())
	{
		language = l;
		language_id = languages[l].id;
		text_table = languages[l].table;
		text_glyphs = languages[l].glyphs;
		NES_DEBUG("text_language: %s\n",languages[l].name);
	}
	else
	{
		language = 0;
		language_id = 0;
		text_table = text_table_default;
		text_glyphs = text_glyphs_default;
		NES_DEBUG("text_language: default\n");
	}
}

void text_language_by_id(uint8 id)
{
	for (unsigned int i=0; i<languages.size(); ++i)
	{
		if (languages[i].id == id)
		{
			return text_language(i);
		}
	}
	return text_language(0);
}

void text_language_advance(bool forward)
{
	unsigned int l = language;
	if (forward)
	{
		++l;
		if (l >= languages.size()) l = 0;
	}
	else
	{
		if (l > 0) --l;
		else
		{
			l = languages.size() - 1;
		}
	}
	text_language(l);
}

uint8 text_language_id()
{
	return language_id;
}

unsigned int text_language_count()
{
	return languages.size();
}

void text_language_init()
{
	languages.clear();
	Language l;
	l.id = 0;
	l.name = text_table_default[TEXT_OPT_CURRENT_LANGUAGE];
	for (int i=0; i<TEXT_COUNT_META; ++i)
	{
		l.table[i] = text_table_default[i];
	}
	l.glyphs = text_glyphs_default;
	languages.push_back(l);
}

bool text_language_add(const char* bin, unsigned int size)
{
	Language l;

	if (size < 1)
	{
		NES_DEBUG("text_language_add: file too small for ID\n");
		return false;
	}

	l.id = uint8(bin[0]);
	--size;
	++bin;

	for (unsigned int i=0; i<languages.size(); ++i)
	{
		if (languages[i].id == l.id)
		{
			NES_DEBUG("text_language_add: duplicate ID %d\n",l.id);
			return false;
		}
	}

	for (int i=0; i<TEXT_COUNT_META; ++i)
	{
		l.table[i] = bin;
		if (size <= 0)
		{
			NES_DEBUG("text_language_add: string %d expected, EOF reached\n",i);
			return false;
		}
		while (size > 0 && *bin != 0)
		{
			--size;
			++bin;
		}
		if (size < 1 || *bin != 0)
		{
			NES_DEBUG("text_language_add: EOF reached before end of string %d\n",i);
			return false;
		}
		--size;
		++bin;
	}

	if (size < 1)
	{
		NES_DEBUG("text_language_add: no glyphs in file\n")
		return false;
	}
	l.glyphs = reinterpret_cast<const uint8*>(bin);
	if (((size-1) % 9) != 0)
	{
		NES_DEBUG("text_language_add: unexpected amount of glyph data (%d bytes)\n", size);
		return false;
	}
	if (l.glyphs[size-1] != 0)
	{
		NES_DEBUG("text_language_add: glyph set missing terminal 0\n");
		return false;
	}

	l.name = l.table[TEXT_OPT_CURRENT_LANGUAGE];
	if (string_len(l.name) < 1)
	{
		NES_DEBUG("text_language_add: no language name\n");
		return false;
	}

	for (unsigned int i=0; i<languages.size(); ++i)
	{
		const char* na = l.name;
		const char* nb = languages[i].name;

		if (!string_less(na,nb) && !string_less(nb,na))
		{
			NES_DEBUG("text_language_add: duplicate name %s\n",na);
			return false;
		}
	}

	languages.push_back(l);
	NES_DEBUG("text_language_add: %s\n",l.name);
	return true;
}

void text_language_sort()
{
	std::sort(languages.begin(),languages.end());

	#ifdef _DEBUG
		NES_DEBUG("text_language_sort:\n");
		for (unsigned int i=0; i<languages.size(); ++i)
		{
			NES_DEBUG("  %d: %d = %s\n",i,languages[i].id,languages[i].name);
		}
		NES_DEBUG("  %d languages.\n",languages.size());
	#endif
}

// game text interface

static const char* text_ptr = ""; // internally used by text_continue

void text_init()
{
	text_ptr = "";
}

char text_convert(char c)
{
	uint8 a = uint8(c);

	// 00-7F = use ascii table
	if (a < 0x80) return ascii_table[a];

	// 80-AF = use special table
	if (a < 0xB0) return special_table[a & 0x7F];

	// B0-FF = directly select tile from lower half
	return (a & 0x7F);
}

void text_confound(uint8 seed)
{
	seed = ((seed + 13) & 15) + 1; // simple rotation, seed=15 produces rot13
	const char a = text_convert('A');
	const char z = text_convert('Z');
	for (int i=0; i<32; ++i)
	{
		char c = stack.nmi_update[i];

		// convert special characters to A-Z
		if (c >= 0x74 && c <= 0x7C)
		{
			c = (c - 0x74) + a;
		}
		else if (c == 0x40)
		{
			c = z;
		}
		else if (c == 0x6D)
		{
			c = z-1;
		}

		if ((c >= a && c <= z))
		{
			c += seed;
			while (c >= (z+1)) c -= (z+1-a);
		}
		stack.nmi_update[i] = c;
	}
}

const uint8* text_get_glyphs()
{
	return text_glyphs;
}

const char* text_get(unsigned int t)
{
	NES_ASSERT(t < TEXT_COUNT_META, "text_get() index out of range!");
	return text_table[t];
}

void text_load(uint8 t)
{
	text_start(t);
	text_continue();
}

void text_load_meta(unsigned int t)
{
	NES_ASSERT(t < TEXT_COUNT_META, "text_load_meta() index out of range!");
	text_ptr = text_get(t);
	h.text_more = 1;
	text_continue();
}

void text_start(uint8 t)
{
	NES_ASSERT(t < TEXT_COUNT, "text_load() index out of range!");
	text_ptr = text_get(t);
	h.text_ptr = 0x8000 + (32 * t); // fake text_ptr to simulate NES
	h.text_more = 1;
}

void text_continue()
{
	h.text_more = 0;

	uint8 x;
	char c;
	for (x=0; x<32; ++x)
	{
		c = text_ptr[0];
		if (c == 0) break; // end of string

		++text_ptr;
		++h.text_ptr;

		if (c == '$') // new line
		{
			h.text_more = 1;
			break;
		}

		stack.nmi_update[x] = c;
	}

	// fill with spaces
	for (; x<32; ++x)
	{
		stack.nmi_update[x] = ' ';
	}

	// convert ASCII to font set
	for (int i=0; i<32; ++i)
	{
		char c = stack.nmi_update[i];
		c = text_convert(c);
		stack.nmi_update[i] = c;
	}
}

} // namespace Game

// end of file
