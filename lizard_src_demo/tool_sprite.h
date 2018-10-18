#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_package.h" // for Editor definition

namespace LizardTool
{

class ColorGrid;
class RoomChrPanel;
class SpritePanel;

class LizardTool::SpritePanel : public wxPanel
{
public:
	Data::Sprite* sprite;
	Data::Chr* chr[4];
	Data::Palette* pal[4];
	Data::Chr* no_chr;
	Data::Palette* no_palette;
	int background;
	bool grid;

	static const int PIXELS = 64;
	static const int STRETCH = 4;

	SpritePanel(wxWindow* parent, wxWindowID id, Data::Sprite* sprite_, Data::Chr* no_chr_, Data::Palette* no_palette_);
	void SetSprite(Data::Sprite* sprite_);
	virtual void OnPaint(wxPaintEvent& event);
};

class EditSprite : public Editor
{
public:
	EditSprite(Data::Item* item_, Data* data_, Package* package_);

	Data::Sprite* sprite;
	Data::Chr* chr[4];
	Data::Palette* palette[4];
	Data::Chr no_chr;
	Data::Palette no_palette;

	int pal_select;
	int page;
	SpritePanel* sprite_panel;
	RoomChrPanel* chr_panel[4];
	ColorGrid* grid_pal[4];
	wxStaticText* status;

	wxStaticText* s_number[16];
	wxTextCtrl* s_tile[16];
	wxTextCtrl* s_attribute[16];
	wxTextCtrl* s_x[16];
	wxTextCtrl* s_y[16];
	wxButton* button_fx[16];
	wxButton* button_fy[16];

	wxListBox* list_chr;
	wxListBox* list_pal;

	// refresh stuff
	void RefreshData();
	void RefreshLists();
	void PaletteSelect(int s);
	void RefreshAllButData();

	// events
	void OnChrMove(wxMouseEvent& event);
	void OnChrRclick(wxMouseEvent& event);
	void OnPalClick(wxMouseEvent& event);
	void OnPalRclick(wxMouseEvent& event);
	void OnSpriteCount(wxCommandEvent& event);
	void OnSpriteCheckVpal(wxCommandEvent& event);
	void OnSpriteBank(wxCommandEvent& event);
	void OnSpriteTile(wxCommandEvent& event);
	void OnSpriteAttrib(wxCommandEvent& event);
	void OnSpriteX(wxCommandEvent& event);
	void OnSpriteY(wxCommandEvent& event);
	void OnSpriteFlipX(wxCommandEvent& event);
	void OnSpriteFlipY(wxCommandEvent& event);
	void OnCheckGrid(wxCommandEvent& event);
	void OnListChrDclick(wxCommandEvent& event);
	void OnListPalDclick(wxCommandEvent& event);
	void OnAssume(wxCommandEvent& event);
	void OnPageUp(wxCommandEvent& event);
	void OnPageDown(wxCommandEvent& event);
	void OnFlip(wxCommandEvent& event);
	void OnShift(wxCommandEvent& event);

	// virtual
	virtual void OnClose(wxCloseEvent& event);
	virtual void RefreshChr(Data::Chr* chr_);
	virtual void RefreshPalette(Data::Palette* pal);
	virtual void DeleteChr(Data::Chr* chr_);
	virtual void DeletePalette(Data::Palette* pal);
};

} // namespace LizardTool
