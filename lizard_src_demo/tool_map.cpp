// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_map.h"
#include "tool_room.h"
#include "wx/dcbuffer.h"
#include "wx/image.h"
#include "wx/rawbmp.h"
#include "enums.h"

using namespace LizardTool;

const wxWindowID ID_MAP_PANEL     = 0x0A00;
const wxWindowID ID_MAP_LOCATION  = 0x0A01;

bool show_hidden = false; // to show the hidden side of single screen rooms


static const int W = MAP_W;
static const int H = MAP_H;
static const int S = 10;

class LizardTool::MapPanel : public wxPanel
{
public:
	Data* data;
	bool recto;

	MapPanel(wxWindow* parent, wxWindowID id, Data* data_) :
		data(data_),
		recto(true),
		wxPanel(parent,id,wxDefaultPosition,wxSize(W*S,H*S))
	{
		SetBackgroundStyle(wxBG_STYLE_CUSTOM);
		Connect(wxEVT_PAINT, wxPaintEventHandler(MapPanel::OnPaint));
	}

	virtual void OnPaint(wxPaintEvent& event)
	{
		wxBufferedPaintDC dc(this);

		const wxColour COLOR_BACK(0x222222);
		const wxColour COLOR_GRID(0x777777);
		const wxColour COLOR_NEED(0x444444);
		const wxColour COLOR_DONE(0x33FFFF);
		const wxColour COLOR_DONT(0xFF00FF);
		const wxColour COLOR_SHIP(0x000000UL);

		const wxColour COLOR_ZONE[Game::TOOL_MUSIC_COUNT] = {
			wxColour(0xDDDDDD), // silent
			wxColour(0x999999), // ruins
			wxColour(0x339933), // maze
			wxColour(0x009955), // marsh
			wxColour(0x44FF33), // forest
			wxColour(0x00EEAA), // palace
			wxColour(0x0000BB), // lava
			wxColour(0xEE4444), // water
			wxColour(0xDD99FF), // lounge
			wxColour(0x005599), // roots
			wxColour(0xFFCCAA), // river
			wxColour(0xAAEEDD), // steam
			wxColour(0xFFAADD), // void
			wxColour(0xFFAA44), // mountain
			wxColour(0x0099DD), // volcano
			wxColour(0x9900DD), // sanctuary
			wxColour(0x88FFAA), // boss
			wxColour(0x115511), // ending
			wxColour(0xDDAAAA), // death
		};

		const wxPen PEN_BACK(COLOR_BACK,1);
		const wxPen PEN_GRID(COLOR_GRID,1);
		const wxPen PEN_NEED(COLOR_NEED,1);
		const wxPen PEN_DONE(COLOR_DONE,1);
		const wxPen PEN_DONT(COLOR_DONT,1);
		const wxPen PEN_SHIP(COLOR_SHIP,1);

		const wxPen PEN_ZONE[Game::TOOL_MUSIC_COUNT] = {
			wxPen(COLOR_ZONE[0],1),
			wxPen(COLOR_ZONE[1],1),
			wxPen(COLOR_ZONE[2],1),
			wxPen(COLOR_ZONE[3],1),
			wxPen(COLOR_ZONE[4],1),
			wxPen(COLOR_ZONE[5],1),
			wxPen(COLOR_ZONE[6],1),
			wxPen(COLOR_ZONE[7],1),
			wxPen(COLOR_ZONE[8],1),
			wxPen(COLOR_ZONE[9],1),
			wxPen(COLOR_ZONE[10],1),
			wxPen(COLOR_ZONE[11],1),
			wxPen(COLOR_ZONE[12],1),
			wxPen(COLOR_ZONE[13],1),
			wxPen(COLOR_ZONE[14],1),
			wxPen(COLOR_ZONE[15],1),
			wxPen(COLOR_ZONE[16],1),
			wxPen(COLOR_ZONE[17],1),
			wxPen(COLOR_ZONE[18],1),
		};

		const wxBrush BRUSH_BACK(COLOR_BACK);
		const wxBrush BRUSH_GRID(COLOR_GRID);
		const wxBrush BRUSH_NEED(COLOR_NEED);
		const wxBrush BRUSH_DONE(COLOR_DONE);
		const wxBrush BRUSH_DONT(COLOR_DONT);
		const wxBrush BRUSH_SHIP(COLOR_SHIP,wxCROSSDIAG_HATCH);

		const wxBrush BRUSH_ZONE[Game::TOOL_MUSIC_COUNT] = {
			wxBrush(COLOR_ZONE[0]),
			wxBrush(COLOR_ZONE[1]),
			wxBrush(COLOR_ZONE[2]),
			wxBrush(COLOR_ZONE[3]),
			wxBrush(COLOR_ZONE[4]),
			wxBrush(COLOR_ZONE[5]),
			wxBrush(COLOR_ZONE[6]),
			wxBrush(COLOR_ZONE[7]),
			wxBrush(COLOR_ZONE[8]),
			wxBrush(COLOR_ZONE[9]),
			wxBrush(COLOR_ZONE[10]),
			wxBrush(COLOR_ZONE[11]),
			wxBrush(COLOR_ZONE[12]),
			wxBrush(COLOR_ZONE[13]),
			wxBrush(COLOR_ZONE[14]),
			wxBrush(COLOR_ZONE[15]),
			wxBrush(COLOR_ZONE[16]),
			wxBrush(COLOR_ZONE[17]),
			wxBrush(COLOR_ZONE[18]),
		};

		// background
		dc.SetPen(PEN_BACK);
		dc.SetBrush(BRUSH_BACK);
		dc.DrawRectangle(0,0,W*S,H*S);

		// grid
		dc.SetPen(PEN_GRID);
		dc.SetBrush(BRUSH_GRID);
		for (int y=0;y<H;++y) dc.DrawLine(0,y*S,W*S,y*S);
		for (int x=0;x<W;++x) dc.DrawLine(x*S,0,x*S,H*S);

		// rooms
		for (unsigned int y=0; y<H; ++y)
		for (unsigned int x=0; x<W; ++x)
		{
			const Data::Room* r = data->find_room(x,y,recto);

			int xc = (x*S) + 1;
			int w = S - 1;

			if (r)
			{
				int m = r->music;
				wxASSERT(m < Game::TOOL_MUSIC_COUNT);
				dc.SetPen(PEN_ZONE[m]);
				dc.SetBrush(BRUSH_ZONE[m]);
				if (r->coord_x != x)
				{
					xc -= 1;
					w += 1;
				}
			}
			else continue;

			dc.DrawRectangle(xc,(y*S)+1,w,S-1);

			if (r && r->ship)
			{
				dc.SetPen(PEN_SHIP);
				dc.SetBrush(BRUSH_SHIP);
				//dc.DrawRectangle((x*S)+4,(y*S)+4,S-7,S-7);
				dc.DrawRectangle((x*S)+1,(y*S)+1,S-1,S-1);
			}
		}
	}

	wxString GetStat(int gx, int gy)
	{
		int x = gx / S;
		int y = gy / S;
		
		const Data::Room* r = data->find_room(x,y,recto);
		if (r) return r->name;
		return wxT("-");
	}
};


MapPicker::MapPicker(Data* data_, Package* package_) :
	data(data_),
	package(package_),
	wxFrame(package_, wxID_ANY, wxT("Lizard Map"), wxDefaultPosition, wxSize(30+(W*S),80+(H*S)), wxDEFAULT_FRAME_STYLE)
{
	wxASSERT(data);
	wxASSERT(package);
	Connect(wxID_ANY, wxEVT_CLOSE_WINDOW, wxCloseEventHandler(MapPicker::OnClose));
	SetIcon(package->GetIcon());
	SetLabel(wxT("Lizard Map"));

	wxSize min_size = wxSize(30+(W*S),80+(H*S));
	SetMinSize(min_size);
	SetMaxSize(min_size);
	SetSize(min_size);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	map_panel = new MapPanel(this, ID_MAP_PANEL,data);
	sizer->Add(map_panel,0,wxALL,8);

	location = new wxStaticText(this,ID_MAP_LOCATION,wxT("Recto"));
	sizer->Add(location,0,wxALL,8);

	map_panel->Connect(ID_MAP_PANEL, wxEVT_MOTION,     wxMouseEventHandler(MapPicker::MapMove), NULL, this);
	map_panel->Connect(ID_MAP_PANEL, wxEVT_LEFT_DOWN,  wxMouseEventHandler(MapPicker::MapDown), NULL, this);
	map_panel->Connect(ID_MAP_PANEL, wxEVT_RIGHT_DOWN, wxMouseEventHandler(MapPicker::MapDown), NULL, this);
}

void MapPicker::MapMove(wxMouseEvent& event)
{
	int gx = event.GetX();
	int gy = event.GetY();

	wxString stat = map_panel->GetStat(gx,gy);

	int x = gx / S;
	int y = gy / S;
	wxString label = wxString::Format(wxT("%d, %d"),x,y) + (map_panel->recto ? wxT(" R: ") : wxT(" V: ")) + stat;

	location->SetLabel(label);
	location->Refresh();
}

void MapPicker::MapDown(wxMouseEvent& event)
{
	if (event.RightIsDown())
	{
		if (!event.Dragging())
		{
			map_panel->recto = !map_panel->recto;
			map_panel->Refresh();
			MapMove(event); // update location
		}
		return;
	}

	if (!event.Dragging())
	{
		wxString stat = map_panel->GetStat(event.GetX(),event.GetY());
		Data::Item* room = data->find_item(Data::ITEM_ROOM,stat);
		if (room)
		{
			package->OpenEditor(room);
		}
	}

	event.Skip(); // required to propagate foucs
}

void MapPicker::OnClose(wxCloseEvent& event)
{
	package->MapClosed(this);
	Destroy();
}

bool MapPicker::render_map(Data* data, int phase, unsigned char render[MAP_RENDER_SIZE])
{
	wxBitmap screen(256*4,240*2,24);
	wxMemoryDC dc(screen);

	wxColour background(0x000000UL);

	// clear render
	if (phase == 0 || phase == 3)
	{
		::memset(render,0x00,MAP_RENDER_SIZE);
	}
	else
	{
		for (int i=0; i<MAP_RENDER_SIZE; i+=3)
		{
			render[i+0] = 0xFF;
			render[i+1] = 0x00;
			render[i+2] = 0xFF;
		}
		background = wxColour(0xFF00FFUL);
	}

	bool recto = phase < 3;

	for (int y=0; y<H; ++y)
	for (int x=0; x<W; ++x)
	{
		Data::Room* room = data->find_room(x,y,recto);
		if (room)
		{
			bool second = x != room->coord_x; // second half of scrolling room

			dc.SetBackground(background);
			dc.Clear();

			EditRoom::render_room(data,room,phase,dc);
			wxNativePixelData raw_bmp(screen);
			if (!raw_bmp) return false;

			wxNativePixelData::Iterator raw_pixels(raw_bmp);

			// blit while halving dimensions
			const int MAP_ROW = 257*MAP_W;
			unsigned char* row = render + (((MAP_ROW * (y*241)) + (x * 257)) * 3);

			for (int by=0; by<240; ++by)
			{
				raw_pixels.OffsetY(raw_bmp,1);

				wxNativePixelData::Iterator raw_row = raw_pixels;
				if (second || (!room->scrolling && show_hidden))
				{
					raw_row.OffsetX(raw_bmp,256*2);
				}

				unsigned char* render_write = row;
				for (int bx=0; bx<256; ++bx)
				{
					unsigned char r = raw_row.Red();
					unsigned char g = raw_row.Green();
					unsigned char b = raw_row.Blue();
					++raw_row;
					++raw_row;
					render_write[0] = r;
					render_write[1] = g;
					render_write[2] = b;
					render_write += 3;
				}
				row += (MAP_ROW * 3);

				raw_pixels.OffsetY(raw_bmp,1);
			}
		}
	}

	return true;
}

// end of file
