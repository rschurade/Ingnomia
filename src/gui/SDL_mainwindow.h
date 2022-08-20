//
// Created by Arcnor on 17/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#ifndef INGNOMIA_SDL_MAINWINDOW_H
#define INGNOMIA_SDL_MAINWINDOW_H

#include "SDL_mainrenderer.h"

#include <SDL_keycode.h>

#include <sigslot/signal.hpp>

struct SDL_Window;

namespace Noesis {
	struct IView;
}

namespace AppGUI {
	struct Context;
}

class SDL_MainWindow {
public:
	SDL_MainWindow();
	~SDL_MainWindow();

	void mainLoop();

	int getWidth() const;
	int getHeight() const;
	int getFBWidth() const;
	int getFBHeight() const;

private:
	void onExit();
	void onResize( int w, int h );

private:
	void initializeBGFX();
	void noesisInit();

	void noesisInstallResourceProviders();
	void noesisRegisterComponents();

	SDL_Window* m_sdlWindow;
	AppGUI::Context* m_noesisContext;
	SDL_MainRenderer m_renderer;

	bool m_done = false;
	int m_width, m_height;
	int m_fbWidth, m_fbHeight;

#ifdef _DEBUG
	void updateImGuiIO();
#endif

public: // signals:
	sigslot::signal<int /*w*/, int /*h*/> signalWindowSize;
	sigslot::signal<int /*level*/> signalViewLevel;


	sigslot::signal<int /*key*/> signalKeyPress;
	sigslot::signal<SDL_KeyCode /*key*/, SDL_Keymod /*mod*/> signalKeyDown;
	sigslot::signal<SDL_KeyCode /*key*/, SDL_Keymod /*mod*/> signalKeyUp;
	sigslot::signal<int /*button*/, int /*x*/, int /*y*/> signalMouseDown;
	sigslot::signal<int /*button*/, int /*x*/, int /*y*/> signalMouseUp;
	sigslot::signal<SDL_KeyCode /*key*/> signalChar;
	sigslot::signal<> signalUpdateRenderOptions;
	sigslot::signal<> signalTogglePause;

	sigslot::signal<int /*width*/, int /*height*/, int /*moveX*/, int /*moveY*/, float /*scale*/, int /*rotation*/> signalRenderParams;
	sigslot::signal<> signalRotateSelection;
	sigslot::signal<int /*mouseX*/, int /*mouseY*/, bool /*shift*/, bool /*ctrl*/> signalMouse;
	sigslot::signal<float /*mouseX*/, int /*mouseY*/, float /*wheelX*/, float /*wheelY*/> signalMouseWheel;
	sigslot::signal<bool /*shift*/, bool /*ctrl*/> signalLeftClick;
	sigslot::signal<> signalRightClick;
};

#endif // INGNOMIA_SDL_MAINWINDOW_H
