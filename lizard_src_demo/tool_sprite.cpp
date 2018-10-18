// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_sprite.h"
#include "tool_palette.h"
#include "tool_chr.h"
#include "tool_room.h"
#include "wx/dcbuffer.h"

using namespace LizardTool;

const wxWindowID ID_SPRITE_PANEL       = 0x900;
const wxWindowID ID_SPRITE_COUNT       = 0x901;
const wxWindowID ID_SPRITE_STATUS      = 0x902;
const wxWindowID ID_SPRITE_ASSUME      = 0x903;
const wxWindowID ID_SPRITE_LIST_CHR    = 0x904;
const wxWindowID ID_SPRITE_LIST_PAL    = 0x905;
const wxWindowID ID_SPRITE_GRID        = 0x906;
const wxWindowID ID_SPRITE_VPAL        = 0x907;
const wxWindowID ID_SPRITE_BANK        = 0x908;
const wxWindowID ID_SPRITE_PAGE_UP     = 0x909;
const wxWindowID ID_SPRITE_PAGE_DOWN   = 0x90A;
const wxWindowID ID_SPRITE_FLIP        = 0x90B;
const wxWindowID ID_SPRITE_SHIFT_L     = 0x90C;
const wxWindowID ID_SPRITE_SHIFT_R     = 0x90D;
const wxWindowID ID_SPRITE_SHIFT_U     = 0x90E;
const wxWindowID ID_SPRITE_SHIFT_D     = 0x90F;
const wxWindowID ID_SPRITE_TILE_CHR    = 0x910;
const wxWindowID ID_SPRITE_TILE_PAL    = 0x920;
const wxWindowID ID_SPRITE_TILE        = 0x930;
const wxWindowID ID_SPRITE_ATTRIB      = 0x940;
const wxWindowID ID_SPRITE_X           = 0x950;
const wxWindowID ID_SPRITE_Y           = 0x960;
const wxWindowID ID_SPRITE_FLIP_X      = 0x970;
const wxWindowID ID_SPRITE_FLIP_Y      = 0x980;

//
// SpritePanel
//

SpritePanel::SpritePanel(wxWindow* parent, wxWindowID id, Data::Sprite* sprite_, Data::Chr* no_chr_, Data::Palette* no_palette_) :
	wxPanel(parent,id,wxDefaultPosition,wxSize(PIXELS*STRETCH,PIXELS*STRETCH)),
	sprite(sprite_),
	no_chr(no_chr_),
	no_palette(no_palette_),
	background(0),
	grid(false)
{
	wxASSERT(no_chr);
	wxASSERT(no_palette);

	for (int i=0; i<4; ++i)
	{
		chr[i] = NULL;
		pal[i] = NULL;
	}

	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	Connect(wxEVT_PAINT, wxPaintEventHandler(SpritePanel::OnPaint));
}

void SpritePanel::SetSprite(Data::Sprite* sprite_)
{
	sprite = sprite_;
}

void SpritePanel::OnPaint(wxPaintEvent& event)
{
	wxBufferedPaintDC dc(this);

	// assemble drawable pal and tile data

	Data::Chr* draw_chr[4];
	for (int i=0; i<4; ++i)
	{
		draw_chr[i] = chr[i];
		if (draw_chr[i] == NULL)
			draw_chr[i] = no_chr;
	}

	Data::Palette* draw_pal[4];
	for (int i=0; i<4; ++i)
	{
		draw_pal[i] = pal[i];
		if (draw_pal[i] == NULL)
			draw_pal[i] = no_palette;
	}

	wxPen draw_pen[4*4];
	wxBrush draw_brush[4*4];
	for (int p=0; p<4; ++p)
	for (int c=0; c<4; ++c)
	{
		const wxColour& color = Data::nes_palette()[draw_pal[p]->data[c]];
		draw_pen[  (4*p)+c] = wxPen(wxPen(color,1));
		draw_brush[(4*p)+c] = wxBrush(color);
	}

	// background
	dc.SetPen(draw_pen[background]);
	dc.SetBrush(draw_brush[background]);
	dc.DrawRectangle(0,0,PIXELS*STRETCH,PIXELS*STRETCH);
		
	if (grid)
	{
		dc.SetPen(*wxRED_PEN);
		dc.SetBrush(*wxRED_BRUSH);
		for (int i=0; i < (PIXELS/8); ++i)
		{
			dc.DrawLine(0,i*(STRETCH*8),PIXELS*STRETCH,i*(STRETCH*8));
			dc.DrawLine(i*(STRETCH*8),0,i*(STRETCH*8),PIXELS*STRETCH);
		}
	}

	if (sprite == NULL) return;

	for (int t=sprite->count-1; t>=0; --t)
	{
		int tile = sprite->tile[t];
		int attrib = sprite->palette[t] & 3;
		const unsigned char* tdata =
			draw_chr[((tile/64)&0x3)]->data[tile&63];

		const int dx = sprite->x[t] + (PIXELS/2);
		const int dy = sprite->y[t] + (PIXELS/2);

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
			dc.DrawRectangle((dx+tx)*STRETCH,(dy+ty)*STRETCH,STRETCH,STRETCH);
		}
	}
}

//
// EditSprite
//

EditSprite::EditSprite(Data::Item* item_, Data* data_, Package* package_) :
	Editor(item_, data_, package_)
{
	wxASSERT(item_->type == Data::ITEM_SPRITE);
	sprite = (Data::Sprite*)item_;

	wxSize min_size = wxSize(526,620);
	SetMinSize(min_size);
	SetMaxSize(min_size);
	SetSize(min_size);

	no_palette.name = wxT("(no palette)");

	// setup default chr with X pattern
	no_chr.name = wxT("(no chr)");
	for (int i=0; i<64; ++i)
	for (int x=0; x<8; ++x)
	{
		no_chr.data[i][   x +(x*8)] ^= 0x02;
		no_chr.data[i][(7-x)+(x*8)] ^= 0x02;
	}

	for (int i=0; i<4; ++i)
	{
		palette[i] = sprite->tool_palette[i];
		chr[i] = sprite->tool_chr[i];

		if (palette[i] == NULL) palette[i] = &no_palette;
		if (chr[i] == NULL) chr[i] = &no_chr;
	}

	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* top_left = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* top_right = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);
	sizer->Add(top_left);
	sizer->Add(top_right,0,wxLEFT,8);

	// left side

	sprite_panel = new SpritePanel(this,ID_SPRITE_PANEL,sprite,&no_chr,&no_palette);
	top_left->Add(sprite_panel,0,wxALL,8);

	wxStaticBoxSizer* box_chr = new wxStaticBoxSizer(wxVERTICAL,this,wxT("CHR"));
	wxBoxSizer* box_pal = new wxBoxSizer(wxHORIZONTAL);
	for (int i=0; i<4; ++i)
	{
		chr_panel[i] = new RoomChrPanel(this, ID_SPRITE_TILE_CHR+i, &no_chr);
		grid_pal[i] = new ColorGrid(this, ID_SPRITE_TILE_PAL+i, 4, 1);

		box_chr->Add(chr_panel[i]);
		box_pal->Add(grid_pal[i],0,wxTOP|wxLEFT,2);

		chr_panel[i]->Connect(ID_SPRITE_TILE_CHR+i, wxEVT_MOTION,     wxMouseEventHandler(EditSprite::OnChrMove),   NULL, this);
		chr_panel[i]->Connect(ID_SPRITE_TILE_CHR+i, wxEVT_RIGHT_DOWN, wxMouseEventHandler(EditSprite::OnChrRclick), NULL, this);
		grid_pal[i]->Connect(ID_SPRITE_TILE_PAL+i, wxEVT_LEFT_DOWN,  wxMouseEventHandler(EditSprite::OnPalClick),  NULL, this);
		grid_pal[i]->Connect(ID_SPRITE_TILE_PAL+i, wxEVT_RIGHT_DOWN, wxMouseEventHandler(EditSprite::OnPalRclick), NULL, this);
	}
	top_left->Add(box_chr);
	box_chr->Add(box_pal);

	// right side

	wxBoxSizer* count_box = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* count_label = new wxStaticText(this,wxID_ANY,wxT("Count:"));
	wxTextCtrl* count = new wxTextCtrl(this,ID_SPRITE_COUNT,wxString::Format(wxT("%d"),sprite->count),
		wxDefaultPosition,wxSize(32,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));
	wxCheckBox* vpal = new wxCheckBox(this, ID_SPRITE_VPAL, wxT("Vpal"));
	vpal->SetValue(sprite->vpal);
	wxStaticText* bank_label = new wxStaticText(this,wxID_ANY,wxT("Bank:"));
	wxTextCtrl* bank = new wxTextCtrl(this,ID_SPRITE_BANK,wxString::Format(wxT("%d"),sprite->bank),
		wxDefaultPosition,wxSize(32,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));

	top_right->Add(count_box,0,wxTOP|wxBOTTOM,8);
	count_box->Add(count_label);
	count_box->Add(count,0,wxLEFT,4);
	count_box->Add(vpal,0,wxLEFT,16);
	count_box->Add(bank_label,0,wxLEFT,16);
	count_box->Add(bank,0,wxLEFT,4);
	
	Connect(ID_SPRITE_COUNT, wxEVT_COMMAND_TEXT_UPDATED,     wxCommandEventHandler(EditSprite::OnSpriteCount));
	Connect(ID_SPRITE_VPAL,  wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(EditSprite::OnSpriteCheckVpal));
	Connect(ID_SPRITE_BANK,  wxEVT_COMMAND_TEXT_UPDATED,     wxCommandEventHandler(EditSprite::OnSpriteBank));

	wxBoxSizer* box_head = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* head_num = new wxStaticText(this,wxID_ANY,wxT("#"    ),wxDefaultPosition,wxSize(16, -1),wxALIGN_CENTER);
	wxStaticText* head_til = new wxStaticText(this,wxID_ANY,wxT("Tile" ),wxDefaultPosition,wxSize(32, -1),wxALIGN_CENTER);
	wxStaticText* head_att = new wxStaticText(this,wxID_ANY,wxT("Pal"  ),wxDefaultPosition,wxSize(24, -1),wxALIGN_CENTER);
	wxStaticText* head_x   = new wxStaticText(this,wxID_ANY,wxT("X"    ),wxDefaultPosition,wxSize(32, -1),wxALIGN_CENTER);
	wxStaticText* head_y   = new wxStaticText(this,wxID_ANY,wxT("Y"    ),wxDefaultPosition,wxSize(32, -1),wxALIGN_CENTER);
	wxStaticText* head_fx  = new wxStaticText(this,wxID_ANY,wxT("FX"   ),wxDefaultPosition,wxSize(32, -1),wxALIGN_CENTER);
	wxStaticText* head_fy  = new wxStaticText(this,wxID_ANY,wxT("FY"   ),wxDefaultPosition,wxSize(32, -1),wxALIGN_CENTER);

	top_right->Add(box_head);
	box_head->Add(head_num);
	box_head->Add(head_til);
	box_head->Add(head_att);
	box_head->Add(head_x);
	box_head->Add(head_y);
	box_head->Add(head_fx);
	box_head->Add(head_fy);

	for (int i=0; i<16; ++i)
	{
		wxBoxSizer* box_t = new wxBoxSizer(wxHORIZONTAL);
		wxStaticText* t_num = new wxStaticText(this,wxID_ANY,wxT(""),
			wxDefaultPosition,wxSize(16,-1),wxALIGN_CENTER);
		wxTextCtrl* t_til = new wxTextCtrl(this,ID_SPRITE_TILE+i,wxT(""),
			wxDefaultPosition,wxSize(32,-1),0,wxTextValidator(wxFILTER_ALPHANUMERIC,NULL));
		wxTextCtrl* t_att = new wxTextCtrl(this,ID_SPRITE_ATTRIB+i,wxT(""),
			wxDefaultPosition,wxSize(24,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));
		wxTextCtrl* t_x = new wxTextCtrl(this,ID_SPRITE_X+i,wxT(""),
			wxDefaultPosition,wxSize(32,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));
		wxTextCtrl* t_y = new wxTextCtrl(this,ID_SPRITE_Y+i,wxT(""),
			wxDefaultPosition,wxSize(32,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));
		wxButton* t_fx = new wxButton(this,ID_SPRITE_FLIP_X+i,wxT(""),
			wxDefaultPosition,wxSize(32,-1));
		wxButton* t_fy = new wxButton(this,ID_SPRITE_FLIP_Y+i,wxT(""),
			wxDefaultPosition,wxSize(32,-1));

		top_right->Add(box_t);
		box_t->Add(t_num);
		box_t->Add(t_til);
		box_t->Add(t_att);
		box_t->Add(t_x);
		box_t->Add(t_y);
		box_t->Add(t_fx);
		box_t->Add(t_fy);

		s_number[i] = t_num;
		s_tile[i] = t_til;
		s_attribute[i] = t_att;
		s_x[i] = t_x;
		s_y[i] = t_y;
		button_fx[i] = t_fx;
		button_fy[i] = t_fy;

		Connect(ID_SPRITE_TILE+i,   wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler(EditSprite::OnSpriteTile));
		Connect(ID_SPRITE_ATTRIB+i, wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler(EditSprite::OnSpriteAttrib));
		Connect(ID_SPRITE_X+i,      wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler(EditSprite::OnSpriteX));
		Connect(ID_SPRITE_Y+i,      wxEVT_COMMAND_TEXT_UPDATED,   wxCommandEventHandler(EditSprite::OnSpriteY));
		Connect(ID_SPRITE_FLIP_X+i, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditSprite::OnSpriteFlipX));
		Connect(ID_SPRITE_FLIP_Y+i, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditSprite::OnSpriteFlipY));
	}

	wxBoxSizer* mid_box = new wxBoxSizer(wxHORIZONTAL);
	top_right->Add(mid_box,0,wxTOP,12);

	wxCheckBox* grid = new wxCheckBox(this, ID_SPRITE_GRID, wxT("Grid"));
	mid_box->Add(grid);
	Connect(ID_SPRITE_GRID, wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(EditSprite::OnCheckGrid));

	wxButton* page_up   = new wxButton(this, ID_SPRITE_PAGE_UP,   wxT("-16"), wxDefaultPosition, wxSize(24,-1));
	wxButton* page_down = new wxButton(this, ID_SPRITE_PAGE_DOWN, wxT("+16"), wxDefaultPosition, wxSize(24,-1));
	wxButton* flip      = new wxButton(this, ID_SPRITE_FLIP,      wxT("Flip"),wxDefaultPosition, wxSize(32,-1));
	mid_box->Add(page_up,  0,wxLEFT,8);
	mid_box->Add(page_down,0,wxLEFT,4);
	mid_box->Add(flip,     0,wxLEFT,8);
	Connect(ID_SPRITE_PAGE_UP,   wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditSprite::OnPageUp));
	Connect(ID_SPRITE_PAGE_DOWN, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditSprite::OnPageDown));
	Connect(ID_SPRITE_FLIP,      wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditSprite::OnFlip));

	status = new wxStaticText(this,ID_SPRITE_STATUS,wxT(""));
	mid_box->Add(status,0,wxLEFT,8);

	wxBoxSizer* list_box = new wxBoxSizer(wxHORIZONTAL);
	wxStaticBoxSizer* list_chr_box = new wxStaticBoxSizer(wxVERTICAL,this,wxT("CHR"));
	wxStaticBoxSizer* list_pal_box = new wxStaticBoxSizer(wxVERTICAL,this,wxT("Palettes"));
	list_chr = new wxListBox(this,ID_SPRITE_LIST_CHR,wxDefaultPosition,wxSize(100,64));
	list_pal = new wxListBox(this,ID_SPRITE_LIST_PAL,wxDefaultPosition,wxSize(100,64));
	list_chr_box->Add(list_chr);
	list_pal_box->Add(list_pal);
	list_box->Add(list_chr_box);
	list_box->Add(list_pal_box);
	top_right->Add(list_box,0,wxTOP,8);
	Connect(ID_SPRITE_LIST_CHR, wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(EditSprite::OnListChrDclick));
	Connect(ID_SPRITE_LIST_PAL, wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(EditSprite::OnListPalDclick));

	wxBoxSizer* shift_box = new wxBoxSizer(wxHORIZONTAL);

	wxButton* assume = new wxButton(this, ID_SPRITE_ASSUME, wxT("Assume CHR/pal..."));
	shift_box->Add(assume,0,wxLEFT,3);
	Connect(ID_SPRITE_ASSUME, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditSprite::OnAssume));

	wxButton* shift_l = new wxButton(this, ID_SPRITE_SHIFT_L, wxT("L"), wxDefaultPosition, wxSize(20,-1));
	wxButton* shift_r = new wxButton(this, ID_SPRITE_SHIFT_R, wxT("R"), wxDefaultPosition, wxSize(20,-1));
	wxButton* shift_u = new wxButton(this, ID_SPRITE_SHIFT_U, wxT("U"), wxDefaultPosition, wxSize(20,-1));
	wxButton* shift_d = new wxButton(this, ID_SPRITE_SHIFT_D, wxT("D"), wxDefaultPosition, wxSize(20,-1));
	shift_box->Add(shift_l,0,wxLEFT,8);
	shift_box->Add(shift_r,0,wxLEFT,2);
	shift_box->Add(shift_u,0,wxLEFT,2);
	shift_box->Add(shift_d,0,wxLEFT,2);
	Connect(ID_SPRITE_SHIFT_L, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditSprite::OnShift));
	Connect(ID_SPRITE_SHIFT_R, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditSprite::OnShift));
	Connect(ID_SPRITE_SHIFT_U, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditSprite::OnShift));
	Connect(ID_SPRITE_SHIFT_D, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditSprite::OnShift));

	top_right->Add(shift_box,0,wxTOP,8);


	pal_select = -1;
	RefreshLists();
	PaletteSelect(0);

	page = 0;
	RefreshData();
}

void EditSprite::RefreshData()
{
	// SetLabel triggers editing events, so restore the "saved" flag after refresh
	bool old_saved0 = sprite->unsaved;
	bool old_saved1 = data->unsaved;

	for (int j=0; j<16; ++j)
	{
		int i = j + page;
		wxASSERT(i < Data::Sprite::MAX_SPRITE_TILES); // page has shifted too far!
		s_number[j]->SetLabel(wxString::Format(wxT("%d"),i));
		s_tile[j]->SetLabel(wxString::Format(wxT("%02X"),sprite->tile[i]));
		s_attribute[j]->SetLabel(wxString::Format(wxT("%d"),sprite->palette[i]));
		s_x[j]->SetLabel(wxString::Format(wxT("%d"),sprite->x[i]));
		s_y[j]->SetLabel(wxString::Format(wxT("%d"),sprite->y[i]));
		button_fx[j]->SetLabel(sprite->flip_x[i]?wxT("-X"):wxT("+X"));
		button_fy[j]->SetLabel(sprite->flip_y[i]?wxT("-Y"):wxT("+Y"));

		//s_number[j]->Refresh();
		//s_tile[j]->Refresh();
		//s_attribute[j]->Refresh();
		//s_x[j]->Refresh();
		//s_y[j]->Refresh();
		//button_fx[j]->Refresh();
		//button_fy[j]->Refresh();
	}

	sprite->unsaved = old_saved0;
	data->unsaved = old_saved1;
}

void EditSprite::RefreshLists()
{
	list_chr->Clear();
	list_pal->Clear();
	for (int i=0; i<4; ++i)
	{
		list_chr->Append(chr[i]->name);
		list_pal->Append(palette[i]->name);

		sprite_panel->chr[i] = chr[i];
		sprite_panel->pal[i] = palette[i];

		chr_panel[i]->SetChr(chr[i]);

		for (int c=0; c<4; ++c)
		{
			grid_pal[i]->SetColor(c,0,Data::nes_palette()[palette[i]->data[c]]);
		}
		grid_pal[i]->Refresh();
	}
}

void EditSprite::PaletteSelect(int s)
{
	int old_s = 0;
	old_s = pal_select;
	if (s == old_s) return; // nothing new
	pal_select = s;

	sprite_panel->background = s*4; // set background colour
	sprite_panel->Refresh();

	for (int i=0;i<4;++i)
	{
		if (i != s)
		{
			grid_pal[i]->highlight = false;
			grid_pal[i]->Refresh();
		}
	}
	grid_pal[s]->highlight = true;
	grid_pal[s]->Refresh();

	Data::Palette* pal = palette[s];
	for (int i=0; i<4; ++i)
	{
		for (int c=0; c<4; ++c)
		{
			chr_panel[i]->SetColor(c,Data::nes_palette()[pal->data[c]]);
		}
		chr_panel[i]->Refresh();
	}
}

void EditSprite::RefreshAllButData()
{
	int old_select = pal_select;
	pal_select = -1;
	RefreshLists();
	PaletteSelect(old_select);
}

//
// EditSprite Events
//

void EditSprite::OnChrMove(wxMouseEvent& event)
{
	int c = event.GetId() - ID_SPRITE_TILE_CHR;
	wxASSERT(c >= 0 && c < 4);

	int x = event.GetX();
	int y = event.GetY();
	int t,tx,ty;

	chr_panel[c]->GetStat(x,y,&t,&tx,&ty);

	int tile = t+(c*64);
	status->SetLabel(wxString::Format(wxT("Tile: %02Xh"),tile,tile));
	status->Refresh();
}

void EditSprite::OnChrRclick(wxMouseEvent& event)
{
	int c = event.GetId() - ID_SPRITE_TILE_CHR;
	wxASSERT(c >= 0 && c < 4);

	if (chr[c] == NULL || chr[c] == &no_chr)
		wxMessageBox(wxT("There is no CHR in that slot."),wxT("Can't do that!"),wxOK|wxICON_EXCLAMATION);
	else
		package->OpenEditor(chr[c]);

}

void EditSprite::OnPalClick(wxMouseEvent& event)
{
	int p = event.GetId() - ID_SPRITE_TILE_PAL;
	wxASSERT(p >= 0 && p < 4);

	PaletteSelect(p);

	event.Skip(); // required to propagate foucs
}

void EditSprite::OnPalRclick(wxMouseEvent& event)
{
	int p = event.GetId() - ID_SPRITE_TILE_PAL;
	wxASSERT(p >= 0 && p < 4);

	if (palette[p] == NULL || palette[p] == &no_palette)
		wxMessageBox(wxT("There is no palette in that slot."),wxT("Can't do that!"),wxOK|wxICON_EXCLAMATION);
	else
		package->OpenEditor(palette[p]);
}

void EditSprite::OnCheckGrid(wxCommandEvent& event)
{
	sprite_panel->grid = event.IsChecked();
	sprite_panel->Refresh();
}

void EditSprite::OnListChrDclick(wxCommandEvent& event)
{
	int i = event.GetSelection();
	if (i < 0 || i >= 4) return;

	PickChr* picker = new PickChr(this, data);
	picker->ShowModal();
	int result = picker->picked;
	picker->Destroy();

	if (result < -1 || result >= (int)data->get_item_count(Data::ITEM_CHR))
		chr[i] = NULL;
	else if (result == -1) // cancel
		return;
	else
		chr[i] = (Data::Chr*)data->get_item(Data::ITEM_CHR,result);

	sprite->tool_chr[i] = chr[i];
	sprite->unsaved = true;
	data->unsaved = true;
	if (chr[i] == NULL) chr[i] = &no_chr;

	RefreshAllButData();
}

void EditSprite::OnListPalDclick(wxCommandEvent& event)
{
	int i = event.GetSelection();
	if (i < 0 || i >= 4) return;

	PickPalette* picker = new PickPalette(this, data);
	picker->ShowModal();
	int result = picker->picked;
	picker->Destroy();

	if (result < -1 || result >= (int)data->get_item_count(Data::ITEM_PALETTE))
		palette[i] = NULL;
	else if (result == -1) // cancel
		return;
	else
		palette[i] = (Data::Palette*)data->get_item(Data::ITEM_PALETTE,result);

	sprite->tool_palette[i] = palette[i];
	sprite->unsaved = true;
	data->unsaved = true;
	if (palette[i] == NULL) palette[i] = &no_palette;

	RefreshAllButData();
}

void EditSprite::OnAssume(wxCommandEvent& event)
{
	PickRoom* picker = new PickRoom(this, data);
	picker->ShowModal();
	int result = picker->picked;
	picker->Destroy();
	if (result < 0 || result >= (int)data->get_item_count(Data::ITEM_ROOM))
		return;

	Data::Room* room = (Data::Room*)data->get_item(Data::ITEM_ROOM,result);
	for (int i=0; i<4; ++i)
	{
		palette[i] = room->palette[i+4];
		sprite->tool_palette[i] = palette[i];
		if (palette[i] == NULL) palette[i] = &no_palette;

		chr[i] = room->chr[i+4];
		sprite->tool_chr[i] = chr[i];
		if (chr[i] == NULL) chr[i] = &no_chr;
	}
	sprite->unsaved = true;
	data->unsaved = true;

	RefreshAllButData();
}

void EditSprite::OnSpriteCount(wxCommandEvent& event)
{
	long v;
	event.GetString().ToLong(&v);
	if (v >= 0 && v <= Data::Sprite::MAX_SPRITE_TILES)
		sprite->count = v;
	sprite_panel->Refresh();
	package->RefreshSprite(sprite);
	data->unsaved = true;
	sprite->unsaved = true;
}

void EditSprite::OnSpriteCheckVpal(wxCommandEvent& event)
{
	sprite->vpal = event.IsChecked();
	data->unsaved = true;
	sprite->unsaved = true;
}

void EditSprite::OnSpriteBank(wxCommandEvent& event)
{
	long v;
	event.GetString().ToLong(&v);
	if (v >= 0)
		sprite->bank = v;
	data->unsaved = true;
	sprite->unsaved = true;
}

void EditSprite::OnSpriteTile(wxCommandEvent& event)
{
	int bs = event.GetId() - ID_SPRITE_TILE;
	int s = bs + page;
	wxASSERT(bs >= 0 && bs < 16);
	wxASSERT(s >= 0 && s < Data::Sprite::MAX_SPRITE_TILES);
	long v;
	event.GetString().ToLong(&v,16);
	if (v >= 0 && v < 256)
		sprite->tile[s] = v;
	sprite_panel->Refresh();
	package->RefreshSprite(sprite);
	data->unsaved = true;
	sprite->unsaved = true;
}

void EditSprite::OnSpriteAttrib(wxCommandEvent& event)
{
	int bs = event.GetId() - ID_SPRITE_ATTRIB;
	int s = bs + page;
	wxASSERT(bs >= 0 && bs < 16);
	wxASSERT(s >= 0 && s < Data::Sprite::MAX_SPRITE_TILES);
	long v;
	event.GetString().ToLong(&v);
	sprite->palette[s] = v & 0x23;
	sprite_panel->Refresh();
	package->RefreshSprite(sprite);
	data->unsaved = true;
	sprite->unsaved = true;
}

void EditSprite::OnSpriteX(wxCommandEvent& event)
{
	int bs = event.GetId() - ID_SPRITE_X;
	int s = bs + page;
	wxASSERT(bs >= 0 && bs < 16);
	wxASSERT(s >= 0 && s < Data::Sprite::MAX_SPRITE_TILES);
	long v;
	event.GetString().ToLong(&v);
	sprite->x[s] = v;
	sprite_panel->Refresh();
	package->RefreshSprite(sprite);
	data->unsaved = true;
	sprite->unsaved = true;
}

void EditSprite::OnSpriteY(wxCommandEvent& event)
{
	int bs = event.GetId() - ID_SPRITE_Y;
	int s = bs + page;
	wxASSERT(bs >= 0 && bs < 16);
	wxASSERT(s >= 0 && s < Data::Sprite::MAX_SPRITE_TILES);
	long v;
	event.GetString().ToLong(&v);
	sprite->y[s] = v;
	sprite_panel->Refresh();
	package->RefreshSprite(sprite);
	data->unsaved = true;
	sprite->unsaved = true;
}

void EditSprite::OnSpriteFlipX(wxCommandEvent& event)
{
	int bs = event.GetId() - ID_SPRITE_FLIP_X;
	int s = bs + page;
	wxASSERT(bs >= 0 && bs < 16);
	wxASSERT(s >= 0 && s < Data::Sprite::MAX_SPRITE_TILES);
	sprite->flip_x[s] = !sprite->flip_x[s];
	button_fx[bs]->SetLabel(sprite->flip_x[s]?wxT("-X"):wxT("+X"));
	button_fx[bs]->Refresh();
	sprite_panel->Refresh();
	package->RefreshSprite(sprite);
	data->unsaved = true;
	sprite->unsaved = true;
}

void EditSprite::OnSpriteFlipY(wxCommandEvent& event)
{
	int bs = event.GetId() - ID_SPRITE_FLIP_Y;
	int s = bs + page;
	wxASSERT(bs >= 0 && bs < 16);
	wxASSERT(s >= 0 && s < Data::Sprite::MAX_SPRITE_TILES);
	sprite->flip_y[s] = !sprite->flip_y[s];
	button_fy[bs]->SetLabel(sprite->flip_y[s]?wxT("-Y"):wxT("+Y"));
	button_fy[bs]->Refresh();
	sprite_panel->Refresh();
	package->RefreshSprite(sprite);
	data->unsaved = true;
	sprite->unsaved = true;
}

void EditSprite::OnPageUp(wxCommandEvent &event)
{
	page -= 16;
	if (page < 0) page = 0;
	RefreshData();
}

void EditSprite::OnPageDown(wxCommandEvent &event)
{
	page += 16;
	if ((page + 16) >= Data::Sprite::MAX_SPRITE_TILES)
	{
		page = Data::Sprite::MAX_SPRITE_TILES - 16;
	}
	RefreshData();
}

void EditSprite::OnFlip(wxCommandEvent &event)
{
	for (int i=0; i < sprite->count; ++i)
	{
		sprite->x[i] = -8 - sprite->x[i];
		sprite->flip_x[i] = !sprite->flip_x[i];
	}
	RefreshData();
	RefreshAllButData();
	data->unsaved = true;
	sprite->unsaved = true;
}

void EditSprite::OnShift(wxCommandEvent &event)
{
	int dir = event.GetId() - ID_SPRITE_SHIFT_L;
	wxASSERT(dir >= 0 && dir <= 3);

	const int dx[4] = { -1, 1, 0, 0 };
	const int dy[4] = { 0, 0, -1, 1 };

	for (int i=0; i < sprite->count; ++i)
	{
		sprite->x[i] += dx[dir];
		sprite->y[i] += dy[dir];
	}
	RefreshData();
	RefreshAllButData();
	data->unsaved = true;
	sprite->unsaved = true;
}

//
// EditSprite events
//

void EditSprite::OnClose(wxCloseEvent& event)
{
	Editor::OnClose(event);
}

void EditSprite::RefreshChr(Data::Chr* chr_)
{
	bool refresh = false;
	for (int i=0; i<4; ++i)
	{
		if (chr_ == chr[i])
		{
			chr_panel[i]->Refresh();
			refresh = true;
		}
	}
	if (refresh)
		sprite_panel->Refresh();
}

void EditSprite::RefreshPalette(Data::Palette* pal)
{
	bool refresh = false;
	for (int i=0; i<4; ++i)
	{
		if (pal == palette[i])
		{
			refresh = true;
		}
	}
	if (refresh)
		RefreshAllButData();
}

void EditSprite::DeleteChr(Data::Chr* chr_)
{
	for (int i=0; i<4; ++i)
	{
		if (chr_ == chr[i])
			chr[i] = &no_chr;
		if (chr_ == sprite->tool_chr[i])
		{
			sprite->tool_chr[i] = NULL;
			sprite->unsaved = true;
			data->unsaved = true;
		}
	}
	RefreshAllButData();
}

void EditSprite::DeletePalette(Data::Palette* pal)
{
	for (int i=0; i<4; ++i)
	{
		if (pal == palette[i])
			palette[i] = &no_palette;
		if (pal == sprite->tool_palette[i])
		{
			sprite->tool_palette[i] = NULL;
			sprite->unsaved = true;
			data->unsaved = true;
		}
	}
	RefreshAllButData();
}

// end of file
