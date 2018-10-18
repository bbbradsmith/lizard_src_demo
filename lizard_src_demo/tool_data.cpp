// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_data.h"
#include "tool_map.h"
#include "wx/filename.h"
#include "wx/textfile.h"
#include "wx/tokenzr.h"
#include "wx/utils.h"
#include "enums.h"
//#include "assets/export/text_set_tool.h"

using namespace LizardTool;
using namespace Game;

#define ELEMENTS_OF(x) (sizeof(x)/sizeof(x[0]))

static const wxChar* ITEM_TYPE_NAME[Data::ITEM_TYPE_COUNT] = {
	wxT("chr"),
	wxT("palette"),
	wxT("sprite"),
	wxT("room")
};

const wxChar* MUSIC_LIST[TOOL_MUSIC_COUNT] = {
	/* 00 */ wxT("silent"),
	/* 01 */ wxT("ruins"),
	/* 02 */ wxT("maze"),
	/* 03 */ wxT("marsh"),
	/* 04 */ wxT("forest"),
	/* 05 */ wxT("palace"),
	/* 06 */ wxT("lava"),
	/* 07 */ wxT("water"),
	/* 08 */ wxT("lounge"),
	/* 09 */ wxT("roots"),
	/* 0A */ wxT("river"),
	/* 0B */ wxT("steam"),
	/* 0C */ wxT("void"),
	/* 0D */ wxT("mountain"),
	/* 0E */ wxT("volcano"),
	/* 0F */ wxT("sanctuary"),
	/* 10 */ wxT("boss"),
	/* 11 */ wxT("ending"),
	/* 12 */ wxT("death"),
};
static_assert(ELEMENTS_OF(MUSIC_LIST) == TOOL_MUSIC_COUNT,"MUSIC_LIST has the wrong number of entries");

const wxChar* DOG_LIST[] = {
#define TABLE_ENTRY(d) wxT(#d),
#include "dogs_table.h"
#undef TABLE_ENTRY
};
static_assert(ELEMENTS_OF(DOG_LIST) == DOG_COUNT,"DOG_LIST has the wrong number of entries");

const int LIZARD_COUNT = LIZARD_OF_COUNT;
const wxChar* LIZARD_LIST[] = {
	/* 00 */ wxT("Knowledge"),
	/* 01 */ wxT("Bounce"),
	/* 02 */ wxT("Swim"),
	/* 03 */ wxT("Heat"),
	/* 04 */ wxT("Surf"),
	/* 05 */ wxT("Push"),
	/* 06 */ wxT("Stone"),
	/* 07 */ wxT("Coffee"),
	/* 08 */ wxT("Lounge"),
	/* 09 */ wxT("Death"),
	/* 10 */ wxT("Beyond"),
};
static_assert(ELEMENTS_OF(LIZARD_LIST) == LIZARD_COUNT,"LIZARD_LIST has the wrong number of entries");

Data::Data()
{
	clear();
}

Data::~Data()
{
	delete [] music_list_string;
	delete [] dog_list_string;
	delete [] dog_sprite_map;
	delete [] dog_sprite_utility;
	clear();
}

void Data::clear()
{
	for (unsigned int i=0; i < room.size();    ++i) delete room[i];
	for (unsigned int i=0; i < chr.size();     ++i) delete chr[i];
	for (unsigned int i=0; i < palette.size(); ++i) delete palette[i];
	for (unsigned int i=0; i < sprite.size();  ++i) delete sprite[i];

	room.clear();
	chr.clear();
	palette.clear();
	sprite.clear();
	package_file = wxT("default.lpk");
	unsaved = false;

	music_list_string = NULL;
	dog_list_string = NULL;
	dog_sprite_map = NULL;
	dog_sprite_utility = NULL;
	dog_sprite_map_dirty = true;

	wxASSERT(sizeof(chr_clipboard) == (8*8));
	::memset(chr_clipboard,0,8*8);
	::memset(nmt_clipboard,0,sizeof(nmt_clipboard));
	nmt_clip_w = 0;
	nmt_clip_h = 0;
}

bool Data::load()
{
	bool result = true;
	file_error = wxT("");

	links.clear();
	wxFileName fn_package = package_file;
	{
		wxTextFile f(fn_package.GetFullPath());
		f.Open();

		if(!f.IsOpened())
		{
			result = false;
			file_error += wxT("Could not open package: ") + fn_package.GetFullPath() + wxT("\n");
		}
		else
		{
			clear();
			package_file = fn_package.GetFullPath();

			wxString s = f.GetFirstLine();
			long v;

			for (int t=0; t<ITEM_TYPE_COUNT; ++t)
			{
				wxFileName fn_path = wxFileName::DirName(fn_package.GetPath());
				fn_path.AppendDir(wxString(wxT("src_"))+ITEM_TYPE_NAME[t]);

				if (t != 0) s = f.GetNextLine();
				// verify that it has the right package name?

				s = f.GetNextLine();
				v = 0;
				s.ToLong(&v);
				int count = v;
				for (int i=0; i<count; ++i)
				{
					bool resulti = true;

					s = f.GetNextLine();
					if (!new_item(t,s))
					{
						file_error += wxT("Could not create new ");
						file_error += ITEM_TYPE_NAME[t];
						file_error += wxT(" with name: ")+ s +wxT("\n");
						resulti = false;
						result = false;
					}
					else
					{
						Item* item = get_item(t,get_item_count(t)-1);

						wxFileName fn_file  = wxFileName(fn_path.GetPath(),s + wxT(".txt"));
						wxTextFile fi(fn_file.GetFullPath());
						fi.Open();
						if (!fi.IsOpened())
						{
							file_error += wxT("Could not open file: ") + fn_file.GetFullName() + wxT("\n");
							resulti = false;
						}
						else
						{
							resulti = item->load(fi,file_error,this);
							fi.Close();
						}

						if (resulti)
							item->unsaved = false;
						else
						{
							file_error += wxT("Error loading file: ") + fn_file.GetFullName() + wxT("\n");
							result = false;
							item->unsaved = true;
						}
					}
				}
			}
			f.Close();
		}
	}

	for (size_t i=0; i<links.size(); ++i)
	{
		LoadLink link = links[i];
		link.room->door[link.door] = (Room*)find_item(ITEM_ROOM,link.link);
		if (link.room->door[link.door] == NULL && link.link != wxT("NULL"))
		{
			file_error += wxT("Unable to find room for link: ")+link.link+wxT(" from ")+link.room->name+wxT("\n");
			result = false;
		}
	}

	if (result)
	{
		file_error = wxT("Load successful.");
		unsaved = false;
	}

	return result;
}

bool Data::save()
{
	bool result = true;
	file_error = wxT("");

	wxFileName fn_package = package_file;
	{
		wxTextFile f(fn_package.GetFullPath());
		
		if (!f.Exists()) f.Create();
		f.Open();

		if(!f.IsOpened())
		{
			result = false;
			file_error += wxT("Could not open package: ") + fn_package.GetFullPath() + wxT("\n");
		}
		else
		{
			f.Clear();
			for (int t=0;t<ITEM_TYPE_COUNT; ++t)
			{
				f.AddLine(ITEM_TYPE_NAME[t]);
				f.AddLine(wxString::Format(wxT("%d"),get_item_count(t)));
				for (unsigned int i=0;i<get_item_count(t);++i)
				{
					f.AddLine(get_item(t,i)->name);
				}
			}

			f.Write();
			f.Close();
		}
	}

	for (int t=0;t<ITEM_TYPE_COUNT;++t)
	{
		wxFileName fn_path = wxFileName::DirName(fn_package.GetPath());
		fn_path.AppendDir(wxString(wxT("src_"))+ITEM_TYPE_NAME[t]);
		if (!fn_path.DirExists())
		{
			if (!fn_path.Mkdir())
			{
				result = false;
				file_error += wxT("Could not make directory: ") + fn_path.GetPath() + wxT("\n");
			}
		}
		
		for (unsigned int i=0;i<get_item_count(t);++i)
		{
			Item* item = get_item(t,i);

			wxString name = item->name + wxT(".txt");
			wxFileName fn_file  = wxFileName(fn_path.GetPath(),name);
			if (!item->unsaved && fn_file.FileExists())
			{
				// item is already saved, please skip it
				continue;
			}

			wxTextFile f(fn_file.GetFullPath());
			if (!f.Exists()) f.Create();
			f.Open();
			if (!f.IsOpened())
			{
				result = false;
				file_error += wxT("Could not open file: ") + fn_file.GetFullPath() + wxT("\n");
			}
			else
			{
				f.Clear();
				item->save(f,file_error);
				f.Write();
				f.Close();
				item->unsaved = false;
			}
		}
	}

	if (result) file_error = wxT("Save successful.");
	return result;
}

bool Data::assign_coins_and_flags(int& coins, int& flags)
{
	bool result = true;
	coins = 0;
	flags = FLAG_COUNT;

	for (unsigned int i=0; i<room.size(); ++i)
	{
		Room* r = room[i];
		for (unsigned int d=0; d<16; ++d)
		{
			switch (r->dog[d].type)
			{
				case DOG_COIN:
					{
						if (r->dog[d].param != coins)
						{
							r->dog[d].param = coins;
							r->unsaved = true;
							unsaved = true;
						}
						++coins;
					}
					break;
				case DOG_ICEBLOCK:
					{
						if (r->dog[d].note != wxT("fixed"))
						{
							if (r->dog[d].param != flags)
							{
								r->dog[d].param = flags;
								r->unsaved = true;
								unsaved = true;
							}
							++flags;
						}
						else
						{
							if (r->dog[d].param == 0)
							{
								file_error += wxString::Format(wxT("Fixed DOG_ICEBLOCK %d has unassigned flag parameter in room: %s\n"),d,r->name);
								result = false;
							}
							else if (r->dog[d].param >= FLAG_COUNT)
							{
								file_error += wxString::Format(wxT("Fixed DOG_ICEBLOCK %d has out of range flag parameter in room: %s\n"),d,r->name);
								result = false;
							}
						}
					}
					break;
				case DOG_SPARKD:
				case DOG_SPARKU:
				case DOG_SPARKL:
				case DOG_SPARKR:
					{
						static_assert(DOG_SPARKU == DOG_SPARKD+1,"DOG_SPARKU out of order.");
						static_assert(DOG_SPARKL == DOG_SPARKD+2,"DOG_SPARKL out of order.");
						static_assert(DOG_SPARKR == DOG_SPARKD+3,"DOG_SPARKR out of order.");
						const unsigned int dir = r->dog[d].type - DOG_SPARKD;
						wxASSERT(dir < 4); // DOG_SPARK enum out of range?

						static const int DX[4] = {0,0,-8,8};
						static const int DY[4] = {8,-8.0,0};
						const int dx = DX[dir];
						const int dy = DY[dir];

						int sx = r->dog[d].x;
						int sy = r->dog[d].y;
						int len = 0;

						unsigned char phase = r->dog[d].param & 0x38; // low 3 bits specify length, mid 3 bits specify starting phase
						unsigned char sp = phase;

						for (int i=0;i<9;++i)
						{
							static const int FORBID[4] = {1,0,3,2}; // forbid reversal connections

							int e = d+1;
							for (; e<16; ++e)
							{
								if (r->dog[e].x != sx) continue;
								if (r->dog[e].y != sy) continue;
								if (r->dog[e].type < DOG_SPARKD || r->dog[e].type > DOG_SPARKR) continue;
								break;
							}

							if (e < 16) // length cut short
							{
								const unsigned int edir = r->dog[e].type - DOG_SPARKD;
								wxASSERT(edir < 4); // DOG_SPARK blocking enum out of range?

								if (edir == FORBID[dir])
								{
									file_error += wxString::Format(wxT("DOG_SPARK %d reversal at %d in room: %s\n"),d,e,r->name);
									result = false;
								}
								if (len == 0)
								{
									file_error += wxString::Format(wxT("DOG_SPARK %d overlap at %d in room: %s\n"),d,e,r->name);
									result = false;
								}

								// automatically adjust phase for seamless connection
								unsigned char new_eparam = (r->dog[e].param & 0x7) | sp;
								if (r->dog[e].param != new_eparam)
								{
									r->dog[e].param = new_eparam;
									r->unsaved = true;
									unsaved = true;
								}

								break;
							}

							++len;
							sx += dx;
							sy += dy;
							sp = (sp - 8) & 0x38;

							// if in a wall or out of bounds, terminate
							if (r->collide_pixel(sx,sy,true)) break;
						}

						if (len == 0)
						{
							file_error += wxString::Format(wxT("DOG_SPARK %d has length 0 in room: %s\n"),d,r->name);
							result = false;
						}
						else
						{
							if (len > 8) len = 8;
							len = (len - 1) & 7; // to pack into low 3 bits of param
						}

						// write back phase/len to param
						unsigned char new_param = phase | (len & 7);
						if (r->dog[d].param != new_param)
						{
							r->dog[d].param = new_param;
							r->unsaved = true;
							unsaved = true;
						}
					}
					break;
				case DOG_BLOCK_ON:
				case DOG_BLOCK_OFF:
				case DOG_ROPE:
					if (r->dog[d].param == 0)
					{
						file_error += wxString::Format(wxT("DOG_BLOCK_ON/OFF/ROPE %d has unassigned flag parameter in room: %s\n"),d,r->name);
						result = false;
					}
					else if (r->dog[d].param >= FLAG_COUNT)
					{
						file_error += wxString::Format(wxT("DOG_BLOCK_ON/OFF/ROPE %d has out of range flag parameter in room: %s\n"),d,r->name);
						result = false;
					}
					break;

				default:
					break;
			}
		}
	}

	if (coins > 128)
	{
		file_error += wxT("Too many coins.\n");
		result = false;
	}
	if (flags > 128)
	{
		file_error += wxT("Too many flags.\n");
		result = false;
	}

	return result;
}

bool Data::validate()
{
	bool valid = true;

	for (unsigned int i=0; i<palette.size(); ++i)
	{
		Palette* p = palette[i];
		for (int c=0; c<4; ++c)
		{
			if ((p->data[c] & 0x0F) == 0x0D)
			{
				file_error += wxT("Palette may not use $XD colours: ") + p->name + wxT("\n");
				valid = false;
			}
		}
	}

	// grid to check for overlap
	bool grid_used[2][MAP_W][MAP_H];
	for (int v=0; v<2; ++v)
	for (int y=0; y<MAP_H; ++y)
	for (int x=0; x<MAP_W; ++x)
	{
		grid_used[v][x][y] = false;
	}

	for (unsigned int i=0; i<room.size(); ++i)
	{
		Room* r = room[i];

		int blocker[4] = { 0, 0, 0, 0 }; // count blockers for conflict
		int nmt_updaters = 0; // count dogs that can update nametable (max 1 allowed)

		// check for map grid overlap
		for (int i=0; i < (r->scrolling ? 2 : 1); ++i)
		{
			int gx = r->coord_x + i;
			int gy = r->coord_y;
			int gv = r->recto ? 1 : 0;

			if (gx < 0 || gy < 0 || gx >= MAP_W || gy >= MAP_H)
			{
				file_error += wxT("Off-grid room: ");
				file_error += r->name;
				file_error += wxT("\n");
				valid = false;
				break;
			}

			if (grid_used[gv][gx][gy])
			{
				file_error +=
					wxString(wxT("Grid overlap at ")) +
					(r->recto ? wxT("recto") : wxT("verso")) +
					wxString::Format(wxT(" %d,%d: "),r->coord_x,r->coord_y) +
					r->name +
					wxT("\n");
				valid = false;
			}
			grid_used[gv][gx][gy] = true;
		}

		for (int d=0;d<16;++d)
		{
			wxString dog_error = wxT("");

			const unsigned char p = r->dog[d].param;
			switch (r->dog[d].type)
			{

			// bank E
			case DOG_NONE: break;
			case DOG_DOOR:
			case DOG_PASS:
				if (p >= 8) dog_error += wxT("door parameter must be <8.");
				break;
			case DOG_PASS_X:
				if (p >= 8) dog_error += wxT("door parameter must be <8.");
				if (d != 11 && d != 12) dog_error += wxT("dog_pass_x must be in slot 11/12.");
				if (d == 11 && r->dog[d].x != 0) dog_error += wxT("dog_pass_x in slot 11 must be on left edge.");
				if (d == 12 && r->dog[d].x != (r->scrolling ? 504 : 248)) dog_error += wxT("dog_pass_x in slot 12 must be on right edge.");
				break;
			case DOG_PASS_Y:
				if (p >= 8) dog_error += wxT("door parameter must be <8.");
				if (d != 13 && d != 14 && d != 15) dog_error += wxT("dog_pass_y must be in slot 13/14/15.");
				if (d == 13 && r->dog[d].y != 0) dog_error += wxT("dog_pass_y in slot 13 must be on top edge.");
				if (d == 14 && r->dog[d].y != 232) dog_error += wxT("dog_pass_y in slot 14 must be on bottom edge.");
				if (d == 15 && r->dog[d].y != 232) dog_error += wxT("dog_pass_y in slot 15 must be on bottom edge."); // 15 alternative allowed for grog room
				break;
			case DOG_PASSWORD_DOOR: break;
			case DOG_LIZARD_EMPTY_LEFT:
			case DOG_LIZARD_EMPTY_RIGHT:
				if (p >= LIZARD_OF_COUNT) dog_error += wxT("lizard_empty paramter must be valid LIZARD_OF enum value.");
				for (int i=0; i<d; ++i)
				{
					if (r->dog[i].type == DOG_LIZARD_EMPTY_LEFT || r->dog[i].type == DOG_LIZARD_EMPTY_RIGHT)
					{
						dog_error += wxT("Only one lizard_empty allowed per room.");
						break;
					}
				}
				if (r->dog[DISMOUNT_SLOT].type != DOG_LIZARD_DISMOUNTER)
					dog_error += wxString::Format(wxT("lizard_empty requires dismounter in slot %d."), DISMOUNT_SLOT);
				break;
			case DOG_LIZARD_DISMOUNTER:
				if (d != DISMOUNT_SLOT)
					dog_error += wxString::Format(wxT("dismounter may only exist in slot %d."),DISMOUNT_SLOT);
				break;
			case DOG_SPLASHER:
				if (d != SPLASHER_SLOT)
					dog_error += wxString::Format(wxT("splasher may only exist in slot %d."),SPLASHER_SLOT);
				break;
			case DOG_DISCO: break;
			case DOG_WATER_PALETTE: break;
			case DOG_GRATE:
			case DOG_GRATE90:
				++blocker[d & 3];
				break;
			case DOG_WATER_FLOW: break;
			case DOG_RAINBOW_PALETTE: break;
			case DOG_PUMP: break;
			case DOG_SECRET_STEAM:
				if (r->scrolling)
					dog_error += wxT("secret_steam does not support scrolling.");
				break;
			case DOG_CEILING_FREE: break;
			case DOG_BLOCK_COLUMN:
				++blocker[d & 3];
				break;
			case DOG_SAVE_STONE: break;
			case DOG_COIN:
				if (p > 127)
					dog_error += wxT("coin parameter out of range (<128).");
				break;
			case DOG_MONOLITH:
				if (r->dog[d].x < 50 || r->dog[d].x > (205 + (r->scrolling ? 256 : 0)))
					dog_error += wxT("monolith too close to edge (<50px).");
				break;
			case DOG_ICEBLOCK:
				if ((r->dog[d].x & 7) != 0 || (r->dog[d].y & 7) != 0)
					dog_error += wxT("iceblock must be aligned to 8x8 grid.");
				break;
			case DOG_VATOR:
				++blocker[d & 3];
				if (p > 5) dog_error += wxT("vator parameter out of range (<6).");
				break;
			case DOG_NOISE:
				if ((r->dog[d].x & 15) == 13)
					dog_error += wxT("noise (x & 15) == 13 reserved for rain.");
				break;
			case DOG_SNOW: break;
			case DOG_RAIN: break;
			case DOG_RAIN_BOSS: break;
			case DOG_DRIP: break;
			case DOG_HOLD_SCREEN:
				if (d != HOLD_SLOT)
					dog_error += wxString::Format(wxT("hold_screen must be in slot %d."),HOLD_SLOT);
				break;
			case DOG_BOSS_DOOR: break;
			case DOG_BOSS_DOOR_RAIN:
				if (d != BOSS_DOOR_RAIN_SLOT)
					dog_error += wxString::Format(wxT("boss_door_rain must be in slot %d."),BOSS_DOOR_RAIN_SLOT);
				break;
			case DOG_BOSS_DOOR_EXIT:
			case DOG_BOSS_DOOR_EXEUNT:
				if (p < 6)
					dog_error += wxString::Format(wxT("boss_door_exit/exeunt must have parameter >= 6 (slot %d)."),d);
				break;
			case DOG_BOSS_RUSH:
				if (d < 1 || d > 3)
					dog_error += wxT("boss_rush may only be in slots 1-3.");
				if (p > 6)
					dog_error += wxT("boss_rush param must be <6.");
				break;
			case DOG_OTHER:
				++blocker[0];
				if (r->dog[OTHER_BONES_SLOT].type != DOG_NONE)
					dog_error += wxString::Format(wxT("other without none in slot %d."),OTHER_BONES_SLOT);
				if (r->scrolling)
					dog_error += wxT("other does not support scrolling.");
				break;
			case DOG_ENDING: break;
			case DOG_RIVER_EXIT: break;
			case DOG_BONES: break;
			case DOG_EASY:
				if (r->scrolling) dog_error += wxT("easy does not support scrolling.");
				break;
			case DOG_SPRITE0: break;
			case DOG_SPRITE2: break;
			case DOG_HINTD: break;
			case DOG_HINTU: break;
			case DOG_HINTL: break;
			case DOG_HINTR: break;
			case DOG_HINT_PENGUIN:
				if (d < 1) dog_error += wxT("hint_penguin may not be used in slot 0.");
				else if (r->dog[d-1].type != DOG_PENGUIN) dog_error += wxT("hint_penguin requires penguin in previous slot.");
				break;
			case DOG_BIRD: break;
			case DOG_FROG: break;
			case DOG_GROG: break;
			case DOG_PANDA:
				++blocker[d & 3];
				break;
			case DOG_GOAT: break;
			case DOG_DOG: break;
			case DOG_WOLF: break;
			case DOG_OWL:
				++blocker[d & 3];
				break;
			case DOG_ARMADILLO: break;
			case DOG_BEETLE:
				++blocker[d & 3];
				break;
			case DOG_SKEETLE:
				break;
			case DOG_SEEKER_FISH: break;
			case DOG_MANOWAR: break;
			case DOG_SNAIL:
				++blocker[d & 3];
				break;
			case DOG_SNAPPER: break;
			case DOG_VOIDBALL:
				if (r->scrolling) dog_error += wxT("voidball does not support scrolling.");
				break;
			case DOG_BALLSNAKE:
				if (r->scrolling) dog_error += wxT("ballsnake does not support scrolling.");
				if ((r->dog[d].x & 7) || (r->dog[d].y & 7))
					dog_error += wxT("ballsnake must be aligned to 8x8 grid.");
				break;
			case DOG_MEDUSA:
				if (r->dog[d].param < 36 || r->dog[d].param > 204)
					dog_error += wxString::Format(wxT("medusa parameter not in range 36-204."),d);
				break;
			case DOG_PENGUIN:
				++blocker[d & 3];
				break;
			case DOG_MAGE:
				if (d > 15 || r->dog[d+2].type != DOG_MAGE_BALL)
					dog_error += wxString::Format(wxT("mage requires mage_ball two slots down (%d)."),d+2);
				break;
			case DOG_MAGE_BALL: break;
			case DOG_GHOST: break;
			case DOG_PIGGY: break;
			case DOG_PANDA_FIRE:
				++blocker[d & 3];
				break;
			case DOG_GOAT_FIRE:
				++blocker[d & 3];
				break;
			case DOG_DOG_FIRE: break;
			case DOG_OWL_FIRE: break;
			case DOG_MEDUSA_FIRE: break;
			case DOG_ARROW_LEFT:
				{
					int tx = r->dog[d].x / 8;
					int ty = r->dog[d].y / 8;
					bool collide = false;
					while (tx >= 0)
					{
						if (r->collide_tile(tx,ty,false)) { collide = true; break; }
						--tx;
					}
					if (!collide)
					{
						dog_error += wxString::Format(wxT("arrow_left with no stopping tile (%d)."),d);
					}
				}
				break;
			case DOG_ARROW_RIGHT:
				{
					int tx = r->dog[d].x / 8;
					int ty = r->dog[d].y / 8;
					bool collide = false;
					while (tx < 64)
					{
						if (r->collide_tile(tx,ty,false)) { collide = true; break; }
						++tx;
					}
					if (!collide)
					{
						dog_error += wxString::Format(wxT("arrow_right with no stopping tile (%d)."),d);
					}
				}
				break;
			case DOG_SAW: break;
			case DOG_STEAM: break;
			case DOG_SPARKD:
			case DOG_SPARKU:
			case DOG_SPARKL:
			case DOG_SPARKR:
				if ((r->dog[d].x & 7) != 0 || (r->dog[d].y & 7) != 0)
					dog_error += wxT("spark must be aligned to 8x8 grid.");
				break;
			case DOG_FROB_FLY:
				if (r->dog[0].type != DOG_FROB) dog_error += dog_list_string[r->dog[d].type] + wxT(" expects frob in slot 0.");
				break;

			// bank D
			case DOG_PASSWORD:
				if (p >= 5 && p != 255) dog_error += wxT("password parameter must be <5 or 255 for lizard changer.");
				if (r->scrolling) dog_error += wxT("password does not support scrolling.");
				break;
			case DOG_LAVA_PALETTE: break;
			case DOG_WATER_SPLIT:
				for (int y = (r->water >> 3); y < 30; ++y)
				{
					if (!r->collide_tile(32,y,true))
					{
						dog_error += wxString::Format(wxT("water_split requires solid blocks below water line at column 32, see row %d."),y);
						break;
					}
				}
				break;
			case DOG_BLOCK:
				++blocker[d & 3];
				break;
			case DOG_BLOCK_ON:
				++blocker[d & 3];
				break;
			case DOG_BLOCK_OFF:
				//++blocker[d & 3]; // assumed by DOG_BLOCK_ON pair
				break;
			case DOG_DRAWBRIDGE:
				++blocker[d & 3];
				++nmt_updaters;
				break;
			case DOG_ROPE:
				++blocker[d & 3];
				break;
			case DOG_BOSS_FLAME:
				if (p >= 6)
					dog_error += wxT("boss_flame parameter out of range (<6).");
				break;
			case DOG_RIVER:
				if (d != RIVER_SLOT)
					dog_error += wxString::Format(wxT("river must be in slot %d."),RIVER_SLOT);
				break;
			case DOG_RIVER_ENTER: break;
			case DOG_SPRITE1: break;
			case DOG_BEYOND_STAR:
				if (d != BEYOND_STAR_SLOT)
					dog_error += wxString::Format(wxT("beyond_star must be in slot %d."),BEYOND_STAR_SLOT);
				break;
			case DOG_BEYOND_END:
				if (r->dog[BEYOND_STAR_SLOT].type != DOG_BEYOND_STAR)
					dog_error += wxString::Format(wxT("beyond_end without beyond_star in slot %d."),BEYOND_STAR_SLOT);
				break;
			case DOG_OTHER_END_LEFT:
			case DOG_OTHER_END_RIGHT:
				{
					bool blocking = true;
					for (int i=(d&3); i<d; i+=4)
					{
						if (r->dog[i].type == DOG_OTHER_END_LEFT || r->dog[i].type == DOG_OTHER_END_RIGHT && labs(r->dog[i].x - r->dog[d].x) > 32)
						{
							int dist = abs(r->dog[i].x - r->dog[d].x);
							if (dist < 32)
							{
								dog_error += wxString::Format(wxT("other_end too close horizontally to other_end sharing blocker (%d and %d)."),i,d);
							}

							blocking = false;
							break;
						}
					}
					if (blocking)
					{
						++blocker[d & 3];
					}

					if (p >= 6)
					{
						dog_error += wxT("other_end parameter out of range <6.");
					}
				}
				break;
			case DOG_PARTICLE: break;
			case DOG_INFO:
				 break;
			case DOG_DIAGNOSTIC:
				if (r->scrolling) dog_error += wxT("diagnostic does not support scrolling.");
				if (r->dog[DISMOUNT_SLOT].type != DOG_LIZARD_DISMOUNTER) dog_error += wxString::Format(wxT("diagnostic requires lizard_dismounter in slot %d."),DISMOUNT_SLOT);
				++nmt_updaters;
				break;
			case DOG_METRICS:
				if (r->scrolling) dog_error += wxT("metrics does not support scrolling.");
				if (r->dog[HOLD_SLOT].type != DOG_HOLD_SCREEN) dog_error += wxString::Format(wxT("metrics requires hold_screen in slot %d."),HOLD_SLOT);
				break;
			case DOG_SUPER_MOOSE: break;
			case DOG_BRAD_DUNGEON: break;
			case DOG_BRAD: break;
			case DOG_HEEP_HEAD:
				++blocker[0];
				++blocker[1];
				++blocker[2];
				++blocker[3];
				break;
			case DOG_HEEP:
			case DOG_HEEP_TAIL:
				{
					bool head_found = false;
					for (int dh=0; dh<16; ++dh)
					{
						if(r->dog[dh].type == DOG_HEEP_HEAD)
						{
							head_found = true;
							break;
						}
					}
					if (!head_found)
					{
						dog_error += wxString::Format(wxT("heep without heep_head (%d)."),d);
					}
					for (int dh=(d&3); dh<16; dh+=4)
					{
						if (dh == d) continue;
						int t = r->dog[dh].type;
						if (t == DOG_HEEP_HEAD || t == DOG_HEEP || t == DOG_HEEP_TAIL)
						{
							int dx = r->dog[dh].x - r->dog[d].x;
							if (dx < 24 && dx >= -24)
							{
								dog_error += wxString::Format(wxT("heep blocker overlap (%d vs %d)."),d,dh);
							}
						}
					}
				}
				break;
			case DOG_LAVA_LEFT: break;
			case DOG_LAVA_RIGHT: break;
			case DOG_LAVA_LEFT_WIDE: break;
			case DOG_LAVA_RIGHT_WIDE: break;
			case DOG_LAVA_LEFT_WIDER: break;
			case DOG_LAVA_RIGHT_WIDER: break;
			case DOG_LAVA_DOWN: break;
			case DOG_LAVA_POOP: break;
			case DOG_LAVA_MOUTH: break;
			case DOG_BOSSTOPUS:
				if (r->scrolling)
					dog_error += wxT("bosstopus does not support scrolling.");
				if (d != 0)
					dog_error += wxT("bosstopus must be in slot 0 for NES optimization.");
				if (r->dog[1].type != DOG_NONE)
					dog_error += wxT("bosstopus requires none in slot 1 for bones.");
				if (r->dog[2].type != DOG_NONE)
					dog_error += wxT("bosstopus requires none in slot 2 for boss_door.");
				break;
			case DOG_BOSSTOPUS_EGG:
				if (r->scrolling)
					dog_error += wxT("bosstopus_egg does not support scrolling.");
				break;
			case DOG_CAT:
				if (r->scrolling)
					dog_error += wxT("cat does not support scrolling.");
				if (d != 2) dog_error += wxT("cat may only be used in slot 2.");
				if (r->dog[0].type  != DOG_NONE          ) dog_error += wxT("cat requires none in slot 0 for boss_door.");
				if (r->dog[1].type  != DOG_BOSS_DOOR_EXIT) dog_error += wxT("cat requires boss_door_exit in slot 1.");
				if (r->dog[3].type  != DOG_NONE          ) dog_error += wxT("cat requires none in slot 3 for crown bones.");
				if (r->dog[4].type  != DOG_CAT_SMILE     ) dog_error += wxT("cat requires cat_smile in slot 4.");
				if (r->dog[5].type  != DOG_CAT_SMILE     ) dog_error += wxT("cat requires cat_smile in slot 5.");
				if (r->dog[9].type  != DOG_CAT_SPARKLE   ) dog_error += wxT("cat requires cat_sparkle in slot 9.");
				if (r->dog[10].type != DOG_CAT_SPARKLE   ) dog_error += wxT("cat requires cat_sparkle in slot 10.");
				if (r->dog[11].type != DOG_CAT_SPARKLE   ) dog_error += wxT("cat requires cat_sparkle in slot 11.");
				++blocker[d & 3];
				break;
			case DOG_CAT_SMILE:
				if (r->scrolling)
					dog_error += wxT("cat_smile does not support scrolling.");
				if (d >= 10 || r->dog[d+6].type != DOG_CAT_SPARKLE)
					dog_error += wxString::Format(wxT("cat_smile requires cat_sparkle six slots down (%d)."),d+6);
				break;
			case DOG_CAT_SPARKLE:
				if (r->scrolling)
					dog_error += wxT("cat_sparkle does not support scrolling.");
				break;
			case DOG_CAT_STAR:
				if (r->scrolling)
					dog_error += wxT("cat_star does not support scrolling.");
				break;
			case DOG_RACCOON:
				if (r->dog[d+9].type != DOG_RACCOON_VALVE   ) dog_error += wxString::Format(wxT("raccoon expects raccoon_valve 9 slots down (%d+9=%d)."),d,d+9);
				if (r->dog[  3].type != DOG_RACCOON_LAUNCHER) dog_error += wxT("raccoon expects raccoon_launcher in slot 3.");
				break;
			case DOG_RACCOON_LAUNCHER:
				if (d != 3) dog_error += wxT("raccoon_launcher may only be used in slot 3.");
				if (r->dog[ 0].type != DOG_SPRITE0         ) dog_error += wxT("raccoon_launcher expects sprite0 in slot 0.");
				if (r->dog[ 1].type != DOG_BOSS_DOOR_EXIT  ) dog_error += wxT("raccoon_launcher expects boss_door_exit in slot 1.");
				if (r->dog[ 4].type != DOG_RACCOON         ) dog_error += wxT("raccoon_launcher expects raccoon in slot 4.");
				if (r->dog[ 5].type != DOG_RACCOON         ) dog_error += wxT("raccoon_launcher expects raccoon in slot 5.");
				if (r->dog[ 6].type != DOG_RACCOON         ) dog_error += wxT("raccoon_launcher expects raccoon in slot 6.");
				if (r->dog[ 7].type != DOG_RACCOON_LAVABALL) dog_error += wxT("raccoon_launcher expects raccoon_lavaball in slot 7.");
				if (r->dog[ 8].type != DOG_RACCOON_LAVABALL) dog_error += wxT("raccoon_launcher expects raccoon_lavaball in slot 8.");
				if (r->dog[ 9].type != DOG_RACCOON_LAVABALL) dog_error += wxT("raccoon_launcher expects raccoon_lavaball in slot 9.");
				if (r->dog[10].type != DOG_RACCOON_LAVABALL) dog_error += wxT("raccoon_launcher expects raccoon_lavaball in slot 10.");
				if (r->dog[11].type != DOG_RACCOON_LAVABALL) dog_error += wxT("raccoon_launcher expects raccoon_lavaball in slot 11.");
				if (r->dog[12].type != DOG_RACCOON_LAVABALL) dog_error += wxT("raccoon_launcher expects raccoon_lavaball in slot 12.");
				break;
			case DOG_RACCOON_LAVABALL:
				break;
			case DOG_RACCOON_VALVE:
				break;
			case DOG_FROB:
				if (d != 0) dog_error += wxT("frob may only be used in slot 0.");
				if (r->dog[ 1].type != DOG_FROB_HAND_LEFT ) dog_error += wxT("frob expects frob_hand_left in slot 1.");
				if (r->dog[ 2].type != DOG_FROB_HAND_RIGHT) dog_error += wxT("frob expects frob_hand_right in slot 2.");
				if (r->dog[ 3].type != DOG_FROB_FLY       ) dog_error += wxT("frob expects frob_fly in slot 3.");
				if (r->dog[ 4].type != DOG_FROB_TONGUE    ) dog_error += wxT("frob expects frob_tongue in slot 4.");
				if (r->dog[ 5].type != DOG_FROB_ZAP       ) dog_error += wxT("frob expects frob_zap in slot 5.");
				if (r->dog[ 6].type != DOG_FROB_BLOCK     && r->dog[ 6].type != DOG_NONE) dog_error += wxT("frob expects frob_block or none in slot 6.");
				if (r->dog[15].type != DOG_BOSS_DOOR_EXIT && r->dog[15].type != DOG_NONE) dog_error += wxT("frob expects boss_door_exit or none in slot 15.");
				for (int fd=7; fd <= 14; ++fd)
				{
					if (r->dog[fd].type != DOG_FROB_PLATFORM && r->dog[fd].type != DOG_NONE) dog_error += wxString::Format(wxT("frob expects frob_platform or none in slot %d."),fd);
				}
				break;
			case DOG_FROB_HAND_LEFT:
			case DOG_FROB_HAND_RIGHT:
			case DOG_FROB_ZAP:
			case DOG_FROB_TONGUE:
			case DOG_FROB_BLOCK:
			case DOG_FROB_PLATFORM:
				if (r->dog[0].type != DOG_FROB) dog_error += dog_list_string[r->dog[d].type] + wxT(" expects frob in slot 0.");
				break;
			case DOG_QUEEN:
				if (r->scrolling) dog_error += wxT("queen does not support scrolling.");
				break;
			case DOG_HARE:
				if (d != HARE_SLOT) dog_error += wxString::Format(wxT("hare may only be used in slot %d."),HARE_SLOT);
				break;
			case DOG_HARECICLE:
				break;
			case DOG_HAREBURN:
				if (r->dog[HARE_SLOT].type != DOG_HARE) dog_error += wxString::Format(wxT("hareburn expects hare in slot %d."),HARE_SLOT);
				break;
			case DOG_ROCK:
			case DOG_LOG:
			case DOG_DUCK:
			case DOG_RAMP:
			case DOG_RIVER_SEEKER:
			case DOG_BARREL:
			case DOG_WAVE:
			case DOG_SNEK_LOOP:
			case DOG_SNEK_HEAD:
			case DOG_SNEK_TAIL:
			case DOG_RIVER_LOOP:
				dog_error += wxString::Format(wxT("river dog type (%s) may not be manually placed, slot %d."),DOG_LIST[r->dog[d].type],d);
				break;
			case DOG_WATT:
				if (d < 1 || d > 9) dog_error += wxString::Format(wxT("witch (%d) slot must be 1-9."),d);
				if (p >= 2) dog_error += wxString::Format(wxT("watt param must be 0/1 (%d)."),d);
				break;
			case DOG_WATERFALL:
				break;

			// bank F
			case DOG_TIP:
				if (d != TIP_SLOT) dog_error += wxString::Format(wxT("tip (%d) slot must be %d."),d,TIP_SLOT);
				break;
			case DOG_WIQUENCE:
				if (d != WIQUENCE_SLOT) dog_error += wxString::Format(wxT("wiquence (%d) slot must be %d."),d,WIQUENCE_SLOT);
				if (p >= 2) dog_error += wxString::Format(wxT("wiquence param must be 0/1 (%d)."),d);
				break;
			case DOG_WITCH:
				if (d != WITCH_SLOT) dog_error += wxString::Format(wxT("witch (%d) slot must be %d."),d,WITCH_SLOT);
				break;
			case DOG_BOOK:
				if (d != BOOK_SLOT) dog_error += wxString::Format(wxT("book (%d) slot must be %d."),d,BOOK_SLOT);
				break;

			default:
				dog_error += wxT("Dog type requires validation.");
				break;
			}

			if (dog_error.Length() > 0)
			{
				file_error += wxString::Format(wxT("Invalid dog %d in room "),d) + r->name + wxT(": ") + dog_error + wxT("\n");
				valid = false;
			}
		}

		for (int i=0; i<4; ++i)
		{
			if (blocker[i] > 1)
			{
				file_error += wxString::Format(wxT("Blocker slot %d conflict in room: "),i) + r->name + wxT("\n");
				valid = false;
			}
		}

		if (nmt_updaters > 1)
		{
			file_error += wxString(wxT("Multiple nametable updaters in room: ")) + r->name + wxT("\n");
			valid = false;
		}

		if (r->water < 240 && r->dog[SPLASHER_SLOT].type != DOG_SPLASHER)
		{
			file_error += wxString::Format(wxT("Water without splasher in dog %d: "),SPLASHER_SLOT) + r->name + wxT("\n");
			valid = false;
		}
	}

	return valid;
}

bool Data::validate_extra()
{
	wxString& s = file_error;
	s = wxT("Export validation:\n");
	bool result = validate();
	s += wxT("End of export validation.\n\n");

	// count unshippable rooms
	{
		s += wxT("Unshipped rooms:\n");

		int unship_count = 0;

		unsigned int count = get_item_count(ITEM_ROOM);
		for (unsigned int i=0; i<count; ++i)
		{
			const Room* r = (Room*)get_item(ITEM_ROOM,i);
			if (!r->ship)
			{
				s += r->name;
				s += wxT("\n");
				++unship_count;
			}
		}
		s += wxString::Format(wxT("%d remaining.\n"),unship_count);
		s += wxT("\n");
	}

	// check for unused CHR banks or palettes in rooms
	{
		s += wxT("Unused or missing CHR/palettes in shipped rooms:\n");

		unsigned int count = get_item_count(ITEM_ROOM);
		for (unsigned int i=0; i<count; ++i)
		{
			const Room* r = (Room*)get_item(ITEM_ROOM,i);

			if (!r->ship) continue;

			// chr 1 always text, chr 4 always lizard, pal 0 always text, pal 4/5 always lizard
			bool chr_used[8] = { false, true,  false, false, true,  false, false, false };
			bool pal_used[8] = { true,  false, false, false, true,  true,  false, false };

			// pal 0 always text, 4/5 always lizard
			bool chr_on[8] = { false, false, false, false, false, false, false, false };
			bool pal_on[8] = { false, false, false, false, true,  true,  false, false };

			for (int c=0; c<8; ++c)
			{
				if (r->chr[c] != NULL) chr_on[c] = true;
				if (r->palette[c] != NULL) pal_on[c] = true;
			}
			for (int n=0; n<(64*30); ++n)
			{
				unsigned char t = r->nametable[n];
				chr_used[t >> 6] = true;
			}
			for (int a=0; a<(32*15); ++a)
			{
				unsigned char t = r->attribute[a];
				pal_used[t] = true;
			}
			for (int d=0; d<16; ++d)
			{
				const Sprite* ds = get_dog_sprite(r->dog[d]);
				if (ds != NULL)
				{
					unsigned char vpal = 0;
					if (ds->vpal) vpal |= d & 1;

					for (int st = 0; st < ds->count; ++st)
					{
						unsigned char p = ds->palette[st] | vpal;
						unsigned char t = ds->tile[st];
						pal_used[(p&3)+4] = true;
						chr_used[(t >> 6) + 4] = true;
					}
				}
			}

			bool bad = false;
			for (int c=0; c<8; ++c)
			{
				if ( chr_on[c] && !chr_used[c]) bad = true;
				if (!chr_on[c] &&  chr_used[c]) bad = true;
				if ( pal_on[c] && !pal_used[c]) bad = true;
				if (!pal_on[c] &&  pal_used[c]) bad = true;
			}
			if (r->palette[4] == NULL) bad = true;
			if (r->palette[5] == NULL) bad = true;

			if (bad)
			{
				s += wxT("Usage problem in room ");
				s += r->name;
				s += wxT(":\n");
				for (int c=0; c<8; ++c)
				{
					if ( chr_on[c] && !chr_used[c]) s += wxString::Format(wxT("- Unused CHR %d: "),c) + r->chr[c]->name + wxT("\n");
					if (!chr_on[c] &&  chr_used[c]) s += wxString::Format(wxT("- Missing CHR %d\n"),c);
				}
				for (int c=0; c<8; ++c)
				{
					if ( pal_on[c] && !pal_used[c]) s += wxString::Format(wxT("- Unused PAL %d: "),c) + r->palette[c]->name + wxT("\n");
					if (!pal_on[c] &&  pal_used[c]) s += wxString::Format(wxT("- Missing PAL %d\n"),c);
				}
				if (r->palette[4] == NULL) s += wxT("Missing lizard PAL 4\n");
				if (r->palette[5] == NULL) s += wxT("Missing human PAL 5\n");
			}
		}
		s += wxT("\n");
	}


	// list placeholder sprites that are removed
	{
		s += wxT("Placeholder sprites:\n");
		unsigned int count = get_item_count(ITEM_ROOM);
		for (unsigned int i=0; i<count; ++i)
		{
			const Room* r = (Room*)get_item(ITEM_ROOM,i);
			for (int d=0; d<16; ++d)
			{
				if (r->dog[d].param == 0 &&
						(
						r->dog[d].type == DOG_SPRITE0 ||
						r->dog[d].type == DOG_SPRITE1 ||
						r->dog[d].type == DOG_SPRITE2
						)
					)
				{
					s += wxString::Format(wxT("Placeholder sprite (%d) removed in room: %s\n"),d,r->name);
				}
			}
		}
		s += wxT("\n");
	}

	// warn about door/pass that doesn't have a link
	{
		s += wxT("Doors to nowhere:\n");

		unsigned int count = get_item_count(ITEM_ROOM);
		for (unsigned int i=0; i<count; ++i)
		{
			const Room* r = (Room*)get_item(ITEM_ROOM,i);
			for (int d=0; d<16; ++d)
			{
				const Room::Dog* dog = &(r->dog[d]);

				unsigned char p = 0;

				switch (dog->type)
				{
				case DOG_DOOR:
				case DOG_PASS:
				case DOG_PASS_X:
				case DOG_PASS_Y:
					p = dog->param & 7;
					break;
				default:
					continue;
				}

				if (r->door[p] == NULL)
				{
					s += wxT("Bad door in room ") + r->name + wxString::Format(wxT(": %d\n"),d);
				}
			}
		}
		s += wxT("\n");
	}

	// warn about door 0 inside a wall
	{
		s += wxT("Door 0s in a wall:\n");

		unsigned int count = get_item_count(ITEM_ROOM);
		for (unsigned int i=0; i<count; ++i)
		{
			const Room* r = (Room*)get_item(ITEM_ROOM,i);

			int dx = r->door_x[0];
			int dy = r->door_y[0];

			int dx0 = dx - 8;
			int dx1 = dx + 7;
			int dy0 = dy - 16;
			int dy1 = dy - 1;

			int corner_x[4] = { dx0, dx1, dx0, dx1 };
			int corner_y[4] = { dy0, dy0, dy1, dy1 };

			bool hit = false;

			for (int c=0; c<4; ++c)
			{
				int tx = corner_x[c] >> 3;
				int ty = corner_y[c] >> 3;

				if (r->collide_tile(tx,ty,true))
				{
					hit = true;
					break;
				}
			}

			if (hit)
			{
				s += wxT("Blocked door 0 in room ") + r->name + wxT("\n");
			}
		}
		s += wxT("\n");
	}

	// validate pass_x/y
	{
		unsigned int room_count = get_item_count(ITEM_ROOM);

		s += wxT("Edge collision pass_x consistency:\n");
		for (unsigned int i=0; i<room_count; ++i)
		{
			const Room* r = (Room*)get_item(ITEM_ROOM,i);
			const int rw = r->scrolling ? 512 : 256;

			for (unsigned int dc=0; dc<16; ++dc)
			{
				const Room::Dog* d = &(r->dog[dc]);
				if (d->type == DOG_PASS_X)
				{
					const int di = d->param & 7;
					const Room* rd = r->door[di];
					if (rd == NULL) rd = r;

					if ((d->x & 7) != 0) s += wxString::Format(wxT("Warning: pass_x (%d) not aligned to tile in room "),dc) + r->name + wxT(" (manually inspect)\n");

					wxString e = wxT("");

					int x = d->x + 4;
					for (int y = 4; (y + 8) < 240; y += 8)
					{
						if (r->collide_pixel(x,y,false) || r->collide_pixel(x,y+8,false)) continue; // no entry
						if (
							(!r->collide_pixel(x-8,y,true) && !r->collide_pixel(x-8,y+8,true)) || // entry from left
							(!r->collide_pixel(x+8,y,true) && !r->collide_pixel(x+8,y+8,true))    // entry from right
						   )
						{
							int dx = rd->door_x[di];
							int dy = y+12;

							int dx0 = dx - 8;
							int dx1 = dx + 7;
							int dy0 = dy - 16;
							int dy1 = dy - 1;

							if (
								rd->collide_pixel(dx0,dy0,true) ||
								rd->collide_pixel(dx1,dy0,true) ||
								rd->collide_pixel(dx0,dy1,true) ||
								rd->collide_pixel(dx1,dy1,true)
							   )
							{
								if (e.length() > 0) e += wxT(",");
								e += wxString::Format(wxT("%d"),int(dy/8));
							}
						}
					}

					if (e.length() > 0)
					{
						s += wxString::Format(wxT("pass_x (%d) in room: "),dc) + r->name + wxT(" > ") + rd->name + wxT(", y tiles: ") + e + wxT("\n");
					}
				}
				else continue;
			}

		}

		s += wxT("\n");
		s += wxT("Edge collision pass_y consistency:\n");
		for (unsigned int i=0; i<room_count; ++i)
		{
			const Room* r = (Room*)get_item(ITEM_ROOM,i);
			const int rw = r->scrolling ? 512 : 256;

			for (unsigned int dc=0; dc<16; ++dc)
			{
				const Room::Dog* d = &(r->dog[dc]);
				if (d->type == DOG_PASS_Y)
				{
					const int di = d->param & 7;
					const Room* rd = r->door[di];
					if (rd == NULL) rd = r;

					if ((d->y & 7) != 0) s += wxString::Format(wxT("Warning: pass_y (%d) not aligned to tile in room "),dc) + r->name + wxT(" (manually inspect)\n");

					wxString e = wxT("");

					int y = d->y + 4;
					for (int x = 4; (x + 8) < rw; x += 8)
					{
						if (r->collide_pixel(x,y,false) || r->collide_pixel(x+8,y,false)) continue; // no entry
						if (
							(!r->collide_pixel(x,y-8,true) && !r->collide_pixel(x+8,y-8,true)) || // entry from above
							(!r->collide_pixel(x,y+8,true) && !r->collide_pixel(x+8,y+8,true))    // entry from below
						   )
						{
							int dx = x+4;
							int dy = rd->door_y[di];

							int dx0 = dx - 8;
							int dx1 = dx + 7;
							int dy0 = dy - 16;
							int dy1 = dy - 1;

							if (
								rd->collide_pixel(dx0,dy0,true) ||
								rd->collide_pixel(dx1,dy0,true) ||
								rd->collide_pixel(dx0,dy1,true) ||
								rd->collide_pixel(dx1,dy1,true)
							   )
							{
								if (e.length() > 0) e += wxT(",");
								e += wxString::Format(wxT("%d"),int(dx/8));
							}
						}
					}

					if (e.length() > 0)
					{
						s += wxString::Format(wxT("pass_y (%d) in room: "),dc) + r->name + wxT(" > ") + rd->name + wxT(", x tiles: ") + e + wxT("\n");
					}
				}
				else continue;
			}
		}
	}

	return result;
}

wxString Data::password_build(unsigned short int zp_current_room, unsigned char zp_current_lizard) const
{
	// password is combination of:
	//   current_room   : 9 bits
	//   current_lizard : 4 bits
	//   parity         : 2 bits
	// packed as 5 3-bit values

	unsigned char zp_password[5];

	//                 bit 0                      bit 1                      bit 2
	zp_password[4] = ((zp_current_room   )&1) | ((zp_current_room>>3)&2) | ((zp_current_lizard<<2)&4) ;
	zp_password[3] = ((zp_current_room>>1)&1) | ((zp_current_room>>4)&2) | ((zp_current_lizard<<1)&4) ;
	zp_password[1] = ((zp_current_room>>2)&1) | ((zp_current_room>>5)&2) | ((zp_current_lizard   )&4) ;
	zp_password[0] = ((zp_current_room>>3)&1) | ((zp_current_room>>6)&2) | ((zp_current_lizard>>1)&4) ;
	zp_password[2] = ((zp_current_room>>8)&1)                                                         ;
	zp_password[2] |= (zp_password[4] ^ zp_password[3] ^ zp_password[1] ^ zp_password[0]) & 0x6; // parity

	// scramble
	zp_password[0] ^= 0x2;
	zp_password[1] ^= 0x1;
	zp_password[3] ^= 0x6;
	zp_password[4] ^= 0x5;

	wxString p = wxString::Format(wxT("%c%c%c%c%c"),
		zp_password[0]+'1',
		zp_password[1]+'1',
		zp_password[2]+'1',
		zp_password[3]+'1',
		zp_password[4]+'1');
	return p;
}

static bool pack_rle(unsigned char (&buffer)[4096], unsigned int unpacked_size, unsigned int &packed_size)
{
	unsigned char uncompressed[sizeof(buffer)];
	memcpy(uncompressed,buffer,sizeof(buffer));

	packed_size = 0;
	unsigned int pos = 0;
	unsigned char v = 0;
	unsigned char run = 0;
	while (true)
	{
		if (pos >= sizeof(buffer))
		{
			return false;
		}

		bool end_reached = (pos >= unpacked_size);
		
		if (end_reached || (run > 0 && uncompressed[pos] != v) || (run > 254))
		{
			if ((v == 255 && run >= 0) || run >= 4)
			{
				buffer[packed_size+0] = 255; // RLE mark
				buffer[packed_size+1] = run;
				buffer[packed_size+2] = v;
				packed_size += 3;
			}
			else
			{
				while (run > 0)
				{
					buffer[packed_size] = v;
					++packed_size;
					--run;
				}
			}

			if (end_reached) break;
			run = 0;
		}

		if (run == 0)
		{
			v = uncompressed[pos];
		}
		else
		{
			wxASSERT(uncompressed[pos] == v);
		}
		++run;
		++pos;
	}

	return true;
}

bool Data::pack_room(Room* r, unsigned char (&buffer)[4096], unsigned int &packed_size)
{
	unsigned char block[4096];
	packed_size = 0;

	// auto-fill some dog parameters
	for (int d=0; d<16; ++d)
	{
		Room::Dog& dog = r->dog[d];

		switch (dog.type)
		{
			case DOG_SPRITE0:
			case DOG_SPRITE1:
			case DOG_SPRITE2:
				{
					// find sprite and assign to param slot
					Sprite* s = (Sprite*)find_item(ITEM_SPRITE,r->dog[d].note);
					if (s == NULL)
					{
						file_error += wxString::Format(wxT("Sprite not found for SPRITE in slot %d: "),d) + r->name + wxT("\n");
						return false;
					}
					else if (s->bank > 2)
					{
						file_error += wxString::Format(wxT("Sprite with invalid bank for SPRITE in slot %d: "),d) + r->name + wxT("\n");
						return false;
					}
					else
					{
						const Game::DogEnum SPRITE_TYPES[3] = { DOG_SPRITE0, DOG_SPRITE1, DOG_SPRITE2 };
						int t = SPRITE_TYPES[s->bank];
						unsigned char p = sprite_export_index(s);
						if (dog.param != p || dog.type != t)
						{
							dog.type = t;
							dog.param = p;
							unsaved = true;
							r->unsaved = true;
						}
					}
				}
				break;
			default:
				break;
		}
	}

	// pack into two blocks
	// 0. bottom of screen, to be recovered when unpausing
	// 1. metadata, top of screen down to pause line
	unsigned int block_unpacked = 0;
	unsigned int block_packed = 0;

	// first RLE block
	// 64 x 8 bytes
	::memset(block,0,sizeof(block));
	block_unpacked = 0;

	// interleaved rows nametable 0/1 (64 bytes x 8)
	// (switch nametabl every 32 bytes)

	for (int y=22; y<30; ++y)
	for (int x=0; x<64; ++x)
	{
		block[block_unpacked] = r->nametable[(y*64)+x];
		++block_unpacked;
	}
	wxASSERT(block_unpacked == (64*8));

	// RLE compression
	block_packed = 0;
	if (!pack_rle(block,block_unpacked,block_packed))
	{
		file_error += wxT("RLE block 0 packing error in room: ") + r->name + wxT("\n");
		return false;
	}
	wxASSERT(packed_size + block_packed < 4096);
	::memcpy(buffer+packed_size,block,block_packed);
	packed_size += block_packed;

	// second RLE block
	// 32 x 49 bytes
	::memset(block,0,sizeof(block));

	// row S0
	// 8 palettes, 8 CHR, water, music, scrolling, 13 pad
	for (int i=0; i<8; ++i)
	{
		int p = find_index(r->palette[i]);
		if (p < 0) p = 0;
		block[(0*32)+ 0+i] = p;

		int c = 254; // 254 = skip
		if (r->chr[i] != NULL)
		{
			c = r->chr[i]->export_index_cache;
			if (c < 0)
			{
				file_error += wxString::Format(wxT("Chr (%s) not exported in slot %d: %s\n"),r->chr[i]->name,i,r->name);
				return false;
			}
		}
		block[(0*32)+ 8+i] = c;
	}
	block[    (0*32)+16+0] = r->water;
	block[    (0*32)+16+1] = r->music;
	block[    (0*32)+16+2] = r->scrolling ? 1 : 0;

	// row S1
	// 8 x door x0, door x1, door link0, door link1
	for (int i=0; i<8; ++i)
	{
		block[(1*32)+ 0+i] = r->door_x[i] >> 8;
		block[(1*32)+ 8+i] = r->door_x[i] & 255;

		int di = find_index(r->door[i]);
		if (di < 0) di = find_index(r);
		if (di < 0) di = 0;
		block[(1*32)+16+i] = di >> 8;
		block[(1*32)+24+i] = di & 255;
	}

	// row S2
	// 8 x door y, pad, 16 x dog type
	for (int i=0; i<8; ++i)
	{
		block[(2*32)+ 0+i] = r->door_y[i];
	}
	for (int i=0; i<16; ++i)
	{
		unsigned char t = r->dog[i].type;
		if (
			(t == DOG_SPRITE0 || t == DOG_SPRITE1 || t == DOG_SPRITE2)
			&& r->dog[i].param == 0)
		{
			t = DOG_NONE;
		}

		block[(2*32)+16+i] = t;
	}

	// row S3
	// 16 x dog x0, dog x1
	for (int i=0; i<16; ++i)
	{
		block[(3*32)+ 0+i] = r->dog[i].x >> 8;
		block[(3*32)+16+i] = r->dog[i].x & 255;
	}

	// row S4
	// 16 x dog y, dog p
	for (int i=0; i<16; ++i)
	{
		block[(4*32)+ 0+i] = r->dog[i].y;
		block[(4*32)+16+i] = r->dog[i].param;
	}

	// 22 rows of nametable 0 (32 bytes x 22)
	// 22 rows of nametable 1 (32 bytes x 22)
	for (int y=0; y<22; ++y)
	for (int x=0; x<32; ++x)
	{
		block[(5*32)+( 0*32)+(y*32)+x] = r->nametable[(y*64)+ 0+x];
		block[(5*32)+(22*32)+(y*32)+x] = r->nametable[(y*64)+32+x];
	}

	// attributes for nametables 0/1 (64 bytes x 2)
	for (int y=0; y<8; ++y)
	{
		for (int x=0; x<16; ++x)
		{
			int ax0 = (x * 2) + 0;
			int ay0 = (y * 2) + 0;
			int ax1 = (x * 2) + 1;
			int ay1 = (y * 2) + 1;
			if (ay1 > 14) ay1 = 14; // duplicate data off the bottom edge

			unsigned int attrib =
				((r->attribute[(ay0 * 32) + ax0] & 3) << 0) |
				((r->attribute[(ay0 * 32) + ax1] & 3) << 2) |
				((r->attribute[(ay1 * 32) + ax0] & 3) << 4) |
				((r->attribute[(ay1 * 32) + ax1] & 3) << 6);

			unsigned int p = (x & 7) + (y * 8);
			if (x >= 8 ) { p += 64; } // second nametable shift +64

			block[((5+22+22)*32)+p] = attrib;
		}
	}

	// RLE compression of first block
	block_unpacked = (5+22+22+4)*32;
	block_packed = 0;

	if (!pack_rle(block,block_unpacked,block_packed))
	{
		file_error += wxT("RLE block 1 packing error in room: ") + r->name + wxT("\n");
		return false;
	}

	wxASSERT(packed_size + block_packed < 4096);
	::memcpy(buffer+packed_size,block,block_packed);
	packed_size += block_packed;

	return true;
}

bool Data::export_(unsigned char start_lizard)
{
	bool result = true;
	file_error = wxT("");

	// create export directory
	wxFileName fn_package = package_file;
	wxFileName fn_path = wxFileName::DirName(fn_package.GetPath());
	fn_path.AppendDir(wxT("export"));
	if (!fn_path.DirExists())
	{
		if (!fn_path.Mkdir())
		{
			result = false;
			file_error += wxT("Could not make directory: ") + fn_path.GetPath() + wxT("\n");
		}
	}

	// create data blobs

	const unsigned int MAX_BLOB_SIZE = 32 * 1024;
	const unsigned int MAX_ROOM_BANK_SIZE = (32 * 1024) - (0x2A7 + 0x66 + 0x6); // leave room for CODE + FIXED + VECTORS
	const unsigned int MAX_CHR_BANK_SIZE = 31 * 1024;
	const unsigned int MAX_ROOMS = 512; // dictated by password
	const unsigned int ROOM_BANKS = 10;
	const unsigned int CHR_BANKS = 2;
	const unsigned int SPRITE_BANKS = 3;

	unsigned char* blob_chr[CHR_BANKS];
	unsigned char* blob_spr[SPRITE_BANKS];
	unsigned char* blob_roo[ROOM_BANKS];
	unsigned char* blob_mini = new unsigned char[MAX_BLOB_SIZE];
	unsigned char* blob_pal = new unsigned char[MAX_BLOB_SIZE];
	unsigned short int blob_chk[256];

	for (unsigned int i=0; i<CHR_BANKS;    ++i) blob_chr[i] = new unsigned char[MAX_BLOB_SIZE];
	for (unsigned int i=0; i<SPRITE_BANKS; ++i) blob_spr[i] = new unsigned char[MAX_BLOB_SIZE];
	for (unsigned int i=0; i<ROOM_BANKS;   ++i) blob_roo[i] = new unsigned char[MAX_BLOB_SIZE];

	unsigned int blob_chr_size[CHR_BANKS] = { 0 };
	unsigned int blob_mini_size = 0;
	unsigned int blob_pal_size = 0;
	unsigned int blob_spr_size[SPRITE_BANKS] = { 0 };
	unsigned int blob_roo_size[ROOM_BANKS];
	unsigned int blob_chk_size = 0;

	::memset(blob_roo_size,0,sizeof(blob_roo_size));

	unsigned int spr_sub_count [SPRITE_BANKS][256];
	bool         spr_vpal      [SPRITE_BANKS][256];
	unsigned int spr_map       [SPRITE_BANKS][256];
	unsigned int spr_count     [SPRITE_BANKS] = { 0 };

	struct RoomEntry {
		unsigned int bank;
		unsigned int offset;
		unsigned int packed_size;
	};
	RoomEntry room_entry[MAX_ROOMS];
	::memset(room_entry,0,sizeof(room_entry));

	// chr blob
	unsigned int chr_bank = 0;
	unsigned int chr_bank_index = 0;
	int chr_export_index = 0;
	for (unsigned int i=0; i<chr.size(); ++i)
	{
		if(chr[i]->name.EndsWith(wxT("_")))
		{
			chr[i]->export_index_cache = -1;
			continue;
		}
		else
		{
			chr[i]->export_index_cache = chr_export_index;
			++chr_export_index;
		}

		bool bad_data = false;

		// allocate blob space
		unsigned int new_size = blob_chr_size[chr_bank] + 1024;
		if (new_size > MAX_CHR_BANK_SIZE)
		{
			++chr_bank;
			chr_bank_index = 0;
			new_size = 1024;

			if (chr_bank >= CHR_BANKS)
			{
				file_error += wxT("Too much CHR for blob export.\n");
				result = false;
				break;
			}
		}

		Chr* c = chr[i];
		for (int t=0; t<64; ++t)
		{
			unsigned char* tile = blob_chr[chr_bank] + blob_chr_size[chr_bank] + (t * 16);
			for (int y=0; y<8; ++y)
			{
				tile[y+0] = 0;
				tile[y+8] = 0;
				for (int x=0; x<8; ++x)
				{
					int pixel = c->data[t][(y*8)+x];
					tile[y+0] |= ((pixel & 1) ? 1:0) << (7-x);
					tile[y+8] |= ((pixel & 2) ? 1:0) << (7-x);
					if (pixel != (pixel & 3)) bad_data = true;
				}
			}
		}
		blob_chr_size[chr_bank] = new_size;

		if (bad_data)
		{
			file_error += wxT("Bad data in CHR: ") + c->name + wxT("\n");
			result = false;
		}
	}

	// mini chr blob
	for (unsigned int i=0; i<chr.size(); ++i)
	{
		if(chr[i]->name.EndsWith(wxT("_")))
		{
			chr[i]->export_index_cache = -1;
			continue;
		}

		// allocate blob space
		unsigned int new_size = blob_mini_size + 64;
		if (new_size > MAX_BLOB_SIZE)
		{
			file_error += wxT("Too much CHR for mini blob export.\n");
			result = false;
			break;
		}

		Chr* c = chr[i];
		unsigned char* pack = blob_mini + blob_mini_size;

		// annoying special cases, see below
		bool chr_bg1 = chr[i]->name == wxT("bg1");
		bool chr_bg5 = chr[i]->name == wxT("bg5");
		bool chr_bg6 = chr[i]->name == wxT("bg6");
		bool chr_fbg = chr[i]->name == wxT("final_bg");

		for (int t=0; t<64; ++t)
		{
			unsigned char tile[4];
			int tx = 0;

			for (int y=0; y<8; y+=4)
			for (int x=0; x<8; x+=4)
			{
				int freq[4] = { // starting freq creates a tie-breaker
						0,
						((t+0)%3)+1,
						((t+1)%3)+1,
						((t+2)%3)+1,
					};
				for (int by=0;by<4;++by)
				for (int bx=0;bx<4;++bx)
				{
					freq[c->data[t][((y+by)*8)+(x+bx)]] += 4;
				}
				int most = 0;
				int most_freq = 3;
				for (int i=1; i<4; ++i)
				{
					if (freq[i] > most_freq)
					{
						most = i;
						most_freq = freq[i];
					}
				}
				tile[tx] = most;
				++tx;
			}

			// annoying hard-wired special case: some exposed electric wires used in steam zone / palace_E_right / void_B5
			// end up completely covering up the sparku/l/r/d dogs, make them transparent to compensate
			bool wipe_tile = false;
			if (chr_bg5 && (
				(t >= 0x04 && t <= 0x06) ||
				(t >= 0x14 && t <= 0x16)))
			{
				wipe_tile = true;
			}
			if (chr_bg6 && (
				t == 0x24 ||
				t == 0x25 ||
				t == 0x37))
			{
				wipe_tile = true;
			}
			if (wipe_tile)
			{
				for (int i=0; i<4; ++i) tile[i] = 0;
			}
			// another special case: the final boss helix looks weird, manually overriding it
			if (chr_fbg)
			{
				if (t == 0x37)
				{
					tile[0] = 3;
					tile[1] = 1;
					tile[2] = 1;
					tile[3] = 3;
				}
				else if (t == 0x38)
				{
					tile[0] = 1;
					tile[1] = 3;
					tile[2] = 3;
					tile[3] = 1;
				}
			}
			// octopus egg floor
			if (chr_bg1 && t == 0x3D)
			{
				for (int i=0; i<4; ++i) tile[i] = 2;
			}

			*pack =
				((tile[0] & 1) << 7) |
				((tile[1] & 1) << 3) |
				((tile[2] & 1) << 6) |
				((tile[3] & 1) << 2) |
				((tile[0] & 2) << 4) |
				((tile[1] & 2) << 0) |
				((tile[2] & 2) << 3) |
				((tile[3] & 2) >> 1) ;
			++pack;
		}
		blob_mini_size = new_size;
	}

	// pal blob
	for (unsigned int i=0; i<palette.size(); ++i)
	{
		bool bad_data = false;

		// allocate blob space
		unsigned int new_size = blob_pal_size + 4;
		if (new_size > MAX_BLOB_SIZE)
		{
			file_error += wxT("Too much palette for blob export.\n");
			result = false;
			break;
		}

		Palette* p = palette[i];
		unsigned char* d = blob_pal + blob_pal_size;
		for (int j=0; j<4; ++j)
		{
			int v = p->data[j];
			if (v == 0x30) v = 0x20; // don't use the second white
			d[j] = v;
			if (v != (v & 0x3F)) bad_data = true;
		}
		blob_pal_size = new_size;

		if (bad_data)
		{
			file_error += wxT("Bad data in palette: ") + p->name + wxT("\n");
			result = false;
		}
	}

	// spr blob
	for (unsigned int i=0; i<sprite.size(); ++i)
	{
		if (sprite[i]->name.EndsWith(wxT("_")))
		{
			continue;
		}

		Sprite* s = sprite[i];
		const int sb = s->bank;
		if (sb < 0 || sb >= SPRITE_BANKS)
		{
			file_error += wxString::Format(wxT("Sprite bank (%d) out of range [0,%d] for sprite: "),sb,SPRITE_BANKS-1) + s->name + wxT("\n");
			result = false;
			continue;
		}

		if (spr_count[sb] >= 256)
		{
			file_error += wxString::Format(wxT("No room in sprite bank %d for sprite: "),sb) + s->name + wxT("\n");
			result = false;
			continue;
		}

		int sc = s->count;
		if (sc < 0 || sc > Sprite::MAX_SPRITE_TILES)
		{
			file_error += wxT("Bad tile count in sprite: ") + s->name + wxT("\n");
			result = false;
			continue;
		}

		spr_sub_count[sb][spr_count[sb]] = sc;
		spr_vpal[sb][spr_count[sb]] = s->vpal;
		spr_map[sb][spr_count[sb]] = i;
		++spr_count[sb];

		for (int j=0; j<sc; ++j)
		{
			if (s->tile[j] < 0 || s->tile[j] > 255 ||
				((s->palette[j] & 0x23) != s->palette[j]))
			{
				file_error += wxT("Bad tile data in sprite: ") + s->name + wxString::Format(wxT(" tile %d\n"),j);
				result = false;
			}

			signed char   sx = s->x[j];
			signed char   sy = s->y[j] - 1; // note NES offsets sprite position by 1 line
			unsigned char st = s->tile[j];
			unsigned char sp = // NES sprite attributes
				(s->palette[j] & 0x23) |
				(s->flip_x[j] ? (1<<6) : 0) |
				(s->flip_y[j] ? (1<<7) : 0) ;

			blob_spr[sb][blob_spr_size[sb]+0] = sy;
			blob_spr[sb][blob_spr_size[sb]+1] = st;
			blob_spr[sb][blob_spr_size[sb]+2] = sp;
			blob_spr[sb][blob_spr_size[sb]+3] = sx;
			blob_spr_size[sb] += 4;
		}
	}

	// assign coins/flags
	int coins = 0;
	int flags = 0;
	if (!assign_coins_and_flags(coins,flags))
	{
		result = false;
	}

	// roo blob
	unsigned int roo_bank = 0;
	unsigned int roo_bank_size = 0;
	unsigned int roo_bank_room_count = 1;
	for (unsigned int i=0; i<room.size(); ++i)
	{
		if (i >= MAX_ROOMS)
		{
			file_error += wxT("Too many rooms!\n");
			result = false;
			break;
		}

		unsigned char buffer[4096];
		unsigned int packed_size;
		if (!pack_room(room[i], buffer, packed_size))
		{
			packed_size = 0;
			result = false;
		}

		if (((packed_size + 2 + // data to add
			  blob_roo_size[roo_bank] + // data already in bank
			  (roo_bank_room_count * 2)) // 2-byte table entries
				>= MAX_ROOM_BANK_SIZE) ||
			(roo_bank_size >= 256)) // max 256 in table
		{
			 // overflow to next bank
			++roo_bank;
			roo_bank_size = 0;
			roo_bank_room_count = 1;
		}
		else
		{
			++roo_bank_size;
			++roo_bank_room_count;
		}


		if (roo_bank >= ROOM_BANKS)
		{
			file_error += wxT("Out of space for room data at room: ") + room[i]->name + wxT("\n");
			result = false;
			break;
		}

		room_entry[i] = { roo_bank, blob_roo_size[roo_bank], packed_size };
		::memcpy(blob_roo[roo_bank] + blob_roo_size[roo_bank], buffer, packed_size);
		blob_roo_size[roo_bank] += packed_size;
	}

	// checkpoints blob
	blob_chk_size = 0;
	for (unsigned int i=0; i<room.size(); ++i)
	{
		if (blob_chk_size >= 255)
		{
			file_error += wxT("Too many checkpoints found.\n");
			result = false;
			break;
		}

		Room* r = room[i];
		bool checkpoint = false;
		for (int d=0; d<16; ++d)
		{
			if (r->dog[d].type == DOG_SAVE_STONE)
			{
				checkpoint = true;
				break;
			}
		}

		if (checkpoint)
		{
			if (i > 65535)
			{
				file_error += wxString::Format(wxT("Room %d out of range for checkpoint: "),i) + r->name + wxT("\n");
				result = false;
			}
			blob_chk[blob_chk_size] = i;
			++blob_chk_size;
		}
	}


	// data files
	const int DATA_FILE_COUNT = 21;
	const wxChar* data_filename_base[DATA_FILE_COUNT] = {
		wxT("data.h"), // 0
		wxT("data.cpp"), // 1
		wxT("chr0.s"), // 2
		wxT("chr1.s"), // 3
		wxT("mini.s"), // 4
		wxT("palette.s"),
		wxT("sprite0.s"),
		wxT("sprite1.s"),
		wxT("sprite2.s"),
		wxT("room0.s"),
		wxT("room1.s"),
		wxT("room2.s"),
		wxT("room3.s"),
		wxT("room4.s"),
		wxT("room5.s"),
		wxT("room6.s"),
		wxT("room7.s"),
		wxT("room8.s"),
		wxT("room9.s"),
		wxT("checkpoints.s"),
		wxT("names.s"),
	};
	wxFileName* data_filename[DATA_FILE_COUNT] = {NULL};
	wxTextFile* data_file    [DATA_FILE_COUNT] = {NULL};

	for (int i=0; i<DATA_FILE_COUNT; ++i)
	{
		data_filename[i] = new wxFileName(fn_path.GetPath(),data_filename_base[i]);
		data_file[i] = new wxTextFile(data_filename[i]->GetFullPath());
	}

	// shorthand access
	wxTextFile& fd             = *data_file[ 0];
	wxTextFile& fc             = *data_file[ 1];
	wxTextFile& fn_chr0        = *data_file[ 2];
	wxTextFile& fn_chr1        = *data_file[ 3];
	wxTextFile& fn_mini        = *data_file[ 4];
	wxTextFile& fn_palette     = *data_file[ 5];
	wxTextFile& fn_sprite0     = *data_file[ 6];
	wxTextFile& fn_sprite1     = *data_file[ 7];
	wxTextFile& fn_sprite2     = *data_file[ 8];
	wxTextFile& fn_room0       = *data_file[ 9];
	wxTextFile& fn_room1       = *data_file[10];
	wxTextFile& fn_room2       = *data_file[11];
	wxTextFile& fn_room3       = *data_file[12];
	wxTextFile& fn_room4       = *data_file[13];
	wxTextFile& fn_room5       = *data_file[14];
	wxTextFile& fn_room6       = *data_file[15];
	wxTextFile& fn_room7       = *data_file[16];
	wxTextFile& fn_room8       = *data_file[17];
	wxTextFile& fn_room9       = *data_file[18];
	wxTextFile& fn_checkpoints = *data_file[19];
	wxTextFile& fn_names       = *data_file[20];

	for (int i=0; i<DATA_FILE_COUNT; ++i)
	{
		if (!data_file[i]->Exists()) data_file[i]->Create();
		data_file[i]->Open();
		if (!data_file[i]->IsOpened())
		{
			file_error += wxT("Could not open file: ")+data_filename[i]->GetFullPath()+wxT("\n");
			result = false;
		}
		else
		{
			data_file[i]->Clear();
		}
	}

	if (result != false)
	{
		wxString now = wxNow();

		fd.AddLine(wxT("#pragma once"));
		fd.AddLine(wxT("// automatically generated export for: ") + fn_package.GetName());
		fd.AddLine(wxT("// ") + now);
		fd.AddLine(wxT(""));

		fc.AddLine(wxT("// automatically generated export for: ") + fn_package.GetName());
		fc.AddLine(wxT("// ") + now);
		fc.AddLine(wxT("#include \"../../lizard_game.h\""));
		fc.AddLine(wxT("#include \"data.h\""));
		fc.AddLine(wxT(""));

		for (int i=2; i < DATA_FILE_COUNT; ++i)
		{
			data_file[i]->AddLine(wxT("; automatically generated export for: ") + fn_package.GetName());
			data_file[i]->AddLine(wxT("; ") + now);
			data_file[i]->AddLine(wxT(""));
		}

		wxTextFile* fn = NULL;

		// Enums
		fn = &fn_names;
		for (int i=0; i<ITEM_TYPE_COUNT; ++i)
		{
			fd.AddLine(wxT("// ") + wxString(ITEM_TYPE_NAME[i]));
			fn->AddLine(wxT("; ") + wxString(ITEM_TYPE_NAME[i]));
			if (i == ITEM_SPRITE)
			{
				for (int sb = 0; sb<SPRITE_BANKS; ++sb)
				{
					for (unsigned int j=0; j<spr_count[sb]; ++j)
					{
						Sprite* s = (Sprite*)get_item(i,spr_map[sb][j]);
						fd.AddLine(wxString::Format(wxT("const int DATA_sprite%d_%s = %d;"),s->bank,s->name,j));
						fn->AddLine(wxString::Format(wxT("DATA_sprite%d_%s = %d"),s->bank,s->name,j));
					}

					fd.AddLine(wxT("const int DATA_") + wxString(ITEM_TYPE_NAME[i])
						+ wxString::Format(wxT("%d_COUNT = %d;"),sb,spr_count[sb]));
					fn->AddLine(wxT("DATA_") + wxString(ITEM_TYPE_NAME[i])
						+ wxString::Format(wxT("%d_COUNT = %d"),sb,spr_count[sb]));

					fd.AddLine(wxT(""));
					fn->AddLine(wxT(""));
				}
			}
			else if (i == ITEM_CHR)
			{
				int jm = 0;
				for (unsigned int j=0; j<get_item_count(i); ++j)
				{
					Chr* c = (Chr*)get_item(i,j);
					if (c->export_index_cache >= 0)
					{
						fd.AddLine(wxT("const int DATA_chr_") +
							c->name + wxString::Format(wxT(" = %d;"),c->export_index_cache));
						fn->AddLine(wxT("DATA_chr_") +
							c->name + wxString::Format(wxT(" = %d"),c->export_index_cache));
						++jm;
					}
				}
				fd.AddLine(wxString::Format(wxT("const int DATA_chr_COUNT = %d;"),jm));
				fn->AddLine(wxString::Format(wxT("DATA_chr_COUNT = %d"),jm));

				fd.AddLine(wxT(""));
				fn->AddLine(wxT(""));
			}
			else
			{
				int jm = 0;
				for (unsigned int j=0; j<get_item_count(i); ++j)
				{
					fd.AddLine(wxT("const int DATA_") + wxString(ITEM_TYPE_NAME[i]) + wxT("_") +
						get_item(i,j)->name + wxString::Format(wxT(" = %d;"),jm));
					fn->AddLine(wxT("DATA_") + wxString(ITEM_TYPE_NAME[i]) + wxT("_") +
						get_item(i,j)->name + wxString::Format(wxT(" = %d"),jm));
					++jm;
				}
				fd.AddLine(wxT("const int DATA_") + wxString(ITEM_TYPE_NAME[i])
					+ wxString::Format(wxT("_COUNT = %d;"),jm));
				fn->AddLine(wxT("DATA_") + wxString(ITEM_TYPE_NAME[i])
					+ wxString::Format(wxT("_COUNT = %d"),jm));

				fd.AddLine(wxT(""));
				fn->AddLine(wxT(""));
			}
		}
		if (start_lizard >= LIZARD_OF_COUNT) start_lizard = 0;
		fd.AddLine(wxString::Format(wxT("const uint8 LIZARD_OF_START = %d;"),start_lizard));
		fd.AddLine(wxT(""));
		fn->AddLine(wxString::Format(wxT("LIZARD_OF_START = %d"),start_lizard));
		fn->AddLine(wxT(""));
		fd.AddLine(wxT("// checkpoints"));
		fd.AddLine(wxString::Format(wxT("const int DATA_checkpoints_COUNT = %d;"),blob_chk_size));
		fd.AddLine(wxString::Format(wxT("extern const uint16 data_checkpoints[%d];"),blob_chk_size));
		fd.AddLine(wxT(""));
		fn_checkpoints.AddLine(wxString::Format(wxT("DATA_checkpoints_COUNT = %d"),blob_chk_size));
		fn_checkpoints.AddLine(wxT(""));

		fn->AddLine(wxString::Format(wxT("DATA_COIN_COUNT = %d"),coins));
		fn->AddLine(wxT(""));
		fd.AddLine(wxString::Format(wxT("const uint8 DATA_COIN_COUNT = %d;"),coins));
		fd.AddLine(wxT(""));

		// CHR
		unsigned char* d = NULL;
		wxTextFile* fn_chr[CHR_BANKS] = { &fn_chr0, &fn_chr1 };
		unsigned int chr_block_start = 0;
		fd.AddLine(wxT("extern const uint8 data_chr[DATA_chr_COUNT][1024];"));
		fc.AddLine(wxT("const uint8 data_chr[DATA_chr_COUNT][1024] = {"));
		for (int cb=0; cb < CHR_BANKS; ++cb)
		{
			if (fn_chr[cb] == NULL)
			{
				file_error += wxString::Format(wxT("Missing file entry for CHR bank %d?\n"),cb);
				result = false;
				continue;
			}

			unsigned int c,ci;

			unsigned int chr_count = blob_chr_size[cb] / 1024;
			d = blob_chr[cb];
			fn = fn_chr[cb];
			fn->AddLine(wxT(".segment \"CHR\""));
			for (c=0, ci=chr_block_start; c<chr_count; ++ci)
			{
				int ce = chr[ci]->export_index_cache;
				if (ce < 0) continue; // not exported
				++c;

				fc.AddLine(wxString::Format(wxT("{ // %02X: "),ce)+chr[ci]->name);
				fn->AddLine(wxString::Format(wxT("data_chr_%02X: ; "),ce)+chr[ci]->name);
				for (int t=0; t<64; ++t)
				{
					wxString l = wxT("\t");
					wxString ln = wxT(".byte ");
					for (int b=0; b<16; ++b)
					{
						l += wxString::Format(wxT("0x%02X,"),*d);
						if (b>0) ln += wxT(",");
						ln += wxString::Format(wxT("$%02X"),*d);
						++d;
					}
					fc.AddLine(l);
					fn->AddLine(ln);
				}
				fc.AddLine(wxT("},"));
				fn->AddLine(wxT(""));
			}

			unsigned int chr_block_end = ci;

			// pointer tables for assembly
			fn->AddLine(wxT(".segment \"DATA\""));
			fn->AddLine(wxT("data_chr_low:"));
			for (unsigned int c=0, ci=chr_block_start; c<chr_count; ++ci)
			{
				int ce = chr[ci]->export_index_cache;
				if (ce < 0) continue; // not exported
				++c;

				fn->AddLine(wxString::Format(wxT(".byte <data_chr_%02X"),ce));
			}
			fn->AddLine(wxT("data_chr_high:"));
			for (unsigned int c=0, ci=chr_block_start; c<chr_count; ++ci)
			{
				int ce = chr[ci]->export_index_cache;
				if (ce < 0) continue; // not exported
				++c;

				fn->AddLine(wxString::Format(wxT(".byte >data_chr_%02X"),ce));
			}
			fn->AddLine(wxT(""));

			chr_block_start = chr_block_end;
		}
		fc.AddLine(wxT("};"));
		fc.AddLine(wxT(""));

		// Mini CHR
		fn = &fn_mini;
		fd.AddLine(wxString::Format(wxT("extern const uint8 data_mini[%d];"),blob_mini_size));
		fc.AddLine(wxString::Format(wxT("const uint8 data_mini[%d] = {"),blob_mini_size));
		fn->AddLine(wxT(".segment \"CHR\""));
		fn->AddLine(wxT("data_mini:"));
		wxASSERT((blob_mini_size & 15) == 0); // size must be multiple of 16
		for (unsigned int i=0; i<blob_mini_size; i+=16)
		{
			wxString l = wxT("\t");
			wxString ln = wxT(".byte ");
			for (int b=0; b<16; ++b)
			{
				l += wxString::Format(wxT("0x%02X,"),blob_mini[i+b]);
				if (b>0) ln += wxT(",");
				ln += wxString::Format(wxT("$%02X"),blob_mini[i+b]);
			}
			fc.AddLine(l);
			fn->AddLine(ln);
		}
		fc.AddLine(wxT("};"));
		fc.AddLine(wxT(""));
		fn->AddLine(wxT(""));

		// Palette
		unsigned int pal_count = blob_pal_size / 4;
		d = blob_pal;
		fn = &fn_palette;
			fn->AddLine(wxT(".segment \"PALETTE\""));
		fd.AddLine(wxT("extern const uint8 data_palette[DATA_palette_COUNT][4];"));
		fc.AddLine(wxT("const uint8 data_palette[DATA_palette_COUNT][4] = {"));
		for (unsigned int p=0; p<pal_count; ++p)
		{
			wxString l = wxT("\t{");
			for (int b=0; b<4; ++b)
			{
				unsigned char v = *d;
				if ((v & 0x0F) == 0x0F) v = 0x0F; // remove diagnostic colour
				l += wxString::Format(wxT(" 0x%02X,"),v);
				++d;
			}
			l += wxString::Format(wxT(" }, // %02X: "),p) + palette[p]->name;
			fc.AddLine(l);
		}
		fc.AddLine(wxT("};"));
		fc.AddLine(wxT(""));

		for (unsigned int c=0; c<4; ++c)
		{
			fn->AddLine(wxString::Format(wxT("data_palette_%X:"),c));
			d = blob_pal + c;
			for (unsigned int p=0; p<pal_count; ++p)
			{
				unsigned char v = *d;
				if ((v & 0x0F) == 0x0F) v = 0x0F; // remove diagnostic colour
				fn->AddLine(wxString::Format(wxT(".byte $%02X ; %02X: "),v,p) + palette[p]->name);
				d += 4;
			}
		}
		fn->AddLine(wxT(""));

		// Sprites
		for (int sb=0; sb<SPRITE_BANKS; ++sb)
		{
			wxTextFile* fn_sprite_select[SPRITE_BANKS] = { &fn_sprite0, &fn_sprite1, &fn_sprite2 };

			if (fn_sprite_select[sb] == NULL)
			{
				file_error += wxString::Format(wxT("Missing file entry for Sprite bank %d?\n"),sb);
				result = false;
				continue;
			}

			d = blob_spr[sb];
			fn = fn_sprite_select[sb];

			fd.AddLine(wxString::Format(wxT("extern const uint8 data_sprite%d_tiles[%d];"),sb,blob_spr_size[sb]));
			fc.AddLine(wxString::Format(wxT("const uint8 data_sprite%d_tiles[%d] = {"),sb,blob_spr_size[sb]));
			fn->AddLine(wxT(".segment \"SPRITE\""));
			fn->AddLine(wxString::Format(wxT("data_sprite%d_tiles:"),sb));
			unsigned int subs = 0;
			unsigned int subsc = 0;
			for (unsigned int s=0; s<blob_spr_size[sb]; s+=4)
			{
				unsigned int sm = spr_map[sb][subs];
				wxASSERT(subs < spr_count[sb] && sm < sprite.size());
				fc.AddLine(wxString::Format(wxT("\t0x%02X, 0x%02X, 0x%02X, 0x%02X, // %02d of %02X: "),
					d[0], d[1], d[2], d[3], subsc,subs) + sprite[sm]->name);
				fn->AddLine(wxString::Format(wxT(".byte $%02X, $%02X, $%02X, $%02X ; %02d of %02X: "),
					d[0], d[1], d[2], d[3], subsc,subs) + sprite[sm]->name);
				d += 4;

				++subsc;
				while (subsc >= spr_sub_count[sb][subs] && subs < spr_count[sb])
				{
					++subs;
					subsc = 0;
				}
			}
			fc.AddLine(wxT("};"));
			fc.AddLine(wxT(""));
			fn->AddLine(wxT(""));
			fd.AddLine(wxString::Format(wxT("extern const uint8* const data_sprite%d[DATA_sprite%d_COUNT];"),sb,sb));
			fc.AddLine(wxString::Format(wxT("const uint8* const data_sprite%d[DATA_sprite%d_COUNT] = {"),sb,sb));
			fn->AddLine(wxString::Format(wxT("data_sprite%d_low:"),sb));
			subsc = 0;
			for (unsigned int s=0; s<spr_count[sb]; ++s)
			{
				unsigned int sm = spr_map[sb][s];
				fc.AddLine(wxString::Format(wxT("\tdata_sprite%d_tiles + %4d, // %02X: "),sb,subsc*4,s) + sprite[sm]->name);
				fn->AddLine(wxString::Format(wxT(".byte <(data_sprite%d_tiles + %4d) ; %02X: "),sb,subsc*4,s) + sprite[sm]->name);
				subsc += spr_sub_count[sb][s];
			}
			fc.AddLine(wxT("};"));
			fc.AddLine(wxT(""));
			fn->AddLine(wxString::Format(wxT("data_sprite%d_high:"),sb));
			subsc = 0;
			for (unsigned int s=0; s<spr_count[sb]; ++s)
			{
				unsigned int sm = spr_map[sb][s];
				fn->AddLine(wxString::Format(wxT(".byte >(data_sprite%d_tiles + %4d) ; %02X: "),sb,subsc*4,s) + sprite[sm]->name);
				subsc += spr_sub_count[sb][s];
			}
			fn->AddLine(wxT(""));
			fd.AddLine(wxString::Format(wxT("extern const int data_sprite%d_tile_count[DATA_sprite%d_COUNT];"),sb,sb));
			fc.AddLine(wxString::Format(wxT("const int data_sprite%d_tile_count[DATA_sprite%d_COUNT] = {"),sb,sb));
			fn->AddLine(wxString::Format(wxT("data_sprite%d_tile_count:"),sb));
			subsc = 0;
			for (unsigned int s=0; s<spr_count[sb]; ++s)
			{
				unsigned int sm = spr_map[sb][s];
				fc.AddLine(wxString::Format(wxT("\t%2d, // "),spr_sub_count[sb][s]) + sprite[sm]->name);
				fn->AddLine(wxString::Format(wxT(".byte %2d ; "),spr_sub_count[sb][s]) + sprite[sm]->name);
			}
			fc.AddLine(wxT("};"));
			fc.AddLine(wxT(""));
			fn->AddLine(wxT(""));
			fd.AddLine(wxString::Format(wxT("extern const bool data_sprite%d_vpal[DATA_sprite%d_COUNT];"),sb,sb));
			fc.AddLine(wxString::Format(wxT("const bool data_sprite%d_vpal[DATA_sprite%d_COUNT] = {"),sb,sb));
			fn->AddLine(wxString::Format(wxT("data_sprite%d_vpal:"),sb));
			subsc = 0;
			for (unsigned int s=0; s<spr_count[sb]; ++s)
			{
				unsigned int sm = spr_map[sb][s];
				fc.AddLine(wxString(wxT("\t")) + (spr_vpal[sb][s]?wxT("true"):wxT("false")) + wxT(", // ") + sprite[sm]->name);
				fn->AddLine(wxString::Format(wxT(".byte %2d ; "),(spr_vpal[sb][s] ? 1 : 0)) + sprite[sm]->name);
			}
			fc.AddLine(wxT("};"));
			fc.AddLine(wxT(""));
			fn->AddLine(wxT(""));
		}

		// Rooms
		unsigned int roo_count = room.size();
		wxTextFile* fn_rooms[ROOM_BANKS] = { &fn_room0, &fn_room1, &fn_room2, &fn_room3, &fn_room4, &fn_room5, &fn_room6, &fn_room7, &fn_room8, &fn_room9 };
		// room data
		for (unsigned int bank=0; bank<ROOM_BANKS; ++bank)
		{
			fn = fn_rooms[bank];
			if (fn_rooms[bank] == NULL)
			{
				file_error += wxString::Format(wxT("Missing file entry for Room bank %d?\n"),bank);
				result = false;
				continue;
			}

			fn->AddLine(wxT(".segment \"ROOM\""));
		}
		for (unsigned int r=0; r<roo_count; ++r)
		{
			unsigned int bank = room_entry[r].bank;
			unsigned int offset = room_entry[r].offset;
			unsigned int packed_size = room_entry[r].packed_size;

			fn = fn_rooms[bank];
			if (fn == NULL) continue;

			fc.AddLine(wxString::Format(wxT("const uint8 data_room_%03X[%d] = {"),r,packed_size));
			fn->AddLine(wxString::Format(wxT("data_room_%03X: ; "),r)+room[r]->name);

			unsigned int p = 0;
			unsigned int lc = 0;
			wxString l = wxT("\t");
			wxString ln = wxT(".byte ");
			while (p < packed_size)
			{
				l += wxString::Format(wxT("0x%02X,"),blob_roo[bank][offset+p]);
				if (lc > 0) ln += wxT(",");
				ln += wxString::Format(wxT("$%02X"),blob_roo[bank][offset+p]);
				++p;
				++lc;
				if (lc >= 32)
				{
					fc.AddLine(l);
					fn->AddLine(ln);
					l = wxT("\t");
					ln = wxT(".byte ");
					lc = 0;
				}
			}
			fc.AddLine(l);
			fc.AddLine(wxT("};"));
			if(lc > 0) fn->AddLine(ln);
			fn->AddLine(wxT(""));
		}
		// room table
		fd.AddLine(wxT("extern const uint8* data_room[DATA_room_COUNT];"));
		fc.AddLine(wxT(""));
		fc.AddLine(wxT("const uint8* data_room[DATA_room_COUNT] = {"));
		for (unsigned int r=0; r<roo_count; ++r)
		{
			fc.AddLine(wxString::Format(wxT("\tdata_room_%03X,"),r));
		}
		fc.AddLine(wxT("};"));
		for (unsigned int bank=0; bank<ROOM_BANKS; ++bank)
		{
			fn = fn_rooms[bank];
			if (fn == NULL) continue;

			fn->AddLine(wxT(""));

			unsigned int bank_start = 0;
			unsigned int bank_end = 0;

			for (unsigned int r=0; r<roo_count; ++r)
			{
				bank_start = r;
				if (bank == room_entry[r].bank) break;
			}
			for (unsigned int r=bank_start; r<(roo_count+1); ++r)
			{
				bank_end = r;
				if (r >= roo_count) break;
				if (bank != room_entry[r].bank) break;
			}

			fn->AddLine(wxString::Format(wxT("DATA_ROOM_BANK_START = $%04X"),bank_start));
			fn->AddLine(wxString::Format(wxT("DATA_ROOM_BANK_END   = $%04X"),bank_end  ));
			fn->AddLine(wxT(""));

			fn->AddLine(wxT(".segment \"DATA\""));
			fn->AddLine(wxT("data_room_low:"));
			for (unsigned int r=bank_start; r<bank_end; ++r)
			{
				fn->AddLine(wxString::Format(wxT(".byte <data_room_%03X"),r));
			}

			fn->AddLine(wxT("data_room_high:"));
			for (unsigned int r=bank_start; r<bank_end; ++r)
			{
				fn->AddLine(wxString::Format(wxT(".byte >data_room_%03X"),r));
			}

			fn->AddLine(wxT(""));
		}
		// size table for cross check
		fd.AddLine(wxT("extern const unsigned int data_room_packed_size[DATA_room_COUNT];"));
		fc.AddLine(wxT("const unsigned int data_room_packed_size[DATA_room_COUNT] = {"));
		for (unsigned int r=0; r<roo_count; ++r)
		{
			fc.AddLine(wxString::Format(wxT("\t%d,"),room_entry[r].packed_size));
		}
		fc.AddLine(wxT("};"));
		fc.AddLine(wxT(""));

		// Checkpoints
		fn = &fn_checkpoints;
		fc.AddLine(wxString::Format(wxT("const uint16 data_checkpoints[%d] = {"),blob_chk_size));
		fn->AddLine(wxT(".segment \"DATA\""));
		fn->AddLine(wxT("data_checkpoints_low:"));
		for (unsigned int i=0; i<blob_chk_size; i += 16)
		{
			wxString l = wxT("\t");
			wxString ln = wxT(".byte ");
			for (unsigned int j=0; j<16; ++j)
			{
				if ((i+j) >= blob_chk_size) break;
				if (j != 0)
					ln += wxT(",");
				l += wxString::Format(wxT("0x%04X,"),blob_chk[i+j]);
				ln += wxString::Format(wxT("$%02X"),blob_chk[i+j] & 255);
			}
			fc.AddLine(l);
			fn->AddLine(ln);
		}
		fc.AddLine(wxT("};"));
		fc.AddLine(wxT(""));
		fn->AddLine(wxT(""));
		fn->AddLine(wxT("data_checkpoints_high:"));
		for (unsigned int i=0; i<blob_chk_size; i += 16)
		{
			wxString ln = wxT(".byte ");
			for (unsigned int j=0; j<16; ++j)
			{
				if ((i+j) >= blob_chk_size) break;
				if (j != 0)
					ln += wxT(",");
				ln += wxString::Format(wxT("$%02X"),blob_chk[i+j] >> 8);
			}
			fn->AddLine(ln);
		}
		fn->AddLine(wxT(""));

		// Names
		/*
		for (int i=0; i<ITEM_TYPE_COUNT; ++i)
		{
			fd.AddLine(wxT("extern const char* const data_") + wxString(ITEM_TYPE_NAME[i])
				+ wxString::Format(wxT("_name[%d];"),get_item_count(i)));
			fc.AddLine(wxT("const char* const data_") + wxString(ITEM_TYPE_NAME[i])
				+ wxString::Format(wxT("_name[%d] = {"),get_item_count(i)));

			for (unsigned int j=0; j<get_item_count(i); ++j)
			{
				fc.AddLine(wxT("\t\"") + get_item(i,j)->name + wxT("\","));
			}
			fc.AddLine(wxT("};"));
			fc.AddLine(wxT(""));
		}
		*/

		fd.AddLine(wxT("// end of file"));
		fc.AddLine(wxT("// end of file"));
		for (int i=2; i<DATA_FILE_COUNT; ++i)
		{
			data_file[i]->AddLine(wxT("; end of file"));
		}

		for (int i=0; i<DATA_FILE_COUNT; ++i)
		{
			data_file[i]->Write();
			data_file[i]->Close();
		}
	}
	else
	{
		for (int i=0; i<DATA_FILE_COUNT; ++i)
		{
			if (data_file[i]->IsOpened())
				data_file[i]->Close();
		}
	}

	for (int i=0; i<DATA_FILE_COUNT; ++i)
	{
		delete data_file[i];
		delete data_filename[i];
	}

	// stats file
	wxFileName fn_stats = wxFileName(fn_path.GetPath(),wxT("stats.txt"));
	wxTextFile fs(fn_stats.GetFullPath());
	if (!fs.Exists()) fs.Create();
	fs.Open();
	if (!fs.IsOpened())
	{
		file_error += wxT("Could not open stats file: ")+fn_stats.GetFullPath()+wxT("\n");
		result = false;
	}
	else
	{
		unsigned int roo_count = room.size();

		fs.Clear();
		for (unsigned int i=0; i<2; ++i)
		{
			fs.AddLine(wxString::Format(wxT("CHR%d:    %4d in %8d bytes (%4dkb)"),i,blob_chr_size[i]/1024,blob_chr_size[i],(blob_chr_size[i]+1023)/1024));
		}
		fs.AddLine(wxString::Format(wxT("Palette: %4d in %8d bytes (%4dkb)"),palette.size(),blob_pal_size,(blob_pal_size+1023)/1024));
		for (unsigned int i=0; i<SPRITE_BANKS; ++i)
		{
			fs.AddLine(wxString::Format(wxT("Sprite%d: %4d in %8d bytes (%4dkb)"),i,spr_count[i],blob_spr_size[i],(blob_spr_size[i]+1023)/1024));
		}
		for (unsigned int i=0; i<ROOM_BANKS; ++i)
		{
			unsigned int rooms = 0;
			for (unsigned int j=0; j<roo_count; ++j) { if (room_entry[j].bank == i) ++rooms; }

			unsigned int fullsize = blob_roo_size[i] + (rooms * 2);
			fs.AddLine(wxString::Format(wxT("Room%d:   %4d in %8d bytes (%4dkb) left: %8d bytes"),i,rooms,fullsize,(fullsize+1023)/1024,MAX_ROOM_BANK_SIZE-fullsize));
		}
		fs.AddLine(wxString::Format(wxT("Mini:            %8d bytes (%4dkb)"),blob_mini_size,(blob_mini_size+1023)/1024));
		fs.AddLine(wxT(""));

		fs.AddLine(wxString::Format(wxT("Coins: %d"),coins));
		fs.AddLine(wxString::Format(wxT("Flags: %d"),flags));
		fs.AddLine(wxString::Format(wxT("Rooms: %d"),roo_count));
		fs.AddLine(wxT(""));

		if (roo_count >= MAX_ROOMS) roo_count = (MAX_ROOMS-1); // overflow (error would be indicated during build above)
		for (unsigned int i=0; i<roo_count; ++i)
		{
			fs.AddLine(wxString::Format(wxT("Room %03X: %4d bytes in bank %d ("),i,room_entry[i].packed_size,room_entry[i].bank)+room[i]->name + wxT(")"));
		}
		fs.AddLine(wxT(""));

		if (!result)
		{
			fs.AddLine(wxT("Reported errors:"));
			fs.AddLine(file_error);
			fs.AddLine(wxT(""));
		}

		fs.AddLine(wxT("End of report."));
		fs.Write();
		fs.Close();
	}

	for (int i=0; i<ROOM_BANKS;   ++i) delete [] blob_roo[i];
	for (int i=0; i<CHR_BANKS;    ++i) delete [] blob_chr[i];
	for (int i=0; i<SPRITE_BANKS; ++i) delete [] blob_spr[i];
	delete [] blob_mini;
	delete [] blob_pal;

	wxFileName fn_password = wxFileName(fn_path.GetPath(),wxT("password.csv"));
	wxTextFile f_password(fn_password.GetFullPath());
	if (!f_password.Exists()) f_password.Create();
	f_password.Open();
	if (!f_password.IsOpened())
	{
		file_error += wxT("Could not open file: ")+fn_password.GetFullPath()+wxT("\n");
		result = false;
	}
	else
	{
		f_password.Clear();

		f_password.AddLine(wxT("Checkpoints:"));

		// header
		wxString s = wxT("Hex,  ");
		for (unsigned char l=0; l < LIZARD_COUNT; ++l)
		{
			wxString liz(LIZARD_LIST[l]);
			liz += wxT(", ");
			s += wxString::Format(wxT("%-11s"),liz);
		}
		s += wxT("Room");
		f_password.AddLine(s);

		// checkpoint list
		for (unsigned short int r=0; r < room.size(); ++r)
		{
			bool checkpoint = false;
			for (unsigned int c=0; c<blob_chk_size; ++c)
			{
				if (blob_chk[c] == r)
				{
					checkpoint = true;
					break;
				}
			}
			if (!checkpoint) continue;

			s = wxString::Format(wxT("%03X,  "),r);
			for (unsigned char l=0; l < LIZARD_COUNT; ++l)
				s += wxString::Format(wxT("%-11s"),password_build(r,l)+wxT(", "));
			s += room[r]->name;
			f_password.AddLine(s);
		}

		// full list

		f_password.AddLine(wxT(""));
		f_password.AddLine(wxT("All:"));

		s = wxT("Hex,  ");
		for (unsigned char l=0; l < LIZARD_COUNT; ++l)
		{
			wxString liz(LIZARD_LIST[l]);
			liz += wxT(", ");
			s += wxString::Format(wxT("%-11s"),liz);
		}
		s += wxT("Room");
		f_password.AddLine(s);

		for (unsigned short int r=0; r < room.size(); ++r)
		{
			s = wxString::Format(wxT("%03X,  "),r);
			for (unsigned char l=0; l < LIZARD_COUNT; ++l)
				s += wxString::Format(wxT("%-11s"),password_build(r,l)+wxT(", "));
			s += room[r]->name;
			f_password.AddLine(s);
		}

		// finished

		f_password.Write();
		f_password.Close();
	}

	if (!validate()) result = false;

	if (result) file_error = wxT("Export successful.");
	return result;
}

bool is_background(unsigned char r, unsigned char g, unsigned char b); // forward declaration

static unsigned char* map_render_buffer = NULL;
bool Data::render_maps(bool underlay)
{
	// note: could use PNG but BMP saves much faster
	const wxString FN_COMPOSITE = underlay ? wxT("map_underlay.bmp") : wxT("map.bmp");
	const wxString FN_COLLISION = wxT("map_collide.bmp");
	const int IMG_FORMAT = wxBITMAP_TYPE_BMP;

	file_error = wxT("");

	// create export directory
	wxFileName fn_package = package_file;
	wxFileName fn_path = wxFileName::DirName(fn_package.GetPath());
	fn_path.AppendDir(wxT("export"));
	if (!fn_path.DirExists())
	{
		if (!fn_path.Mkdir())
		{
			file_error += wxT("Could not make directory: ") + fn_path.GetPath() + wxT("\n");
			return false;
		}
	}

	if (map_render_buffer == NULL)
	{
		map_render_buffer = new unsigned char [MAP_RENDER_SIZE * (6+2+2)];
	}
	
	unsigned char* const map_rgb[6] =
	{
		map_render_buffer + (0 * MAP_RENDER_SIZE),
		map_render_buffer + (1 * MAP_RENDER_SIZE),
		map_render_buffer + (2 * MAP_RENDER_SIZE),
		map_render_buffer + (3 * MAP_RENDER_SIZE),
		map_render_buffer + (4 * MAP_RENDER_SIZE),
		map_render_buffer + (5 * MAP_RENDER_SIZE),
	};
	unsigned char* const map_comp = map_render_buffer + (6 * MAP_RENDER_SIZE);
	unsigned char* const map_coll = map_render_buffer + (8 * MAP_RENDER_SIZE);

	const int MAP_ROW = 257*MAP_W;
	const int MAP_HALF = 241*MAP_H;

	for (int i=0; i<6; ++i)
	{
		if (!MapPicker::render_map(this,i,map_rgb[i]))
		{
			file_error += wxT("Could not render map.\n");
			return false;
		}

		// save the individual image steps
		#if 0
		wxImage img(MAP_W*257,MAP_H*241,map_rgb[i],true);
		img.SetMaskColour(0xFF,0x00,0xFF);

		const wxString FN_MAP[6] = {
			wxT("map_recto0.png"),
			wxT("map_recto1.png"),
			wxT("map_recto2.png"),
			wxT("map_verso0.png"),
			wxT("map_verso1.png"),
			wxT("map_verso2.png"),
		};

		wxFileName fn = wxFileName(fn_path.GetPath(),FN_MAP[i]);
		if (!img.SaveFile(fn.GetFullPath(),wxBITMAP_TYPE_PNG))
		{
			file_error += wxT("Could not save file: ") + fn.GetFullPath() + wxT("\n");
			return false;
		}
		#endif
	}

	for (int y=0; y<(241*MAP_H*2); ++y)
	{
		unsigned char* comp = map_comp + (y*MAP_ROW*3);
		unsigned char* coll = map_coll + (y*MAP_ROW*3);

		int poff = (y<MAP_HALF) ? 0 : 3;
		int py   = (y<MAP_HALF) ? y : (y-MAP_HALF);

		unsigned char* po = map_rgb[0+poff] + (py*MAP_ROW*3);
		unsigned char* ps = map_rgb[1+poff] + (py*MAP_ROW*3);
		unsigned char* pu = map_rgb[3-poff] + (py*MAP_ROW*3) + ((MAP_ROW-2)*3);
		unsigned char* pc = map_rgb[2+poff] + (py*MAP_ROW*3);

		for (int x=0; x<(257*MAP_W); ++x)
		{
			// overlay
			unsigned char or = po[0];
			unsigned char og = po[1];
			unsigned char ob = po[2];
			po += 3;

			// sprites
			unsigned char sr = ps[0];
			unsigned char sg = ps[1];
			unsigned char sb = ps[2];
			ps += 3;

			// underlay
			unsigned char ur = pu[0];
			unsigned char ug = pu[1];
			unsigned char ub = pu[2];
			pu -= 3;

			if (sr != 0xFF || sg != 0x00 || sb != 0xFF)
			{
				// sprite
				or = sr;
				og = sg;
				ob = sb;
			}
			else if (!is_background(ur,ug,ub) && is_background(or,og,ob))
			{
				// blend underlay where there is background

				// ox = (ox * 3/4) + (ux * 1/4)
				//or = (or >> 2) + (or >> 1) + (ur >> 2);
				//og = (og >> 2) + (og >> 1) + (ug >> 2);
				//ob = (ob >> 2) + (ob >> 1) + (ub >> 2);

				if (underlay)
				{
					// ox = (ox * 7/8) + (ux * 1/8)
					or = (or >> 3) + (or >> 2) + (or >> 1) + (ur >> 3);
					og = (og >> 3) + (og >> 2) + (og >> 1) + (ug >> 3);
					ob = (ob >> 3) + (ob >> 2) + (ob >> 1) + (ub >> 3);
				}
			}

			comp[0] = or;
			comp[1] = og;
			comp[2] = ob;
			comp += 3;

			// collision
			coll[0] = pc[0];
			coll[1] = pc[1];
			coll[2] = pc[2];
			coll += 3;
			pc += 3;
		}
	}

	// fill in break lines for scrolling areas
	for (int y=0; y < MAP_H; ++y)
	for (int x=1; x < MAP_W; ++x)
	for (int r=0; r < 2; ++r)
	{
		const Room* room = find_room(x,y,r ? false : true);
		if (room && room->coord_x != x)
		{
			int sy = (y + (r * MAP_H)) * 241;
			int sx = (x * 257) - 1;
			unsigned char* fill_in = map_comp + (3 * ((MAP_ROW * sy) + sx));

			for (int iy = 0; iy < 240; ++iy)
			{
				for (int i=0;i<3;++i)
				{
					unsigned char pl = fill_in[i-3];
					unsigned char pr = fill_in[i+3];
					unsigned int mix = unsigned int(pl) + unsigned int(pr);
					fill_in[0+i] = ((pl + pr) * 3) / 8;
				}
				fill_in += 3 * MAP_ROW;
			}
		}
	}

	wxFileName fn_composite(fn_path.GetPath(),FN_COMPOSITE);
	wxFileName fn_collision(fn_path.GetPath(),FN_COLLISION);

	wxImage compi(257*MAP_W,241*MAP_H*2,map_comp,true);
	if (!compi.SaveFile(fn_composite.GetFullPath(),IMG_FORMAT))
	{
		file_error += wxT("Could not save file: ") + fn_composite.GetFullPath() + wxT("\n");
		return false;
	}

	wxImage colli(257*MAP_W,241*MAP_H*2,map_coll,true);
	colli.SetMaskColour(255,0,255);
	if (!colli.SaveFile(fn_collision.GetFullPath(),IMG_FORMAT))
	{
		file_error += wxT("Could not save file: ") + fn_collision.GetFullPath() + wxT("\n");
		return false;
	}

	// I don't bother cleaning this up on failure because it doesn't happen often,
	// and having a large unused allocation sitting around isn't a big problem.
	delete [] map_render_buffer;
	map_render_buffer = NULL;

	return true;
}

static unsigned char* sprite_render_buffer = NULL;
bool Data::render_sprites()
{
	file_error = wxT("");

	// create export directory
	wxFileName fn_package = package_file;
	wxFileName fn_path = wxFileName::DirName(fn_package.GetPath());
	fn_path.AppendDir(wxT("export"));
	fn_path.AppendDir(wxT("sprites"));
	if (!fn_path.DirExists())
	{
		if (!fn_path.Mkdir(0777,wxPATH_MKDIR_FULL))
		{
			file_error += wxT("Could not make directory: ") + fn_path.GetPath() + wxT("\n");
			return false;
		}
	}

	const int SPRITE_RENDER_PAD = 1; // pixels to add around sprite render area
	const int SPRITE_RENDER_DIM = (256 * 8) + (2 * SPRITE_RENDER_PAD);

	if (sprite_render_buffer == NULL)
	{
		sprite_render_buffer = new unsigned char [SPRITE_RENDER_DIM * SPRITE_RENDER_DIM * 3];
	}
	unsigned char* const pix = sprite_render_buffer;

	const wxColour* palette = nes_palette();

	for (unsigned int i=0; i < sprite.size(); ++i)
	{
		const Sprite* s = sprite[i];

		const int IMG_FORMAT = wxBITMAP_TYPE_PNG;
		wxString fn_sprite = wxString::Format(wxT("%s_%3d.png"),s->name,i);

		const int count = s->count;
		if (count < 1) continue;

		// find bounds
		int min_tx = s->x[0];
		int max_tx = s->x[0];
		int min_ty = s->y[0];
		int max_ty = s->y[0];
		for (int t=1; t<count; ++t)
		{
			int tx = s->x[t];
			int ty = s->y[t];
			if (tx < min_tx) min_tx = tx;
			if (ty < min_ty) min_ty = ty;
			if (tx > max_tx) max_tx = tx;
			if (ty > max_ty) max_ty = ty;
		}

		// padding around bounds
		min_tx -= SPRITE_RENDER_PAD;
		min_ty -= SPRITE_RENDER_PAD;
		max_tx += SPRITE_RENDER_PAD + 8; // include tile as well
		max_ty += SPRITE_RENDER_PAD + 8;

		const int wx = max_tx - min_tx;
		const int wy = max_ty - min_ty;
		if (wx > SPRITE_RENDER_DIM || wy > SPRITE_RENDER_DIM || wx < 0 || wy < 0)
		{
			file_error += wxString::Format(wxT("Sprite render with unexpected dimensions (%d,%d): %s\n"),wx,wy,s->name);
		}

		// validate dependent tile and palette data
		bool chr_error[4] = { false };
		bool pal_error[4] = { false };
		for (int t=0; t<count; ++t)
		{
			int tile = s->tile[t];
			int tc = (tile >> 6) & 3;
			int tp = s->palette[t] & 3;

			if (s->tool_chr[tc] == NULL && !chr_error[tc])
			{
				chr_error[tc] = true;
				file_error += wxString::Format(wxT("Sprite missing CHR in slot %d: %s\n"),tc,s->name);
			}
			if (s->tool_palette[tp] == NULL && !pal_error[tp])
			{
				pal_error[tp] = true;
				file_error += wxString::Format(wxT("Sprite missing palette in slot %d: %s\n"),tp,s->name);
			}
		}
		if (file_error.Len() > 0)
		{
			return false;
		}

		// clear to magenta
		const int data_size = wx * wy * 3;
		for (int p=0; p<data_size; p+=3)
		{
			pix[p+0] = 255;
			pix[p+1] =   0;
			pix[p+2] = 255;
		}

		// render it
		for (int t=count-1; t>=0; --t)
		{
			int tile = s->tile[t] & 255;
			int tc = (tile >> 6) & 3;
			int tp = s->palette[t] & 3;

			int px = s->x[t] - min_tx;
			int py = s->y[t] - min_ty;
			int incx = s->flip_x[t] ? -1 : 1;
			int incy = s->flip_y[t] ? -1 : 1;
			if (s->flip_x[t]) px += 7;
			if (s->flip_y[t]) py += 7;

			const unsigned char* chr_tile = s->tool_chr[tc]->data[tile & 63];

			for (int iy = 0; iy < 8; ++iy)
			{
				int rpx = px;
				for (int ix = 0; ix < 8; ++ix)
				{
					// read a pixel from CHR
					unsigned char chr_data = *chr_tile & 3;
					++chr_tile;

					if (chr_data != 0) // not transparent
					{
						// fetch a palete entry
						unsigned char pal_colour = s->tool_palette[tp]->data[chr_data];
						if ((pal_colour & 0xF) > 0xD) pal_colour = 0x0F; // force true black
						wxColor wc = palette[pal_colour];

						// write pixel to buffer
						int pixel_index = ((py * wx) + rpx) * 3;
						pix[pixel_index+0] = wc.Red();
						pix[pixel_index+1] = wc.Green();
						pix[pixel_index+2] = wc.Blue();
					}
					rpx += incx;
				}
				py += incy;
			}
		}

		// save to file
		wxFileName fn_simg(fn_path.GetPath(),fn_sprite);
		wxImage simg(wx,wy,pix,true);
		if (!simg.SaveFile(fn_simg.GetFullPath(),IMG_FORMAT))
		{
			file_error += wxT("Could not save file: ") + fn_simg.GetFullPath() + wxT("\n");
			return false;
		}
	}

	// not a big deal if I don't clean up the failure paths
	delete [] sprite_render_buffer;
	sprite_render_buffer = NULL;

	return true;
}


const wxString& Data::get_file_error() const
{
	return file_error;
}

unsigned int Data::get_item_count(int t) const
{
	switch (t)
	{
	case ITEM_CHR:     return chr.size();
	case ITEM_PALETTE: return palette.size();
	case ITEM_SPRITE:  return sprite.size();
	case ITEM_ROOM:    return room.size();
	default:
		wxASSERT(false);
		return 0;
	}
}

Data::Item* Data::get_item(int t, unsigned int i)
{
	if (i >= get_item_count(t)) return NULL;
	switch (t)
	{
	case ITEM_CHR:     return chr[i];
	case ITEM_PALETTE: return palette[i];
	case ITEM_SPRITE:  return sprite[i];
	case ITEM_ROOM:    return room[i];
	default:
		return NULL;
	}
}

Data::Item* Data::find_item(int t, wxString name)
{
	const unsigned int count = get_item_count(t);
	for (unsigned int i=0; i<count; ++i)
	{
		Item* item = get_item(t,i);
		if (item->name == name)
		{
			wxASSERT(item->type == t);
			return item;
		}
	}
	return NULL;
}

int Data::find_index(const Item* item)
{
	if (item == NULL) return -1;

	int t = item->type;
	const unsigned int count = get_item_count(t);
	for (unsigned int i=0; i<count; ++i)
	{
		if (item == get_item(t,i))
			return i;
	}
	wxFAIL;
	return 0;
}

Data::Room* Data::find_room(int x, int y, bool recto)
{
	const unsigned int count = room.size();
	for (unsigned int i=0; i<count; ++i)
	{
		Room* r = room[i];
		if (
			r->coord_y == y && 
			r->recto == recto &&
			(
				(r->coord_x == x) ||
				(((r->coord_x+1) == x) && r->scrolling)
			)
			)
		{
			return r;
		}
	}
	return NULL;
}

unsigned int Data::sprite_export_index(Sprite* s)
{
	const unsigned int count = sprite.size();
	unsigned int index = 0;
	for (unsigned int i=0; i<count; ++i)
	{
		Sprite* si = sprite[i];
		if (si->name.EndsWith(wxT("_"))) continue; // skip utility sprites
		if (si->bank != s->bank) continue; // skip other bank sprites
		
		if (si == s) return index;
		++index;
	}
	return 0;
}

bool Data::new_item(int t, const wxString& name)
{
	if (name.Len() < 1) return false;
	if (name.Contains(wxT(" "))) return false;
	if (name == wxT("NULL")) return false;

	for (unsigned int i=0; i < get_item_count(t); ++i)
	{
		if (name == get_item(t,i)->name) // name already used
		{
			return false;
		}
	}

	switch (t)
	{
	case ITEM_CHR:
		{
			Chr* e = new Chr;
			e->name = name;
			chr.push_back(e);
		} break;
	case ITEM_PALETTE:
		{
			Palette* e = new Palette;
			e->name = name;
			palette.push_back(e);
		} break;
	case ITEM_SPRITE:
		{
			Sprite* e = new Sprite;
			e->name = name;
			sprite.push_back(e);
			dog_sprite_map_dirty = true;
		} break;
	case ITEM_ROOM:
		{
			Room* e = new Room;
			e->name = name;

			// default CHR
			e->chr[1] = (Chr*)find_item(ITEM_CHR,wxT("font"));
			e->chr[4] = (Chr*)find_item(ITEM_CHR,wxT("lizard"));

			room.push_back(e);
		} break;
	default:
		wxASSERT(false);
		return false;
	}

	unsaved = true;
	return true;
}

bool Data::copy_item(int t, unsigned int i, const wxString& name)
{
	if (!new_item(t,name))
		return false;

	const Item* src = get_item(t,i);
	Item* dst = get_item(t,get_item_count(t)-1);

	if (dst == NULL) return false;
	return dst->copy(src);
}

bool Data::rename_item(int t, unsigned int s, const wxString& name)
{
	if (name.Len() < 1) return false;

	for (unsigned int i=0; i < get_item_count(t); ++i)
	{
		if (i == s) continue;
		if (name == get_item(t,i)->name) // name already used
		{
			return false;
		}
	}

	get_item(t,s)->name = name;

	if (t = ITEM_SPRITE)
		dog_sprite_map_dirty = true;

	unsaved = true;

	// rooms or sprites may link the renamed object, need to be resaved
	for (unsigned int r=0; r<room.size(); ++r)
	{
		// would be more efficient to mark only the ones
		// that link the renamed object unsaved, but this is easier
		room[r]->unsaved = true;
	}
	for (unsigned int s=0; s<sprite.size(); ++s)
	{
		sprite[s]->unsaved = true;
	}
	
	return true;
}

bool Data::delete_item(int t, unsigned int i)
{
	wxASSERT(i < get_item_count(t));

	switch (t)
	{
	case ITEM_CHR:
		{
			Chr* e = chr[i];
			for (unsigned int r=0; r < room.size(); ++r)
			for (unsigned int c=0; c < 8; ++c)
				if (room[r]->chr[c] == e) return false;
			for (unsigned int s=0; s < sprite.size(); ++s)
			for (unsigned int c=0; c < 4; ++c)
				if (sprite[s]->tool_chr[c] == e) return false;
		} break;
	case ITEM_PALETTE:
		{
			Palette* e = palette[i];
			for (unsigned int r=0; r < room.size(); ++r)
			for (unsigned int p=0; p < 8; ++p)
				if (room[r]->palette[p] == e) return false;
			for (unsigned int s=0; s < sprite.size(); ++s)
			for (unsigned int p=0; p < 4; ++p)
				if (sprite[s]->tool_palette[p] == e) return false;
		} break;
	case ITEM_SPRITE:
		{
		} break;
	case ITEM_ROOM:
		{
			Room* e = room[i];
			for (unsigned int r=0; r < room.size(); ++r)
			{
				if (e == room[r]) continue;
				for (unsigned int d=0; d < 8; ++d)
					if (room[r]->door[d] == e)
						return false;
			}
		} break;
	default:
		break;
	}

	switch (t)
	{
	case ITEM_CHR:     delete chr[i];     chr.erase(    chr.begin()    +i); break;
	case ITEM_PALETTE: delete palette[i]; palette.erase(palette.begin()+i); break;
	case ITEM_SPRITE:  delete sprite[i];  sprite.erase( sprite.begin() +i); break;
	case ITEM_ROOM:    delete room[i];    room.erase(   room.begin()   +i); break;
	default:
		wxASSERT(false);
		return false;
	}

	unsaved = true;
	return true;
}

bool Data::swap_item(int t, unsigned int a, unsigned int b)
{
	wxASSERT(a < get_item_count(t));
	wxASSERT(b < get_item_count(t));
	switch (t)
	{
	case ITEM_CHR:
		{
			Chr* s = chr[a];
			chr[a] = chr[b];
			chr[b] = s;
		} break;
	case ITEM_PALETTE:
		{
			Palette* s = palette[a];
			palette[a] = palette[b];
			palette[b] = s;
		} break;
	case ITEM_SPRITE:
		{
			Sprite* s = sprite[a];
			sprite[a] = sprite[b];
			sprite[b] = s;
		} break;
	case ITEM_ROOM:
		{
			Room* s = room[a];
			room[a] = room[b];
			room[b] = s;
		} break;
	default:
		wxASSERT(false);
		return false;
	}

	unsaved = true;
	return true;
}

wxString Data::reference_item(int t, unsigned int i)
{
	wxASSERT(i < get_item_count(t));
	wxString refs = wxT("");

	switch (t)
	{
	case ITEM_CHR:
		{
			Chr* e = chr[i];
			for (unsigned int r=0; r < room.size(); ++r)
			{
				bool refd = false;
				for (unsigned int c=0; c < 8; ++c)
					if (room[r]->chr[c] == e)
						refd = true;
				if (refd) refs = refs + wxT("Room: ") + room[r]->name + wxT("\n");
			}
			for (unsigned int s=0; s < sprite.size(); ++s)
			{
				bool refd = false;
				for (unsigned int c=0; c < 4; ++c)
					if (sprite[s]->tool_chr[c] == e)
						refd = true;
				if (refd) refs = refs + wxT("Sprite: ") + sprite[s]->name + wxT("\n");
			}
		} break;
	case ITEM_PALETTE:
		{
			Palette* e = palette[i];
			for (unsigned int r=0; r < room.size(); ++r)
			{
				bool refd = false;
				for (unsigned int p=0; p < 8; ++p)
					if (room[r]->palette[p] == e)
						refd = true;
				if (refd) refs = refs + wxT("Room: ") + room[r]->name + wxT("\n");
			}
			for (unsigned int s=0; s < sprite.size(); ++s)
			{
				bool refd = false;
				for (unsigned int p=0; p < 4; ++p)
					if (sprite[s]->tool_palette[p] == e)
						refd = true;
				if (refd) refs = refs + wxT("Sprite: ") + sprite[s]->name + wxT("\n");
			}
		} break;
	case ITEM_SPRITE:
		{
		} break;
	case ITEM_ROOM:
		{
			Room* e = room[i];
			for (unsigned int r=0; r < room.size(); ++r)
			{
				bool refd = false;
				for (unsigned int d=0; d < 8; ++d)
					if (room[r]->door[d] == e)
						refd = true;
				if (refd) refs = refs + wxT("Room: ") + room[r]->name + wxT("\n");
			}
		} break;
	default:
		break;
	}
	return refs;
}

Data::Room::Room()
{
	type = ITEM_ROOM;
	unsaved = true;
	for (int i=0; i<8; ++i) chr[i] = NULL;
	for (int i=0; i<8; ++i) palette[i] = NULL;
	for (int i=0; i<16; ++i)
	{
		dog[i].type = DOG_NONE;
		dog[i].x = 0;
		dog[i].y = 0;
		dog[i].param = 0;
		dog[i].note = wxT("");
		dog[i].cached_sprite = NULL;
	}
	for (int i=0; i<8; ++i)
	{
		door[i] = NULL;
		door_x[i] = 128;
		door_y[i] = 120;
	}
	::memset(nametable,0,sizeof(nametable));
	::memset(attribute,0,sizeof(attribute));
	coord_x = -1;
	coord_y = -1;
	recto = false;
	music = 0;
	water = 255;
	scrolling = false;
	ship = false;

	// default fill
	for (int y=0; y<30; ++y)
	for (int x=0; x<64; ++x)
	{
		//nametable[x+(y*64)] = (x + (y * 16)) & 0xFF;
		nametable[x+(y*64)] = 0x70;
	}
	for (int y=0; y<15; ++y)
	for (int x=0; x<32; ++x)
	{
		//attribute[x+(y*32)] = (x + y) & 0x3;
		attribute[x+(y*32)] = 0;
	}
}

Data::Chr::Chr()
{
	type = ITEM_CHR;
	unsaved = true;
	//::memset(data,0,sizeof(data));
	for (int t=0; t<64; ++t)
	for (int p=0; p<64; ++p)
	{
		int c = (t + (t>>4)) & 3;
		//int c = (t + (t>>4) + (p + (p>>3))) & 3;
		data[t][p] = c;
	}
	export_index_cache = -1;
}

Data::Palette::Palette()
{
	type = ITEM_PALETTE;
	unsaved = true;
	data[0] = 0x0F; // black
	data[1] = 0x00; // dark grey
	data[2] = 0x10; // light grey
	data[3] = 0x30; // white
}

Data::Sprite::Sprite()
{
	type = ITEM_SPRITE;
	unsaved = true;
	vpal = false;
	bank = 0;
	count = 0;
	for (int i=0; i<MAX_SPRITE_TILES; ++i)
	{
		tile   [i] = 0;
		palette[i] = 0;
		x      [i] = 0;
		y      [i] = 0;
		flip_x [i] = 0;
		flip_y [i] = 0;
	}
	for (int i=0; i<4; ++i)
	{
		tool_chr[i] = NULL;
		tool_palette[i] = NULL;
	}
}

//
// misc global data
//

bool Data::nes_palette_init = false;
wxColour Data::nes_palette_data[64];

static const unsigned int NES_PALETTE[64] = // RGB data
{
	// palette by shiru
	// 0x0X
	0x656665, 0x001E9C, 0x200DAC, 0x44049C,
	0x6A036F, 0x71021D, 0x651000, 0x461E00,
	0x232D00, 0x003900, 0x003C00, 0x003720,
	0x003266, 0x000000, 0x000000, 0x000000,
	// 0x1X
	0xB0B1B0, 0x0955EB, 0x473DFF, 0x7730FE,
	0xAE2CCE, 0xBC2964, 0xB43900, 0x8F4B00,
	0x635F00, 0x1B7000, 0x007700, 0x00733B,
	0x006D99, 0x000000, 0x000000, 0x110011,
	// 0x2X
	0xFFFFFF, 0x4DADFF, 0x8694FF, 0xB885FF,
	0xF17FFF, 0xFF79D4, 0xFF865F, 0xF19710,
	0xC8AB00, 0x7EBE00, 0x47C81F, 0x2BC86F,
	0x2EC4CC, 0x50514D, 0x000000, 0x220022,
	// 0x3X
	0xFFFFFF, 0xB9E5FF, 0xD0DBFF, 0xE6D5FF,
	0xFDD1FF, 0xFFCEF5, 0xFFD4C5, 0xFFDAA3,
	0xEEE290, 0xD0EB8E, 0xB9EFA5, 0xAEEFC7,
	0xAEEEEE, 0xBABCB9, 0x000000, 0x330033,
};

wxColour* Data::nes_palette()
{
	if (!nes_palette_init) // lazy init
	{
		for (int i=0; i < (16*4); ++i)
		{
			int x = (i % 16);
			int y = (i / 16);
			nes_palette_data[i].Set(
				(NES_PALETTE[i] >> 16) & 0xFF,
				(NES_PALETTE[i] >>  8) & 0xFF,
				(NES_PALETTE[i]      ) & 0xFF);
		}
		nes_palette_init = true;
	}

	return nes_palette_data;
}

static bool is_background(unsigned char r, unsigned char g, unsigned char b)
{
	if (r == 0x00 && g == 0x00 && b == 0x00) return true; // black 0x0F
	if (r == 0x4D && g == 0xAD && b == 0xFF) return true; // sky   0x21
	if (r == 0x47 && g == 0x3D && b == 0xFF) return true; // water 0x12
	return false;
}

unsigned int Data::lizard_count()
{
	return LIZARD_COUNT;
}

const wxString* Data::lizard_list()
{
	if (lizard_list_string == NULL) // lazy init
	{
		lizard_list_string = new wxString[LIZARD_COUNT];
		for (int i=0; i<LIZARD_COUNT; ++i)
		{
			lizard_list_string[i] = LIZARD_LIST[i];
		}
	}
	return lizard_list_string;
}

int Data::music_count()
{
	return TOOL_MUSIC_COUNT;
}

const wxString* Data::music_list()
{
	if (music_list_string == NULL) // lazy init
	{
		music_list_string = new wxString[TOOL_MUSIC_COUNT];
		for (int i=0; i<TOOL_MUSIC_COUNT; ++i)
		{
			music_list_string[i] = MUSIC_LIST[i];
		}
	}
	return music_list_string;
}

int Data::dog_count()
{
	return DOG_COUNT;
}

const wxString* Data::dog_list()
{
	if (dog_list_string == NULL) // lazy init
	{
		dog_list_string = new wxString[DOG_COUNT];
		for (int i=0; i<DOG_COUNT; ++i)
		{
			dog_list_string[i] = DOG_LIST[i];
		}
	}
	return dog_list_string;
}

void Data::dog_sprite_validate_cache()
{
	if (dog_sprite_map == NULL) // lazy init
	{
		empty_sprite.count = 0;
		dog_sprite_map = new Sprite*[DOG_COUNT];
		dog_sprite_utility = new bool[DOG_COUNT];
		for (int i=0; i< DOG_COUNT; ++i)
		{
			dog_sprite_map[i] = &empty_sprite;
			dog_sprite_utility[i] = false;
		}
		dog_sprite_map_dirty = true;
	}
	if (dog_sprite_map_dirty)
	{
		for (int i=0; i<DOG_COUNT; ++i)
		{
			Sprite* s = &empty_sprite;
			bool utility = false;
			for (unsigned int j=0; j < sprite.size(); ++j)
			{
				Sprite* sj = sprite[j];
				if (sj->name == DOG_LIST[i] ||
					sj->name == (wxString(DOG_LIST[i]) + wxT("_")) ||
					sj->name == (wxString(DOG_LIST[i]) + wxT("__")) )
				{
					s = sj;
					if (sj->name.EndsWith(wxT("__")))
						utility = true;
					break;
				}
			}
			dog_sprite_map[i] = s;
			dog_sprite_utility[i] = utility;
		}
		dog_sprite_map_dirty = false;
	}
}

const Data::Sprite* Data::get_dog_sprite(const Room::Dog& dog)
{
	if (dog.cached_sprite)
	{
		return dog.cached_sprite;
	}

	int d = dog.type;
	wxASSERT(d<DOG_COUNT);

	if (d == DOG_SPRITE0 || d == DOG_SPRITE1 || d == DOG_SPRITE2)
	{
		dog.cached_sprite = (Sprite*)find_item(ITEM_SPRITE,dog.note);
		if (dog.cached_sprite == NULL)
		{
			dog.cached_sprite = &empty_sprite;
		}
		return dog.cached_sprite;
	}

	dog_sprite_validate_cache();
	dog.cached_sprite = dog_sprite_map[d];
	return dog.cached_sprite;
}

//
// Data loading
//

bool Data::Chr::load(wxTextFile& f, wxString& err, Data* data_)
{
	bool result = true;
	::memset(data,0,sizeof(data));

	wxString l = f.GetFirstLine();
	for (int i=0; i<64; ++i)
	{
		for (int y=0; y<8; ++y)
		{
			wxStringTokenizer st(l);
			for (int x=0; x<8; ++x)
			{
				if (!st.HasMoreTokens())
				{
					err += wxString::Format(wxT("Invalid CHR line: %d\n"),f.GetCurrentLine());
					result = false;
					break;
				}

				wxString s = st.GetNextToken();
				long v = 0;
				s.ToLong(&v,16);
				data[i][(y*8)+x] = v;
			}
			l = f.GetNextLine();
		}
	}
	return result;
}

bool Data::Palette::load(wxTextFile& f, wxString& err, Data* data_)
{
	bool result = true;
	::memset(data,0,sizeof(data));

	wxString l = f.GetFirstLine();
	wxStringTokenizer st(l);
	for (int i=0; i<4; ++i)
	{
		if (!st.HasMoreTokens())
		{
			err += wxString::Format(wxT("Invalid palette line: %d\n"),f.GetCurrentLine());
			result = false;
			break;
		}

		wxString s = st.GetNextToken();
		long v = 0;
		s.ToLong(&v,16);
		if (v == 0x0F) v |= (i << 4); // diagnostic colour
		data[i] = v;
	}
	return result;
}

bool Data::Sprite::load(wxTextFile& f, wxString& err, Data* data_)
{
	bool result = true;

	long v = 0;
	wxString l = f.GetFirstLine();
	{
		wxStringTokenizer st(l);

		if (st.CountTokens() < 3)
		{
			err += wxString::Format(wxT("Invalid sprite line: %d\n"),f.GetCurrentLine());
			result = false;
		}

		wxString s;
		s = st.GetNextToken(); v = 0; s.ToLong(&v); count = v;
		s = st.GetNextToken(); v = 0; s.ToLong(&v); vpal = (v != 0);
		s = st.GetNextToken(); v = 0; s.ToLong(&v); bank = v;

		if (count < 0 || count > MAX_SPRITE_TILES)
		{
			err += wxT("Invalid sprite count.\n");
			result = false;
			count = 0;
		}
	}

	for (int i=0; i<MAX_SPRITE_TILES; ++i)
	{
		l = f.GetNextLine();
		wxStringTokenizer st(l);

		if (st.CountTokens() < 6)
		{
			err += wxString::Format(wxT("Invalid sprite line: %d\n"),f.GetCurrentLine());
			result = false;
		}
		else
		{
			wxString s;
			s = st.GetNextToken(); v = 0; s.ToLong(&v,16); tile[i] = v;
			s = st.GetNextToken(); v = 0; s.ToLong(&v); palette[i] = v;
			s = st.GetNextToken(); v = 0; s.ToLong(&v);       x[i] = v;
			s = st.GetNextToken(); v = 0; s.ToLong(&v);       y[i] = v;
			s = st.GetNextToken(); v = 0; s.ToLong(&v);  flip_x[i] = v != 0;
			s = st.GetNextToken(); v = 0; s.ToLong(&v);  flip_y[i] = v != 0;
		}

		if (tile[i] < 0 || tile[i] > 255)
		{
			err += wxString::Format(wxT("Invalid sprite tile on line: %d\n"),f.GetCurrentLine());
			result = false;
			tile[i] = 0;
		}
	}

	for (int i=0; i<4; ++i)
	{
		l = f.GetNextLine();
		Chr* item = (Chr*)data_->find_item(ITEM_CHR, l);
		if (item == NULL && l != wxT("NULL"))
		{
			err += wxT("Unable to link chr: ") + l + wxT("\n");
			result = false;
		}
		tool_chr[i] = item;
	}
	for (int i=0; i<4; ++i)
	{
		l = f.GetNextLine();
		Palette* item = (Palette*)data_->find_item(ITEM_PALETTE, l);
		if (item == NULL && l != wxT("NULL"))
		{
			err += wxT("Unable to link palette: ") + l + wxT("\n");
			result = false;
		}
		tool_palette[i] = item;
	}

	return result;
}

bool Data::Room::load(wxTextFile& f, wxString& err, Data* data_)
{
	bool result = true;
	wxString l = f.GetFirstLine();
	wxString s;
	long v;

	::memset(nametable,0,sizeof(nametable));
	::memset(attribute,0,sizeof(attribute));

	for (int i=0; i<8; ++i)
	{
		if (i != 0) l = f.GetNextLine();

		// temporarily store link (linked doors may not be loaded yet)
		LoadLink link = { this, i, l };
		data_->links.push_back(link);
	}
	for (int i=0; i<8; ++i)
	{
		l = f.GetNextLine();
		Chr* item = (Chr*)data_->find_item(ITEM_CHR, l);
		if (item == NULL && l != wxT("NULL"))
		{
			err += wxT("Unable to link chr: ") + l + wxT("\n");
			result = false;
		}
		chr[i] = item;
	}
	for (int i=0; i<8; ++i)
	{
		l = f.GetNextLine();
		Palette* item = (Palette*)data_->find_item(ITEM_PALETTE, l);
		if (item == NULL && l != wxT("NULL"))
		{
			err += wxT("Unable to link palette: ") + l + wxT("\n");
			result = false;
		}
		palette[i] = item;
	}

	v = -1; l = f.GetNextLine(); l.ToLong(&v); coord_x = v;
	v = -1; l = f.GetNextLine(); l.ToLong(&v); coord_y = v;
	v =  0; l = f.GetNextLine(); l.ToLong(&v); recto = v ? true : false;
	v =  0; l = f.GetNextLine(); l.ToLong(&v); music = v;
	v =  0; l = f.GetNextLine(); l.ToLong(&v); water = v;
	v =  0; l = f.GetNextLine(); l.ToLong(&v); scrolling = v ? true : false;
	v =  0; l = f.GetNextLine(); l.ToLong(&v); ship = v ? true : false;

	if (music < 0 || music >= TOOL_MUSIC_COUNT)
	{
		err += wxString::Format(wxT("Invalid music value at line: %d\n"),f.GetCurrentLine());
		result = false;
		music = 0;
	}

	for (int i=0; i<8; ++i) // doors
	{
		l = f.GetNextLine();
		wxStringTokenizer st(l);

		if (st.CountTokens() < 2)
		{
			err += wxString::Format(wxT("Invalid door coordinates at line: %d\n"),f.GetCurrentLine());
			result = false;
		}
		else
		{
			s = st.GetNextToken(); v = 0; s.ToLong(&v); door_x[i] = v;
			s = st.GetNextToken(); v = 0; s.ToLong(&v); door_y[i] = v;
		}
	}
	for (int i=0; i<16; ++i) // dogs
	{
		l = f.GetNextLine();
		wxStringTokenizer st(l);

		if (st.CountTokens() < 4)
		{
			err += wxString::Format(wxT("Invalid dog line: %d\n"),f.GetCurrentLine());
			result = false;
		}
		else
		{
			dog[i].type = -1;
			s = st.GetNextToken();

			for (int j=0; j<DOG_COUNT; ++j)
			{
				if (s == DOG_LIST[j])
				{
					dog[i].type = j;
					break;
				}
			}
			
			s = st.GetNextToken(); v = 0; s.ToLong(&v); dog[i].x = v;
			s = st.GetNextToken(); v = 0; s.ToLong(&v); dog[i].y = v;
			s = st.GetNextToken(); v = 0; s.ToLong(&v); dog[i].param = v;
		}

		l = f.GetNextLine();
		dog[i].note = l;

		if (dog[i].type < 0 || dog[i].type >= DOG_COUNT)
		{
			err += wxString::Format(wxT("Invalid dog type at line: %d\n"),f.GetCurrentLine());
			result = false;
			dog[i].type = 0;
		}

		dog[i].cached_sprite = NULL;
	}
	for (int y=0; y<30; ++y) // nametable
	{
		l = f.GetNextLine();
		wxStringTokenizer st(l);

		if (st.CountTokens() < 64)
		{
			err += wxString::Format(wxT("Invalid nametable line: %d\n"),f.GetCurrentLine());
			result = false;
		}
		else
		{
			for (int x=0; x<64; ++x)
			{
				s = st.GetNextToken();
				v = 0;
				s.ToLong(&v,16);
				nametable[(y*64)+x] = v;
			}
		}
	}
	for (int y=0; y<15; ++y) // attribute
	{
		l = f.GetNextLine();
		wxStringTokenizer st(l);

		if (st.CountTokens() < 32)
		{
			err += wxString::Format(wxT("Invalid attribute line: %d\n"),f.GetCurrentLine());
			result = false;
		}
		else
		{
			for (int x=0; x<32; ++x)
			{
				s = st.GetNextToken();
				v = 0;
				s.ToLong(&v,16);
				attribute[(y*32)+x] = v;
			}
		}
	}
	return result;
}

//
// Data saving
//

bool Data::Chr::save(wxTextFile& f, wxString& err) const
{
	for (int i=0; i<64; ++i)
	{
		for (int y=0; y<8; ++y)
		{
			const unsigned char* l = &(data[i][y*8]);
			f.AddLine(wxString::Format(wxT("%01X %01X %01X %01X %01X %01X %01X %01X"),
				l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]));
		}
	}
	return true;
}

bool Data::Palette::save(wxTextFile& f, wxString& err) const
{
	unsigned char d[4];
	for (int i=0; i<4; ++i)
	{
		d[i] = data[i];
		if ((d[i] & 0x0F) == 0x0F) d[i] = 0x0F; // remove diagnostic colour
	}

	f.AddLine(wxString::Format(wxT("%02X %02X %02X %02X"),d[0],d[1],d[2],d[3]));
	return true;
}

bool Data::Sprite::save(wxTextFile& f, wxString& err) const
{
	f.AddLine(wxString::Format(wxT("%d %d %d"),count,vpal?1:0,bank));
	for (int i=0; i<Data::Sprite::MAX_SPRITE_TILES; ++i)
	{
		f.AddLine(wxString::Format(wxT("%02X %d %d %d %d %d"),
			tile[i],palette[i],x[i],y[i],
			flip_x[i]?1:0,flip_y[i]?1:0));
	}
	for (int i=0; i<4; ++i)
	{
		Chr* c = tool_chr[i];
		if (c != NULL) f.AddLine(c->name);
		else           f.AddLine(wxT("NULL"));
	}
	for (int i=0; i<4; ++i)
	{
		Palette* p = tool_palette[i];
		if (p != NULL) f.AddLine(p->name);
		else           f.AddLine(wxT("NULL"));
	}
	return true;
}

bool Data::Room::save(wxTextFile& f, wxString& err) const
{
	// linked items
	for (int i=0; i<8; ++i)
	{
		Room* d = door[i];
		if (d != NULL) f.AddLine(d->name);
		else           f.AddLine(wxT("NULL"));
	}
	for (int i=0; i<8; ++i)
	{
		Chr* c = chr[i];
		if (c != NULL) f.AddLine(c->name);
		else           f.AddLine(wxT("NULL"));
	}
	for (int i=0; i<8; ++i)
	{
		Palette* p = palette[i];
		if (p != NULL) f.AddLine(p->name);
		else           f.AddLine(wxT("NULL"));
	}

	// individual items
	f.AddLine(wxString::Format(wxT("%d"),coord_x));
	f.AddLine(wxString::Format(wxT("%d"),coord_y));
	f.AddLine(wxString::Format(wxT("%d"),recto ? 1 : 0));
	f.AddLine(wxString::Format(wxT("%d"),music));
	f.AddLine(wxString::Format(wxT("%d"),water));
	f.AddLine(wxString::Format(wxT("%d"),scrolling ? 1 : 0));
	f.AddLine(wxString::Format(wxT("%d"),ship ? 1 : 0));

	for (int i=0; i<8; ++i) // doors
	{
		f.AddLine(wxString::Format(wxT("%d %d"),door_x[i],door_y[i]));
	}
	for (int i=0; i<16; ++i) // dogs
	{
		f.AddLine(wxString::Format(wxT("%s %d %d %d"),DOG_LIST[dog[i].type],dog[i].x,dog[i].y,dog[i].param));
		f.AddLine(dog[i].note);
	}
	for (int y=0; y<30; ++y) // nametable
	{
		wxString l = wxT("");
		for (int x=0; x<64; ++x)
			l += wxString::Format(wxT("%02X "),nametable[(y*64)+x]);
		f.AddLine(l);
	}
	for (int y=0; y<15; ++y) // attribute
	{
		wxString l = wxT("");
		for (int x=0; x<32; ++x)
			l += wxString::Format(wxT("%02X "),attribute[(y*32)+x]);
		f.AddLine(l);
	}

	return true;
}

//
// Data copying
//

bool Data::Chr::copy(const Item* src)
{
	if (src == NULL) return false;
	if (src->type != ITEM_CHR) return false;
	const Chr* s = (const Chr*)(src);
	::memcpy(data,s->data,sizeof(data));
	return true;
}

bool Data::Palette::copy(const Item* src)
{
	if (src == NULL) return false;
	if (src->type != ITEM_PALETTE) return false;
	const Palette* s = (const Palette*)(src);
	::memcpy(data,s->data,sizeof(data));
	return true;
}

bool Data::Sprite::copy(const Item* src)
{
	if (src == NULL) return false;
	if (src->type != ITEM_SPRITE) return false;
	const Sprite* s = (const Sprite*)(src);
	vpal = s->vpal;
	bank = s->bank;
	count =  s->count;
	::memcpy(tile   ,s->tile   ,sizeof(tile   ));
	::memcpy(palette,s->palette,sizeof(palette));
	::memcpy(x      ,s->x      ,sizeof(x      ));
	::memcpy(y      ,s->y      ,sizeof(y      ));
	::memcpy(flip_x ,s->flip_x ,sizeof(flip_x ));
	::memcpy(flip_y ,s->flip_y ,sizeof(flip_y ));
	for (int i=0; i<4; ++i)
	{
		tool_chr[i] = s->tool_chr[i];
		tool_palette[i] = s->tool_palette[i];
	}
	return true;
}

bool Data::Room::copy(const Item* src)
{
	if (src == NULL) return false;
	if (src->type != ITEM_ROOM) return false;
	const Room* s = (const Room*)(src);
	::memcpy(nametable,s->nametable,sizeof(nametable));
	::memcpy(attribute,s->attribute,sizeof(attribute));
	::memcpy(chr      ,s->chr      ,sizeof(chr      ));
	::memcpy(palette  ,s->palette  ,sizeof(palette  ));
	coord_x = s->coord_x;
	coord_y = s->coord_y;
	recto = s->recto;
	music = s->music;
	water = s->water;
	scrolling = s->scrolling;
	ship = s->ship;
	for (int i=0; i<16; ++i)
	{
		dog[i].type  = s->dog[i].type;
		dog[i].x     = s->dog[i].x;
		dog[i].y     = s->dog[i].y;
		dog[i].param = s->dog[i].param;
		dog[i].note  = s->dog[i].note;
	}
	::memcpy(door     ,s->door     ,sizeof(door     ));
	::memcpy(door_x   ,s->door_x   ,sizeof(door_x   ));
	::memcpy(door_y   ,s->door_y   ,sizeof(door_y   ));
	return true;
}

bool Data::Room::collide_pixel(int x, int y, bool oob) const
{
	return collide_tile(x / 8, y / 8, oob);
}

bool Data::Room::collide_tile(int tx, int ty, bool oob) const
{
	if (tx < 0) return oob;
	if (ty < 0) return oob;
	if (ty >= 30) return oob;
	if (tx >= 64 || (tx >= 32 && scrolling == false)) return oob;

	return (nametable[(ty*64)+tx] & 0x80) != 0;
}

bool Data::is_utility(int d)
{
	dog_sprite_validate_cache();
	return dog_sprite_utility[d];
}

// end of file
