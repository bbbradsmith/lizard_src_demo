// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_anim.h"
#include "tool_sprite.h"
#include "tool_palette.h"
#include "tool_chr.h"
#include "tool_room.h"
#include "wx/dcbuffer.h"

using namespace LizardTool;

const wxWindowID ID_ANIM_PANEL       = 0xB00;
const wxWindowID ID_ANIM_COUNT       = 0xB01;
const wxWindowID ID_ANIM_ASSUME      = 0xB02;
const wxWindowID ID_ANIM_LIST_CHR    = 0xB03;
const wxWindowID ID_ANIM_LIST_PAL    = 0xB04;
const wxWindowID ID_ANIM_GRID        = 0xB05;
const wxWindowID ID_ANIM_MS          = 0xB06;
const wxWindowID ID_ANIM_START       = 0xB07;
const wxWindowID ID_ANIM_STOP        = 0xB08;
const wxWindowID ID_ANIM_TIMER       = 0xB09;
const wxWindowID ID_ANIM_TILE_CHR    = 0xB10;
const wxWindowID ID_ANIM_TILE_PAL    = 0xB20;
const wxWindowID ID_ANIM_RADIO       = 0xB30;
const wxWindowID ID_ANIM_SPRITE      = 0xB40;

const int TIMER_DEFAULT = 300;

//
// AnimShow
//

Data::Palette* AnimShow::default_palette[4] = { NULL, NULL, NULL, NULL };
Data::Chr* AnimShow::default_chr[4] = { NULL, NULL, NULL, NULL };

AnimShow::AnimShow(Data* data_, Package* package_) :
	data(data_),
	package(package_),
	timer(this,ID_ANIM_TIMER),
	wxFrame(package_,wxID_ANY,wxT("Animation"),wxDefaultPosition,wxSize(526,620), wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL)
{
	wxSize min_size = wxSize(526,620);
	SetMinSize(min_size);
	SetMaxSize(min_size);
	SetSize(min_size);

	SetIcon(package_->GetIcon());

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
		palette[i] = default_palette[i];
		chr[i] = default_chr[i];

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

	sprite_panel = new SpritePanel(this,ID_ANIM_PANEL,NULL,&no_chr,&no_palette);
	top_left->Add(sprite_panel,0,wxALL,8);

	wxStaticBoxSizer* box_chr = new wxStaticBoxSizer(wxVERTICAL,this,wxT("CHR"));
	wxBoxSizer* box_pal = new wxBoxSizer(wxHORIZONTAL);
	for (int i=0; i<4; ++i)
	{
		chr_panel[i] = new RoomChrPanel(this, ID_ANIM_TILE_CHR+i, &no_chr);
		grid_pal[i] = new ColorGrid(this, ID_ANIM_TILE_PAL+i, 4, 1);

		box_chr->Add(chr_panel[i]);
		box_pal->Add(grid_pal[i],0,wxTOP|wxLEFT,2);

		chr_panel[i]->Connect(ID_ANIM_TILE_CHR+i, wxEVT_RIGHT_DOWN, wxMouseEventHandler(AnimShow::OnChrRclick), NULL, this);
		grid_pal[i]->Connect(ID_ANIM_TILE_PAL+i, wxEVT_LEFT_DOWN,  wxMouseEventHandler(AnimShow::OnPalClick),  NULL, this);
		grid_pal[i]->Connect(ID_ANIM_TILE_PAL+i, wxEVT_RIGHT_DOWN, wxMouseEventHandler(AnimShow::OnPalRclick), NULL, this);
	}
	top_left->Add(box_chr);
	box_chr->Add(box_pal);

	// right side

	wxBoxSizer* count_box = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* count_label = new wxStaticText(this,wxID_ANY,wxT("Count:"));
	wxTextCtrl* count_text = new wxTextCtrl(this,ID_ANIM_COUNT,wxString::Format(wxT("%d"),0),
		wxDefaultPosition,wxSize(32,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));

	wxStaticText* timer_label = new wxStaticText(this,wxID_ANY,wxT("Milliseconds:"));
	wxTextCtrl* timer_text = new wxTextCtrl(this,ID_ANIM_MS,wxString::Format(wxT("%d"),TIMER_DEFAULT),
		wxDefaultPosition,wxSize(64,-1),0,wxTextValidator(wxFILTER_NUMERIC,NULL));

	count_box->Add(count_label);
	count_box->Add(count_text);
	count_box->Add(timer_label,0,wxLEFT,8);
	count_box->Add(timer_text);
	top_right->Add(count_box,0,wxTOP|wxBOTTOM,8);

	Connect(ID_ANIM_COUNT, wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(AnimShow::OnSpriteCount));
	Connect(ID_ANIM_MS,    wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(AnimShow::OnSpriteTime));

	wxBoxSizer* play_box = new wxBoxSizer(wxHORIZONTAL);
	wxButton* button_start = new wxButton(this,ID_ANIM_START,wxT("Start"));
	wxButton* button_stop  = new wxButton(this,ID_ANIM_STOP ,wxT("Stop"));
	play_box->Add(button_start);
	play_box->Add(button_stop,0,wxLEFT,8);
	top_right->Add(play_box,0,wxTOP|wxBOTTOM,8);
	Connect(ID_ANIM_START, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AnimShow::OnStart));
	Connect(ID_ANIM_STOP,  wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AnimShow::OnStop));

	Connect(ID_ANIM_TIMER, wxEVT_TIMER, wxTimerEventHandler(AnimShow::OnTimer));

	wxBoxSizer* box_head = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* head_num = new wxStaticText(this,wxID_ANY,wxT("#"),      wxDefaultPosition,wxSize(32, -1),wxALIGN_CENTER);
	wxStaticText* head_spr = new wxStaticText(this,wxID_ANY,wxT("Sprite" ),wxDefaultPosition,wxSize(128, -1),wxALIGN_CENTER);
	box_head->Add(head_num);
	box_head->Add(head_spr);
	top_right->Add(box_head);


	for (int i=0; i<FRAME_MAX; ++i)
	{
		wxBoxSizer* box_t = new wxBoxSizer(wxHORIZONTAL);

		long style = (i == 0) ? wxRB_GROUP : 0;
		wxRadioButton* t_radio = new wxRadioButton(this,ID_ANIM_RADIO+i,wxString::Format(wxT("%02d"),i),wxDefaultPosition,wxSize(32,-1),style);

		wxTextCtrl* t_sprite = new wxTextCtrl(this,ID_ANIM_SPRITE+i,wxT(""),wxDefaultPosition,wxSize(128,-1));

		box_t->Add(t_radio);
		box_t->Add(t_sprite);
		top_right->Add(box_t);

		sprite_radio[i] = t_radio;
		sprite_name[i] = t_sprite;

		Connect(ID_ANIM_RADIO+i,  wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler(AnimShow::OnSpriteRadio));
		Connect(ID_ANIM_SPRITE+i, wxEVT_COMMAND_TEXT_UPDATED,         wxCommandEventHandler(AnimShow::OnSpriteName));
	}

	wxCheckBox* grid = new wxCheckBox(this, ID_ANIM_GRID, wxT("Grid"));
	top_right->Add(grid,0,wxTOP,8);
	Connect(ID_ANIM_GRID, wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(AnimShow::OnCheckGrid));

	wxBoxSizer* list_box = new wxBoxSizer(wxHORIZONTAL);
	wxStaticBoxSizer* list_chr_box = new wxStaticBoxSizer(wxVERTICAL,this,wxT("CHR"));
	wxStaticBoxSizer* list_pal_box = new wxStaticBoxSizer(wxVERTICAL,this,wxT("Palettes"));
	list_chr = new wxListBox(this,ID_ANIM_LIST_CHR,wxDefaultPosition,wxSize(100,64));
	list_pal = new wxListBox(this,ID_ANIM_LIST_PAL,wxDefaultPosition,wxSize(100,64));
	list_chr_box->Add(list_chr);
	list_pal_box->Add(list_pal);
	list_box->Add(list_chr_box);
	list_box->Add(list_pal_box);
	top_right->Add(list_box,0,wxTOP,8);
	Connect(ID_ANIM_LIST_CHR, wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(AnimShow::OnListChrDclick));
	Connect(ID_ANIM_LIST_PAL, wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(AnimShow::OnListPalDclick));

	wxButton* assume = new wxButton(this, ID_ANIM_ASSUME, wxT("Assume CHR/palettes..."));
	top_right->Add(assume,0,wxLEFT|wxTOP,8);
	Connect(ID_ANIM_ASSUME, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(AnimShow::OnAssume));

	frame_time = TIMER_DEFAULT;
	frame_count = 0;
	frame = 0;
	ClearSpriteCache();

	pal_select = -1;
	RefreshLists();
	PaletteSelect(0);

	Connect(wxID_ANY, wxEVT_CLOSE_WINDOW, wxCloseEventHandler(AnimShow::OnClose));
}

void AnimShow::ClearSpriteCache()
{
	for (int i=0; i<FRAME_MAX; ++i)
	{
		sprite_cache[i] = NULL;
	}
}

void AnimShow::RefreshFrame()
{
	if (frame < 0) frame = 0;
	if (frame >= FRAME_MAX) frame = FRAME_MAX-1;

	sprite_radio[frame]->SetValue(true);

	if (sprite_cache[frame] == NULL)
	{
		Data::Sprite* s = (Data::Sprite*)data->find_item(Data::ITEM_SPRITE,sprite_name[frame]->GetValue());
		if (s == NULL) s = &(data->empty_sprite);
		sprite_cache[frame] = s;
	}

	sprite_panel->SetSprite(sprite_cache[frame]);
	sprite_panel->Refresh();
}

void AnimShow::RefreshLists()
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

void AnimShow::PaletteSelect(int s)
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

void AnimShow::RefreshAll()
{
	int old_select = pal_select;
	pal_select = -1;
	RefreshLists();
	PaletteSelect(old_select);
}

//
// AnimShow Events
//

void AnimShow::OnChrRclick(wxMouseEvent& event)
{
	int c = event.GetId() - ID_ANIM_TILE_CHR;
	wxASSERT(c >= 0 && c < 4);

	if (chr[c] == NULL || chr[c] == &no_chr)
		wxMessageBox(wxT("There is no CHR in that slot."),wxT("Can't do that!"),wxOK|wxICON_EXCLAMATION);
	else
		package->OpenEditor(chr[c]);

}

void AnimShow::OnPalClick(wxMouseEvent& event)
{
	int p = event.GetId() - ID_ANIM_TILE_PAL;
	wxASSERT(p >= 0 && p < 4);

	PaletteSelect(p);

	event.Skip(); // required to propagate foucs
}

void AnimShow::OnPalRclick(wxMouseEvent& event)
{
	int p = event.GetId() - ID_ANIM_TILE_PAL;
	wxASSERT(p >= 0 && p < 4);

	if (palette[p] == NULL || palette[p] == &no_palette)
		wxMessageBox(wxT("There is no palette in that slot."),wxT("Can't do that!"),wxOK|wxICON_EXCLAMATION);
	else
		package->OpenEditor(palette[p]);
}

void AnimShow::OnCheckGrid(wxCommandEvent& event)
{
	sprite_panel->grid = event.IsChecked();
	sprite_panel->Refresh();
}

void AnimShow::OnListChrDclick(wxCommandEvent& event)
{
	int i = event.GetSelection();
	if (i < 0 || i >= 4) return;

	PickChr* picker = new PickChr(this, data);
	picker->ShowModal();
	int result = picker->picked;
	picker->Destroy();

	if (result < 0 || result >= (int)data->get_item_count(Data::ITEM_CHR))
		return;

	chr[i] = (Data::Chr*)data->get_item(Data::ITEM_CHR,result);
	RefreshAll();
}

void AnimShow::OnListPalDclick(wxCommandEvent& event)
{
	int i = event.GetSelection();
	if (i < 0 || i >= 4) return;

	PickPalette* picker = new PickPalette(this, data);
	picker->ShowModal();
	int result = picker->picked;
	picker->Destroy();

	if (result < 0 || result >= (int)data->get_item_count(Data::ITEM_PALETTE))
		return;

	palette[i] = (Data::Palette*)data->get_item(Data::ITEM_PALETTE,result);
	RefreshAll();
}

void AnimShow::OnAssume(wxCommandEvent& event)
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
		if (palette[i] == NULL) palette[i] = &no_palette;

		chr[i] = room->chr[i+4];
		if (chr[i] == NULL) chr[i] = &no_chr;

		default_palette[i] = palette[i];
		default_chr[i] = chr[i];
	}
	RefreshAll();
}

void AnimShow::OnSpriteCount(wxCommandEvent& event)
{
	long v;
	event.GetString().ToLong(&v);
	if (v < 0) v = 0;
	if (v > FRAME_MAX) v = FRAME_MAX;

	frame_count = v;
}

void AnimShow::OnSpriteTime(wxCommandEvent& event)
{
	long v;
	event.GetString().ToLong(&v);
	if (v < 15) v = 0;

	frame_time = v;
	if (timer.IsRunning())
	{
		timer.Start(frame_time,false);
	}
}

void AnimShow::OnStart(wxCommandEvent& event)
{
	timer.Start(frame_time,false);
}

void AnimShow::OnStop(wxCommandEvent& event)
{
	timer.Stop();
}

void AnimShow::OnTimer(wxTimerEvent& event)
{
	++frame;
	if (frame >= frame_count) frame = 0;
	wxASSERT(frame < FRAME_MAX);
	RefreshFrame();
}

void AnimShow::OnSpriteRadio(wxCommandEvent& event)
{
	int s = event.GetId() - ID_ANIM_RADIO;
	wxASSERT(s >= 0 && s < FRAME_MAX);

	timer.Stop();

	frame = s;
	RefreshFrame();
}

void AnimShow::OnSpriteName(wxCommandEvent& event)
{
	int s = event.GetId() - ID_ANIM_SPRITE;
	wxASSERT(s >= 0 && s < FRAME_MAX);

	sprite_cache[s] = NULL;
	if (frame == s)
	{
		RefreshFrame();
	}
}

//
// AnimShow events
//

void AnimShow::OnClose(wxCloseEvent& event)
{
	// save last used
	for (int i=0; i<4; ++i)
	{
		default_chr[i] = chr[i];
		default_palette[i] = palette[i];
		if (default_chr[i] == &no_chr) default_chr[i] = NULL;
		if (default_palette[i] == &no_palette) default_palette[i] = NULL;
	}
	package->AnimClosed(this);
	Destroy();
}

void AnimShow::RefreshChr(Data::Chr* chr_)
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

void AnimShow::RefreshPalette(Data::Palette* pal)
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
		RefreshAll();
}

void AnimShow::RefreshSprite(Data::Sprite* sprite)
{
	if (sprite_cache[frame] == sprite)
	{
		RefreshFrame();
	}
}

void AnimShow::DeleteChr(Data::Chr* chr_)
{
	for (int i=0; i<4; ++i)
	{
		if (chr_ == chr[i])
			chr[i] = &no_chr;
	}
	RefreshAll();
}

void AnimShow::DeletePalette(Data::Palette* pal)
{
	for (int i=0; i<4; ++i)
	{
		if (pal == palette[i])
			palette[i] = &no_palette;
	}
	RefreshAll();
}

void AnimShow::DeleteSprite(Data::Sprite* sprite)
{
	ClearSpriteCache();
}

// end of file
