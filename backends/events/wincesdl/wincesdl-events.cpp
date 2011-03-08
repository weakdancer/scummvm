/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "common/scummsys.h"

#ifdef _WIN32_WCE

#include "common/config-manager.h"

#include "backends/events/wincesdl/wincesdl-events.h"
#include "backends/graphics/wincesdl/wincesdl-graphics.h"
#include "backends/platform/wince/CEActionsPocket.h"
#include "backends/platform/wince/CEActionsSmartphone.h"
#include "backends/platform/wince/CEDevice.h"

#include "backends/platform/sdl/sdl.h"

WINCESdlEventSource::WINCESdlEventSource()
	: _tapTime(0), _closeClick(false), _rbutton(false),
	_freeLook(false) {
}

void WINCESdlEventSource::fillMouseEvent(Common::Event &event, int x, int y) {
	event.mouse.x = x;
	event.mouse.y = y;

	// Update the "keyboard mouse" coords
	_km.x = event.mouse.x;
	_km.y = event.mouse.y;

	// Adjust for the screen scaling
	if (((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_zoomDown)
		event.mouse.y += 240;

	event.mouse.x = event.mouse.x * ((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_scaleFactorXd / ((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_scaleFactorXm;
	event.mouse.y = event.mouse.y * ((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_scaleFactorYd / ((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_scaleFactorYm;
}

bool WINCESdlEventSource::pollEvent(Common::Event &event) {
	SDL_Event ev;
	ev.type = SDL_NOEVENT;
	DWORD currentTime;
	bool keyEvent = false;
	int deltaX, deltaY;

	memset(&event, 0, sizeof(Common::Event));

	handleKbdMouse();

	// If the screen changed, send an Common::EVENT_SCREEN_CHANGED
	int screenID = ((OSystem_SDL *)g_system)->getGraphicsManager()->getScreenChangeID();
	if (screenID != _lastScreenID) {
		_lastScreenID = screenID;
		event.type = Common::EVENT_SCREEN_CHANGED;
		return true;
	}

	CEDevice::wakeUp();

	currentTime = GetTickCount();

	while (SDL_PollEvent(&ev)) {
		switch (ev.type) {
		case SDL_KEYDOWN:
			debug(1, "Key down %X %s", ev.key.keysym.sym, SDL_GetKeyName((SDLKey)ev.key.keysym.sym));
			// KMOD_RESERVED is used if the key has been injected by an external buffer
			if (ev.key.keysym.mod != KMOD_RESERVED && !GUI::Actions::Instance()->mappingActive()) {
				keyEvent = true;
				((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_lastKeyPressed = ev.key.keysym.sym;
				((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_keyRepeatTime = currentTime;
				((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_keyRepeat = 0;

				if (!GUI_Actions::Instance()->mappingActive() && GUI_Actions::Instance()->performMapped(ev.key.keysym.sym, true))
					return true;
			}

			if (GUI_Actions::Instance()->mappingActive())
				event.kbd.flags = 0xFF;
			else if (ev.key.keysym.sym == SDLK_PAUSE) {
				((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_lastKeyPressed = 0;
				event.type = Common::EVENT_PREDICTIVE_DIALOG;
				return true;
			} 			event.type = Common::EVENT_KEYDOWN;
			if (!GUI::Actions::Instance()->mappingActive())
				event.kbd.keycode = (Common::KeyCode)ev.key.keysym.sym;
			else
				event.kbd.keycode = (Common::KeyCode)mapKeyCE(ev.key.keysym.sym, ev.key.keysym.mod, ev.key.keysym.unicode, GUI::Actions::Instance()->mappingActive());
			event.kbd.ascii = mapKeyCE(ev.key.keysym.sym, ev.key.keysym.mod, ev.key.keysym.unicode, GUI::Actions::Instance()->mappingActive());

			if (ev.key.keysym.mod == KMOD_RESERVED && ev.key.keysym.unicode == KMOD_SHIFT) {
				event.kbd.ascii ^= 0x20;
				event.kbd.flags = Common::KBD_SHIFT;
			}

			return true;

		case SDL_KEYUP:
			debug(1, "Key up %X %s", ev.key.keysym.sym, SDL_GetKeyName((SDLKey)ev.key.keysym.sym));
			// KMOD_RESERVED is used if the key has been injected by an external buffer
			if (ev.key.keysym.mod != KMOD_RESERVED && !GUI::Actions::Instance()->mappingActive()) {
				keyEvent = true;
				((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_lastKeyPressed = 0;

				if (!GUI_Actions::Instance()->mappingActive() && GUI_Actions::Instance()->performMapped(ev.key.keysym.sym, false))
					return true;
			}

			if (GUI_Actions::Instance()->mappingActive())
				event.kbd.flags = 0xFF;
			else if (ev.key.keysym.sym == SDLK_PAUSE) {
				((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_lastKeyPressed = 0;
				return false;	// chew up the show agi dialog key up event
			}

			event.type = Common::EVENT_KEYUP;
			if (!GUI::Actions::Instance()->mappingActive())
				event.kbd.keycode = (Common::KeyCode)ev.key.keysym.sym;
			else
				event.kbd.keycode = (Common::KeyCode)mapKeyCE(ev.key.keysym.sym, ev.key.keysym.mod, ev.key.keysym.unicode, GUI::Actions::Instance()->mappingActive());
			event.kbd.ascii = mapKeyCE(ev.key.keysym.sym, ev.key.keysym.mod, ev.key.keysym.unicode, GUI::Actions::Instance()->mappingActive());

			if (ev.key.keysym.mod == KMOD_RESERVED && ev.key.keysym.unicode == KMOD_SHIFT) {
				event.kbd.ascii ^= 0x20;
				event.kbd.flags = Common::KBD_SHIFT;
			}

			return true;

		case SDL_MOUSEMOTION:
			event.type = Common::EVENT_MOUSEMOVE;
			fillMouseEvent(event, ev.motion.x, ev.motion.y);
			((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->setMousePos(event.mouse.x, event.mouse.y);

			return true;

		case SDL_MOUSEBUTTONDOWN:
			if (ev.button.button == SDL_BUTTON_LEFT)
				event.type = Common::EVENT_LBUTTONDOWN;
			else if (ev.button.button == SDL_BUTTON_RIGHT)
				event.type = Common::EVENT_RBUTTONDOWN;
			else
				break;
			fillMouseEvent(event, ev.button.x, ev.button.y);


			if (event.mouse.x > _tapX)
				deltaX = event.mouse.x - _tapX;
			else
				deltaX = _tapX - event.mouse.x;
			if (event.mouse.y > _tapY)
				deltaY = event.mouse.y - _tapY;
			else
				deltaY = _tapY - event.mouse.y;
			_closeClick = (deltaX <= 5 && deltaY <= 5);

			if (!_isSmartphone) {
				// handle double-taps
				if (_tapTime) {		// second tap
					if (_closeClick && (GetTickCount() - _tapTime < 1000)) {
						if ( event.mouse.y <= 20 &&
								((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_panelInitialized) {
							// top of screen (show panel)
							((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->swap_panel_visibility();
						} else if (!((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_noDoubleTapRMB) {
							// right click
							event.type = Common::EVENT_RBUTTONDOWN;
							_rbutton = true;
						}
					}
					_tapTime = 0;
				} else {
					_tapTime = GetTickCount();
					_tapX = event.mouse.x;
					_tapY = event.mouse.y;
				}
			}

			if (_freeLook && !_closeClick) {
				_rbutton = false;
				_tapTime = 0;
				_tapX = event.mouse.x;
				_tapY = event.mouse.y;
				event.type = Common::EVENT_MOUSEMOVE;
				((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->setMousePos(event.mouse.x, event.mouse.y);
			}


			if (((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_toolbarHandler.action(event.mouse.x, event.mouse.y, true)) {
				if (!((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_toolbarHandler.drawn()) {
					((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_toolbarHighDrawn = false;
					((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->internUpdateScreen();
				}
				if (((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_newOrientation != ((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_orientationLandscape){
					((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_orientationLandscape = ((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_newOrientation;
					((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_toolbarHighDrawn = false;
					ConfMan.setInt("landscape", ((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_orientationLandscape);
					ConfMan.flushToDisk();
					((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->hotswapGFXMode();
				}
				return false;
			}

			return true;

		case SDL_MOUSEBUTTONUP:
			if (ev.button.button == SDL_BUTTON_LEFT)
				event.type = Common::EVENT_LBUTTONUP;
			else if (ev.button.button == SDL_BUTTON_RIGHT)
				event.type = Common::EVENT_RBUTTONUP;
			else
				break;

			if (_rbutton) {
				event.type = Common::EVENT_RBUTTONUP;
				_rbutton = false;
			}

			fillMouseEvent(event, ev.button.x, ev.button.y);

			if (_freeLook && !_closeClick) {
				_tapX = event.mouse.x;
				_tapY = event.mouse.y;
				event.type = Common::EVENT_MOUSEMOVE;
				((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->setMousePos(event.mouse.x, event.mouse.y);
			}

			if (((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_toolbarHandler.action(event.mouse.x, event.mouse.y, false)) {
				if (!((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_toolbarHandler.drawn()) {
					((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_toolbarHighDrawn = false;
					((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->internUpdateScreen();
				}
				return false;

			}
			return true;

		case SDL_VIDEOEXPOSE:
			// HACK: Send a fake event, handled by SdlGraphicsManager
			event.type = (Common::EventType)OSystem_SDL::kSdlEventExpose;
			break;

		case SDL_QUIT:
			event.type = Common::EVENT_QUIT;
			return true;

		case SDL_ACTIVEEVENT:
			if (ev.active.state & SDL_APPMOUSEFOCUS)
				debug(2, "%s mouse focus.", ev.active.gain ? "Got" : "Lost");
			if (ev.active.state & SDL_APPINPUTFOCUS)
				debug(2, "%s input focus.", ev.active.gain ? "Got" : "Lost");
			if (ev.active.state & SDL_APPACTIVE)
				debug(2, "%s total focus.", ev.active.gain ? "Got" : "Lost");
			if (ev.active.state & SDL_APPINPUTFOCUS) {
				((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_hasfocus = ev.active.gain;
				SDL_PauseAudio(!((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_hasfocus);
				if (((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_hasfocus) {
					event.type = (Common::EventType)OSystem_SDL::kSdlEventExpose;
				}
			}
			break;
		}
	}

	// Simulate repeated key for backend
	if (!keyEvent && ((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_lastKeyPressed && (int)currentTime > ((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_keyRepeatTime + ((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_keyRepeatTrigger) {
		((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_keyRepeatTime = currentTime;
		((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_keyRepeat++;
		GUI_Actions::Instance()->performMapped(((WINCESdlGraphicsManager *)((OSystem_SDL *)g_system)->getGraphicsManager())->_lastKeyPressed, true);
	}

	return false;
}

int WINCESdlEventSource::mapKeyCE(SDLKey key, SDLMod mod, Uint16 unicode, bool unfilter) {
	if (GUI::Actions::Instance()->mappingActive())
		return key;

	if (unfilter) {
		switch (key) {
		case SDLK_ESCAPE:
			return SDLK_BACKSPACE;
		case SDLK_F8:
			return SDLK_ASTERISK;
		case SDLK_F9:
			return SDLK_HASH;
		default:
			return key;
		}
	}

	if (key >= SDLK_KP0 && key <= SDLK_KP9) {
		return key - SDLK_KP0 + '0';
	} else if (key >= SDLK_UP && key <= SDLK_PAGEDOWN) {
		return key;
	} else if (key >= SDLK_NUMLOCK && key <= SDLK_EURO) {
		return 0;
	}
	return key;
}

void WINCESdlEventSource::swap_freeLook() {
	_freeLook = !_freeLook;
}

#endif /* _WIN32_WCE */
