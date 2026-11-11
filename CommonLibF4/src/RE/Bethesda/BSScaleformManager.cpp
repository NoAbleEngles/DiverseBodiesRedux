#include "F4SE/Impl/PCH.h"
#include "RE/Bethesda/BSScaleformManager.h"

#include "RE/Bethesda/BSSystemFileStreamer.h"
#include "RE/Bethesda/IMenu.h"
#include "RE/Bethesda/Settings.h"
#include "RE/Scaleform/GFx/GFx_Loader.h"
#include "RE/Scaleform/GFx/GFx_Player.h"

namespace RE
{
	bool BSScaleformManager::LoadMovieEx(
		IMenu& a_menu,
		stl::zstring a_filePath,
		stl::zstring a_menuObjPath,
		ScaleModeType a_scaleMode,
		float a_backgroundAlpha)
	{
		return LoadMovie(
			a_menu,
			a_menu.uiMovie,
			a_filePath.data(),
			a_menuObjPath.empty() ? nullptr : a_menuObjPath.data(),
			a_scaleMode,
			a_backgroundAlpha);
	}
}

