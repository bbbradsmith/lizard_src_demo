// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "tool_package.h"
#include "tool_data.h"

namespace LizardTool
{

static Data data;

class ToolApp : public wxApp
{
public:
	bool OnInit()
	{
		wxImage::AddHandler(new wxPNGHandler());

		wxChar* file_arg = NULL;
		if (argc > 1) file_arg = argv[1];
		Package* main_window = new LizardTool::Package(&data, file_arg);

		main_window->Show(true);
		return true;
	}
};

} // namespace LizardTool

IMPLEMENT_APP(LizardTool::ToolApp);
