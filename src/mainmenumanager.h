/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef MAINMENUMANAGER_HEADER
#define MAINMENUMANAGER_HEADER

/*
	All kinds of stuff that needs to be exposed from main.cpp
*/
#include "debug.h" // assert
#include "modalMenu.h"
#include <list>

class IGameCallback
{
public:
	virtual void exitToOS() = 0;
	virtual void keyConfig() = 0;
	virtual void disconnect() = 0;
	virtual void changePassword() = 0;
	virtual void changeVolume() = 0;

	virtual void signalKeyConfigChange() = 0;
};

extern gui::IGUIEnvironment *guienv;
extern gui::IGUIStaticText *guiroot;

// Handler for the modal menus

class MainMenuManager : public IMenuManager
{
public:
	virtual void createdMenu(gui::IGUIElement *menu)
	{
		for(std::list<gui::IGUIElement*>::iterator
				i = m_stack.begin();
				i != m_stack.end(); ++i)
		{
			assert(*i != menu);
		}

		if(!m_stack.empty())
			m_stack.back()->setVisible(false);
		m_stack.push_back(menu);
	}

	virtual void deletingMenu(gui::IGUIElement *menu)
	{
		// Remove all entries if there are duplicates
		m_stack.remove(menu);

		/*core::list<GUIModalMenu*>::Iterator i = m_stack.getLast();
		assert(*i == menu);
		m_stack.erase(i);*/

		if(!m_stack.empty())
			m_stack.back()->setVisible(true);
	}

	// Returns true to prevent further processing
	virtual bool preprocessEvent(const SEvent& event)
	{
		if (m_stack.empty())
			return false;
		GUIModalMenu *mm = dynamic_cast<GUIModalMenu*>(m_stack.back());
		return mm && mm->preprocessEvent(event);
	}

	u32 menuCount()
	{
		return m_stack.size();
	}

	bool pausesGame()
	{
		for(std::list<gui::IGUIElement*>::iterator
				i = m_stack.begin(); i != m_stack.end(); ++i)
		{
			GUIModalMenu *mm = dynamic_cast<GUIModalMenu*>(*i);
			if (mm && mm->pausesGame())
				return true;
		}
		return false;
	}

	std::list<gui::IGUIElement*> m_stack;
};

extern MainMenuManager g_menumgr;

extern bool isMenuActive();

class MainGameCallback : public IGameCallback
{
public:
	MainGameCallback(IrrlichtDevice *a_device):
		disconnect_requested(false),
		changepassword_requested(false),
		changevolume_requested(false),
		keyconfig_requested(false),
		shutdown_requested(false),
		keyconfig_changed(false),
		device(a_device)
	{
	}

	virtual void exitToOS()
	{
		shutdown_requested = true;
#ifndef __ANDROID__
		device->closeDevice();
#endif
	}

	virtual void disconnect()
	{
		disconnect_requested = true;
	}

	virtual void changePassword()
	{
		changepassword_requested = true;
	}

	virtual void changeVolume()
	{
		changevolume_requested = true;
	}

	virtual void keyConfig()
	{
		keyconfig_requested = true;
	}

	virtual void signalKeyConfigChange()
	{
		keyconfig_changed = true;
	}


	bool disconnect_requested;
	bool changepassword_requested;
	bool changevolume_requested;
	bool keyconfig_requested;
	bool shutdown_requested;

	bool keyconfig_changed;

	IrrlichtDevice *device;
};

extern MainGameCallback *g_gamecallback;

#endif

