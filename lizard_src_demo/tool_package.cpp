// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_package.h"
#include "tool_data.h"
#include "lizard_version.h"
#include "tool_room.h"
#include "tool_chr.h"
#include "tool_palette.h"
#include "tool_sprite.h"
#include "tool_map.h"
#include "tool_anim.h"
#include "wx/filename.h"
#include "wx/textfile.h"

using namespace LizardTool;

Package* LizardTool::main_package = NULL;

//
// Editor
//

Editor::Editor(Data::Item* item_, Data* data_, Package* package_) :
	item(item_),
	data(data_),
	package(package_),
	wxFrame(package_, wxID_ANY, wxT("Lizard Tool Editor"), wxDefaultPosition, wxSize(200,300), wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL)
{
	wxASSERT(item);
	wxASSERT(data);
	wxASSERT(package);
	Connect(wxID_ANY, wxEVT_CLOSE_WINDOW, wxCloseEventHandler(Editor::OnClose));
	SetIcon(package->GetIcon());
	SetLabel(item->name);
}

void Editor::RefreshChr(Data::Chr* chr)
{
	// virtual
}

void Editor::RefreshPalette(Data::Palette* pal)
{
	// virtual
}

void Editor::RefreshSprite(Data::Sprite* sprite)
{
	// virtual
}

void Editor::DeleteChr(Data::Chr* chr)
{
	// virtual
}

void Editor::DeletePalette(Data::Palette* pal)
{
	// virtual
}

void Editor::DeleteSprite(Data::Sprite* sprite)
{
	// virtual
}

void Editor::DeleteRoom(Data::Room* room)
{
	// virtual
}

void Editor::OnClose(wxCloseEvent& event)
{
	//wxLogDebug(wxT("Editor::OnClose %s\n"),item->name);
	package->EditorClosed(this);
	Destroy();
}

//
// Package
//

#ifndef _DEBUG
	static const wxChar* WINDOW_TITLE = wxT("Lizard Tool");
#else
	static const wxChar* WINDOW_TITLE = wxT("Lizard Tool Debug");
#endif


Package::Package(Data* data_, const wxChar* file_arg) :
	data(data_),
	wxFrame(NULL, wxID_ANY, WINDOW_TITLE, wxDefaultPosition, wxSize(800,600), wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL)
{
	wxASSERT(data != NULL);

	SetIcon(wxIcon(wxT("IDI_MAIN_ICON"), wxBITMAP_TYPE_ICO_RESOURCE));
	SetMinSize(wxSize(300,300));

	// handle close box
	Connect(wxID_ANY, wxEVT_CLOSE_WINDOW, wxCloseEventHandler(Package::OnClose));

	// menu bar

	wxMenu* menufile = new wxMenu();
	menufile->Append(wxID_NEW,           wxT("&New"));
	menufile->Append(wxID_OPEN,          wxT("&Open"));
	menufile->Append(wxID_SAVE,          wxT("&Save"));
	menufile->Append(wxID_REDO,          wxT("&Export"));
	menufile->Append(wxID_PREVIEW,       wxT("Test &PC"));
	menufile->Append(wxID_PRINT,         wxT("Test &NES"));
	menufile->Append(wxID_EXIT,          wxT("&Quit"));
	wxMenu* menuact = new wxMenu();
	menuact->Append(wxID_CLEAR,          wxT("Mark All &Unsaved"));
	menuact->Append(wxID_VIEW_SORTDATE,  wxT("&Validate"));
	menuact->Append(wxID_VIEW_SMALLICONS,wxT("&Render Maps"));
	menuact->Append(wxID_VIEW_LARGEICONS,wxT("Render Maps w/&Underlay"));
	menuact->Append(wxID_VIEW_SORTSIZE,  wxT("Render &Sprites"));
	wxMenu* menuhelp = new wxMenu();
	menuhelp->Append(wxID_HELP_CONTENTS, wxT("&Map"));
	menuhelp->Append(wxID_HELP_CONTEXT,  wxT("&Animation"));
	menuhelp->Append(wxID_ABOUT,         wxT("A&bout"));
	wxMenuBar* menubar = new wxMenuBar();
	menubar->Append(menufile,            wxT("&File"));
	menubar->Append(menuact,             wxT("&Action"));
	menubar->Append(menuhelp,            wxT("&Help"));
	SetMenuBar(menubar);

	Connect(wxID_NEW,            wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnNew));
	Connect(wxID_OPEN,           wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnOpen));
	Connect(wxID_SAVE,           wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnSave));
	Connect(wxID_REDO,           wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnExport));
	Connect(wxID_PREVIEW,        wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnTest));
	Connect(wxID_PRINT,          wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnTest));
	Connect(wxID_EXIT,           wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnQuit));
	Connect(wxID_CLEAR,          wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnMarkAllUnsaved));
	Connect(wxID_VIEW_SORTDATE,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnValidate));
	Connect(wxID_VIEW_SMALLICONS,wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnRenderMaps));
	Connect(wxID_VIEW_LARGEICONS,wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnRenderMaps));
	Connect(wxID_VIEW_SORTSIZE,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnRenderSprites));
	Connect(wxID_HELP_CONTENTS,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnMap));
	Connect(wxID_HELP_CONTEXT,   wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnAnim));
	Connect(wxID_ABOUT,          wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Package::OnAbout));

	// package filename in static box at top
	wxStaticBoxSizer* package_frame = new wxStaticBoxSizer(wxVERTICAL,this,wxT("package"));
	package_file = new wxStaticText(this,wxID_ANY,wxT("[package filename]"));
	package_frame->Add(package_file,0,wxEXPAND);

	// list of rooms
	wxStaticBoxSizer* rooms_frame = new wxStaticBoxSizer(wxVERTICAL,this,wxT("room"));
	list_room = BuildList(rooms_frame,LIST_ROOM);

	// list of CHR
	wxStaticBoxSizer* chr_frame = new wxStaticBoxSizer(wxVERTICAL,this,wxT("chr"));
	list_chr = BuildList(chr_frame,LIST_CHR);

	// list of Palettes
	wxStaticBoxSizer* palette_frame = new wxStaticBoxSizer(wxVERTICAL,this,wxT("palette"));
	list_palette = BuildList(palette_frame,LIST_PALETTE);

	// list of Sprites
	wxStaticBoxSizer* sprite_frame = new wxStaticBoxSizer(wxVERTICAL,this,wxT("sprite"));
	list_sprite = BuildList(sprite_frame,LIST_SPRITE);

	// overall layout
	wxBoxSizer* boxtop = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* boxmid = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* boxright = new wxBoxSizer(wxVERTICAL);

	// top level box (package, everything else)
	boxtop->Add(package_frame,0,wxEXPAND);
	boxtop->Add(boxmid,1,wxEXPAND);
	// mid level box (rooms on left, everything else)
	boxmid->Add(rooms_frame,1,wxEXPAND);
	boxmid->Add(boxright,1,wxEXPAND);
	// right side box (chr/pal/sprite)
	boxright->Add(chr_frame,1,wxEXPAND);
	boxright->Add(palette_frame,1,wxEXPAND);
	boxright->Add(sprite_frame,1,wxEXPAND);

	if (file_arg)
	{
		data->package_file = file_arg;
		if (!data->load())
		{
			wxMessageBox(wxT("Unable to load: ") + data->package_file + wxT("\n")
				+ data->get_file_error(),
				wxT("File error!"),
				wxOK | wxICON_ERROR, this);
		}
	}

	RefreshData();
	SetSizer(boxtop);
	Center();

	map_picker = NULL;
	for (int i=0; i < ANIM_MAX; ++i) anim_show[i] = NULL;
	main_package = this;
}

wxListBox* Package::BuildList(wxStaticBoxSizer* sizer, int id_base)
{
	wxBoxSizer* bar = new wxBoxSizer(wxHORIZONTAL);
	bar->Add(new wxButton(this,id_base+BAR_EDIT  ,wxT("Edit"   )),1);
	bar->Add(new wxButton(this,id_base+BAR_NEW   ,wxT("New"   )),1);
	bar->Add(new wxButton(this,id_base+BAR_COPY  ,wxT("Copy"  )),1);
	bar->Add(new wxButton(this,id_base+BAR_UP    ,wxT("Up"    )),1);
	bar->Add(new wxButton(this,id_base+BAR_DOWN  ,wxT("Down"  )),1);
	bar->Add(new wxButton(this,id_base+BAR_RENAME,wxT("Rename")),1);
	bar->Add(new wxButton(this,id_base+BAR_DELETE,wxT("Delete")),1);

	wxListBox* list = new wxListBox(this,id_base+BAR_LIST);

	sizer->Add(bar ,0,wxEXPAND);
	sizer->Add(list,1,wxEXPAND);

	Connect(id_base+BAR_LIST  , wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(Package::OnListEdit));
	Connect(id_base+BAR_EDIT  , wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Package::OnListEdit));
	Connect(id_base+BAR_NEW   , wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Package::OnListNew));
	Connect(id_base+BAR_COPY  , wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Package::OnListCopy));
	Connect(id_base+BAR_UP    , wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Package::OnListUp));
	Connect(id_base+BAR_DOWN  , wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Package::OnListDown));
	Connect(id_base+BAR_RENAME, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Package::OnListRename));
	Connect(id_base+BAR_DELETE, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(Package::OnListDelete));
	return list;
}

void Package::RefreshData()
{
	package_file->SetLabel(data->package_file);

	wxListBox* list[4] = { list_chr, list_palette, list_sprite, list_room };
	for (int t=0; t<4; ++t)
	{
		int select = list[t]->GetSelection();
		list[t]->Clear();
		for (unsigned int i=0; i < data->get_item_count(t); ++i)
		{
			list[t]->Append(data->get_item(t,i)->name);
		}
		if (select < (int)data->get_item_count(t))
			list[t]->SetSelection(select);
	}

	Refresh();
}

void Package::OpenEditor(Data::Item* item)
{
	// if already open, raise its editor
	for (unsigned int i=0; i<open_editor.size(); ++i)
	{
		Editor* e = open_editor[i];
		if (e->item == item)
		{
			e->Raise();
			e->SetFocus();
			return;
		}
	}

	// otherwise open a new edit window
	Editor* en = NULL;
	switch (item->type)
	{
	case Data::ITEM_CHR:
		en = new EditChr(item,data,this);
		break;
	case Data::ITEM_PALETTE:
		en = new EditPalette(item,data,this);
		break;
	case Data::ITEM_SPRITE:
		en = new EditSprite(item,data,this);
		break;
	case Data::ITEM_ROOM:
		en = new EditRoom(item,data,this);
		PickRoom::set_default(data->find_index(item));
		break;
	default:
		wxASSERT(false);
		return;
	}

	wxASSERT(en != NULL);
	open_editor.push_back(en);
	en->Show();
	en->SetFocus();
}

void Package::EditorClosed(Editor* editor)
{
	//wxLogDebug(wxT("EditorClosed: %s\n"),editor->item->name);
	for (unsigned int i=0; i<open_editor.size(); ++i)
	{
		Editor* e = open_editor[i];
		if (e == editor)
		{
			open_editor.erase(open_editor.begin() + i);
			return;
		}
	}
	wxASSERT(false);
}

void Package::MapClosed(MapPicker* map)
{
	wxASSERT(map_picker == map);
	if (map_picker == map)
	{
		map_picker = NULL;
	}
}

void Package::AnimClosed(AnimShow* anim)
{
	for (int i=0; i<ANIM_MAX; ++i)
	{
		if(anim_show[i] == anim)
		{
			anim_show[i] = NULL;
			return;
		}
	}
	wxASSERT(false);
}

void Package::RefreshChr(Data::Chr* chr)
{
	for (unsigned int i=0; i<open_editor.size(); ++i)
	{
		Editor* e = open_editor[i];
		e->RefreshChr(chr);
	}

	for (int i=0; i<ANIM_MAX; ++i)
	{
		if (anim_show[i] != NULL)
		{
			anim_show[i]->RefreshChr(chr);
		}
	}
}

void Package::RefreshPalette(Data::Palette* pal)
{
	for (unsigned int i=0; i<open_editor.size(); ++i)
	{
		Editor* e = open_editor[i];
		e->RefreshPalette(pal);
	}

	for (int i=0; i<ANIM_MAX; ++i)
	{
		if (anim_show[i] != NULL)
		{
			anim_show[i]->RefreshPalette(pal);
		}
	}
}

void Package::RefreshSprite(Data::Sprite* sprite)
{
	for (unsigned int i=0; i<open_editor.size(); ++i)
	{
		Editor* e = open_editor[i];
		e->RefreshSprite(sprite);
	}

	for (int i=0; i<ANIM_MAX; ++i)
	{
		if (anim_show[i] != NULL)
		{
			anim_show[i]->RefreshSprite(sprite);
		}
	}
}

void Package::RefreshMap()
{
	if (map_picker)
	{
		map_picker->Refresh();
	}
}

void Package::DeleteChr(Data::Chr* chr)
{
	for (int i=0; i<4; ++i)
	{
		if (AnimShow::default_chr[i] == chr)
			AnimShow::default_chr[i] = NULL;
	}

	for (unsigned int i=0; i<open_editor.size(); ++i)
	{
		Editor* e = open_editor[i];
		e->DeleteChr(chr);
	}

	for (int i=0; i<ANIM_MAX; ++i)
	{
		if (anim_show[i] != NULL)
		{
			anim_show[i]->DeleteChr(chr);
		}
	}
}

void Package::DeletePalette(Data::Palette* pal)
{
	// clear it from the default palette lists
	for (int i=0; i<8; ++i)
	{
		if (EditChr::default_palette[i] == pal)
			EditChr::default_palette[i] = NULL;
	}
	for (int i=0; i<4; ++i)
	{
		if (AnimShow::default_palette[i] == pal)
			AnimShow::default_palette[i] = NULL;
	}

	for (unsigned int i=0; i<open_editor.size(); ++i)
	{
		Editor* e = open_editor[i];
		e->DeletePalette(pal);
	}

	for (int i=0; i<ANIM_MAX; ++i)
	{
		if (anim_show[i] != NULL)
		{
			anim_show[i]->DeletePalette(pal);
		}
	}
}

void Package::DeleteSprite(Data::Sprite* sprite)
{
	for (unsigned int i=0; i<open_editor.size(); ++i)
	{
		Editor* e = open_editor[i];
		e->DeleteSprite(sprite);
	}

	for (int i=0; i<ANIM_MAX; ++i)
	{
		if (anim_show[i] != NULL)
		{
			anim_show[i]->DeleteSprite(sprite);
		}
	}
}

void Package::DeleteRoom(Data::Room* room)
{
	for (unsigned int i=0; i<open_editor.size(); ++i)
	{
		Editor* e = open_editor[i];
		e->DeleteRoom(room);
	}
}

void Package::DoTest(bool nes, unsigned char start_lizard)
{
	if (!data->export_(start_lizard))
	{
		wxMessageBox(wxT("Unable to export:") + data->package_file + wxT("\n")
			+ data->get_file_error(),
			wxT("File error!"),
			wxOK | wxICON_ERROR, this);
	}
	else
	{
		wxString working_dir = wxGetCwd();

		wxFileName fn_path = data->package_file;
		fn_path.AppendDir(wxString(wxT("..")));
		wxString test_dir = fn_path.GetFullPath();
		wxSetWorkingDirectory(test_dir);

		if (nes)
		{
			wxExecute(wxT("test_nes.bat"));
		}
		else
		{
			wxExecute(wxT("test_pc.bat"));
		}

		wxSetWorkingDirectory(working_dir);
	}

}

void Package::OnClose(wxCloseEvent& event)
{
	//wxLogDebug(wxT("Package::Onclose (%d editors open)\n"),open_editor.size());

	if (event.CanVeto() && data->unsaved)
	{
		if(wxYES != wxMessageBox(
			wxT("You have unsaved changes.\nDo you really want to quit?"),
			wxT("Quit"),wxYES_NO | wxNO_DEFAULT, this))
		{
			event.Veto(true);
			return;
		}
	}
	
	main_package = NULL;
	Destroy();
}

void Package::OnNew(wxCommandEvent& event)
{
	if (data->unsaved && wxYES != wxMessageBox(
			wxT("You have unsaved changes.\nDo you really want to do this?"),
			wxT("New Package"),wxYES_NO | wxNO_DEFAULT, this))
	{
		return;
	}

	wxFileDialog * openFileDialog = new wxFileDialog(
		this, wxT("New Package"), wxT(""), wxT(""), wxT("*.lpk"),
		wxFD_SAVE | wxOVERWRITE_PROMPT);
	if (openFileDialog->ShowModal() == wxID_OK)
	{
		wxString filename = openFileDialog->GetPath();
		data->clear();
		data->package_file = filename;
		RefreshData();
	}
	openFileDialog->Destroy();
}

void Package::OnOpen(wxCommandEvent& event)
{
	if (data->unsaved && wxYES != wxMessageBox(
			wxT("You have unsaved changes.\nDo you really want to do this?"),
			wxT("Open"),wxYES_NO | wxNO_DEFAULT, this))
	{
		return;
	}

	wxFileDialog * openFileDialog = new wxFileDialog(
		this, wxT("Open Package"), wxT(""), wxT(""), wxT("*.lpk"),
		wxFD_OPEN);
	if (openFileDialog->ShowModal() == wxID_OK)
	{
		wxString filename = openFileDialog->GetPath();
		data->clear();
		data->package_file = filename;
		if (!data->load())
		{
			wxMessageBox(wxT("Unable to load: ") + data->package_file + wxT("\n")
				+ data->get_file_error(),
				wxT("File error!"),
				wxOK | wxICON_ERROR, this);
		}
		RefreshData();
	}
	openFileDialog->Destroy();
}

void Package::OnSave(wxCommandEvent& event)
{
	if (!data->save())
	{
		wxMessageBox(wxT("Unable to save: ") + data->package_file + wxT("\n")
			+ data->get_file_error(),
			wxT("File error!"),
			wxOK | wxICON_ERROR, this);
	}
	data->unsaved = false;
}

void Package::OnExport(wxCommandEvent& event)
{
	if (!data->export_(0))
	{
		wxMessageBox(wxT("Unable to export:") + data->package_file + wxT("\n")
			+ data->get_file_error(),
			wxT("File error!"),
			wxOK | wxICON_ERROR, this);
	}
	else
	{
		wxMessageBox(wxT("Success!"), wxT("Export"), wxOK, this);
	}
}

void Package::OnTest(wxCommandEvent& event)
{
	DoTest(event.GetId() == wxID_PRINT,0);
}

void Package::OnQuit(wxCommandEvent& event)
{
	Close(false);
}

void Package::OnMarkAllUnsaved(wxCommandEvent& event)
{
	data->unsaved = true;
	for (int t=0; t<Data::ITEM_TYPE_COUNT; ++t)
	{
		for (unsigned int i=0; i<data->get_item_count(t); ++i)
		{
			data->get_item(t,i)->unsaved = true;
		}
	}
}

void Package::OnValidate(wxCommandEvent& event)
{
	data->validate_extra();
	wxString file_error = data->get_file_error();

	// save to file

	wxFileName fn_package = data->package_file;
	wxFileName fn_path = wxFileName::DirName(fn_package.GetPath());
	fn_path.AppendDir(wxT("export"));
	if (!fn_path.DirExists())
	{
		if (!fn_path.Mkdir())
		{
			file_error += wxT("Could not make directory: ") + fn_path.GetPath() + wxT("\n");
		}
	}

	wxFileName fn_validate = wxFileName(fn_path.GetPath(),wxT("validate.txt"));
	wxTextFile f(fn_validate.GetFullPath());

	if (!f.Exists()) f.Create();
	f.Open();
	if (!f.IsOpened())
	{
		wxMessageBox(wxT("Validation results failed to save to:\n") + fn_validate.GetFullPath(),wxT("Validation Log"),wxOK | wxICON_EXCLAMATION,this);
	}
	else
	{
		f.Clear();
		f.AddLine(file_error);
		f.Write();
		f.Close();
		wxMessageBox(wxT("Validation results logged to:\n") + fn_validate.GetFullPath(),wxT("Validation Log"),wxOK,this);
	}
}

void Package::OnRenderMaps(wxCommandEvent& event)
{
	bool underlay = (event.GetId() == wxID_VIEW_LARGEICONS);

	if (!data->render_maps(underlay))
	{
		wxMessageBox(wxT("Unable to render maps!\n") + data->get_file_error(),wxT("Render Maps"), wxOK | wxICON_EXCLAMATION, this);
	}
	else
	{
		wxMessageBox(wxT("Maps rendered!"),wxT("Render Maps"), wxOK, this);
	}
}

void Package::OnRenderSprites(wxCommandEvent& event)
{
	if (!data->render_sprites())
	{
		wxMessageBox(wxT("Unable to render sprites!\n") + data->get_file_error(),wxT("Render Sprites"), wxOK | wxICON_EXCLAMATION, this);
	}
	else
	{
		wxMessageBox(wxT("Sprites rendered!"),wxT("Render Sprites"), wxOK, this);
	}
}

void Package::OnMap(wxCommandEvent& event)
{
	if (map_picker != NULL)
	{
		map_picker->Raise();
		map_picker->SetFocus();
		return;
	}

	wxASSERT(map_picker == NULL);
	map_picker = new MapPicker(data,this);
	map_picker->Show();
	map_picker->SetFocus();
}

void Package::OnAnim(wxCommandEvent& event)
{
	int index = 0;
	for (;index < ANIM_MAX; ++index)
	{
		if (anim_show[index] == NULL)
			break;
	}
	if (index >= ANIM_MAX)
	{
		wxMessageBox(wxT("Too many animation windows open!"),wxT("Animation"), wxOK | wxICON_EXCLAMATION, this);
		return;
	}

	anim_show[index] = new AnimShow(data,this);
	anim_show[index]->Show();
	anim_show[index]->SetFocus();
}

void Package::OnAbout(wxCommandEvent& event)
{
	wxString about_text =
		wxT("lizard tool version ") wxT(VERSION_STRING) wxT("\n\n")
		wxT("build: ") wxT(__DATE__) wxT(" ") wxT(__TIME__) wxT("\n\n")
		wxT("Keyboard commands:\n")
		wxT("    CHR: 1/2/3/4 select colour\n")
		wxT("    CHR: C copy tile\n")
		wxT("    CHR: V paste tile\n")
		wxT("    CHR: F fill tile\n")
		wxT("    CHR: [/] increment/decrement tile (+shift=up/down)\n")
		wxT("    Room: [/] select tile\n")
		wxT("    Room: A/G/S/D/C toggle attribute/grid/sprites/doors/collide\n")
		wxT("    Room: ,/. right click to set selection\n")
		wxT("    Room: F fill selection\n")
		wxT("    Room: K copy selection\n")
		wxT("    Room: L right click to paste selection\n")
		wxT("    Room: E/R flood fill same/boundary\n")
		wxT("    Room: M/N random-multi for E flood fill / cancel\n")
		wxT("    Room: T volcano zone flood fill\n")
		wxT("    Room: Y demo fill selection (multi 0/1 or $80/$91)\n")
		wxT("    Room: B replace tile\n")
		wxT("    Room: ~ toggles tile-typing mode\n")
		wxT("    Room: Z undo\n")
		wxT("    Room: Esc cancel action and deselect\n")
		wxT("Room:\n")
		wxT("    Right click CHR, Palette, or door to edit linked item\n")
		wxT("    Right click nametable to pick tile/attribute\n")
		wxT("    Hold shift to draw only attribute\n")
		wxT("CHR:\n")
		wxT("    Right click to pick colour\n")
		wxT("    Suffix _ suppresses export of CHR.\n")
		wxT("Map:\n")
		wxT("    Right click map to switch Recto/Verso\n")
		wxT("Sprite:\n")
		wxT("    Suffix _ suppresses export of sprite.\n")
		wxT("    Suffix __ suppresses export and hides in map render.\n")
		;
	wxMessageBox(about_text,wxT("About"),wxOK,this);
}

// 4 list buttons

const wxChar* LIST_NAME[4] = { wxT("chr"), wxT("palette"), wxT("sprite"), wxT("room") };

void Package::OnListEdit(wxCommandEvent& event)
{
	int t = ((event.GetId() & 0xF00) - LIST_CHR) >> 8; wxASSERT(t >= 0 && t < 4);
	wxListBox* list[4] = { list_chr, list_palette, list_sprite, list_room };
	int s = list[t]->GetSelection();
	if (s >= 0 && s < (signed int)data->get_item_count(t))
	{
		OpenEditor(data->get_item(t,s));
	}
}

void Package::OnListNew(wxCommandEvent& event)
{
	int t = ((event.GetId() & 0xF00) - LIST_CHR) >> 8; wxASSERT(t >= 0 && t < 4);

	wxString default_name;
	default_name.Printf(wxT("%s%03d"),LIST_NAME[t],data->get_item_count(t));

	wxTextEntryDialog* new_name = new wxTextEntryDialog(
		this,
		wxString(wxT("Choose a name:")),
		wxString(wxT("New "))+LIST_NAME[t],
		default_name);
	if (wxID_OK == new_name->ShowModal())
	{
		if (!data->new_item(t,new_name->GetValue()))
			wxMessageBox(wxString(wxT("Unable to add new "))+LIST_NAME[t],wxT("Error!"),wxOK|wxICON_EXCLAMATION);
		else
			RefreshData();
	}

	if (t == Data::ITEM_ROOM && map_picker != NULL) map_picker->Refresh();
}

void Package::OnListCopy(wxCommandEvent& event)
{
	int t = ((event.GetId() & 0xF00) - LIST_CHR) >> 8; wxASSERT(t >= 0 && t < 4);

	wxListBox* list[4] = { list_chr, list_palette, list_sprite, list_room };
	int s = list[t]->GetSelection();

	if (s < 0 || s > (signed int)data->get_item_count(t))
	{
		wxMessageBox(wxT("No item selected to copy."),wxT("Error!"),wxOK|wxICON_EXCLAMATION);
		return;
	}

	wxString default_name;
	default_name.Printf(wxT("%s%03d"),LIST_NAME[t],data->get_item_count(t));

	wxTextEntryDialog* new_name = new wxTextEntryDialog(
		this,
		wxString(wxT("Choose a name:")),
		wxString(wxT("Copy "))+LIST_NAME[t],
		default_name);
	if (wxID_OK == new_name->ShowModal())
	{
		if (!data->copy_item(t,s,new_name->GetValue()))
		{
			wxMessageBox(wxString(wxT("Unable to copy to new "))+LIST_NAME[t],wxT("Error!"),wxOK|wxICON_EXCLAMATION);
		}
		else
		{
			RefreshData();
		}
	}

	if (t == Data::ITEM_ROOM && map_picker != NULL) map_picker->Refresh();
}

void Package::OnListUp(wxCommandEvent& event)
{
	int t = ((event.GetId() & 0xF00) - LIST_CHR) >> 8; wxASSERT(t >= 0 && t < 4);
	wxListBox* list[4] = { list_chr, list_palette, list_sprite, list_room };
	int s = list[t]->GetSelection();
	if (s >= 1 && s < (signed int)data->get_item_count(t))
	{
		if(!data->swap_item(t,(unsigned int)s,(unsigned int)s-1))
			wxMessageBox(wxString(wxT("Unable to move "))+LIST_NAME[t],wxT("Error!"),wxOK|wxICON_EXCLAMATION);
		else
		{
			RefreshData();
			list[t]->SetSelection(s-1);
		}
	}
}

void Package::OnListDown(wxCommandEvent& event)
{
	int t = ((event.GetId() & 0xF00) - LIST_CHR) >> 8; wxASSERT(t >= 0 && t < 4);
	wxListBox* list[4] = { list_chr, list_palette, list_sprite, list_room };
	int s = list[t]->GetSelection();
	if (s >= 0 && (s+1) < (signed int)data->get_item_count(t))
	{
		if(!data->swap_item(t,(unsigned int)s,(unsigned int)s+1))
			wxMessageBox(wxString(wxT("Unable to move "))+LIST_NAME[t],wxT("Error!"),wxOK|wxICON_EXCLAMATION);
		else
		{
			RefreshData();
			list[t]->SetSelection(s+1);
		}
	}
}

void Package::OnListRename(wxCommandEvent& event)
{
	int t = ((event.GetId() & 0xF00) - LIST_CHR) >> 8; wxASSERT(t >= 0 && t < 4);
	wxListBox* list[4] = { list_chr, list_palette, list_sprite, list_room };
	int s = list[t]->GetSelection();
	if (s >= 0 && s < (signed int)data->get_item_count(t))
	{
		Data::Item* item = data->get_item(t,s);

		wxTextEntryDialog* new_name = new wxTextEntryDialog(
			this,
			wxString(wxT("Choose a new name:")),
			wxString(wxT("Rename "))+LIST_NAME[t],
			item->name);

		if (wxID_OK == new_name->ShowModal())
		{
			if (!data->rename_item(t,s,new_name->GetValue()))
				wxMessageBox(wxString(wxT("Unable to rename "))+LIST_NAME[t],wxT("Error!"),wxOK|wxICON_EXCLAMATION);
			else
				RefreshData();
		}
	}

	if (t == Data::ITEM_ROOM && map_picker != NULL) map_picker->Refresh();
}

void Package::OnListDelete(wxCommandEvent& event)
{
	int t = ((event.GetId() & 0xF00) - LIST_CHR) >> 8; wxASSERT(t >= 0 && t < 4);
	wxListBox* list[4] = { list_chr, list_palette, list_sprite, list_room };
	int s = list[t]->GetSelection();
	if (s >= 0 && s < (signed int)data->get_item_count(t))
	{
		// close editor for this item if opened
		Data::Item* item = data->get_item(t,(unsigned int)s);
		for (unsigned int i=0; i < open_editor.size(); ++i)
		{
			Editor* e = open_editor[i];
			if (e->item == item)
			{
				e->Close();
				break;
			}
		}

		switch (item->type)
		{
		case Data::ITEM_CHR:
			DeleteChr((Data::Chr*)item);
			break;
		case Data::ITEM_PALETTE:
			DeletePalette((Data::Palette*)item);
			break;
		case Data::ITEM_SPRITE:
			DeleteSprite((Data::Sprite*)item);
			break;
		case Data::ITEM_ROOM:
			DeleteRoom((Data::Room*)item);
			if (map_picker != NULL) map_picker->Refresh();
			break;
		default:
			break;
		};

		if(!data->delete_item(t,(unsigned int)s))
		{
			wxString refs = data->reference_item(t,(unsigned int)s);
			wxMessageBox(wxString(wxT("Unable to delete "))+LIST_NAME[t]+wxT(", referenced in:\n")+refs,
				wxT("Error!"),wxOK|wxICON_EXCLAMATION);
		}
		else
		{
			RefreshData();
			if (s > 0) list[t]->SetSelection(s-1);
		}
	}
}

// end of file
