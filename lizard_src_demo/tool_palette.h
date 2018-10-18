#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_package.h" // for Editor definition

class wxGrid;

namespace LizardTool
{

// defined in tool_palette.cpp
class ColorGrid : public wxPanel
{
public:
	static const int GRID_SIZE = 16;
	wxColour* colors;
	int grid_w, grid_h;
	int selection;
	bool highlight;
	bool drag_init; // prevents destructive edit on focus change with mouse down

	ColorGrid(wxWindow* parent, wxWindowID id, int w, int h);
	~ColorGrid();
	void SetColor(int x, int y, const wxColour& color);
	const wxColour& GetColor(int x, int y) const;
	void SetSelection(int s);
	int GetSelection() const;
	virtual void OnPaint(wxPaintEvent& event);
};

class EditPalette : public Editor
{
public:
	EditPalette(Data::Item* item_, Data* data_, Package* package_);

	void RebuildPalette();

	void PalGridMove(wxMouseEvent& event);
	void PalGridDown(wxMouseEvent& event);

	void ColorGridMove(wxMouseEvent& event);
	void ColorGridDown(wxMouseEvent& event);

	ColorGrid* pal_grid;
	ColorGrid* color_grid;
	wxStaticText* status;

	Data::Palette* palette;
};

class PickPalette : public wxDialog
{
public:
	PickPalette(wxWindow* parent, const Data* data);
	void OnOk(wxCommandEvent& event);
	void OnNone(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);

	int picked; // return value, -1 if cancel, -2 if none
	
	wxGrid* grid;
	static int last_select;
};

} // namespace LizardTool
