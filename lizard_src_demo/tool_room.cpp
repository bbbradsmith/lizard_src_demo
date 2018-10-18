// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_room.h"
#include "tool_palette.h"
#include "tool_chr.h"
#include "wx/grid.h"
#include "wx/dcbuffer.h"
#include "wx/statline.h"
#include "wx/colour.h"
#include "enums.h"

using namespace LizardTool;

const wxWindowID ID_ROOM_PANEL          = 0x800;
const wxWindowID ID_ROOM_STATUS         = 0x801;
const wxWindowID ID_ROOM_CHR_LIST_BACK  = 0x802;
const wxWindowID ID_ROOM_CHR_LIST_FORE  = 0x803;
const wxWindowID ID_ROOM_PAL_LIST_BACK  = 0x804;
const wxWindowID ID_ROOM_PAL_LIST_FORE  = 0x805;
const wxWindowID ID_ROOM_CHECK_ATTRIB   = 0x806;
const wxWindowID ID_ROOM_CHECK_SPRITE   = 0x807;
const wxWindowID ID_ROOM_CHECK_GRID     = 0x808;
const wxWindowID ID_ROOM_CHECK_DOORS    = 0x809;
const wxWindowID ID_ROOM_CHECK_COLLIDE  = 0x80A;
const wxWindowID ID_ROOM_COORD_X        = 0x80B;
const wxWindowID ID_ROOM_COORD_Y        = 0x80C;
const wxWindowID ID_ROOM_RECTO          = 0x80D;
const wxWindowID ID_ROOM_MUSIC          = 0x80E;
const wxWindowID ID_ROOM_WATER          = 0x80F;
const wxWindowID ID_ROOM_SCROLLING      = 0x810;
const wxWindowID ID_ROOM_ASSUME         = 0x811;
const wxWindowID ID_ROOM_AUTO_LINK      = 0x812;
const wxWindowID ID_ROOM_PASSWORD_COMBO = 0x813;
const wxWindowID ID_ROOM_PASSWORD_TEXT  = 0x814;
const wxWindowID ID_ROOM_UNLINK         = 0x815;
const wxWindowID ID_ROOM_EDIT_STATUS    = 0x816;
const wxWindowID ID_ROOM_SHIP           = 0x817;
const wxWindowID ID_ROOM_TEST_NES       = 0x818;
const wxWindowID ID_ROOM_TEST_PC        = 0x819;
const wxWindowID ID_ROOM_STRIP          = 0x81A;

const wxWindowID ID_ROOM_CHR            = 0x820;
const wxWindowID ID_ROOM_PAL            = 0x830;
const wxWindowID ID_ROOM_DOOR_LINK      = 0x840;
const wxWindowID ID_ROOM_DOOR_X         = 0x850;
const wxWindowID ID_ROOM_DOOR_Y         = 0x860;
const wxWindowID ID_ROOM_DOG_SPRITE     = 0x870;
const wxWindowID ID_ROOM_DOG_X          = 0x880;
const wxWindowID ID_ROOM_DOG_Y          = 0x890;
const wxWindowID ID_ROOM_DOG_PARAM      = 0x8A0;
const wxWindowID ID_ROOM_DOG_NOTE       = 0x8B0;
const wxWindowID ID_ROOM_PANEL_DUMMY    = 0x8C0;

const wxColour COLOR_DOOR[8] = {
	wxColour(0xCD329A),// purple
	wxColour(0x00EE76),// green
	wxColour(0xEE861C),// blue
	wxColour(0x00D7FF),// yellow
	wxColour(0xDD99FF),// pink
	wxColour(0x009900),// dark green
	wxColour(0xFFDDDD),// pale blue
	wxColour(0xAAAA00),// dark teal
};

const wxColour COLOR_COLLIDE =
	wxColour(0x2626CD); // red

const wxColour COLOR_WATER =
	wxColour(0xFF0000); // blue

const wxColour COLOR_SELECT =
	wxColour(0x00FF00); // green

const wxColor COLOR_GRID0 =
	wxColour(0x000088); // dark red
const wxColor COLOR_GRID1 =
	wxColour(0x000055); // dark red

//
// RoomPanel
//

const unsigned char ATTRIB_TILE[64] = {
	0,0,0,0,1,1,1,1,
	0,0,0,0,1,1,1,1,
	0,0,0,0,1,1,1,1,
	0,0,0,0,1,1,1,1,
	2,2,2,2,3,3,3,3,
	2,2,2,2,3,3,3,3,
	2,2,2,2,3,3,3,3,
	2,2,2,2,3,3,3,3};

class LizardTool::RoomPanel : public wxPanel
{
public:
	Data* data;
	Data::Room* room;
	Data::Chr* no_chr;
	Data::Palette* no_palette;

	static Data::Chr* tool_chr;
	static Data::Palette* boss_door_palette;
	static Data::Sprite* boss_door_exit_sprite;

	bool hide_tile;
	bool no_tile;
	bool hide_sprite;
	bool hide_utility;
	bool hide_diagnostic;
	bool grid;
	bool doors;
	bool collide;
	bool collide_solid;

	int sel_x0, sel_y0, sel_x1, sel_y1;
	int sel_ready;

	bool drag_init; // prevents destructive edit on double click to edit

	static const int STRETCH = 2;

	RoomPanel(wxWindow* parent, wxWindowID id, Data* data_, Data::Room* room_, Data::Chr* no_chr_, Data::Palette* no_palette_) :
		wxPanel(parent,id,wxDefaultPosition,wxSize(8*64*STRETCH,8*30*STRETCH)),
		data(data_),
		room(room_),
		no_chr(no_chr_),
		no_palette(no_palette_),
		hide_tile(false),
		no_tile(false),
		hide_sprite(false),
		hide_utility(false),
		hide_diagnostic(false),
		grid(false),
		doors(true),
		collide(false),
		collide_solid(false),
		drag_init(false)
	{
		wxASSERT(no_chr);
		wxASSERT(no_palette);

		tool_chr = (Data::Chr*)data->find_item(Data::ITEM_CHR,wxT("tool_"));

		boss_door_palette = (Data::Palette*)data->find_item(Data::ITEM_PALETTE,wxT("boss_door"));
		boss_door_exit_sprite = (Data::Sprite*)data->find_item(Data::ITEM_SPRITE,wxT("boss_door_exit_"));
		if (boss_door_palette == NULL) boss_door_palette = no_palette;

		sel_x0 = 0;
		sel_y0 = 0;
		sel_x1 = 0;
		sel_y1 = 0;
		sel_ready = 0;

		SetBackgroundStyle(wxBG_STYLE_CUSTOM);
		Connect(wxEVT_PAINT, wxPaintEventHandler(RoomPanel::OnPaint));
	}

	void PaintRegion(int x0, int y0, int x1, int y1, wxDC& dc)
	{

		// assemble drawable pal and tile data

		Data::Chr* draw_chr[8];
		for (int i=0; i<8; ++i)
		{
			draw_chr[i] = room->chr[i];
			if (draw_chr[i] == NULL)
				draw_chr[i] = no_chr;
		}
		if (tool_chr != NULL)
		{
			// replace lizard sprite page with tool sprite page
			draw_chr[4] = tool_chr;
		}

		Data::Palette* draw_pal[9];
		for (int i=0; i<8; ++i)
		{
			draw_pal[i] = room->palette[i];
			if (draw_pal[i] == NULL)
				draw_pal[i] = no_palette;
		}
		draw_pal[8] = boss_door_palette;

		wxPen draw_pen[9*4];
		wxBrush draw_brush[9*4];
		for (int p=0; p<9; ++p)
		for (int c=0; c<4; ++c)
		{
			unsigned char d = draw_pal[p]->data[c];
			if (hide_diagnostic && (d & 0x0F) == 0x0F) d = 0x0F;

			const wxColour& color = Data::nes_palette()[d];
			draw_pen[  (4*p)+c] = wxPen(wxPen(color,1));
			draw_brush[(4*p)+c] = wxBrush(color);
		}

		if (!no_tile)
		{
			for (int y=y0; y<y1; ++y)
			for (int x=x0; x<x1; ++x)
			{
				int tile = room->nametable[x+(y*64)];
				int attrib = room->attribute[(x>>1)+((y>>1)*32)];
				const unsigned char* tdata =
					hide_tile ? ATTRIB_TILE :
					draw_chr[(tile/64)&0x3]->data[tile&63];

				for (int ty=0; ty<8; ++ty)
				for (int tx=0; tx<8; ++tx)
				{
					int c = tdata[(ty*8)+tx] + (attrib*4);
					dc.SetPen(draw_pen[c]);
					dc.SetBrush(draw_brush[c]);
					dc.DrawRectangle(((x*8)+tx)*STRETCH,((y*8)+ty)*STRETCH,STRETCH,STRETCH);
				}
			}
		}
		else
		{
			dc.SetBackground(wxBrush(wxColour(0xFF00FF)));
			dc.Clear();
		}

		if (grid)
		{
			dc.SetPen(wxPen(COLOR_GRID0,1));
			dc.SetBrush(wxBrush(COLOR_GRID0));
			for (int y=y0; y<y1; y+=2)
				dc.DrawLine(
					(x0*8)*STRETCH,
					(y *8)*STRETCH,
					(x1*8)*STRETCH,
					(y *8)*STRETCH);
			for (int x=x0; x<x1; x+=2)
				dc.DrawLine(
					(x *8)*STRETCH,
					(y0*8)*STRETCH,
					(x *8)*STRETCH,
					(y1*8)*STRETCH);
			dc.SetPen(wxPen(COLOR_GRID1,1));
			dc.SetBrush(wxBrush(COLOR_GRID1));
			for (int y=y0+1; y<y1; y+=2)
				dc.DrawLine(
					(x0*8)*STRETCH,
					(y *8)*STRETCH,
					(x1*8)*STRETCH,
					(y *8)*STRETCH);
			for (int x=x0+1; x<x1; x+=2)
				dc.DrawLine(
					(x *8)*STRETCH,
					(y0*8)*STRETCH,
					(x *8)*STRETCH,
					(y1*8)*STRETCH);
		}

		if (!hide_sprite)
		{
			for (int d=15; d>=0; --d)
			{
				const int dt = room->dog[d].type;
				if (dt <= 0) continue;

				if (hide_utility && data->is_utility(dt))
				{
					continue;
				}

				const Data::Sprite* sprite = data->get_dog_sprite(room->dog[d]);
				if (sprite->count < 1) continue;

				const int EPS = 2; // region to redraw sprites around
				const int dx = room->dog[d].x;
				const int dy = room->dog[d].y;
				if ((dx/8) < (x0-EPS) || (dx/8) >= (x1+EPS)) continue;
				if ((dy/8) < (y0-EPS) || (dy/8) >= (y1+EPS)) continue;

				for (int t=sprite->count-1; t>=0; --t)
				{
					int tile = sprite->tile[t];
					int attrib = (sprite->palette[t] & 3) + 4 + (sprite->vpal ? (d&1) : 0);
					const unsigned char* tdata =
						draw_chr[4+((tile/64)&0x3)]->data[tile&63];

					if (sprite == boss_door_exit_sprite) attrib = 8;

					const int ox = dx + sprite->x[t];
					const int oy = dy + sprite->y[t];

					for (int ty=0; ty<8; ++ty)
					for (int tx=0; tx<8; ++tx)
					{
						const int fx = sprite->flip_x[t] ? (7-tx) : tx;
						const int fy = sprite->flip_y[t] ? (7-ty) : ty;
						int td = tdata[(fy*8)+fx]; 
						if (td == 0) continue; // transparent
						int c = td + (attrib*4);
						dc.SetPen(draw_pen[c]);
						dc.SetBrush(draw_brush[c]);
						dc.DrawRectangle((ox+tx)*STRETCH,(oy+ty)*STRETCH,STRETCH,STRETCH);
					}
				}
			}
		}

		if (collide)
		{
			dc.SetPen(wxPen(COLOR_COLLIDE,1));
			if (collide_solid)
				dc.SetBrush(wxBrush(COLOR_COLLIDE));
			else
				dc.SetBrush(*wxTRANSPARENT_BRUSH);
			for (int y=y0; y<y1; ++y)
			for (int x=x0; x<x1; ++x)
			{
				unsigned char tile = room->nametable[x+(y*64)];
				if (tile >= 0x80)
				{
					int tx = (x*8)*STRETCH;
					int ty = (y*8)*STRETCH;
					dc.DrawRectangle(tx,ty,8*STRETCH,8*STRETCH);
					dc.DrawLine(tx,ty,tx+(8*STRETCH),ty+(8*STRETCH));
					dc.DrawLine(tx+(8*STRETCH),ty,tx,ty+(8*STRETCH));
				}
			}

			// water level
			dc.SetPen(wxPen(COLOR_WATER,1));
			dc.DrawLine(0,room->water*STRETCH,512*STRETCH,room->water*STRETCH);
		}

		if (doors)
		{
			for (int d=0; d<8; ++d)
			{
				int dx = room->door_x[d];
				int dy = room->door_y[d];
				if (dx < (x0*8-16) || dx >= (x1*8+16)) continue;
				if (dy < (y0*8-16) || dy >= (y1*8+16)) continue;
				int tx = (dx-8 )*STRETCH;
				int ty = (dy-16)*STRETCH;
				int dw = 3+d;
				dc.SetPen(wxPen(COLOR_DOOR[d],1));
				dc.SetBrush(*wxTRANSPARENT_BRUSH);
				dc.DrawRectangle(tx+1,ty+1,(16*STRETCH)-2,(16*STRETCH)-2);
				dc.DrawRectangle(tx+dw,ty+dw,(16*STRETCH)-(2*dw),(16*STRETCH)-(2*dw));
			}
		}

		if (sel_x0 < sel_x1 && sel_y0 < sel_y1)
		{
			dc.SetPen(wxPen(COLOR_SELECT,1));
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(sel_x0*8*STRETCH,sel_y0*8*STRETCH,(sel_x1-sel_x0)*8*STRETCH,(sel_y1-sel_y0)*8*STRETCH);
		}
	}

	virtual void OnPaint(wxPaintEvent& event)
	{
		wxBufferedPaintDC dc(this);
		PaintRegion(0,0,64,30,dc);
	}

	void RefreshTile(int x, int y)
	{
		wxClientDC dc(this);
		int ax = x & ~1; // expand to cover 2x2 attribute region
		int ay = y & ~1;
		PaintRegion(ax,ay,ax+2,ay+2,dc);
	}

	void GetStat(int gx, int gy, int* tile, int* attrib, int* tx, int* ty)
	{
		int x = gx / (STRETCH * 8);
		int y = gy / (STRETCH * 8);
		*tx = x;
		*ty = y;
		*tile = room->nametable[(y*64)+x];
		*attrib = room->attribute[((y>>1)*32)+(x>>1)];
	}
};

Data::Chr* LizardTool::RoomPanel::tool_chr = NULL;
Data::Palette* LizardTool::RoomPanel::boss_door_palette = NULL;
Data::Sprite* LizardTool::RoomPanel::boss_door_exit_sprite = NULL;

//
// RoomChrPanel
//

RoomChrPanel::RoomChrPanel(wxWindow* parent, wxWindowID id, const Data::Chr* chr_) :
	wxPanel(parent,id,wxDefaultPosition,wxSize(8*16*STRETCH,8*4*STRETCH)),
	chr(chr_),
	select(-1)
{
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	Connect(wxEVT_PAINT, wxPaintEventHandler(RoomChrPanel::OnPaint));
}

void RoomChrPanel::OnPaint(wxPaintEvent& event)
{
	wxBufferedPaintDC dc(this);

	for (int y=0; y<32; ++y)
	for (int x=0; x<128; ++x)
	{
		const int t = (x/8)+((y/8)*16);
		const int p = (x&7)+((y&7)*8);
		const int c = chr->data[t][p];

		const wxColour& color = col[c];
		dc.SetPen(wxPen(color,1));
		dc.SetBrush(wxBrush(color));
		dc.DrawRectangle((x*STRETCH),(y*STRETCH),STRETCH,STRETCH);
	}

	if (select < 0) return;

	int sx = select & 15;
	int sy = select / 16;
	dc.SetPen(*wxRED_PEN);
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.DrawRectangle(sx*8*STRETCH,sy*8*STRETCH,8*STRETCH,8*STRETCH);
}

void RoomChrPanel::SetChr(Data::Chr* chr_)
{
	wxASSERT(chr_->type == Data::ITEM_CHR);
	chr = chr_;
}

void RoomChrPanel::SetColor(int i, wxColour c)
{
	col[i] = c;
}

bool RoomChrPanel::SetSelect(int s)
{
	bool changed = (select != s);
	select = s;
	return changed;
}

void RoomChrPanel::GetStat(int gx, int gy, int* tile, int* tx, int* ty)
{
	int x = gx / (STRETCH * 8);
	int y = gy / (STRETCH * 8);
	*tx = x;
	*ty = y;
	*tile = x + (y * 16);
}

//
// EditRoom
//

unsigned int LizardTool::EditRoom::password_lizard = 0;

EditRoom::EditRoom(Data::Item* item_, Data* data_, Package* package_) :
	Editor(item_, data_, package_)
{
	wxASSERT(item_->type == Data::ITEM_ROOM);
	room = (Data::Room*)item_;

	wxSize min_size = wxSize(1150+512,710);
	SetMinSize(min_size);
	SetMaxSize(min_size);
	SetSize(min_size);

	no_palette.name = wxT("(no palette)");
	for (int i=0; i<4; ++i) no_palette.data[i] = 0x0F | (i << 4); // editor-fake-black

	// setup default chr with X pattern
	no_chr.name = wxT("(no chr)");
	for (int i=0; i<64; ++i)
	for (int x=0; x<8; ++x)
	{
		no_chr.data[i][   x +(x*8)] ^= 0x02;
		no_chr.data[i][(7-x)+(x*8)] ^= 0x02;
	}

	// three regions (512px left, 262px mid, 115px right)

	wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* top_left = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* top_mid = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* top_right = new wxBoxSizer(wxVERTICAL);

	SetSizer(top_sizer);
	top_sizer->Add(top_left);
	top_sizer->Add(top_mid);
	top_sizer->Add(top_right);

	// left side: room panel, tile/palette lists

	wxBoxSizer* room_status_box = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* left_lower = new wxBoxSizer(wxHORIZONTAL);

	room_panel = new RoomPanel(this, ID_ROOM_PANEL, data, room, &no_chr, &no_palette);
	top_left->Add(room_panel);
	top_left->Add(room_status_box);
	top_left->Add(left_lower,0,wxLEFT|wxTOP,8);
	room_panel->Connect(ID_ROOM_PANEL, wxEVT_MOTION,    wxMouseEventHandler(EditRoom::OnRoomPanelMove ), NULL, this);
	room_panel->Connect(ID_ROOM_PANEL, wxEVT_LEFT_DOWN, wxMouseEventHandler(EditRoom::OnRoomPanelClick), NULL, this);
	room_panel->Connect(ID_ROOM_PANEL, wxEVT_RIGHT_DOWN,wxMouseEventHandler(EditRoom::OnRoomPanelRclick),NULL, this);
	room_panel->Connect(ID_ROOM_PANEL, wxEVT_KEY_DOWN,  wxKeyEventHandler(EditRoom::OnKeyDown),          NULL, this);
	
	check_attrib  = new wxCheckBox(this,   ID_ROOM_CHECK_ATTRIB, wxT("Attribute"));
	check_sprite  = new wxCheckBox(this,   ID_ROOM_CHECK_SPRITE, wxT("Sprites"));
	check_grid    = new wxCheckBox(this,   ID_ROOM_CHECK_GRID,   wxT("Grid"));
	check_doors   = new wxCheckBox(this,   ID_ROOM_CHECK_DOORS,  wxT("Doors"));
	check_collide = new wxCheckBox(this,   ID_ROOM_CHECK_COLLIDE,wxT("Collide"));
	room_status   = new wxStaticText(this, ID_ROOM_STATUS,       wxT(""));
	room_status_box->Add(check_attrib ,0,wxLEFT|wxTOP,4);
	room_status_box->Add(check_sprite ,0,wxLEFT|wxTOP,4);
	room_status_box->Add(check_grid   ,0,wxLEFT|wxTOP,4);
	room_status_box->Add(check_doors  ,0,wxLEFT|wxTOP,4);
	room_status_box->Add(check_collide,0,wxLEFT|wxTOP,4);
	room_status_box->Add(room_status ,1,wxEXPAND|wxLEFT|wxTOP,4);
	check_sprite->SetValue(true); // sprites are shown by default
	check_doors->SetValue(true); // doors are shown by default
	Connect(ID_ROOM_CHECK_ATTRIB,  wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(EditRoom::OnCheckAttrib ));
	Connect(ID_ROOM_CHECK_SPRITE,  wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(EditRoom::OnCheckSprite ));
	Connect(ID_ROOM_CHECK_GRID,    wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(EditRoom::OnCheckGrid   ));
	Connect(ID_ROOM_CHECK_DOORS,   wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(EditRoom::OnCheckDoors  ));
	Connect(ID_ROOM_CHECK_COLLIDE, wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(EditRoom::OnCheckCollide));

	wxStaticBoxSizer* chr_list_box_back = new wxStaticBoxSizer(wxVERTICAL,this,wxT("Chr (Tile)"));
	wxStaticBoxSizer* chr_list_box_fore = new wxStaticBoxSizer(wxVERTICAL,this,wxT("Chr (Sprite)"));
	wxStaticBoxSizer* pal_list_box_back = new wxStaticBoxSizer(wxVERTICAL,this,wxT("Palette (Tile)"));
	wxStaticBoxSizer* pal_list_box_fore = new wxStaticBoxSizer(wxVERTICAL,this,wxT("Palette (Sprite)"));
	list_chr_back = new wxListBox(this, ID_ROOM_CHR_LIST_BACK, wxDefaultPosition, wxSize(100,64));
	list_chr_fore = new wxListBox(this, ID_ROOM_CHR_LIST_FORE, wxDefaultPosition, wxSize(100,64));
	list_pal_back = new wxListBox(this, ID_ROOM_PAL_LIST_BACK, wxDefaultPosition, wxSize(100,64));
	list_pal_fore = new wxListBox(this, ID_ROOM_PAL_LIST_FORE, wxDefaultPosition, wxSize(100,64));
	left_lower->Add(chr_list_box_back);
	left_lower->Add(pal_list_box_back);
	left_lower->Add(chr_list_box_fore);
	left_lower->Add(pal_list_box_fore);
	chr_list_box_back->Add(list_chr_back);
	chr_list_box_fore->Add(list_chr_fore);
	pal_list_box_back->Add(list_pal_back);
	pal_list_box_fore->Add(list_pal_fore);
	Connect(ID_ROOM_CHR_LIST_BACK, wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(EditRoom::OnListDclick));
	Connect(ID_ROOM_CHR_LIST_FORE, wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(EditRoom::OnListDclick));
	Connect(ID_ROOM_PAL_LIST_BACK, wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(EditRoom::OnListDclick));
	Connect(ID_ROOM_PAL_LIST_FORE, wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(EditRoom::OnListDclick));

	wxButton* assume_chr_pal = new wxButton(this, ID_ROOM_ASSUME, wxT("Assume CHR/palettes..."));
	Connect(ID_ROOM_ASSUME, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditRoom::OnAssume));

	wxButton* auto_link = new wxButton(this, ID_ROOM_AUTO_LINK, wxT("Auto Link"));
	Connect(ID_ROOM_AUTO_LINK, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditRoom::OnAutoLink));

	wxButton* unlink = new wxButton(this, ID_ROOM_UNLINK, wxT("Unlink"));
	Connect(ID_ROOM_UNLINK, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditRoom::OnUnlink));

	if (password_lizard >= data->lizard_count()) password_lizard = 0;
	wxString password_text_init = data->password_build(data->find_index(room),password_lizard);

	wxStaticText* password_label = new wxStaticText(this,wxID_ANY,wxT("Password:"));

	password_combo = new wxChoice(this, ID_ROOM_PASSWORD_COMBO,
		wxDefaultPosition,wxSize(-1,-1),data->lizard_count(),data->lizard_list());
	password_combo->SetSelection(password_lizard);
	Connect(ID_ROOM_PASSWORD_COMBO, wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(EditRoom::OnPasswordCombo));

	password_text = new wxTextCtrl(this, ID_ROOM_PASSWORD_TEXT,password_text_init,
		wxDefaultPosition,wxSize(60,-1),wxTE_READONLY);

	wxBoxSizer* box_tools_vertical = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* box_tools = new wxBoxSizer(wxHORIZONTAL);
	box_tools->Add(assume_chr_pal);
	box_tools->Add(auto_link);
	box_tools->Add(unlink,0,wxLEFT,8);
	box_tools->Add(password_label,0,wxLEFT,8);
	box_tools->Add(password_combo);
	box_tools->Add(password_text);
	edit_status = new wxStaticText(this, ID_ROOM_EDIT_STATUS, wxT(""),wxDefaultPosition,wxSize(400,16),wxST_NO_AUTORESIZE | wxALIGN_CENTRE);
	edit_status->SetBackgroundColour(*wxLIGHT_GREY);

	wxButton* test_nes = new wxButton(this, ID_ROOM_TEST_NES, wxT("Test NES"));
	wxButton* test_pc  = new wxButton(this, ID_ROOM_TEST_PC,  wxT("Test PC"));
	wxButton* strip    = new wxButton(this, ID_ROOM_STRIP,    wxT("Strip"));
	Connect(ID_ROOM_TEST_NES, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditRoom::OnTest));
	Connect(ID_ROOM_TEST_PC,  wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditRoom::OnTest));
	Connect(ID_ROOM_STRIP,    wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditRoom::OnStrip));

	wxBoxSizer* box_test = new wxBoxSizer(wxHORIZONTAL);
	box_test->Add(test_nes);
	box_test->Add(test_pc,0,wxLEFT,8);
	box_test->Add(strip,0,wxLEFT,20);

	box_tools_vertical->Add(box_tools);
	box_tools_vertical->Add(edit_status, 0, wxTOP, 4);
	box_tools_vertical->Add(box_test,0,wxTOP,4);
	left_lower->Add(box_tools_vertical, 0, wxLEFT | wxTOP, 8);

	// mid: chr and palettes

	wxStaticBoxSizer* box_back = new wxStaticBoxSizer(wxVERTICAL,this,wxT("Background"));
	wxStaticBoxSizer* box_fore = new wxStaticBoxSizer(wxVERTICAL,this,wxT("Sprites"));
	wxBoxSizer* chr_back = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* chr_fore = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* palette_back = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* palette_fore = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* asplit_back = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* asplit_fore = new wxBoxSizer(wxHORIZONTAL);

	top_mid->Add(box_back);
	top_mid->Add(box_fore);
	asplit_back->Add(chr_back);
	asplit_fore->Add(chr_fore);
	box_back->Add(asplit_back);
	box_fore->Add(asplit_fore);
	box_back->Add(palette_back);
	box_fore->Add(palette_fore);

	for (int i=0; i<8; ++i)
	{
		room_chr[i] = new RoomChrPanel(this, ID_ROOM_CHR + i, &no_chr);
		room_pal[i] = new ColorGrid(this, ID_ROOM_PAL + i, 4, 1);

		room_chr[i]->Connect(ID_ROOM_CHR+i, wxEVT_MOTION,     wxMouseEventHandler(EditRoom::OnChrMove  ), NULL, this);
		room_chr[i]->Connect(ID_ROOM_CHR+i, wxEVT_RIGHT_DOWN, wxMouseEventHandler(EditRoom::OnChrRclick), NULL, this);
		if (i<4)
			room_chr[i]->Connect(ID_ROOM_CHR+i, wxEVT_LEFT_DOWN, wxMouseEventHandler(EditRoom::OnChrClick ), NULL, this);

		room_pal[i]->Connect(ID_ROOM_PAL+i, wxEVT_LEFT_DOWN,  wxMouseEventHandler(EditRoom::OnPalClick ), NULL, this);
		room_pal[i]->Connect(ID_ROOM_PAL+i, wxEVT_RIGHT_DOWN, wxMouseEventHandler(EditRoom::OnPalRclick), NULL, this);

		room_chr[i]->Connect(ID_ROOM_CHR+i, wxEVT_KEY_DOWN, wxKeyEventHandler(EditRoom::OnKeyDown), NULL, this);
		room_pal[i]->Connect(ID_ROOM_PAL+i, wxEVT_KEY_DOWN, wxKeyEventHandler(EditRoom::OnKeyDown), NULL, this);
	}
	for (int i=0; i<4; ++i)
	{
		chr_back->Add(room_chr[i+0]);
		chr_fore->Add(room_chr[i+4]);
		palette_back->Add(room_pal[i+0],0,wxTOP|wxLEFT,2);
		palette_fore->Add(room_pal[i+4],0,wxTOP|wxLEFT,2);
	}

	// right side: room properties and dog list

	wxStaticBoxSizer* box_prop = new wxStaticBoxSizer(wxVERTICAL,this,wxT("Properties"));
	wxStaticBoxSizer* box_dogs = new wxStaticBoxSizer(wxVERTICAL,this,wxT("Dogs"));
	top_right->Add(box_prop);
	top_right->Add(box_dogs);

	wxBoxSizer* box_coord = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* room_coord_label = new wxStaticText(this,wxID_ANY,wxT("Coords:"));
	room_coord_x = new wxTextCtrl(this,ID_ROOM_COORD_X,wxString::Format(wxT("%d"),room->coord_x),
		wxDefaultPosition,wxSize(32,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));
	room_coord_y = new wxTextCtrl(this,ID_ROOM_COORD_Y,wxString::Format(wxT("%d"),room->coord_y),
		wxDefaultPosition,wxSize(32,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));
	room_recto = new wxCheckBox(this,ID_ROOM_RECTO,wxT("Recto"));
	room_recto->SetValue(room->recto);
	room_ship = new wxCheckBox(this,ID_ROOM_SHIP,wxT("Ship"));
	room_ship->SetValue(room->ship);
	Connect(ID_ROOM_COORD_X, wxEVT_COMMAND_TEXT_UPDATED,     wxCommandEventHandler(EditRoom::OnCoordX));
	Connect(ID_ROOM_COORD_Y, wxEVT_COMMAND_TEXT_UPDATED,     wxCommandEventHandler(EditRoom::OnCoordY));
	Connect(ID_ROOM_RECTO,   wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(EditRoom::OnRecto ));
	Connect(ID_ROOM_SHIP,    wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(EditRoom::OnShip  ));
	box_prop->Add(box_coord);
	box_coord->Add(room_coord_label);
	box_coord->Add(room_coord_x);
	box_coord->Add(room_coord_y);
	box_coord->Add(room_recto,0,wxLEFT,4);
	box_coord->Add(room_ship,0,wxLEFT,8);

	wxBoxSizer* box_music = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* room_music_label = new wxStaticText(this,wxID_ANY,wxT("Music:"));

	room_music = new wxChoice(this,ID_ROOM_MUSIC,
		wxDefaultPosition, wxSize(100,-1),data->music_count(),data->music_list());
	if (room->music > data->music_count()) room->music = 0;
	room_music->SetSelection(room->music);
	Connect(ID_ROOM_MUSIC, wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(EditRoom::OnMusic));
	
	box_prop->Add(box_music,0,wxTOP,4);
	box_music->Add(room_music_label);
	box_music->Add(room_music);

	wxBoxSizer* box_water = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* room_water_label = new wxStaticText(this,wxID_ANY,wxT("Water:"));
	room_water = new wxTextCtrl(this,ID_ROOM_WATER,wxString::Format(wxT("%d"),room->water),
		wxDefaultPosition,wxSize(32,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));
	Connect(ID_ROOM_WATER, wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(EditRoom::OnWater));

	box_prop->Add(box_water,0,wxTOP,4);
	box_water->Add(room_water_label);
	box_water->Add(room_water);

	room_scrolling = new wxCheckBox(this, ID_ROOM_SCROLLING,wxT("Scrolling"));
	room_scrolling->SetValue(room->scrolling);
	box_water->Add(room_scrolling,0,wxLEFT,16);
	Connect(ID_ROOM_SCROLLING, wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(EditRoom::OnScrolling));
	
	wxStaticText* doors_label = new wxStaticText(this,wxID_ANY,wxT("Doors:"));
	box_prop->Add(doors_label,0,wxTOP,4);

	for (int i=0; i<8; ++i)
	{
		wxBoxSizer* box_door = new wxBoxSizer(wxHORIZONTAL);

		wxStaticText* door_num = new wxStaticText(this,wxID_ANY,wxString::Format(wxT("%d"),i),
			wxDefaultPosition,wxSize(16,-1),wxALIGN_CENTER);
		wxButton* idoor_link = new wxButton(this, ID_ROOM_DOOR_LINK + i, wxT("(none)"),
			wxDefaultPosition,wxSize(124,-1),wxBU_LEFT);
		wxTextCtrl* idoor_x = new wxTextCtrl(this,ID_ROOM_DOOR_X+i,wxT("-"),
			wxDefaultPosition,wxSize(32,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));
		wxTextCtrl* idoor_y = new wxTextCtrl(this,ID_ROOM_DOOR_Y+i,wxT("-"),
			wxDefaultPosition,wxSize(32,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));

		door_num->SetForegroundColour(COLOR_DOOR[i]);
		if (room->door[i] != NULL)
			idoor_link->SetLabel(room->door[i]->name);
		idoor_x->SetValue(wxString::Format(wxT("%d"),room->door_x[i]));
		idoor_y->SetValue(wxString::Format(wxT("%d"),room->door_y[i]));

		box_prop->Add(box_door);
		box_door->Add(door_num,0,wxTOP,1);
		box_door->Add(idoor_link);
		box_door->Add(idoor_x,0,wxLEFT,4);
		box_door->Add(idoor_y);

		door_link[i] = idoor_link;
		door_x[i] = idoor_x;
		door_y[i] = idoor_y;

		Connect(ID_ROOM_DOOR_LINK+i, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditRoom::OnDoorLink));
		Connect(ID_ROOM_DOOR_X+i,    wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler(EditRoom::OnDoorPosition));
		Connect(ID_ROOM_DOOR_Y+i,    wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler(EditRoom::OnDoorPosition));
		door_link[i]->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(EditRoom::OnDoorOpen), NULL, this);
	}

	wxBoxSizer* box_dog_header = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* dog_head_num = new wxStaticText(this,wxID_ANY,wxT("#"     ),wxDefaultPosition,wxSize(16, -1),wxALIGN_CENTER);
	wxStaticText* dog_head_spr = new wxStaticText(this,wxID_ANY,wxT("Sprite"),wxDefaultPosition,wxSize(100,-1),wxALIGN_CENTER);
	wxStaticText* dog_head_x   = new wxStaticText(this,wxID_ANY,wxT("X"     ),wxDefaultPosition,wxSize(32, -1),wxALIGN_CENTER);
	wxStaticText* dog_head_y   = new wxStaticText(this,wxID_ANY,wxT("Y"     ),wxDefaultPosition,wxSize(32, -1),wxALIGN_CENTER);
	wxStaticText* dog_head_p   = new wxStaticText(this,wxID_ANY,wxT("Param" ),wxDefaultPosition,wxSize(32, -1),wxALIGN_CENTER);

	box_dogs->Add(box_dog_header);
	box_dog_header->Add(dog_head_num);
	box_dog_header->Add(dog_head_spr);
	box_dog_header->Add(dog_head_x);
	box_dog_header->Add(dog_head_y);
	box_dog_header->Add(dog_head_p);

	for (int i=0; i<16; ++i)
	{
		wxBoxSizer* box_dog = new wxBoxSizer(wxHORIZONTAL);

		wxStaticText* dog_num = new wxStaticText(this,wxID_ANY,wxString::Format(wxT("%2d"),i),
			wxDefaultPosition,wxSize(16,-1),wxALIGN_CENTER);
		wxChoice* idog_sprite = new wxChoice(this,ID_ROOM_DOG_SPRITE+i,
			wxDefaultPosition,wxSize(100,-1),data->dog_count(),data->dog_list());
		wxTextCtrl* idog_x = new wxTextCtrl(this,ID_ROOM_DOG_X+i,wxT("-"),
			wxDefaultPosition,wxSize(32,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));
		wxTextCtrl* idog_y = new wxTextCtrl(this,ID_ROOM_DOG_Y+i,wxT("-"),
			wxDefaultPosition,wxSize(32,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));
		wxTextCtrl* idog_param = new wxTextCtrl(this,ID_ROOM_DOG_PARAM+i,wxT("-"),
			wxDefaultPosition,wxSize(32,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));
		wxTextCtrl* idog_note = new wxTextCtrl(this,ID_ROOM_DOG_NOTE+i,wxT("-"),
			wxDefaultPosition,wxSize(100,-1),0);

		if (i >= 11 && i <= 14) // 11-14 are typically reserved for AutoLink doors
		{
			dog_num->SetForegroundColour(COLOR_DOOR[i+4-11]);
		}

		box_dogs->Add(box_dog);
		box_dog->Add(dog_num);
		box_dog->Add(idog_sprite);
		box_dog->Add(idog_x);
		box_dog->Add(idog_y);
		box_dog->Add(idog_param);
		box_dog->Add(idog_note);

		if (room->dog[i].type >= data->dog_count()) room->dog[i].type = 0;
		idog_sprite->SetSelection(room->dog[i].type);
		idog_x->SetValue(wxString::Format(wxT("%d"),room->dog[i].x));
		idog_y->SetValue(wxString::Format(wxT("%d"),room->dog[i].y));
		idog_param->SetValue(wxString::Format(wxT("%d"),room->dog[i].param));
		idog_note->SetValue(room->dog[i].note);

		dog_sprite[i] = idog_sprite;
		dog_x[i] = idog_x;
		dog_y[i] = idog_y;
		dog_param[i] = idog_param;
		dog_note[i] = idog_note;

		Connect(ID_ROOM_DOG_SPRITE+i, wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(EditRoom::OnDogSprite));
		Connect(ID_ROOM_DOG_X+i,      wxEVT_COMMAND_TEXT_UPDATED,    wxCommandEventHandler(EditRoom::OnDogPosition));
		Connect(ID_ROOM_DOG_Y+i,      wxEVT_COMMAND_TEXT_UPDATED,    wxCommandEventHandler(EditRoom::OnDogPosition));
		Connect(ID_ROOM_DOG_PARAM+i,  wxEVT_COMMAND_TEXT_UPDATED,    wxCommandEventHandler(EditRoom::OnDogParam));
		Connect(ID_ROOM_DOG_NOTE+i,   wxEVT_COMMAND_TEXT_UPDATED,    wxCommandEventHandler(EditRoom::OnDogNote));
	}

	Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(EditRoom::OnKeyDown), NULL, this);

	// setup

	// clear dog sprite caches to prevent out-of-date rendering
	for (int d=0; d<16; ++d) room->dog[d].cached_sprite = NULL;

	tile_select = 0;
	chr_pal_select = -1;
	spr_pal_select = -1;
	undo_count = 0;
	multi_count = 0;
	ascii_tile = false;
	last_draw = 0;

	RefreshTileSelect();
	RefreshChrs();
	RefreshPalettes();

	PaletteSelect(false,0);
	PaletteSelect(true ,0);
}

//
// EditRoom refresh stuff
//

void EditRoom::RefreshTileSelect()
{
	int t = (tile_select / 64) & 3;
	for (int i=0; i<4; ++i)
	{
		int s = (i==t) ? (tile_select & 63) : -1;
		if (room_chr[i]->SetSelect(s))
			room_chr[i]->Refresh();
	}
}

void EditRoom::RefreshChrs()
{
	list_chr_back->Clear();
	list_chr_fore->Clear();
	for (int p=0; p<4; ++p)
	{
		Data::Chr* chr_back = room->chr[p+0];
		Data::Chr* chr_fore = room->chr[p+4];
		if (chr_back == NULL) chr_back = &no_chr;
		if (chr_fore == NULL) chr_fore = &no_chr;
		list_chr_back->Append(chr_back->name);
		list_chr_fore->Append(chr_fore->name);
		room_chr[p+0]->SetChr(chr_back);
		room_chr[p+4]->SetChr(chr_fore);
		room_chr[p+0]->Refresh();
		room_chr[p+4]->Refresh();
	}
}

void EditRoom::RefreshPalettes()
{
	for (int p=0; p<8; ++p)
	{
		Data::Palette* pal = room->palette[p];
		if (pal == NULL) pal = &no_palette;
		for (int c=0; c<4; ++c)
		{
			room_pal[p]->SetColor(c,0,Data::nes_palette()[pal->data[c]]);
		}
		room_pal[p]->Refresh();
	}

	list_pal_back->Clear();
	list_pal_fore->Clear();
	for (int p=0; p<4; ++p)
	{
		Data::Palette* pal_back = room->palette[p+0];
		Data::Palette* pal_fore = room->palette[p+4];
		if (pal_back == NULL) pal_back = &no_palette;
		if (pal_fore == NULL) pal_fore = &no_palette;
		list_pal_back->Append(pal_back->name);
		list_pal_fore->Append(pal_fore->name);
	}
}

void EditRoom::RefreshRight()
{
	for (int i=0; i<8; ++i)
	{
		door_link[i]->SetLabel((room->door[i] != NULL) ? room->door[i]->name : wxT("(none)"));
		door_x[i]->SetValue(wxString::Format(wxT("%d"),room->door_x[i]));
		door_y[i]->SetValue(wxString::Format(wxT("%d"),room->door_y[i]));
	}

	for (int i=0; i<16; ++i)
	{
		dog_sprite[i]->SetSelection(room->dog[i].type);
		dog_x[i]->SetValue(wxString::Format(wxT("%d"),room->dog[i].x));
		dog_y[i]->SetValue(wxString::Format(wxT("%d"),room->dog[i].y));
		dog_param[i]->SetValue(wxString::Format(wxT("%d"),room->dog[i].param));
	}
}

void EditRoom::PaletteSelect(bool sprite, int s)
{
	int offset = 0;
	int old_s = 0;

	if (sprite)
	{
		old_s = spr_pal_select;
		spr_pal_select = s;
		offset = 4;
	}
	else
	{
		old_s = chr_pal_select;
		chr_pal_select = s;
		offset = 0;
	}
	if (s == old_s) return; // nothing new

	for (int i=0;i<4;++i)
	{
		if (i != s)
		{
			room_pal[i+offset]->highlight = false;
			room_pal[i+offset]->Refresh();
		}
	}
	room_pal[s+offset]->highlight = true;
	room_pal[s+offset]->Refresh();

	Data::Palette* pal = room->palette[s+offset];
	if (pal == NULL) pal = &no_palette;
	for (int i=0; i<4; ++i)
	{
		for (int c=0; c<4; ++c)
		{
			room_chr[i+offset]->SetColor(c,Data::nes_palette()[pal->data[c]]);
		}
		room_chr[i+offset]->Refresh();
	}
}

//
// EditRoom events
//

void EditRoom::OnRoomPanelMove(wxMouseEvent& event)
{
	int x = event.GetX();
	int y = event.GetY();
	int t,a,tx,ty;
	room_panel->GetStat(x,y,&t,&a,&tx,&ty);

	room_status->SetLabel(wxString::Format(wxT("Nametable: %2d, %2d (%3d, %3d) = %02Xh (%1X)"),
		tx, ty, tx*8, ty*8, t, a));
	room_status->Refresh();

	if (room_panel->drag_init && event.Dragging())
	{
		if (event.RightIsDown()) OnRoomPanelRclick(event);
		else                     OnRoomPanelClick(event);
	}
}

void EditRoom::OnRoomPanelClick(wxMouseEvent& event)
{
	room_panel->drag_init = true;

	int x = event.GetX();
	int y = event.GetY();
	int t,a,tx,ty;
	room_panel->GetStat(x,y,&t,&a,&tx,&ty);

	wxASSERT(tx >= 0 && tx < 64);
	wxASSERT(ty >= 0 && ty < 30);

	int ax = tx >> 1;
	int ay = ty >> 1;

	if (t != tile_select || a != chr_pal_select || ascii_tile)
	{
		PushUndo();

		if (!room_panel->hide_tile && !event.ShiftDown())
			t = tile_select;
		a = chr_pal_select;
		room->nametable[(ty*64)+tx] = t;
		room->attribute[(ay*32)+ax] = a;
		last_draw = (ty*64)+tx;

		if (ascii_tile)
		{
			room_panel->sel_x0 = tx;
			room_panel->sel_y0 = ty;
			room_panel->sel_x1 = tx+1;
			room_panel->sel_y1 = ty+1;
			room_panel->Refresh();
		}
		else
		{
			room_panel->RefreshTile(tx,ty);
		}

		room_status->SetLabel(wxString::Format(wxT("Nametable: %2d, %2d = %02Xh (%1X)"),tx, ty, t, a));
		room_status->Refresh();
		data->unsaved = true;
		room->unsaved = true;
		room->ship = false;
		room_ship->SetValue(room->ship);
	}

	event.Skip(); // required to propagate focus
}

void EditRoom::OnRoomPanelRclick(wxMouseEvent& event)
{
	room_panel->drag_init = true;

	int x = event.GetX();
	int y = event.GetY();
	int t,a,tx,ty;
	room_panel->GetStat(x,y,&t,&a,&tx,&ty);

	wxASSERT(tx >= 0 && tx < 64);
	wxASSERT(ty >= 0 && ty < 30);

	if (room_panel->sel_ready == 1) // selection upper left corner
	{
		room_panel->sel_x0 = tx;
		room_panel->sel_y0 = ty;
		if (room_panel->sel_x1 <= tx) room_panel->sel_x1 = tx+1;
		if (room_panel->sel_y1 <= ty) room_panel->sel_y1 = ty+1;
		room_panel->Refresh();
		ClearEditStatus();
	}
	else if (room_panel->sel_ready == 2) // selection lower right corner
	{
		room_panel->sel_x1 = tx+1;
		room_panel->sel_y1 = ty+1;
		room_panel->Refresh();
		ClearEditStatus();
	}
	else if (room_panel->sel_ready == 3) // paste
	{
		PushUndo();

		const unsigned int w = data->nmt_clip_w;
		const unsigned int h = data->nmt_clip_h;
		for (unsigned int ny=0; ny<h; ++ny)
		for (unsigned int nx=0; nx<w; ++nx)
		{
			int px = tx + nx;
			int py = ty + ny;
			if (px >= 64 || py >= 30) continue;
			room->nametable[(py*64)+px] = data->nmt_clipboard[(ny*w)+nx];
		}
		room_panel->Refresh();
		ClearEditStatus();
		data->unsaved = true;
		room->unsaved = true;
		room->ship = false;
		room_ship->SetValue(room->ship);
	}
	else if (room_panel->sel_ready == 4 || room_panel->sel_ready == 5 || room_panel->sel_ready == 6) // flood fill
	{

		bool fill_attrib = room_panel->hide_tile || event.ShiftDown();

		int mode                             = 0; // fill similar
		if (room_panel->sel_ready == 5) mode = 1; // fill border
		if (room_panel->sel_ready == 6) mode = 2; // fill volcano

		unsigned char fill_value = fill_attrib ? chr_pal_select : tile_select;
		unsigned char fill_test  = fill_attrib ? a              : t;

		if (mode == 2 && !fill_attrib) fill_value = ~tile_select; // to prevent cancel if fill-value = fill_test

		if (fill_value != fill_test)
		{
			PushUndo();
			FloodFill(tx,ty, fill_attrib, mode, fill_value, fill_test);

			room_panel->Refresh();
			ClearEditStatus();
			data->unsaved = true;
			room->unsaved = true;
			room->ship = false;
			room_ship->SetValue(room->ship);
		}
		else
		{
			ClearEditStatus();
		}
	}
	else if (room_panel->sel_ready == 7) // replace
	{
		unsigned char fill_value = tile_select;
		unsigned char fill_test  = t;
		if (fill_value != fill_test)
		{
			PushUndo();
			for (int i=0; i<(64*30); ++i)
			{
				if(room->nametable[i] == fill_test) room->nametable[i] = fill_value;
			}

			room_panel->Refresh();
			ClearEditStatus();
			data->unsaved = true;
			room->unsaved = true;
			room->ship = false;
			room_ship->SetValue(room->ship);
		}
		else
		{
			ClearEditStatus();
		}
	}
	else if (t != tile_select || a != chr_pal_select)
	{
		tile_select = t;
		RefreshTileSelect();
		PaletteSelect(false,a);
	}

	this->SetFocus(); // right click does not seem to assume focus on its own
	event.Skip(); // required to propagate focus ?
}

void EditRoom::OnCheckAttrib(wxCommandEvent& event)
{
	room_panel->hide_tile = event.IsChecked();
	room_panel->Refresh();
}

void EditRoom::OnCheckSprite(wxCommandEvent& event)
{
	room_panel->hide_sprite = !event.IsChecked();
	room_panel->Refresh();
}

void EditRoom::OnCheckGrid(wxCommandEvent& event)
{
	room_panel->grid = event.IsChecked();
	room_panel->Refresh();
}

void EditRoom::OnCheckDoors(wxCommandEvent& event)
{
	room_panel->doors = event.IsChecked();
	room_panel->Refresh();
}

void EditRoom::OnCheckCollide(wxCommandEvent& event)
{
	room_panel->collide = event.IsChecked();
	room_panel->Refresh();
}

void EditRoom::OnListDclick(wxCommandEvent& event)
{
	bool fore = true;
	int index = 4;

	switch (event.GetId())
	{
	case ID_ROOM_CHR_LIST_BACK: index -= 4; fore = false; // fallthrough
	case ID_ROOM_CHR_LIST_FORE:
		{
			int i = event.GetSelection();
			if (i < 0 || i >= 4) return;
			index += i;

			PickChr* picker = new PickChr(this, data);
			picker->ShowModal();
			int result = picker->picked;
			picker->Destroy();

			if (result >= 0 && result < (int)data->get_item_count(Data::ITEM_CHR))
			{
				room->chr[index] = (Data::Chr*)data->get_item(Data::ITEM_CHR,result);
			}
			else if (result == -2)
			{
				room->chr[index] = NULL;
			}
			else return;

			RefreshChrs();

			room_panel->Refresh();
			data->unsaved = true;
			room->unsaved = true;
			room->ship = false;
			room_ship->SetValue(room->ship);
		}
		break;
	case ID_ROOM_PAL_LIST_BACK: index -= 4; fore = false; // fallthrough
	case ID_ROOM_PAL_LIST_FORE:
		{
			int i = event.GetSelection();
			if (i < 0 || i >= 4) return;
			index += i;

			PickPalette* picker = new PickPalette(this, data);
			picker->ShowModal();
			int result = picker->picked;
			picker->Destroy();

			if (result >= 0 && result < (int)data->get_item_count(Data::ITEM_PALETTE))
			{
				room->palette[index] = (Data::Palette*)data->get_item(Data::ITEM_PALETTE,result);
			}
			else if (result == -2)
			{
				room->palette[index] = NULL;
			}
			else return;

			RefreshPalettes();

			if (fore) spr_pal_select = -1; // refresh CHR
			else      chr_pal_select = -1;
			PaletteSelect(fore,i);

			room_panel->Refresh();
			data->unsaved = true;
			room->unsaved = true;
			room->ship = false;
			room_ship->SetValue(room->ship);
		}
		break;
	}
}

void EditRoom::OnChrMove(wxMouseEvent& event)
{
	int c = event.GetId() - ID_ROOM_CHR;
	wxASSERT(c >= 0 && c < 8);

	int x = event.GetX();
	int y = event.GetY();
	int t,tx,ty;

	room_chr[c]->GetStat(x,y,&t,&tx,&ty);

	room_status->SetLabel(wxString::Format(wxT("Tile: %02Xh"),t+((c&3)*64)));
	room_status->Refresh();

	if (event.Dragging() && c < 4) OnChrClick(event);
}

void EditRoom::OnChrClick(wxMouseEvent& event)
{
	int c = event.GetId() - ID_ROOM_CHR;
	wxASSERT(c >= 0 && c < 4);

	int x = event.GetX();
	int y = event.GetY();
	int t,tx,ty;

	room_chr[c]->GetStat(x,y,&t,&tx,&ty);

	tile_select = t + (c*64);
	RefreshTileSelect();

	event.Skip(); // required to propagate focus
}

void EditRoom::OnChrRclick(wxMouseEvent& event)
{
	int c = event.GetId() - ID_ROOM_CHR;
	wxASSERT(c >= 0 && c < 8);

	Data::Chr* chr = room->chr[c];
	if (chr == NULL)
		wxMessageBox(wxT("There is no CHR in that slot."),wxT("Can't do that!"),wxOK|wxICON_EXCLAMATION);
	else
		package->OpenEditor(chr);
}

void EditRoom::OnPalClick(wxMouseEvent& event)
{
	int p = event.GetId() - ID_ROOM_PAL;
	wxASSERT(p >= 0 && p < 8);

	PaletteSelect(p >= 4, p&3);

	event.Skip(); // required to propagate focus
}

void EditRoom::OnPalRclick(wxMouseEvent& event)
{
	int p = event.GetId() - ID_ROOM_PAL;
	wxASSERT(p >= 0 && p < 8);

	Data::Palette* pal = room->palette[p];
	if (pal == NULL)
		wxMessageBox(wxT("There is no palette in that slot."),wxT("Can't do that!"),wxOK|wxICON_EXCLAMATION);
	else
		package->OpenEditor(pal);
}

void EditRoom::OnCoordX(wxCommandEvent& event)
{
	long v = 0;
	room_coord_x->GetValue().ToLong(&v);
	room->coord_x = v;
	data->unsaved = true;
	room->unsaved = true;
	room->ship = false;
	room_ship->SetValue(room->ship);
	package->RefreshMap();
}

void EditRoom::OnCoordY(wxCommandEvent& event)
{
	long v = 0;
	room_coord_y->GetValue().ToLong(&v);
	room->coord_y = v;
	data->unsaved = true;
	room->unsaved = true;
	room->ship = false;
	room_ship->SetValue(room->ship);
	package->RefreshMap();
}

void EditRoom::OnRecto(wxCommandEvent& event)
{
	room->recto = event.IsChecked();
	data->unsaved = true;
	room->unsaved = true;
	room->ship = false;
	room_ship->SetValue(room->ship);
	package->RefreshMap();
}

void EditRoom::OnShip(wxCommandEvent& event)
{
	room->ship = event.IsChecked();
	data->unsaved = true;
	room->unsaved = true;
	package->RefreshMap();
}

void EditRoom::OnMusic(wxCommandEvent& event)
{
	room->music = room_music->GetSelection();
	data->unsaved = true;
	room->unsaved = true;
	room->ship = false;
	room_ship->SetValue(room->ship);
	package->RefreshMap();
}

void EditRoom::OnWater(wxCommandEvent& event)
{
	long v = 0;
	room_water->GetValue().ToLong(&v);
	room->water = v & 0xFF;
	if (room_panel->collide)
		room_panel->Refresh();
	data->unsaved = true;
	room->unsaved = true;
	room->ship = false;
	room_ship->SetValue(room->ship);
}

void EditRoom::OnScrolling(wxCommandEvent& event)
{
	room->scrolling = event.IsChecked();
	data->unsaved = true;
	room->unsaved = true;
	room->ship = false;
	room_ship->SetValue(room->ship);
	package->RefreshMap();
}

void EditRoom::OnDoorLink(wxCommandEvent& event)
{
	int d = event.GetId() - ID_ROOM_DOOR_LINK;
	wxASSERT(d >= 0 && d < 8);

	PickRoom* picker = new PickRoom(this, data);
	picker->ShowModal();
	int result = picker->picked;
	picker->Destroy();
	if (result >= 0 && result < (int)data->get_item_count(Data::ITEM_ROOM))
	{
		room->door[d] = (Data::Room*)data->get_item(Data::ITEM_ROOM,result);
	}
	else if (result == -2)
	{
		room->door[d] = NULL;
	}
	else return;


	RefreshRight();

	data->unsaved = true;
	room->unsaved = true;
	room->ship = false;
	room_ship->SetValue(room->ship);
}

void EditRoom::OnDoorOpen(wxMouseEvent& event)
{
	int d = event.GetId() - ID_ROOM_DOOR_LINK;
	wxASSERT(d >= 0 && d < 8);

	Data::Room* r = room->door[d];
	if (r) package->OpenEditor(r);
}

void EditRoom::OnDoorPosition(wxCommandEvent& event)
{
	int d = -1;
	wxTextCtrl* ctrl = NULL;
	int* val = NULL;
	int id = event.GetId();
	if (id >= ID_ROOM_DOOR_Y)
	{
		d = id - ID_ROOM_DOOR_Y;
		ctrl = door_y[d];
		val = &(room->door_y[d]);
	}
	else
	{
		d = id - ID_ROOM_DOOR_X;
		ctrl = door_x[d];
		val = &(room->door_x[d]);
	}
	wxASSERT(d >= 0 && d < 8);

	long v = 0;
	ctrl->GetValue().ToLong(&v);
	*val = v;

	if (room_panel->doors)
		room_panel->Refresh();
	data->unsaved = true;
	room->unsaved = true;
	room->ship = false;
	room_ship->SetValue(room->ship);
}

void EditRoom::OnDogSprite(wxCommandEvent& event)
{
	int s = event.GetId() - ID_ROOM_DOG_SPRITE;
	wxASSERT(s >= 0 && s < 16);

	room->dog[s].type = dog_sprite[s]->GetSelection();
	room->dog[s].cached_sprite = NULL;

	if (!room_panel->hide_sprite)
		room_panel->Refresh();
	data->unsaved = true;
	room->unsaved = true;
	room->ship = false;
	room_ship->SetValue(room->ship);
}

void EditRoom::OnDogPosition(wxCommandEvent& event)
{
	int s = -1;
	wxTextCtrl* ctrl = NULL;
	int* val = NULL;
	int id = event.GetId();
	if (id >= ID_ROOM_DOG_Y)
	{
		s = id - ID_ROOM_DOG_Y;
		ctrl = dog_y[s];
		val = &(room->dog[s].y);
	}
	else
	{
		s = id - ID_ROOM_DOG_X;
		ctrl = dog_x[s];
		val = &(room->dog[s].x);
	}
	wxASSERT(s >= 0 && s < 16);

	long v = 0;
	ctrl->GetValue().ToLong(&v);
	*val = v;

	if (!room_panel->hide_sprite)
		room_panel->Refresh();
	data->unsaved = true;
	room->unsaved = true;
	room->ship = false;
	room_ship->SetValue(room->ship);
}

void EditRoom::OnDogParam(wxCommandEvent& event)
{
	int s = event.GetId() - ID_ROOM_DOG_PARAM;
	wxASSERT(s >= 0 && s < 16);

	long v = 0;
	dog_param[s]->GetValue().ToLong(&v);
	room->dog[s].param = v;
	data->unsaved = true;
	room->unsaved = true;
	room->ship = false;
	room_ship->SetValue(room->ship);
}

void EditRoom::OnDogNote(wxCommandEvent& event)
{
	int s = event.GetId() - ID_ROOM_DOG_NOTE;
	wxASSERT(s >= 0 && s < 16);

	room->dog[s].note = dog_note[s]->GetValue();

	if (
		room->dog[s].type == Game::DOG_SPRITE0 ||
		room->dog[s].type == Game::DOG_SPRITE1 ||
		room->dog[s].type == Game::DOG_SPRITE2 )
	{
		room->dog[s].cached_sprite = NULL;
		if (!room_panel->hide_sprite)
			room_panel->Refresh();
	}

	data->unsaved = true;
	room->unsaved = true;
	room->ship = false;
	room_ship->SetValue(room->ship);
}

void EditRoom::OnAssume(wxCommandEvent& event)
{
	PickRoom* picker = new PickRoom(this, data);
	picker->ShowModal();
	int result = picker->picked;
	picker->Destroy();
	if (result < 0 || result >= (int)data->get_item_count(Data::ITEM_ROOM))
		return;

	Data::Room* r = (Data::Room*)data->get_item(Data::ITEM_ROOM,result);
	for (int i=0; i<8; ++i)
	{
		room->palette[i] = r->palette[i];
		room->chr[i] = r->chr[i];
	}
	RefreshPalettes();
	RefreshChrs();
	room_panel->Refresh();
	data->unsaved = true;
	room->unsaved = true;
	room->ship = false;
	room_ship->SetValue(room->ship);
}

void EditRoom::OnAutoLink(wxCommandEvent& event)
{
	bool valid = true;
	wxString output = wxT("");

	for (int i=11; i<15; ++i)
	{
	}

	if (room->coord_x < 0 || room->coord_y < 0)
	{
		output += wxT("Room coordinates are invalid for auto-linking (both must be positive).");
		valid = false;
	}

	if (valid)
	{
		int adjx[4] =
		{
			room->coord_x-1,
			room->coord_x+(room->scrolling?2:1),
			room->coord_x,
			room->coord_x,
		};
		int adjy[4] =
		{
			room->coord_y,
			room->coord_y,
			room->coord_y-1,
			room->coord_y+1,
		};

		const int LINK_TYPE[4] = { Game::DOG_PASS_X, Game::DOG_PASS_X, Game::DOG_PASS_Y, Game::DOG_PASS_Y };

		const int LINK_X[4] = {
			0,
			248 + (room->scrolling ? 256 : 0),
			0,
			0,
		};
		const int LINK_Y[4] = { 0, 0, 0, 232 };

		const unsigned char LINK_PARAM[4] = { 4, 5, 6, 7 };

		const int LINK_DX[4] = {
			240 + (room->scrolling ? 256 : 0),
			16,
			128 + (room->scrolling ? 128 : 0),
			128 + (room->scrolling ? 128 : 0),
		};
		const int LINK_DY[4] = { 120, 120, 232, 24 };

		for (int i=0; i<4; ++i)
		{
			bool skip = false;
			if (room->dog[i+11].type != Game::DOG_NONE &&
				room->dog[i+11].type != Game::DOG_PASS_X &&
				room->dog[i+11].type != Game::DOG_PASS_Y)
			{
				output += wxString::Format(wxT("Dog %d already in use.\n"),i+11);
				skip = true;
			}

			if (skip)
			{
				output += wxString::Format(wxT("Skipped dog: %d\n"),i+11);
			}
			else
			{
				room->dog[i+11].type = LINK_TYPE[i];
				room->dog[i+11].x = LINK_X[i];
				room->dog[i+11].y = LINK_Y[i];
				room->dog[i+11].param = LINK_PARAM[i];
				room->door_x[i+4] = LINK_DX[i];
				room->door_y[i+4] = LINK_DY[i];

				Data::Room* link = data->find_room(adjx[i],adjy[i],room->recto);
				if (link)
				{
					room->door[i+4] = link;
					output += wxT("Linked: ") + link->name + wxT("\n");
				}
				else
				{
					output += wxString::Format(wxT("No link: %d,%d\n"),adjx[i],adjy[i]);
				}
			}
		}

		room_panel->Refresh();
		RefreshRight();
	
		data->unsaved = true;
		room->unsaved = true;
		room->ship = false;
		room_ship->SetValue(room->ship);
	}

	wxMessageBox(output, wxT("Auto Link"), wxOK, this);
}

void EditRoom::OnUnlink(wxCommandEvent& event)
{
	if(wxYES != wxMessageBox(
		wxT("Are you sure you want to unlink this room?"),
		wxT("Unlink Room"),wxYES_NO | wxNO_DEFAULT, this))
	{
		return;
	}

	for (unsigned int i=0; i < data->get_item_count(Data::ITEM_ROOM); ++i)
	{
		Data::Room* r = (Data::Room*)data->get_item(Data::ITEM_ROOM,i);
		for (unsigned int d=0; d<8; ++d)
		{
			if (r->door[d] == room)
			{
				r->door[d] = NULL;
				data->unsaved = true;
				r->unsaved = true;
			}
		}
	}
	package->DeleteRoom(room); // cause a RefreshRight on all upon room editors
}

void EditRoom::OnPasswordCombo(wxCommandEvent& event)
{
	password_lizard = password_combo->GetSelection();
	if (password_lizard >= data->lizard_count()) password_lizard = 0;

	wxString password_text_init = data->password_build(data->find_index(room),password_lizard);
	password_text->SetValue(password_text_init);
}

void EditRoom::OnTest(wxCommandEvent& event)
{
	Data::Room* temp_door = NULL;
	unsigned char temp_door_param = 0;

	Data::Room* start_room = (Data::Room*)data->find_item(Data::ITEM_ROOM,wxT("start"));
	if (start_room)
	{
		if (start_room->dog[1].type != Game::DOG_DOOR)
		{
			wxMessageBox(wxT("No door in dog slot 1 in start room!"),wxT("Test error!"),wxOK | wxICON_ERROR, this);
			return;
		}

		temp_door = start_room->door[0];
		temp_door_param = start_room->dog[1].param;

		start_room->door[0] = room;
		start_room->dog[1].param = 0;
	}
	else
	{
		wxMessageBox(wxT("No room named start!"),wxT("Test error!"),wxOK | wxICON_ERROR, this);
		return;
	}

	package->DoTest(event.GetId() == ID_ROOM_TEST_NES, password_combo->GetSelection());

	start_room->door[0] = temp_door;
	start_room->dog[1].param = temp_door_param;
}

void EditRoom::OnStrip(wxCommandEvent& event)
{
	Data::Room* r = room;

	// chr 1 always text, chr 4 always lizard, pal 0 always text, pal 4/5 always lizard
	bool chr_used[8] = { false, true,  false, false, true,  false, false, false };
	bool pal_used[8] = { true,  false, false, false, true,  true,  false, false };

	// pal 4/5 always lizard
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
		const Data::Sprite* ds = data->get_dog_sprite(r->dog[d]);
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

	if (!bad)
	{
		wxMessageBox(wxT("Nothing to strip!"), wxT("Strip Room"), wxOK, this);
		return;
	}

	if (bad)
	{
		wxString s = wxT("");
		for (int c=0; c<8; ++c)
		{
			if ( chr_on[c] && !chr_used[c])
			{
				s += wxString::Format(wxT("Unused CHR %d: "),c) + r->chr[c]->name + wxT("\n");
				r->chr[c] = NULL;
			}
			if (!chr_on[c] &&  chr_used[c])
			{
				s += wxString::Format(wxT("Missing CHR %d\n"),c);
			}
		}
		for (int c=0; c<8; ++c)
		{
			if ( pal_on[c] && !pal_used[c])
			{
				s += wxString::Format(wxT("Unused PAL %d: "),c) + r->palette[c]->name + wxT("\n");
				r->palette[c] = NULL;
			}
			if (!pal_on[c] &&  pal_used[c])
			{
				s += wxString::Format(wxT("Missing PAL %d\n"),c);
			}
		}
		if (r->palette[4] == NULL)
		{
			// assign random lizard palette
			s += wxT("Missing lizard PAL 4\n");
			int lizard_pal = data->find_index(data->find_item(Data::ITEM_PALETTE,wxT("lizard0")));
			if (lizard_pal >= 0) lizard_pal += rand() % Game::LIZARD_OF_COUNT;
			r->palette[4] = (Data::Palette*)data->get_item(Data::ITEM_PALETTE,lizard_pal);
		}
		if (r->palette[5] == NULL)
		{
			s += wxT("Missing human PAL 5\n");
			int human_pal = data->find_index(data->find_item(Data::ITEM_PALETTE,wxT("human0")));
			if (human_pal >= 0) human_pal += rand() % 3;
			r->palette[5] = (Data::Palette*)data->get_item(Data::ITEM_PALETTE,human_pal);
		}

		wxMessageBox(s, wxT("Strip Room"), wxOK, this);

		data->unsaved = true;
		room->unsaved = true;
		room->ship = false;
		RefreshChrs();
		RefreshPalettes();
		RefreshRight();
		room_panel->Refresh();
		room_ship->SetValue(room->ship);
	}
}

void EditRoom::OnKeyDown(wxKeyEvent& event)
{
	if (ascii_tile)
	{
		if (event.GetUnicodeKey() == 192 || event.GetUnicodeKey() == 27) // ~ or Esc
		{
			// leave typing mode
			ascii_tile = false;
			room_panel->sel_x0 = 0;
			room_panel->sel_y0 = 0;
			room_panel->sel_x1 = 0;
			room_panel->sel_y1 = 0;
			ClearEditStatus();
			room_panel->Refresh();
			return;
		}

		if (event.GetUnicodeKey() == 8) // Backspace
		{
			--last_draw;
			if (last_draw < 0) last_draw = (64*30)-1;

			int tx = last_draw % 64;
			int ty = last_draw / 64;
			room_panel->sel_x0 = tx;
			room_panel->sel_y0 = ty;
			room_panel->sel_x1 = tx+1;
			room_panel->sel_y1 = ty+1;
			room_panel->Refresh();
			return;
		}

		if (event.GetModifiers() != 0)
		{
			// discard anything with modifier keys pressed, including modifier keys themselves
			return;
		}

		int t = event.GetKeyCode() & 0xFF;

		if      (t == 0x20) t = 0x70; // SPACE
		else if (t == 0x2C) t = 0x6B; // ,
		else if (t == 0x2D) t = 0x6E; // -
		else if (t == 0x2E) t = 0x5D; // .
		else if (t == 0x27) t = 0x5F; // '
		else if (t == 0x2F) t = 0x5C; // /
		else if (t == 0x3B) t = 0x5E; // :
		else if (t == 0x30) t = 0x60; // 0
		else if (t == 0x31) t = 0x61; // 1
		else if (t == 0x32) t = 0x62; // 2
		else if (t == 0x33) t = 0x63; // 3
		else if (t == 0x34) t = 0x64; // 4
		else if (t == 0x35) t = 0x65; // 5
		else if (t == 0x36) t = 0x66; // 6
		else if (t == 0x37) t = 0x67; // 7
		else if (t == 0x38) t = 0x68; // 8
		else if (t == 0x39) t = 0x69; // 9
		else if (t < 0x41 || t > 0x5A) return; // discard outside A-Z

		tile_select = t;

		PushUndo();
		room->nametable[last_draw] = tile_select;
		//room_panel->RefreshTile(last_draw % 64, last_draw / 64);

		++last_draw;
		if (last_draw >= (64*30)) last_draw = 0;

		int tx = last_draw % 64;
		int ty = last_draw / 64;
		room_panel->sel_x0 = tx;
		room_panel->sel_y0 = ty;
		room_panel->sel_x1 = tx+1;
		room_panel->sel_y1 = ty+1;
		room_panel->Refresh();

		RefreshTileSelect();
		return;
	}

	switch (event.GetUnicodeKey())
	{
	case 27: // Esc
	{
		room_panel->sel_x0 = 0;
		room_panel->sel_y0 = 0;
		room_panel->sel_x1 = 0;
		room_panel->sel_y1 = 0;
		ClearEditStatus();
		room_panel->Refresh();
	} break;
	case 219: // [
	{
		int inc = (event.GetModifiers() & wxMOD_SHIFT) ? 16 : 1;
		tile_select = (tile_select - inc) & 0xFF;
		RefreshTileSelect();
	} break;
	case 221: // ]
	{
		int inc = (event.GetModifiers() & wxMOD_SHIFT) ? 16 : 1;
		tile_select = (tile_select + inc) & 0xFF;
		RefreshTileSelect();
	} break;
	case wxT('A'):
	{
		room_panel->hide_tile = !room_panel->hide_tile;
		room_panel->Refresh();
		check_attrib->SetValue(room_panel->hide_tile);
	} break;
	case wxT('G'):
	{
		room_panel->grid = !room_panel->grid;
		room_panel->Refresh();
		check_grid->SetValue(room_panel->grid);
	} break;
	case wxT('S'):
	{
		room_panel->hide_sprite = !room_panel->hide_sprite;
		room_panel->Refresh();
		check_sprite->SetValue(!room_panel->hide_sprite);
	} break;
	case wxT('D'):
	{
		room_panel->doors = !room_panel->doors;
		room_panel->Refresh();
		check_doors->SetValue(room_panel->doors);
	} break;
	case wxT('C'):
	{
		room_panel->collide = !room_panel->collide;
		room_panel->Refresh();
		check_collide->SetValue(room_panel->collide);
	} break;
	case 188: // ,
	{
		room_panel->sel_ready = 1;
		edit_status->SetBackgroundColour(*wxGREEN);
		edit_status->SetLabel(wxT("Right click to set selection upper left."));
		edit_status->Refresh();
	} break;
	case 190: // .
	{
		room_panel->sel_ready = 2;
		edit_status->SetBackgroundColour(*wxGREEN);
		edit_status->SetLabel(wxT("Right click to set selection bottom right."));
		edit_status->Refresh();
	} break;
	case wxT('F'): // fill selection
	{
		PushUndo();

		const int x0 = room_panel->sel_x0;
		const int y0 = room_panel->sel_y0;
		const int x1 = room_panel->sel_x1;
		const int y1 = room_panel->sel_y1;
		for (int y = y0; y < y1; ++y)
		for (int x = x0; x < x1; ++x)
		{
			room->nametable[x+(y*64)] = tile_select;
		}
		room_panel->Refresh();
		room->unsaved = true;
		data->unsaved = true;
		room->ship = false;
		room_ship->SetValue(room->ship);
	} break;
	case wxT('Y'): // fill demo selection
	{
		if (multi_count < 2)
		{
			multi_count = 2;
			multi[0] = 0x80;
			multi[1] = 0x91;
		}

		PushUndo();
		const int x0 = room_panel->sel_x0;
		const int y0 = room_panel->sel_y0;
		const int x1 = room_panel->sel_x1;
		const int y1 = room_panel->sel_y1;
		for (int y = y0; y < y1; ++y)
		for (int x = x0; x < x1; ++x)
		{
			// checkerboard
			room->nametable[x+(y*64)] = multi[(x ^ y) & 1];
			room->attribute[(x/2)+((y/2)*32)] = 0;
		}
		room_panel->Refresh();
		room->unsaved = true;
		data->unsaved = true;
		room->ship = false;
		room_ship->SetValue(room->ship);
	}
	case wxT('K'): // copy selection
	{
		room_panel->sel_ready = 0;

		const int x0 = room_panel->sel_x0;
		const int y0 = room_panel->sel_y0;
		const int x1 = room_panel->sel_x1;
		const int y1 = room_panel->sel_y1;
		if (x1 > x0 && y1 > y0 && x0 >= 0 && y0 >= 0 && x1 <= 64 && y1 <= 30)
		{
			const unsigned int w = x1 - x0;
			const unsigned int h = y1 - y0;
			data->nmt_clip_w = w;
			data->nmt_clip_h = h;
			for (unsigned int y=0;y<h;++y)
			for (unsigned int x=0;x<w;++x)
			{
				data->nmt_clipboard[(y*w)+x] = room->nametable[(x+x0)+((y+y0)*64)];
			}
			wxASSERT((data->nmt_clip_w * data->nmt_clip_h) < 2048);
			edit_status->SetBackgroundColour(wxColour(0xFFFF00));
			edit_status->SetLabel(wxT("Selection copied to clipboard."));
			edit_status->Refresh();
		}
		else
		{
			edit_status->SetBackgroundColour(wxColour(0x8800FF));
			edit_status->SetLabel(wxT("No selection!"));
			edit_status->Refresh();
		}
		room_panel->Refresh();
	} break;
	case wxT('L'): // paste from clipboard
	{
		room_panel->sel_ready = 3;
		edit_status->SetBackgroundColour(*wxRED);
		edit_status->SetLabel(wxT("Right click to paste from clipboard."));
		edit_status->Refresh();
	} break;
	case wxT('Z'):
	{
		room_panel->sel_ready = 0;
		if (Undo())
		{
			edit_status->SetBackgroundColour(wxColour(0xFF8800));
			edit_status->SetLabel(wxString::Format(wxT("Undos remaining: %d"),undo_count));
			edit_status->Refresh();
			room_panel->Refresh();
		}
		else
		{
			edit_status->SetBackgroundColour(wxColour(0x8800FF));
			edit_status->SetLabel(wxT("Undo unavailable!"));
			edit_status->Refresh();
		}
	} break;
	case wxT('M'):
	{
		if (multi_count >= 8)
		{
			for (int i=0; i<7; ++i)
			{
				multi[i] = multi[i+1];
			}
			multi[7] = tile_select;
		}
		else
		{
			multi[multi_count] = tile_select;
			++multi_count;
		}

		wxString s = wxString::Format(wxT("Multi-fill[%d]:"),multi_count);
		for (int i=0; i<multi_count; ++i)
		{
			s += wxString::Format(wxT(" %02X"),multi[i]);
		}

		edit_status->SetBackgroundColour(wxColour(0x00AAFF));
		edit_status->SetLabel(s);
		edit_status->Refresh();
	} break;
	case wxT('N'):
	{
		multi_count = 0;
		edit_status->SetBackgroundColour(wxColour(0x00AAFF));
		edit_status->SetLabel(wxT("Multi-fill off."));
		edit_status->Refresh();
	} break;
	case wxT('E'):
	{
		room_panel->sel_ready = 4;
		edit_status->SetBackgroundColour(*wxRED);
		edit_status->SetLabel(wxT("Right click to same flood fill."));
		edit_status->Refresh();
	} break;
	case wxT('R'):
	{
		room_panel->sel_ready = 5;
		edit_status->SetBackgroundColour(wxColour(0xFF00FF));
		edit_status->SetLabel(wxT("Right click to boundary flood fill."));
		edit_status->Refresh();
	} break;
	case wxT('T'):
	{
		room_panel->sel_ready = 6;
		multi_count = 0;
		edit_status->SetBackgroundColour(*wxRED);
		edit_status->SetLabel(wxT("Right click to volcano flood fill."));
		edit_status->Refresh();
	} break;
	case wxT('B'):
	{
		room_panel->sel_ready = 7;
		edit_status->SetBackgroundColour(0xFF9999);
		edit_status->SetLabel(wxT("Right click to replace all of a tile."));
		edit_status->Refresh();
	} break;
	case 192:
	{
		ascii_tile = true;

		int tx = last_draw % 64;
		int ty = last_draw / 64;
		room_panel->sel_x0 = tx;
		room_panel->sel_y0 = ty;
		room_panel->sel_x1 = tx+1;
		room_panel->sel_y1 = ty+1;
		room_panel->Refresh();

		edit_status->SetBackgroundColour(*wxGREEN);
		edit_status->SetLabel(wxT("Typing mode active."));
		edit_status->Refresh();
	} break;
	default:
		event.Skip();
		return;
	}
}

void EditRoom::ClearEditStatus()
{
	room_panel->sel_ready = 0;
	edit_status->SetBackgroundColour(*wxLIGHT_GREY);
	edit_status->SetLabel(wxT(""));
	edit_status->Refresh();
}

//
// Undo
//

const int NMT_SIZE = 64 * 30;
const int ATT_SIZE = 32 * 15;

void EditRoom::PushUndo()
{
	// shift undo buffer back by one
	for (int i=(NMT_SIZE*(UNDO_LEVEL-1))-1; i >= 0; --i)
	{
		undo_nametable[i+NMT_SIZE] = undo_nametable[i];
	}
	for (int i=(ATT_SIZE*(UNDO_LEVEL-1))-1; i >= 0; --i)
	{
		undo_attribute[i+ATT_SIZE] = undo_attribute[i];
	}

	// fill the first entry of the undo buffer
	::memcpy(undo_nametable,room->nametable,NMT_SIZE);
	::memcpy(undo_attribute,room->attribute,ATT_SIZE);

	++undo_count;
	if (undo_count > UNDO_LEVEL)
	{
		undo_count = UNDO_LEVEL;
	}
}

bool EditRoom::Undo()
{
	if (undo_count <= 0)
	{
		return false;
	}

	// undo action
	::memcpy(room->nametable,undo_nametable,NMT_SIZE);
	::memcpy(room->attribute,undo_attribute,ATT_SIZE);

	// shift undo buffer forward by one
	for (int i=0; i<(NMT_SIZE*(UNDO_LEVEL-1))-1; ++i)
	{
		undo_nametable[i] = undo_nametable[i+NMT_SIZE];
	}
	for (int i=0; i<(ATT_SIZE*(UNDO_LEVEL-1))-1; ++i)
	{
		undo_attribute[i] = undo_attribute[i+ATT_SIZE];
	}

	--undo_count;
	return true;
}

void EditRoom::FloodFill(int x, int y, bool attrib, int mode, unsigned char fill_value, unsigned char fill_test)
{
	if (x < 0 || x >= 64) return;
	if (y < 0 || y >= 30) return;

	bool border = (mode == 1);

	if (attrib)
	{
		int ax = x / 2;
		int ay = y / 2;

		unsigned char v = room->attribute[(32*ay)+ax];
		if (border && v == fill_value) return;
		if (!border && v != fill_test) return;

		room->attribute[(32*ay)+ax] = fill_value;
		FloodFill(x-2,y,attrib,mode,fill_value,fill_test);
		FloodFill(x+2,y,attrib,mode,fill_value,fill_test);
		FloodFill(x,y-2,attrib,mode,fill_value,fill_test);
		FloodFill(x,y+2,attrib,mode,fill_value,fill_test);
	}
	else
	{
		unsigned char v = room->nametable[(64*y)+x];
		if (!border && v != fill_test) return;
		if (border && v == fill_value) return;

		unsigned char fv = fill_value;
		if (multi_count > 0)
		{
			for (int m=0; m<multi_count; ++m)
			{
				if (v == multi[m]) return;
			}
			// randomly select fv from multi
			for (int attempt=0; attempt<8; ++attempt)
			{
				fv = multi[rand() % multi_count];
				bool success = true;
				if      (x >  0 && room->nametable[(64*y)+(x-1)] == fv) success = false;
				else if (x < 63 && room->nametable[(64*y)+(x+1)] == fv) success = false;
				else if (y >  0 && room->nametable[(64*(y-1))+x] == fv) success = false;
				else if (y < 29 && room->nametable[(64*(y+1))+x] == fv) success = false;
				if (success) break;
				if (multi_count == 2 && attempt >= 3) break; // checkerboard pattern breaker
			}
		}
		else if (mode == 2) // volcano mode
		{
			int vpos = (((y&1)*2)+x)&3;

			multi_count = 0;
			fv                 = 0xDA + vpos;
			if (rand() & 1) fv = 0xEA + ((vpos+2)&3);
		}

		room->nametable[(64*y)+x] = fv;
		FloodFill(x-1,y,attrib,mode,fill_value,fill_test);
		FloodFill(x+1,y,attrib,mode,fill_value,fill_test);
		FloodFill(x,y-1,attrib,mode,fill_value,fill_test);
		FloodFill(x,y+1,attrib,mode,fill_value,fill_test);
	}
}

//
// EditChr external edits
//

void EditRoom::RefreshChr(Data::Chr* chr)
{
	bool refresh_room = false;
	for (int i=0; i<8; ++i)
	{
		if (room->chr[i] == chr)
		{
			room_chr[i]->Refresh();
			if (i<4 || !(room_panel->hide_sprite))
				refresh_room = true;
		}
	}
	if (refresh_room)
		room_panel->Refresh();
}

void EditRoom::RefreshPalette(Data::Palette* pal)
{
	bool refresh_room = false;
	for (int i=0; i<8; ++i)
	{
		if (room->palette[i] == pal)
		{
			for (int c=0; c<4; ++c)
			{
				room_pal[i]->SetColor(c,0,Data::nes_palette()[pal->data[c]]);
			}
			room_pal[i]->Refresh();
			if (i<4 || !(room_panel->hide_sprite))
				refresh_room = true;
		}
	}
	if (refresh_room)
		room_panel->Refresh();
}

void EditRoom::RefreshSprite(Data::Sprite* sprite)
{
	if (room_panel->hide_sprite) return; // nothing to update
	for (int i=0; i<16; ++i)
	{
		if (data->get_dog_sprite(room->dog[i]) == sprite)
		{
			room_panel->Refresh();
			return;
		}
	}
}

void EditRoom::DeleteChr(Data::Chr* chr)
{
	RefreshChrs();
	room_panel->Refresh();
}

void EditRoom::DeletePalette(Data::Palette* pal)
{
	RefreshPalettes();
	room_panel->Refresh();
}

void EditRoom::DeleteSprite(Data::Sprite* sprite)
{
	for (int i=0; i<16; ++i)
	{
		room->dog[i].cached_sprite = NULL;
	}

	room_panel->Refresh();
}

void EditRoom::DeleteRoom(Data::Room* room)
{
	RefreshRight();
}

// render_room

static RoomPanel* render_panel = NULL;

bool EditRoom::render_room(Data* data, Data::Room* room, int phase, wxDC& dc)
{
	if (render_panel == NULL)
	{
		Data::Chr* no_chr = static_cast<Data::Chr*>(data->find_item(Data::ITEM_CHR,wxT("title")));
		Data::Palette* no_palette = static_cast<Data::Palette*>(data->get_item(Data::ITEM_PALETTE,0));

		if (no_chr == NULL || no_palette == NULL) return false;

		render_panel = new RoomPanel(main_package, ID_ROOM_PANEL_DUMMY, data, room, no_chr, no_palette);
		render_panel->Hide();
	}

	render_panel->data = data;
	render_panel->room = room;

	if (phase == 0 || phase == 3)
	{
		render_panel->no_tile = false;
		render_panel->hide_sprite = true;
		render_panel->doors = false;
		render_panel->collide = false;
		render_panel->hide_diagnostic = true;
	}
	else if (phase == 1 || phase == 4)
	{
		render_panel->no_tile = true;
		render_panel->hide_sprite = false;
		render_panel->hide_utility = true;
		render_panel->doors = false;
		render_panel->collide = false;
	}
	else if (phase == 2 || phase == 5)
	{
		render_panel->no_tile = true;
		render_panel->hide_sprite = true;
		render_panel->doors = false;
		render_panel->collide = true;
		render_panel->collide_solid = true;
	}

	render_panel->PaintRegion(0,0,64,30,dc);
	return true;
}

//
// PickRoom
//

int PickRoom::last_select = -1;

PickRoom::PickRoom(wxWindow* parent, const Data* data) :
	wxDialog(parent, -1, wxT("Pick a room..."), wxDefaultPosition, wxSize(215,400), wxRESIZE_BORDER)
{
	SetMinSize(wxSize(215,200));

	wxBoxSizer* main_box = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* ok_box = new wxBoxSizer(wxHORIZONTAL);

	wxASSERT(data);
	const int room_count = (int)data->room.size();

	listbox = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(-1,300));
	for (int i=0; i < room_count; ++i)
	{
		listbox->Append(data->room[i]->name);
	}
	main_box->Add(listbox,1,wxALIGN_CENTER | wxEXPAND);

	picked = -1;
	if (last_select >= room_count) last_select = room_count - 1;
	if (last_select >= 0)
	{
		listbox->SetSelection(last_select);
		listbox->SetFirstItem(last_select);
	}

	wxButton* button_ok = new wxButton(this, wxID_OK, wxT("OK"), wxDefaultPosition, wxSize(50,-1));
	wxButton* button_none = new wxButton(this, wxID_ABORT, wxT("None"), wxDefaultPosition, wxSize(50,-1));
	wxButton* button_cancel = new wxButton(this, wxID_CLOSE, wxT("Cancel"), wxDefaultPosition, wxSize(50,-1));

	ok_box->Add(button_ok);
	ok_box->Add(button_none);
	ok_box->Add(button_cancel);
	main_box->Add(ok_box,0,wxALIGN_CENTER);

	SetSizer(main_box);

	Connect(wxID_OK,    wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PickRoom::OnOk));
	Connect(wxID_ABORT, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PickRoom::OnNone));
	Connect(wxID_CLOSE, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PickRoom::OnClose));
}

void PickRoom::OnOk(wxCommandEvent& event)
{
	picked = listbox->GetSelection();
	last_select = picked;
	Close();
}

void PickRoom::OnNone(wxCommandEvent& event)
{
	picked = -2;
	Close();
}

void PickRoom::OnClose(wxCommandEvent& event)
{
	picked = -1;
	Close();
}

void PickRoom::set_default(int d)
{
	last_select = d;
}


// end of file
