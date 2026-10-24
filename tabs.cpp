#include "tabs.h"
#include "menu.h"

// The tab bodies.  Keeping these includes here - rather than in menu.h - is the
// whole point of the split: the shell no longer has to know about them, and
// main.cpp (which only builds the window and the hooks) does not either.
#include "default.h"
#include "exec.h"
#include "resources.h"
#include "luamenus.h"
#include "config.h"

namespace Tabs
{
	void Render()
	{
		// Was `if (Tab == 0) {...} if (Tab == 1) {...} else if ...` - the missing
		// `else` on the first branch made the chain asymmetric for no reason.
		switch (Menu::Tab)
		{
		case Menu::TabExecutor:  Executor::Render();  break;
		case Menu::TabResources: Resources::Render(); break;
		case Menu::TabMenus:     Menus::Render();     break;
		case Menu::TabConfig:    Config::Render();    break;

		case Menu::TabDefault:
		default:                 Default::Render();   break;
		}
	}
}
