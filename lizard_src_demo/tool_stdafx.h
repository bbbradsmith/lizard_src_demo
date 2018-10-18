#pragma once
#pragma message("Precompiling tool_stdafx.h...")

#include <vector>
#include "wx/wx.h"

#ifdef _DEBUG
#ifdef __WXMSW__
	#include "wx/msw/msvcrt.h"
	#pragma message("Memory logging enabled.")
#endif
#endif
