#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_package.h"

namespace LizardTool
{

static const int MAP_W = 26;
static const int MAP_H = 18;

static const int MAP_RENDER_SIZE = 257 * 241 * MAP_W * MAP_H * 3;

class MapPanel;

class MapPicker : public wxFrame
{
public:
	MapPicker(Data* data_, Package* package_);

	Data* data;
	Package* package;
	MapPanel* map_panel;
	wxStaticText* location;

	void MapMove(wxMouseEvent& event);
	void MapDown(wxMouseEvent& event);

	void OnClose(wxCloseEvent& event);

	static bool render_map(Data* data, int phase, unsigned char render[MAP_RENDER_SIZE]);
};

} // namespace LizardTool
