#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_data.h"

namespace LizardTool
{

// forward declaration
class Package;
class MapPicker;
class AnimShow;

const int ANIM_MAX = 8; // maximum number of AnimShow instances

// Editor interface so Package can manage a list of open editors
class Editor : public wxFrame
{
public:
	Editor(Data::Item* item_, Data* data_, Package* package_);

	virtual void RefreshChr(Data::Chr* chr);
	virtual void RefreshPalette(Data::Palette* pal);
	virtual void RefreshSprite(Data::Sprite* sprite);
	virtual void DeleteChr(Data::Chr* chr);
	virtual void DeletePalette(Data::Palette* pal);
	virtual void DeleteSprite(Data::Sprite* sprite);
	virtual void DeleteRoom(Data::Room* room);
	virtual void OnClose(wxCloseEvent& event);

	Data::Item* item;
	Data* data;
	Package* package;
};

// main parent window for opening data editors

class Package : public wxFrame
{
public:
	Package(Data* data_, const wxChar* file_arg);

	void RefreshData();
	void OpenEditor(Data::Item* item);
	void EditorClosed(Editor* editor);
	void MapClosed(MapPicker* map);
	void AnimClosed(AnimShow* anim);

	// For editors to signal that their data is changed (to update other editors)
	void RefreshChr(Data::Chr* chr);
	void RefreshPalette(Data::Palette* pal);
	void RefreshSprite(Data::Sprite* sprite);
	void RefreshMap();

	// For editors to signal that data is removed
	void DeleteChr(Data::Chr* chr);
	void DeletePalette(Data::Palette* pal);
	void DeleteSprite(Data::Sprite* sprite);
	void DeleteRoom(Data::Room* room);

	// For initiating an export and test
	void DoTest(bool nes, unsigned char start_lizard);

	void OnClose(wxCloseEvent& event);

	void OnNew(wxCommandEvent& event);
	void OnOpen(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
	void OnExport(wxCommandEvent& event);
	void OnTest(wxCommandEvent& event);
	void OnQuit(wxCommandEvent& event);
	void OnMarkAllUnsaved(wxCommandEvent& event);
	void OnValidate(wxCommandEvent& event);
	void OnRenderMaps(wxCommandEvent& event);
	void OnRenderSprites(wxCommandEvent& event);
	void OnMap(wxCommandEvent& event);
	void OnAnim(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);

	void OnListEdit(wxCommandEvent& event);
	void OnListNew(wxCommandEvent& event);
	void OnListCopy(wxCommandEvent& event);
	void OnListUp(wxCommandEvent& event);
	void OnListDown(wxCommandEvent& event);
	void OnListRename(wxCommandEvent& event);
	void OnListDelete(wxCommandEvent& event);

protected:
	wxListBox* BuildList(wxStaticBoxSizer* sizer, int id_base);

	enum PackageWidgetEnum
	{
		BAR_EDIT    = 0,
		BAR_NEW     = 1,
		BAR_COPY    = 2,
		BAR_UP      = 3,
		BAR_DOWN    = 4,
		BAR_RENAME  = 5,
		BAR_DELETE  = 6,
		BAR_LIST    = 7,
		LIST_CHR     = 0x0200,
		LIST_PALETTE = 0x0300,
		LIST_SPRITE  = 0x0400,
		LIST_ROOM    = 0x0500,
	};

	Data* data;

	wxStaticText* package_file;
	wxListBox* list_room;
	wxListBox* list_chr;
	wxListBox* list_palette;
	wxListBox* list_sprite;

	std::vector<Editor*> open_editor;
	MapPicker* map_picker;
	AnimShow* anim_show[ANIM_MAX];
};

extern Package* main_package;

} // namespace LizardTool
