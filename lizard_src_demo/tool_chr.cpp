// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_chr.h"
#include "tool_palette.h"
#include "tool_room.h"
#include "wx/dcbuffer.h"

using namespace LizardTool;

const wxWindowID ID_CHR_PANEL     = 0x0700;
const wxWindowID ID_TILE_PANEL    = 0x0701;
const wxWindowID ID_STATUS_SELECT = 0x0702;
const wxWindowID ID_STATUS_TILE   = 0x0703;
const wxWindowID ID_STATUS_TX     = 0x0704;
const wxWindowID ID_STATUS_TY     = 0x0705;
const wxWindowID ID_STATUS_TP     = 0x0706;
const wxWindowID ID_PICK_ROOM     = 0x0707;
const wxWindowID ID_IMG_IMPORT    = 0x0708;
const wxWindowID ID_IMG_EXPORT    = 0x0709;
const wxWindowID ID_NAME_PAL      = 0x0710;
const wxWindowID ID_GRID_PAL      = 0x0720;
const wxWindowID ID_PICK_PAL      = 0x0730;
const wxWindowID ID_EDIT_PAL      = 0x0740;

class LizardTool::ChrPanel : public wxPanel
{
public:
	Data::Chr* chr;
	wxColour col[4];
	int select;

	static const int STRETCH = 4;

	ChrPanel(wxWindow* parent, wxWindowID id, Data::Chr* chr_) :
		wxPanel(parent,id,wxDefaultPosition,wxSize(8*16*STRETCH,8*4*STRETCH)),
		chr(chr_),
		select(0)
	{
		SetBackgroundStyle(wxBG_STYLE_CUSTOM);
		Connect(wxEVT_PAINT, wxPaintEventHandler(ChrPanel::OnPaint));
	}

	virtual void OnPaint(wxPaintEvent& event)
	{
		wxBufferedPaintDC dc(this);

		for (int y=0; y<32; ++y)
		for (int x=0; x<128; ++x)
		{
			const int t = (x/8)+((y/8)*16);
			const int p = (x&7) + ((y&7)*8);
			const int c = chr->data[t][p];

			const wxColour& color = col[c];
			dc.SetPen(wxPen(color,1));
			dc.SetBrush(wxBrush(color));
			dc.DrawRectangle((x*STRETCH),(y*STRETCH),STRETCH,STRETCH);
		}

		int sx = select & 15;
		int sy = select / 16;
		dc.SetPen(*wxRED_PEN);
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.DrawRectangle(sx*8*STRETCH,sy*8*STRETCH,8*STRETCH,8*STRETCH);
	}

	void RefreshPixel(int t, int x, int y, int v)
	{
		wxClientDC dc(this);
		const wxColour& color = col[v];
		dc.SetPen(wxPen(color,1));
		dc.SetBrush(wxBrush(color));

		int px = x + ((t & 15) * 8);
		int py = y + ((t / 16) * 8);

		dc.DrawRectangle((px*STRETCH),(py*STRETCH),STRETCH,STRETCH);

		if (t == select)
		{
			int sx = select & 15;
			int sy = select / 16;
			dc.SetPen(*wxRED_PEN);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(sx*8*STRETCH,sy*8*STRETCH,8*STRETCH,8*STRETCH);
		}
	}

	void SetColor(int i, wxColour c) { col[i] = c; }
	void SetSelect(int s) { select = s; }

	void GetStat(int gx, int gy, int* tile, int* x, int* y, int* v)
	{
		gx /= STRETCH;
		gy /= STRETCH;
		int tx = gx / 8;
		int ty = gy / 8;
		*tile = (tx + (ty*16)) & 63;
		*x = gx - (8*tx);
		*y = gy - (8*ty);
		*v = chr->data[*tile][*x+(*y*8)];
	}
};

class LizardTool::TilePanel : public wxPanel
{
public:
	Data::Chr* chr;
	wxColour col[4];
	int select;

	bool drag_init; // prevents destructive edit on double click to edit

	static const int STRETCH = 8;

	TilePanel(wxWindow* parent, wxWindowID id, Data::Chr* chr_) :
		wxPanel(parent,id,wxDefaultPosition,wxSize(8*3*STRETCH,8*3*STRETCH)),
		chr(chr_),
		select(0),
		drag_init(false)
	{
		SetBackgroundStyle(wxBG_STYLE_CUSTOM);
		Connect(wxEVT_PAINT, wxPaintEventHandler(TilePanel::OnPaint));
	}

	virtual void OnPaint(wxPaintEvent& event)
	{
		wxBufferedPaintDC dc(this);

		// background grid
		dc.SetPen(wxPen(*wxLIGHT_GREY,1));
		dc.SetBrush(wxBrush(*wxLIGHT_GREY));
		dc.DrawRectangle(0,0,8*3*STRETCH,8*3*STRETCH);

		for (int ty=0;ty<3;++ty)
		for (int tx=0;tx<3;++tx)
		{
			int t = ((select + tx - 1) + ((ty - 1) * 16)) & 63;
			for (int y=0; y<8; ++y)
			for (int x=0; x<8; ++x)
			{
				const int p = x + (y*8);
				const int c = chr->data[t][p];

				const wxColour& color = col[c];
				dc.SetPen(wxPen(color,1));
				dc.SetBrush(wxBrush(color));
				dc.DrawRectangle(1+(x+tx*8)*STRETCH,1+(y+ty*8)*STRETCH,STRETCH-1,STRETCH-1);
			}

			if (tx == 1 && ty == 1)
			{
				dc.SetPen(*wxRED_PEN);
				dc.SetBrush(*wxTRANSPARENT_BRUSH);
				dc.DrawRectangle(8*STRETCH,8*STRETCH,(8*STRETCH)+1,(8*STRETCH)+1);
			}
		}
	}

	void RefreshPixel(int t, int x, int y, int v)
	{
		wxClientDC dc(this);
		const wxColour& color = col[v];
		dc.SetPen(wxPen(color,1));
		dc.SetBrush(wxBrush(color));

		int px = 8 + x;
		int py = 8 + y;

		dc.DrawRectangle((px*STRETCH)+1,(py*STRETCH)+1,STRETCH-1,STRETCH-1);
	}

	void SetColor(int i, wxColour c) { col[i] = c; }
	void SetSelect(int s) { select = s; }

	void GetStat(int gx, int gy, int* tile, int* x, int* y, int* v)
	{
		gx /= STRETCH;
		gy /= STRETCH;
		int tx = gx / 8;
		int ty = gy / 8;
		*tile = (select + (tx-1) + ((ty-1)*16)) & 63;
		*x = gx - (8*tx);
		*y = gy - (8*ty);
		*v = chr->data[*tile][*x+(*y*8)];
	}

	void SetPalette(const Data::Palette* pal)
	{
		for(int i=0;i<4;++i)
			SetColor(i,Data::nes_palette()[pal->data[i]]);
	}
};

//
// EditChr
//

Data::Palette* EditChr::default_palette[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

EditChr::EditChr(Data::Item* item_, Data* data_, Package* package_) :
	Editor(item_, data_, package_)
{
	wxASSERT(item_->type == Data::ITEM_CHR);
	chr = (Data::Chr*)item_;

	wxSize min_size = wxSize(512+32,590);
	SetMinSize(min_size);
	SetMaxSize(min_size);
	SetSize(min_size);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	chr_panel = new ChrPanel(this,ID_CHR_PANEL,chr);
	sizer->Add(chr_panel,0,wxALL,8);

	tile_panel    = new TilePanel(this,ID_TILE_PANEL,chr);
	status_select = new wxStaticText(this,ID_STATUS_TILE,wxT("Selected: "));
	status_tile   = new wxStaticText(this,ID_STATUS_TILE,wxT("Tile: "));
	status_tx     = new wxStaticText(this,ID_STATUS_TX  ,wxT("X: "));
	status_ty     = new wxStaticText(this,ID_STATUS_TY  ,wxT("Y: "));
	status_tp     = new wxStaticText(this,ID_STATUS_TP  ,wxT("Value: "));
	wxButton* pick_room = new wxButton(this,ID_PICK_ROOM,wxT("Assume palettes..."));
	wxButton* import_img = new wxButton(this,ID_IMG_IMPORT,wxT("Import"));
	wxButton* export_img = new wxButton(this,ID_IMG_EXPORT,wxT("Export"));

	wxBoxSizer* tile_status = new wxBoxSizer(wxVERTICAL);
	tile_status->Add(status_select,0,wxEXPAND);
	tile_status->Add(status_tile  ,0,wxEXPAND);
	tile_status->Add(status_tx    ,0,wxEXPAND);
	tile_status->Add(status_ty    ,0,wxEXPAND);
	tile_status->Add(status_tp    ,0,wxEXPAND);
	tile_status->Add(pick_room    ,0,wxEXPAND | wxTOP, 16);
	tile_status->Add(import_img   ,0,wxTOP,16);
	tile_status->Add(export_img   ,9,wxTOP,4);

	wxBoxSizer* tile_sizer = new wxBoxSizer(wxHORIZONTAL);
	tile_sizer->Add(tile_panel,0,0);
	tile_sizer->Add(tile_status,1,wxALL,8);
	sizer->Add(tile_sizer,0,wxALL,8);

	wxBoxSizer* pal_sizer = new wxBoxSizer(wxVERTICAL);
	for (int i=0; i<8; ++i)
	{
		wxButton* pick_pal = new wxButton(this, ID_PICK_PAL+i, wxT("..."));
		wxButton* edit_pal = new wxButton(this, ID_EDIT_PAL+i, wxT("Edit"));
		grid_pal[i] = new ColorGrid(this,ID_GRID_PAL+i,4,1);
		name_pal[i] = new wxStaticText(this,ID_NAME_PAL+i,wxT("---"),wxDefaultPosition,wxSize(128,20),wxST_NO_AUTORESIZE);

		wxBoxSizer* pal_line_sizer = new wxBoxSizer(wxHORIZONTAL);
		pal_line_sizer->Add(pick_pal,0,wxRIGHT,8);
		pal_line_sizer->Add(grid_pal[i],0,0);
		pal_line_sizer->Add(name_pal[i],1,wxLEFT, 8);
		pal_line_sizer->Add(edit_pal,0,wxLEFT,8);
		pal_sizer->Add(pal_line_sizer,1,wxEXPAND);

		name_pal[i]->SetBackgroundColour(wxColour(230,230,230));
		grid_pal[i]->SetSelection(-1);

		Connect(ID_EDIT_PAL+i, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditChr::OnEditPal));
		Connect(ID_PICK_PAL+i, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditChr::OnPickPal));
	}
	sizer->Add(pal_sizer,0,wxALL,8);

	no_palette.name = wxT("(no palette)");
	for (int i=0; i<8; ++i)
	{
		palette[i] = default_palette[i];
		if (palette[i] == NULL)
			palette[i] = &no_palette;
	}
	pal_select = 0;
	pal_color = 0;
	RebuildPalettes();

	chr_select = 0;
	RebuildChr();

	chr_panel->Connect(ID_CHR_PANEL, wxEVT_MOTION,    wxMouseEventHandler(EditChr::ChrMove), NULL, this);
	chr_panel->Connect(ID_CHR_PANEL, wxEVT_LEFT_DOWN, wxMouseEventHandler(EditChr::ChrDown), NULL, this);
	chr_panel->Connect(ID_CHR_PANEL, wxEVT_KEY_DOWN,  wxKeyEventHandler(EditChr::KeyDown),   NULL, this);

	tile_panel->Connect(ID_TILE_PANEL, wxEVT_MOTION,    wxMouseEventHandler(EditChr::TileMove), NULL, this);
	tile_panel->Connect(ID_TILE_PANEL, wxEVT_LEFT_DOWN, wxMouseEventHandler(EditChr::TileDown), NULL, this);
	tile_panel->Connect(ID_TILE_PANEL, wxEVT_RIGHT_DOWN,wxMouseEventHandler(EditChr::TileRdown),NULL, this);
	tile_panel->Connect(ID_TILE_PANEL, wxEVT_KEY_DOWN,  wxKeyEventHandler(EditChr::KeyDown),    NULL, this);

	for (int i=0; i<8; ++i)
	{
		grid_pal[i]->Connect(ID_GRID_PAL+i, wxEVT_MOTION,    wxMouseEventHandler(EditChr::PalMove), NULL, this);
		grid_pal[i]->Connect(ID_GRID_PAL+i, wxEVT_LEFT_DOWN, wxMouseEventHandler(EditChr::PalDown), NULL, this);
		grid_pal[i]->Connect(ID_GRID_PAL+i, wxEVT_KEY_DOWN,  wxKeyEventHandler(EditChr::KeyDown),   NULL, this);
	}

	Connect(ID_PICK_ROOM,  wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditChr::OnPickRoom));
	Connect(ID_IMG_IMPORT, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditChr::OnImageImport));
	Connect(ID_IMG_EXPORT, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EditChr::OnImageExport));
	Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(EditChr::KeyDown), NULL, this);
}

void EditChr::RebuildChr()
{
	for (int c=0; c<4; ++c)
	{
		chr_panel->SetColor(c,Data::nes_palette()[palette[pal_select]->data[c]]);
		tile_panel->SetColor(c,Data::nes_palette()[palette[pal_select]->data[c]]);

		chr_panel->SetSelect(chr_select);
		tile_panel->SetSelect(chr_select);
	}
}

void EditChr::RebuildPalettes()
{
	for (int p=0; p<8; ++p)
	{
		for (int i=0; i<4; ++i)
		{
			int ci = palette[p]->data[i];
			const wxColour& c = Data::nes_palette()[ci];
			grid_pal[p]->SetColor(i,0,c);
		}
		grid_pal[p]->SetSelection((p == pal_select) ? pal_color : -1);
		name_pal[p]->SetLabel(palette[p]->name);
	}
}

void EditChr::RedrawStats(int t, int x, int y, int v)
{
	wxString s;

	s.Printf(wxT("Selected: %02Xh"),chr_select);
	status_select->SetLabel(s);
	status_select->Refresh();

	s.Printf(wxT("Tile: %02Xh"),t);
	status_tile->SetLabel(s);
	status_tile->Refresh();

	s.Printf(wxT("X: %d"),x);
	status_tx->SetLabel(s);
	status_tx->Refresh();

	s.Printf(wxT("Y: %d"),y);
	status_ty->SetLabel(s);
	status_ty->Refresh();

	s.Printf(wxT("Value: %d"),v);
	status_tp->SetLabel(s);
	status_tp->Refresh();
}

void EditChr::ChrMove(wxMouseEvent& event)
{
	int t,x,y,v;
	chr_panel->GetStat(event.GetX(),event.GetY(),&t,&x,&y,&v);
	RedrawStats(t,x,y,v);

	if (event.Dragging()) ChrDown(event);
}

void EditChr::ChrDown(wxMouseEvent& event)
{
	int t,x,y,v;
	chr_panel->GetStat(event.GetX(),event.GetY(),&t,&x,&y,&v);

	chr_select = t;
	RebuildChr();
	chr_panel->Refresh();
	tile_panel->Refresh();
	RedrawStats(t,x,y,v);

	event.Skip(); // required to propagate foucs
}

void EditChr::TileMove(wxMouseEvent& event)
{
	if (tile_panel->drag_init && event.Dragging())
	{
		if (event.RightIsDown()) TileRdown(event);
		else                     TileDown(event);
		return;
	}

	int t,x,y,v;
	tile_panel->GetStat(event.GetX(),event.GetY(),&t,&x,&y,&v);
	RedrawStats(t,x,y,v);
}

void EditChr::TileDown(wxMouseEvent& event)
{
	tile_panel->drag_init = true;

	int t,x,y,v;
	tile_panel->GetStat(event.GetX(),event.GetY(),&t,&x,&y,&v);

	if (t == chr_select)
	{
		v = pal_color;
		chr->data[t][x+(y*8)] = v;
		//chr_panel->Refresh();
		//tile_panel->Refresh();
		chr_panel->RefreshPixel(t,x,y,v); // selective update more efficient
		tile_panel->RefreshPixel(t,x,y,v); // selective update more efficient
		RedrawStats(t,x,y,v);
		package->RefreshChr(chr);
		data->unsaved = true;
		chr->unsaved = true;
	}
	else if (event.LeftDown())
	{
		chr_select = t & 0x3F;
		chr_panel->SetSelect(chr_select);
		tile_panel->SetSelect(chr_select);
		chr_panel->Refresh();
		tile_panel->Refresh();
	}

	event.Skip(); // required to propagate foucs
}

void EditChr::TileRdown(wxMouseEvent& event)
{
	tile_panel->drag_init = true;

	int t,x,y,v;
	tile_panel->GetStat(event.GetX(),event.GetY(),&t,&x,&y,&v);

	if (v != pal_color)
	{
		pal_color = v;
		grid_pal[pal_select]->SetSelection(v);
		grid_pal[pal_select]->Refresh();
	}
}

void EditChr::PalMove(wxMouseEvent& event)
{
	if (event.Dragging()) PalDown(event);
}

void EditChr::PalDown(wxMouseEvent& event)
{
	int p = event.GetId() - ID_GRID_PAL;
	wxASSERT(p >= 0 && p < 8);

	int v = event.GetX() / ColorGrid::GRID_SIZE;

	if (p != pal_select) // full redraw
	{
		pal_select = p;
		pal_color = v;
		RebuildPalettes();
		RebuildChr();
		for (int i=0; i<8; ++i)
			grid_pal[i]->Refresh();
		chr_panel->Refresh();
		tile_panel->Refresh();
	}
	else if (v != pal_color) // only redraw current box
	{
		grid_pal[pal_select]->SetSelection(v);
		grid_pal[pal_select]->Refresh();
		pal_select = p;
		pal_color = v;
	}

	event.Skip(); // required to propagate foucs
}

void EditChr::KeyDown(wxKeyEvent& event)
{
	// hijack keys 1/2/3/4 for quick color change
	switch (event.GetUnicodeKey())
	{
	case wxT('1'): pal_color = 0; break;
	case wxT('2'): pal_color = 1; break;
	case wxT('3'): pal_color = 2; break;
	case wxT('4'): pal_color = 3; break;
	case wxT('C'):
	{
		::memcpy(data->chr_clipboard,chr->data[chr_select],8*8);
	} break;
	case wxT('V'):
	{
		::memcpy(chr->data[chr_select],data->chr_clipboard,8*8);
		chr_panel->Refresh();
		tile_panel->Refresh();
		package->RefreshChr(chr);
		chr->unsaved = true;
		data->unsaved = true;
	} break;
	case wxT('F'):
	{
		::memset(chr->data[chr_select],pal_color,8*8);
		chr_panel->Refresh();
		tile_panel->Refresh();
		package->RefreshChr(chr);
		chr->unsaved = true;
		data->unsaved = true;
	} break;
	case 219: // [
	{
		int inc = (event.GetModifiers() & wxMOD_SHIFT) ? 16 : 1;
		chr_select = (chr_select - inc) & 0x3F;
		chr_panel->SetSelect(chr_select);
		tile_panel->SetSelect(chr_select);
		chr_panel->Refresh();
		tile_panel->Refresh();
	} break;
	case 221: // ]
	{
		int inc = (event.GetModifiers() & wxMOD_SHIFT) ? 16 : 1;
		chr_select = (chr_select + inc) & 0x3F;
		chr_panel->SetSelect(chr_select);
		tile_panel->SetSelect(chr_select);
		chr_panel->Refresh();
		tile_panel->Refresh();
	} break;

	default:
		event.Skip();
		return;
	}
	grid_pal[pal_select]->SetSelection(pal_color);
	grid_pal[pal_select]->Refresh();
}

void EditChr::OnEditPal(wxCommandEvent& event)
{
	int p = event.GetId() - ID_EDIT_PAL;
	wxASSERT(p >= 0 && p < 8);

	Data::Palette* pp = palette[p];
	if (pp == &no_palette)
	{
		wxMessageBox(wxT("There is no palette in that slot."),wxT("Can't do that!"),wxOK|wxICON_EXCLAMATION);
	}
	else
	{
		package->OpenEditor(pp);
	}
}

void EditChr::OnPickPal(wxCommandEvent& event)
{
	int p = event.GetId() - ID_PICK_PAL;
	wxASSERT(p >= 0 && p < 8);

	PickPalette* picker = new PickPalette(this, data);
	picker->ShowModal();
	int result = picker->picked;
	picker->Destroy();

	if (result < 0 || result >= (int)data->get_item_count(Data::ITEM_PALETTE))
		return;

	palette[p] = (Data::Palette*)data->get_item(Data::ITEM_PALETTE,result);
	RebuildPalettes();
	RebuildChr();
	grid_pal[p]->Refresh();
	name_pal[p]->Refresh();
	chr_panel->Refresh();
	tile_panel->Refresh();
}

void EditChr::OnPickRoom(wxCommandEvent& event)
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
		palette[i] = r->palette[i];
		if (palette[i] == NULL) palette[i] = &no_palette;

		default_palette[i] = palette[i];
	}
	RebuildPalettes();
	RebuildChr();
	tile_panel->Refresh();
	chr_panel->Refresh();
	for (int i=0; i<8; ++i)
		grid_pal[i]->Refresh();
}

void EditChr::OnImageImport(wxCommandEvent& event)
{
	wxFileDialog * openFileDialog = new wxFileDialog(
		this, wxT("Import image"), wxT(""), wxT(""), wxT("*.bmp"),
		wxFD_OPEN);
	if (openFileDialog->ShowModal() == wxID_OK)
	{
		wxString filename = openFileDialog->GetPath();
		openFileDialog->Destroy();

		wxImage img;
		if (!img.LoadFile(filename))
		{
			wxMessageBox(wxT("Unable to import: ") + filename, wxT("File error!"), wxOK | wxICON_ERROR, this);
			return;
		}

		if (img.GetWidth() != (16*8) || img.GetHeight() != (4*8))
		{
			wxMessageBox(wxT("Image must be 128x32 in size."), wxT("Import error!"), wxOK | wxICON_ERROR, this);
			return;
		}

		// build palette

		int col_count = 0;
		wxColour cols[4];

		for (int y=0; y<(4*8); ++y)
		for (int x=0; x<(16*8); ++x)
		{
			unsigned char r = img.GetRed(x,y);
			unsigned char g = img.GetGreen(x,y);
			unsigned char b = img.GetBlue(x,y);
			wxColour col(r,g,b);

			int c = -1;
			for (int i=0;i<col_count;++i)
			{
				if (col == cols[i])
				{
					c = i;
					break;
				}
			}
			if (c == -1)
			{
				if (col_count >= 4)
				{
					wxMessageBox(wxT("Image must have no more than 4 colours."), wxT("Import error!"), wxOK | wxICON_ERROR, this);
					return;
				}
				cols[col_count] = col;
				++col_count;
			}
		}

		// find best match to current palette

		wxColour pout[4] = {
			Data::nes_palette()[palette[pal_select]->data[0]],
			Data::nes_palette()[palette[pal_select]->data[1]],
			Data::nes_palette()[palette[pal_select]->data[2]],
			Data::nes_palette()[palette[pal_select]->data[3]] };

		int pal_match[4] = { 0, 0, 0, 0 };
		for (int i=0;i<col_count;++i)
		{
			int pal_fit = 256*256*3;
			for (int j=0; j<4; ++j)
			{
				int fitr = cols[i].Red()   - pout[j].Red();
				int fitg = cols[i].Green() - pout[j].Green();
				int fitb = cols[i].Blue()  - pout[j].Blue();
				int fit = (fitr*fitr) + (fitg*fitg) + (fitb*fitb);
				if (fit < pal_fit)
				{
					pal_fit = fit;
					pal_match[i] = j;
				}
			}
		}

		// read

		for (int ty=0; ty<4; ++ty)
		for (int tx=0; tx<16; ++tx)
		{
			int t = tx + (ty*16);
			for (int y=0;y<8;++y)
			for (int x=0;x<8;++x)
			{
				int rx = (tx*8)+x;
				int ry = (ty*8)+y;
				unsigned char r = img.GetRed(rx,ry);
				unsigned char g = img.GetGreen(rx,ry);
				unsigned char b = img.GetBlue(rx,ry);
				wxColour col(r,g,b);
				int c = -1;
				for (int i=0;i<col_count;++i)
				{
					if (col == cols[i])
					{
						c = i;
						break;
					}
				}
				wxASSERT(c >= 0 && c < 4);
				chr->data[t][(y*8)+x] = pal_match[c&3];
			}
		}

		chr_panel->Refresh();
		tile_panel->Refresh();
		package->RefreshChr(chr);
		chr->unsaved = true;
		data->unsaved = true;
	}
	else
		openFileDialog->Destroy();
}

void EditChr::OnImageExport(wxCommandEvent& event)
{
	wxFileDialog * openFileDialog = new wxFileDialog(
		this, wxT("Export image"), wxT(""), wxT(""), wxT("*.bmp"),
		wxFD_SAVE | wxOVERWRITE_PROMPT);
	if (openFileDialog->ShowModal() == wxID_OK)
	{
		wxString filename = openFileDialog->GetPath();

		wxColour pout[4] = {
			Data::nes_palette()[palette[pal_select]->data[0]],
			Data::nes_palette()[palette[pal_select]->data[1]],
			Data::nes_palette()[palette[pal_select]->data[2]],
			Data::nes_palette()[palette[pal_select]->data[3]] };

		wxImage img(16*8,4*8,false);
		for (int ty=0; ty<4; ++ty)
		for (int tx=0; tx<16; ++tx)
		{
			int t = tx + (ty*16);
			for (int y=0;y<8;++y)
			for (int x=0;x<8;++x)
			{
				wxColour p = pout[chr->data[t][(y*8)+x]];
				img.SetRGB(x+(tx*8),y+(ty*8),p.Red(),p.Green(),p.Blue());
			}
		}

		if (!img.SaveFile(filename))
		{
			wxMessageBox(wxT("Unable to export: ") + filename,
				wxT("File error!"),
				wxOK | wxICON_ERROR, this);
		}
	}
	openFileDialog->Destroy();
}

void EditChr::RefreshPalette(Data::Palette* pal)
{
	bool refresh_pal = false;
	bool refresh_chr = false;
	for (int p=0; p<8; ++p)
	{
		if (palette[p] == pal)
		{
			refresh_pal = true;
			if (p == pal_select) refresh_chr = true;
		}
	}

	if (refresh_pal)
	{
		RebuildPalettes();
		for (int p=0; p<8; ++p)
			grid_pal[p]->Refresh();
	}
	if (refresh_chr)
	{
		RebuildChr();
		chr_panel->Refresh();
		tile_panel->Refresh();
	}
}

void EditChr::DeletePalette(Data::Palette* pal)
{
	bool refresh = false;
	for (int p=0; p<8; ++p)
	{
		if (palette[p] == pal)
		{
			refresh = true;
			palette[p] = &no_palette;
		}
	}

	if (refresh)
	{
		RebuildPalettes();
		RebuildChr();
		for (int p=0; p<8; ++p)
			grid_pal[p]->Refresh();
		chr_panel->Refresh();
		tile_panel->Refresh();
	}
}

void EditChr::OnClose(wxCloseEvent& event)
{
	// save last used palettes
	for (int i=0; i<8; ++i)
	{
		default_palette[i] = palette[i];
		if (default_palette[i] == &no_palette)
			default_palette[i] = NULL;
	}

	Editor::OnClose(event);
}

//
// PickChr
//

int PickChr::last_select = -1;

PickChr::PickChr(wxWindow* parent, const Data* data) :
	wxDialog(parent, -1, wxT("Pick a CHR..."), wxDefaultPosition, wxSize(215,400), wxRESIZE_BORDER)
{
	SetMinSize(wxSize(215,200));

	wxBoxSizer* main_box = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* ok_box = new wxBoxSizer(wxHORIZONTAL);

	wxASSERT(data);
	const int chr_count = (int)data->chr.size();

	listbox = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(-1,300));
	for (int i=0; i < chr_count; ++i)
	{
		listbox->Append(data->chr[i]->name);
	}
	main_box->Add(listbox,1,wxALIGN_CENTER | wxEXPAND);

	picked = -1;
	if (last_select >= chr_count) last_select = chr_count - 1;
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

	Connect(wxID_OK,    wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PickChr::OnOk));
	Connect(wxID_ABORT, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PickChr::OnNone));
	Connect(wxID_CLOSE, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PickChr::OnClose));
}

void PickChr::OnOk(wxCommandEvent& event)
{
	picked = listbox->GetSelection();
	last_select = picked;
	Close();
}

void PickChr::OnNone(wxCommandEvent& event)
{
	picked = -2;
	Close();
}

void PickChr::OnClose(wxCommandEvent& event)
{
	picked = -1;
	Close();
}

// end of file
