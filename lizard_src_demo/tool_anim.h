#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_package.h"

namespace LizardTool
{

class ColorGrid;
class RoomChrPanel;
class SpritePanel;

class AnimShow : public wxFrame
{
public:
	static const int FRAME_MAX = 16;

	AnimShow(Data* data_, Package* package_);

	Data* data;
	Package* package;
	Data::Chr* chr[4];
	Data::Palette* palette[4];
	Data::Chr no_chr;
	Data::Palette no_palette;
	static Data::Chr* default_chr[4];
	static Data::Palette* default_palette[4];

	int frame_count;
	int frame_time;
	int frame;
	Data::Sprite* sprite_cache[FRAME_MAX];

	int pal_select;
	SpritePanel* sprite_panel;
	RoomChrPanel* chr_panel[4];
	ColorGrid* grid_pal[4];
	wxListBox* list_chr;
	wxListBox* list_pal;
	wxRadioButton* sprite_radio[FRAME_MAX];
	wxTextCtrl* sprite_name[FRAME_MAX];
	wxTimer timer;

	void ClearSpriteCache();
	void RefreshFrame();

	// refresh stuff
	void RefreshLists();
	void PaletteSelect(int s);
	void RefreshAll();

	// events
	void OnChrRclick(wxMouseEvent& event);
	void OnPalClick(wxMouseEvent& event);
	void OnPalRclick(wxMouseEvent& event);
	void OnSpriteCount(wxCommandEvent& event);
	void OnSpriteTime(wxCommandEvent& event);
	void OnStart(wxCommandEvent& event);
	void OnStop(wxCommandEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnSpriteRadio(wxCommandEvent& event);
	void OnSpriteName(wxCommandEvent& event);
	void OnCheckGrid(wxCommandEvent& event);
	void OnListChrDclick(wxCommandEvent& event);
	void OnListPalDclick(wxCommandEvent& event);
	void OnAssume(wxCommandEvent& event);

	void OnClose(wxCloseEvent& event);
	void RefreshChr(Data::Chr* chr_);
	void RefreshPalette(Data::Palette* pal);
	void RefreshSprite(Data::Sprite* sprite);
	void DeleteChr(Data::Chr* chr_);
	void DeletePalette(Data::Palette* pal);
	void DeleteSprite(Data::Sprite* sprite);
};

} // namespace LizardTool
