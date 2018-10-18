#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_package.h" // for Editor definition

namespace LizardTool
{

class ChrPanel;
class TilePanel;
class ColorGrid; // in tool_palette.h

class EditChr : public Editor
{
public:
	EditChr(Data::Item* item_, Data* data_, Package* package_);

	int tile;
	Data::Chr* chr;
	Data::Palette* palette[8];
	Data::Palette no_palette;
	static Data::Palette* default_palette[8];

	int pal_select;
	int pal_color;
	int chr_select;

	ChrPanel* chr_panel;
	TilePanel* tile_panel;
	wxStaticText* status_select;
	wxStaticText* status_tile;
	wxStaticText* status_tx;
	wxStaticText* status_ty;
	wxStaticText* status_tp;

	wxStaticText* name_pal[8];
	ColorGrid* grid_pal[8];

	void RebuildChr();
	void RebuildPalettes();
	void RedrawStats(int t, int x, int y, int v);

	void ChrMove(wxMouseEvent& event);
	void TileMove(wxMouseEvent& event);
	void PalMove(wxMouseEvent& event);

	void ChrDown(wxMouseEvent& event);
	void TileDown(wxMouseEvent& event);
	void TileRdown(wxMouseEvent& event);
	void PalDown(wxMouseEvent& event);

	void KeyDown(wxKeyEvent& event);

	void OnEditPal(wxCommandEvent& event);
	void OnPickPal(wxCommandEvent& event);
	void OnPickRoom(wxCommandEvent& event);
	void OnImageImport(wxCommandEvent& event);
	void OnImageExport(wxCommandEvent& event);

	virtual void RefreshPalette(Data::Palette* pal);
	virtual void DeletePalette(Data::Palette* pal);
	virtual void OnClose(wxCloseEvent& event);
};

class PickChr : public wxDialog
{
public:
	PickChr(wxWindow* parent, const Data* data);
	void OnOk(wxCommandEvent& event);
	void OnNone(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);

	int picked; // return value, -1 if cancel, -2 if none
	
	wxListBox* listbox;
	static int last_select;
};

} // namespace LizardTool
