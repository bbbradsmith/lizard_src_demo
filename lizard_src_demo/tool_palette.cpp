// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_palette.h"
#include "wx/grid.h"
#include "wx/dcbuffer.h"

using namespace LizardTool;

const wxWindowID ID_PALETTE_GRID = 0x0600;
const wxWindowID ID_COLOR_GRID   = 0x0601;
const wxWindowID ID_STATUS       = 0x0602;

//
// ColorGrid
//

ColorGrid::ColorGrid(wxWindow* parent, wxWindowID id, int w, int h) :
	wxPanel(parent,id,wxDefaultPosition,wxSize(w*GRID_SIZE,h*GRID_SIZE))
{
	colors = new wxColour[w * h];
	grid_w = w;
	grid_h = h;
	selection = -1;
	highlight = false;
	drag_init = false;

	SetBackgroundStyle(wxBG_STYLE_CUSTOM);

	Connect(wxEVT_PAINT, wxPaintEventHandler(ColorGrid::OnPaint));
}

ColorGrid::~ColorGrid()
{
	delete [] colors;
}

void ColorGrid::SetColor(int x, int y, const wxColour& color)
{
	int coord = (y*grid_w) + x;
	wxASSERT(coord >= 0 && coord < (grid_w*grid_h));
	colors[coord] = color;
}

const wxColour& ColorGrid::GetColor(int x, int y) const
{
	int coord = (y*grid_w) + x;
	wxASSERT(coord >= 0 && coord < (grid_w*grid_h));
	return colors[coord];
}

void ColorGrid::SetSelection(int s)
{
	selection = s;
}

int ColorGrid::GetSelection() const
{
	return selection;
}

void ColorGrid::OnPaint(wxPaintEvent& event)
{
	wxBufferedPaintDC dc(this);

	int i = 0;
	for (int y=0; y<grid_h; ++y)
	{
		for (int x=0; x<grid_w; ++x)
		{
			const wxColour& color = colors[i];
			dc.SetPen(wxPen(color,1));
			dc.SetBrush(wxBrush(color));
			dc.DrawRectangle((x*GRID_SIZE),(y*GRID_SIZE),GRID_SIZE,GRID_SIZE);

			if (selection == i)
			{
				wxColour inv_color =
					wxColour(255-color.Red(), 255-color.Green(), 255-color.Blue());
				dc.SetPen(wxPen(inv_color,3));
				dc.SetBrush(*wxTRANSPARENT_BRUSH);
				dc.DrawRectangle((x*GRID_SIZE),(y*GRID_SIZE),GRID_SIZE,GRID_SIZE);
			}

			++i;
		}
	}

	if (highlight)
	{
		dc.SetPen(wxPen(*wxRED,3));
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.DrawRectangle(0,0,grid_w*GRID_SIZE,grid_h*GRID_SIZE);
	}
}

//
// EditPalette
//

EditPalette::EditPalette(Data::Item* item_, Data* data_, Package* package_) :
	Editor(item_, data_, package_)
{
	wxASSERT(item_->type == Data::ITEM_PALETTE);
	palette = (Data::Palette*)item_;

	wxSize min_size = wxSize(ColorGrid::GRID_SIZE * 19, ColorGrid::GRID_SIZE * 13);
	SetMinSize(min_size);
	SetMinSize(min_size);
	SetSize(min_size);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	pal_grid = new ColorGrid(this,ID_PALETTE_GRID,4,1);
	color_grid = new ColorGrid(this,ID_COLOR_GRID,16,4);
	status = new wxStaticText(this,ID_STATUS,wxT(""));

	sizer->Add(pal_grid,0,wxTOP|wxLEFT|wxRIGHT,ColorGrid::GRID_SIZE);
	sizer->Add(color_grid,0,wxALL,ColorGrid::GRID_SIZE);
	sizer->Add(status,1,wxEXPAND|wxLEFT,ColorGrid::GRID_SIZE);

	pal_grid->Connect(ID_PALETTE_GRID, wxEVT_MOTION, wxMouseEventHandler(EditPalette::PalGridMove), NULL, this);
	pal_grid->Connect(ID_PALETTE_GRID, wxEVT_LEFT_DOWN, wxMouseEventHandler(EditPalette::PalGridDown), NULL, this);

	color_grid->Connect(ID_COLOR_GRID, wxEVT_MOTION, wxMouseEventHandler(EditPalette::ColorGridMove), NULL, this);
	color_grid->Connect(ID_COLOR_GRID, wxEVT_LEFT_DOWN, wxMouseEventHandler(EditPalette::ColorGridDown), NULL, this);

	for (int i=0; i < (16*4); ++i)
	{
		int x = (i % 16);
		int y = (i / 16);
		color_grid->SetColor(x,y,Data::nes_palette()[i]);
	}
	color_grid->SetSelection(0);
	pal_grid->SetSelection(0);

	RebuildPalette();
}

void EditPalette::RebuildPalette()
{
	for (int i=0; i<4; ++i)
	{
		int ci = palette->data[i];
		const wxColour& c = color_grid->GetColor(ci%16,ci/16);
		pal_grid->SetColor(i,0,c);
	}
}

void EditPalette::PalGridMove(wxMouseEvent& event)
{
	int x = event.GetX() / ColorGrid::GRID_SIZE;
	if (x < 0 || x >= 4) return;

	wxString s;
	s.Printf(wxT("Palette %d = %02Xh"),x,palette->data[x]);
	status->SetLabel(s);
	status->Refresh();

	if (event.Dragging()) PalGridDown(event);
}

void EditPalette::PalGridDown(wxMouseEvent& event)
{
	int x = event.GetX() / ColorGrid::GRID_SIZE;
	if (x < 0 || x >= 4) return;

	pal_grid->SetSelection(x);
	pal_grid->Refresh();

	color_grid->SetSelection(palette->data[x]);
	color_grid->Refresh();

	event.Skip(); // required to propagate foucs
}

void EditPalette::ColorGridMove(wxMouseEvent& event)
{
	int x = event.GetX() / ColorGrid::GRID_SIZE;
	int y = event.GetY() / ColorGrid::GRID_SIZE;
	if (x < 0 || x >= 16 || y < 0 || y >= 4) return;
	int i = x + (y * 16);

	wxString s;
	s.Printf(wxT("Color %02Xh"),i);
	status->SetLabel(s);
	status->Refresh();

	if (color_grid->drag_init && event.Dragging()) ColorGridDown(event);
}

void EditPalette::ColorGridDown(wxMouseEvent& event)
{
	color_grid->drag_init = true;

	int x = event.GetX() / ColorGrid::GRID_SIZE;
	int y = event.GetY() / ColorGrid::GRID_SIZE;
	if (x < 0 || x >= color_grid->grid_w || y < 0 || y >= color_grid->grid_h)
	{
		event.Skip(); // required to propagate foucs
		return;
	}
	int i = x + (y * 16);
	int p = pal_grid->GetSelection();

	// forbidden colours redirected to black $0F
	if (i != 0x3D && i != 0x2D && (i & 0xF) >= 0xD)
	{
		i = 0x0F;
	}

	if (i == 0x0F) i |= (p << 4); // diagnostic colour

	color_grid->SetSelection(i);
	color_grid->Refresh();

	// optional: pal 0 forced to black $0F
	// if (p == 0) i = 0x0F;

	palette->data[p] = i;
	RebuildPalette();
	pal_grid->Refresh();

	package->RefreshPalette(palette);
	data->unsaved = true;
	palette->unsaved = true;

	event.Skip(); // required to propagate foucs
}

//
// PickPalette
//

int PickPalette::last_select = -1;

PickPalette::PickPalette(wxWindow* parent, const Data* data) :
	wxDialog(parent, -1, wxT("Pick a palette..."), wxDefaultPosition, wxSize(215,400), wxRESIZE_BORDER)
{
	SetMinSize(wxSize(215,200));

	wxBoxSizer* main_box = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* ok_box = new wxBoxSizer(wxHORIZONTAL);

	wxASSERT(data);
	const int palette_count = (int)data->palette.size();

	if (palette_count < 1)
	{
		wxStaticText* message = new wxStaticText(this, wxID_ANY, wxT("No palettes available."),
			wxDefaultPosition, wxSize(-1,300), wxALIGN_CENTRE);
		main_box->Add(message,1,wxEXPAND);
		last_select = -1;
		grid = NULL;
	}
	else
	{
		grid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(-1,300));
		grid->CreateGrid(palette_count,5);
		grid->SetSelectionMode(wxGrid::wxGridSelectCells); // so the selection doesn't override everything
		grid->SetColSize(0,16);
		grid->SetColSize(1,16);
		grid->SetColSize(2,16);
		grid->SetColSize(3,16);
		grid->SetColSize(4,128);

		grid->SetRowLabelSize(0);
		grid->SetColLabelSize(0);
		
		for (int i=0; i < palette_count; ++i)
		{
			grid->SetCellValue(i,4,data->palette[i]->name);
			grid->SetReadOnly(i,4,true);
			for (int c=0; c<4; ++c)
			{
				grid->SetReadOnly(i,c,true);
				grid->SetCellBackgroundColour(Data::nes_palette()[data->palette[i]->data[c]],i,c);
			}
		}

		main_box->Add(grid,1,wxALIGN_CENTER | wxEXPAND);

		picked = -1;
		if (last_select >= palette_count) last_select = palette_count - 1;
		if (last_select >= 0)
		{
			grid->SetGridCursor(last_select,4);
			grid->MakeCellVisible(last_select,4);
		}
	}

	wxButton* button_ok = new wxButton(this, wxID_OK, wxT("OK"), wxDefaultPosition, wxSize(50,-1));
	wxButton* button_none = new wxButton(this, wxID_ABORT, wxT("None"), wxDefaultPosition, wxSize(50,-1));
	wxButton* button_cancel = new wxButton(this, wxID_CLOSE, wxT("Cancel"), wxDefaultPosition, wxSize(50,-1));

	ok_box->Add(button_ok);
	ok_box->Add(button_none);
	ok_box->Add(button_cancel);
	main_box->Add(ok_box,0,wxALIGN_CENTER);

	SetSizer(main_box);

	Connect(wxID_OK,    wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PickPalette::OnOk));
	Connect(wxID_ABORT, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PickPalette::OnNone));
	Connect(wxID_CLOSE, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PickPalette::OnClose));
}

void PickPalette::OnOk(wxCommandEvent& event)
{
	if (grid)
	{
		int row = grid->GetGridCursorRow();
		picked = row;
	}
	else
	{
		picked = -1;
	}
	last_select = picked;
	Close();
}

void PickPalette::OnNone(wxCommandEvent& event)
{
	picked = -2;
	Close();
}

void PickPalette::OnClose(wxCommandEvent& event)
{
	picked = -1;
	Close();
}

// end of file
