#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_package.h" // for Editor definition

namespace LizardTool
{

class RoomPanel;
class RoomChrPanel;
class ColorGrid;

class LizardTool::RoomChrPanel : public wxPanel
{
public:
	const Data::Chr* chr;
	wxColour col[4];
	int select;

	static const int STRETCH = 2;

	RoomChrPanel(wxWindow* parent, wxWindowID id, const Data::Chr* chr_);
	virtual void OnPaint(wxPaintEvent& event);
	void SetChr(Data::Chr* chr_);
	void SetColor(int i, wxColour c);
	bool SetSelect(int s);
	void GetStat(int gx, int gy, int* tile, int* tx, int* ty);
};

class EditRoom : public Editor
{
	static const int UNDO_LEVEL = 32;

public:
	EditRoom(Data::Item* item_, Data* data_, Package* package_);

	Data::Room* room;
	Data::Chr no_chr;
	Data::Palette no_palette;

	// left
	RoomPanel* room_panel;
	wxCheckBox* check_attrib;
	wxCheckBox* check_sprite;
	wxCheckBox* check_grid;
	wxCheckBox* check_doors;
	wxCheckBox* check_collide;
	wxStaticText* room_status;
	wxListBox* list_chr_back;
	wxListBox* list_chr_fore;
	wxListBox* list_pal_back;
	wxListBox* list_pal_fore;
	wxChoice* password_combo;
	wxTextCtrl* password_text;
	wxStaticText* edit_status;

	// mid
	RoomChrPanel* room_chr[8];
	ColorGrid* room_pal[8];

	// right
	wxTextCtrl* room_coord_x;
	wxTextCtrl* room_coord_y;
	wxCheckBox* room_recto;
	wxCheckBox* room_ship;
	wxChoice* room_music;
	wxTextCtrl* room_water;
	wxCheckBox* room_scrolling;
	wxButton* door_link[8];
	wxTextCtrl* door_x[8];
	wxTextCtrl* door_y[8];
	wxChoice* dog_sprite[16];
	wxTextCtrl* dog_x[16];
	wxTextCtrl* dog_y[16];
	wxTextCtrl* dog_param[16];
	wxTextCtrl* dog_note[16];

	// state
	int tile_select;
	int chr_pal_select;
	int spr_pal_select;
	static unsigned int password_lizard;

	int multi[8];
	int multi_count;
	bool ascii_tile;
	int last_draw;

	unsigned char undo_nametable[UNDO_LEVEL*64*30];
	unsigned char undo_attribute[UNDO_LEVEL*32*15];
	int undo_count;

	// referesh/redraw consolidation
	void RefreshTileSelect();
	void RefreshChrs();
	void RefreshPalettes();
	void RefreshRight();
	void PaletteSelect(bool sprite, int s); // refreshes CHR blocks on change

	// left events
	void OnRoomPanelMove(wxMouseEvent& event);
	void OnRoomPanelClick(wxMouseEvent& event);
	void OnRoomPanelRclick(wxMouseEvent& event);
	void OnCheckAttrib(wxCommandEvent& event);
	void OnCheckSprite(wxCommandEvent& event);
	void OnCheckGrid(wxCommandEvent& event);
	void OnCheckDoors(wxCommandEvent& event);
	void OnCheckCollide(wxCommandEvent& event);
	void OnListDclick(wxCommandEvent& event);

	// mid events
	void OnChrMove(wxMouseEvent& event);
	void OnChrClick(wxMouseEvent& event);
	void OnChrRclick(wxMouseEvent& event);
	void OnPalClick(wxMouseEvent& event);
	void OnPalRclick(wxMouseEvent& event);

	// right events
	void OnCoordX(wxCommandEvent& event);
	void OnCoordY(wxCommandEvent& event);
	void OnRecto(wxCommandEvent& event);
	void OnShip(wxCommandEvent& event);
	void OnMusic(wxCommandEvent& event);
	void OnWater(wxCommandEvent& event);
	void OnScrolling(wxCommandEvent& event);
	void OnDoorLink(wxCommandEvent& event);
	void OnDoorOpen(wxMouseEvent& event);
	void OnDoorPosition(wxCommandEvent& event);
	void OnDogSprite(wxCommandEvent& event);
	void OnDogPosition(wxCommandEvent& event);
	void OnDogParam(wxCommandEvent& event);
	void OnDogNote(wxCommandEvent& event);
	void OnAssume(wxCommandEvent& event);
	void OnAutoLink(wxCommandEvent& event);
	void OnUnlink(wxCommandEvent& event);
	void OnPasswordCombo(wxCommandEvent& event);
	void OnTest(wxCommandEvent& event);
	void OnStrip(wxCommandEvent& event);

	// other events
	void OnKeyDown(wxKeyEvent& event);
	void ClearEditStatus();
	void PushUndo();
	bool Undo();
	void FloodFill(int x, int y, bool attrib, int mode, unsigned char fill_value, unsigned char fill_test);

	// external edits
	virtual void RefreshChr(Data::Chr* chr);
	virtual void RefreshPalette(Data::Palette* pal);
	virtual void RefreshSprite(Data::Sprite* sprite);
	virtual void DeleteChr(Data::Chr* chr);
	virtual void DeletePalette(Data::Palette* pal);
	virtual void DeleteSprite(Data::Sprite* sprite);
	virtual void DeleteRoom(Data::Room* room);

	// rendering
	static bool render_room(Data* data, Data::Room* room, int phase, wxDC& dc);
};

class PickRoom : public wxDialog
{
public:
	PickRoom(wxWindow* parent, const Data* data);
	void OnOk(wxCommandEvent& event);
	void OnNone(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);

	static void set_default(int d);

	int picked; // return value, -1 if cancel, -2 if none
	
	wxListBox* listbox;
	static int last_select;
};

} // namespace LizardTool
