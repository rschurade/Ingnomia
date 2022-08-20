//
// Created by Arcnor on 17/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#include "SDL_mainwindow.h"

#include "../base/config.h"
#include "../base/global.h"
#include "../gfx/constants.h"
#include "../bgfxUtils.h"
#include "../version.h"

#include "eventconnector.h"

#include "license.h"
#include "mainwindowrenderer.h"
#include "noesis/Context.hpp"
#include "noesis/Events.hpp"
#include "spdlog/spdlog.h"
#include "xaml/GameGui.xaml.h"
#include "xaml/GameModel.h"
#include "xaml/IngamePage.xaml.h"
#include "xaml/LoadGameModel.h"
#include "xaml/LoadGamePage.xaml.h"
#include "xaml/Main.xaml.h"
#include "xaml/MainMenu.xaml.h"
#include "xaml/MainPage.xaml.h"
#include "xaml/NewGameModel.h"
#include "xaml/NewGamePage.xaml.h"
#include "xaml/Population.xaml.h"
#include "xaml/PopulationModel.h"
#include "xaml/SettingsModel.h"
#include "xaml/SettingsPage.xaml.h"
#include "xaml/StockpileModel.h"
#include "xaml/TileInfo.xaml.h"
#include "xaml/TileInfoModel.h"
#include "xaml/ViewModel.h"
#include "xaml/WaitPage.xaml.h"
#include "xaml/agriculture.xaml.h"
#include "xaml/agriculturemodel.h"
#include "xaml/converters.h"
#include "xaml/creatureinfo.xaml.h"
#include "xaml/creatureinfomodel.h"
#include "xaml/debug.xaml.h"
#include "xaml/debugmodel.h"
#include "xaml/inventory.xaml.h"
#include "xaml/inventorymodel.h"
#include "xaml/military.xaml.h"
#include "xaml/militarymodel.h"
#include "xaml/neighbors.xaml.h"
#include "xaml/neighborsmodel.h"
#include "xaml/selection.xaml.h"
#include "xaml/selectionmodel.h"
#include "xaml/stockpilegui.xaml.h"
#include "xaml/workshopgui.xaml.h"
#include "xaml/workshopmodel.h"

#include <NsApp/Launcher.h>
#include <NsApp/LocalFontProvider.h>
#include <NsApp/LocalTextureProvider.h>
#include <NsApp/LocalXamlProvider.h>
#include <NsCore/EnumConverter.h>
#include <NsCore/RegisterComponent.h>
#include <NsGui/Grid.h>
#include <NsGui/IRenderer.h>
#include <NsGui/IntegrationAPI.h>

#include <SDL.h>
#include <SDL_image.h>

#include <filesystem>

namespace fs = std::filesystem;

#include <bx/bx.h>
#include <bx/string.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#ifdef _DEBUG
#include "../debug/debugmanager.h"
#include "../debug/imgui/bgfx_imgui.h"
#include "../game/game.h"
#include "../gfx/spritefactory.h"
#include "../gui/xaml/debugproxy.h"
#endif

constexpr auto DefaultScreenScale = 3 / 4.0;

bgfx::ViewId guiViewId  = 0xFE;

void SDL_MainWindow::initializeBGFX()
{
	bgfx::Init bgfxInit;
	bgfxInit.vendorId = BGFX_PCI_ID_NONE;          // Auto select first device, but we can be smarter and allow to select another using setup
#ifdef _WIN32
	bgfxInit.type = bgfx::RendererType::Vulkan; // DX11 shaders are broken, go with safe bet for now.
#else
	bgfxInit.type = bgfx::RendererType::Count; // Automatically choose a renderer.
#endif // _WIN32

	bgfxInit.debug = true;
	bgfxInit.profile = true;

	bgfxInit.resolution.width  = m_fbWidth;
	bgfxInit.resolution.height = m_fbHeight;
	bgfxInit.resolution.reset  = BGFX_RESET_VSYNC | BGFX_RESET_HIDPI;
	bgfxInit.platformData      = bgfxInitializePlatformData( m_sdlWindow );

	// FIXME: This disables multithread rendering, it fails under Metal because https://github.com/bkaradzic/bgfx/issues/2036
	bgfx::renderFrame();

	if ( !bgfx::init( bgfxInit ) )
	{
		throw std::runtime_error( "Cannot initialize BGFX" );
	}

#ifdef _DEBUG
	imguiCreate();

	auto& style = ImGui::GetStyle();
	style.WindowRounding = 5.0f;
	style.PopupRounding = 3.0f;

#endif
}

void uninitializeBGFX( SDL_Window* win )
{
#ifdef _DEBUG
	imguiDestroy();
#endif

	bgfxUninitializePlatformData( win );
}

SDL_MainWindow::SDL_MainWindow()
{
	SDL_Rect screenRect;
	if ( SDL_GetDisplayBounds( 0, &screenRect ) != 0 )
	{
		spdlog::critical( "Cannot get display bounds: {}", SDL_GetError() );
		throw std::runtime_error( "Cannot get display bounds" );
	}

	m_width  = std::max( (int)( screenRect.w * DefaultScreenScale ), (int)Global::cfg->get_or_default<double>( "WindowWidth", -1 ) );
	m_height = std::max( (int)( screenRect.h * DefaultScreenScale ), (int)Global::cfg->get_or_default<double>( "WindowHeight", -1 ) );

	int winX = SDL_WINDOWPOS_CENTERED;
	int winY = SDL_WINDOWPOS_CENTERED;
	if ( Global::cfg->object().contains( "WindowPosX" ) && Global::cfg->object().contains( "WindowPosY" ) )
	{
		winX = Global::cfg->get<double>( "WindowPosX" );
		winY = Global::cfg->get<double>( "WindowPosY" );
	}

	int windowFlags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
	if ( Global::cfg->get_or_default<bool>( "fullscreen", false ) )
	{
		windowFlags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	m_sdlWindow = SDL_CreateWindow( PROJECT_NAME, winX, winY, m_width, m_height, windowFlags );
	SDL_GL_GetDrawableSize( m_sdlWindow, &m_fbWidth, &m_fbHeight );

	initializeBGFX();

	const auto iconPath = Global::exePath / "content" / "icon.png";
	auto* iconImg       = IMG_Load( iconPath.string().c_str() );
	SDL_SetWindowIcon( m_sdlWindow, iconImg );
	SDL_FreeSurface( iconImg );

	Global::eventConnector->signalExit.connect( &SDL_MainWindow::onExit, this );
	this->signalWindowSize.connect( &EventConnector::onWindowSize, Global::eventConnector );
	this->signalViewLevel.connect( &EventConnector::onViewLevel, Global::eventConnector );
	this->signalKeyPress.connect( &EventConnector::onKeyPress, Global::eventConnector );
	this->signalTogglePause.connect( &EventConnector::onTogglePause, Global::eventConnector );
	this->signalUpdateRenderOptions.connect( &EventConnector::onUpdateRenderOptions, Global::eventConnector );

	this->signalMouse.connect( &AggregatorSelection::onMouse, Global::eventConnector->aggregatorSelection() );                     // TODO: Qt::QueuedConnection
	this->signalLeftClick.connect( &AggregatorSelection::onLeftClick, Global::eventConnector->aggregatorSelection() );             // TODO: Qt::QueuedConnection
	this->signalRightClick.connect( &AggregatorSelection::onRightClick, Global::eventConnector->aggregatorSelection() );           // TODO: Qt::QueuedConnection
	this->signalRotateSelection.connect( &AggregatorSelection::onRotateSelection, Global::eventConnector->aggregatorSelection() ); // TODO: Qt::QueuedConnection
	this->signalRenderParams.connect( &AggregatorSelection::onRenderParams, Global::eventConnector->aggregatorSelection() );       // TODO: Qt::QueuedConnection

//	Global::eventConnector->aggregatorDebug()->signalSetWindowSize.connect( &SDL_MainWindow::onSetWindowSize, this ); // TODO: Qt::QueuedConnection

//	Global::eventConnector->aggregatorSettings()->signalFullScreen.connect( &SDL_MainWindow::onFullScreen, this ); // TODO: Qt::QueuedConnection

//	Global::eventConnector->signalInitView.connect( &SDL_MainWindow::onInitViewAfterLoad, this ); // TODO: Qt::QueuedConnection

	noesisInit();

	m_renderer.resize( m_fbWidth, m_fbHeight );
	m_renderer.initialize();
}

SDL_MainWindow::~SDL_MainWindow()
{
	uninitializeBGFX( m_sdlWindow );

	SDL_DestroyWindow( m_sdlWindow );
}

#ifdef _DEBUG
// Mostly from ImGui's SDL2 backend
void SDL_MainWindow::updateImGuiIO()
{
	static bool         g_MousePressed[3] = { false, false, false };
	static SDL_Cursor*  g_MouseCursors[ImGuiMouseCursor_COUNT] = {};

	ImGuiIO& io = ImGui::GetIO();

	// Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
	if (io.WantSetMousePos)
		SDL_WarpMouseInWindow(m_sdlWindow, (int)io.MousePos.x, (int)io.MousePos.y);
	else
		io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

	int mx, my;
	Uint32 mouse_buttons = SDL_GetMouseState(&mx, &my);
	io.MouseDown[0] = g_MousePressed[0] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;  // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release       events that are shorter than 1 frame.
	io.MouseDown[1] = g_MousePressed[1] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	io.MouseDown[2] = g_MousePressed[2] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
	g_MousePressed[0] = g_MousePressed[1] = g_MousePressed[2] = false;

#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE && !defined(__EMSCRIPTEN__) && !defined(__ANDROID__) && !(defined(__APPLE__) && TARGET_OS_IOS)
	SDL_Window* focused_window = SDL_GetKeyboardFocus();
	if (m_sdlWindow == focused_window)
	{
		if (g_MouseCanUseGlobalState)
		{
			// SDL_GetMouseState() gives mouse position seemingly based on the last window entered/focused(?)
			// The creation of a new windows at runtime and SDL_CaptureMouse both seems to severely mess up with that, so we retrieve that position globally.
			// Won't use this workaround when on Wayland, as there is no global mouse position.
			int wx, wy;
			SDL_GetWindowPosition(focused_window, &wx, &wy);
			SDL_GetGlobalMouseState(&mx, &my);
			mx -= wx;
			my -= wy;
		}
		io.MousePos = ImVec2((float)mx, (float)my);
	}

	// SDL_CaptureMouse() let the OS know e.g. that our imgui drag outside the SDL window boundaries shouldn't e.g. trigger the OS window resize cursor.
	// The function is only supported from SDL 2.0.4 (released Jan 2016)
	bool any_mouse_button_down = ImGui::IsAnyMouseDown();
	SDL_CaptureMouse(any_mouse_button_down ? SDL_TRUE : SDL_FALSE);
#else
	if (SDL_GetWindowFlags(m_sdlWindow) & SDL_WINDOW_INPUT_FOCUS)
		io.MousePos = ImVec2((float)mx, (float)my);
#endif

	// MouseUpdate //

	if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
		return;

	ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
	if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
	{
		// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
		SDL_ShowCursor(SDL_FALSE);
	}
	else
	{
		// Show OS mouse cursor
		SDL_SetCursor(g_MouseCursors[imgui_cursor] ? g_MouseCursors[imgui_cursor] : g_MouseCursors[ImGuiMouseCursor_Arrow]);
		SDL_ShowCursor(SDL_TRUE);
	}
}

#endif // _DEBUG

void SDL_MainWindow::mainLoop()
{
#ifdef _DEBUG
	DebugManager debugMan;
#endif

	SDL_Event ev;
	SDL_Keymod lastMod = KMOD_NONE;
	while ( !m_done )
	{
#ifdef _DEBUG
		ImGuiIO& io = ImGui::GetIO();
		updateImGuiIO();
#endif
		float scrollX = 0, scrollY = 0;
		while (SDL_PollEvent( &ev )) {
			switch ( ev.type )
			{
				// TODO: Window events, input, etc
				case SDL_QUIT:
				{
					m_done = true;
					break;
				}
				case SDL_KEYDOWN:
				{
#ifdef _DEBUG
					if (io.WantCaptureKeyboard) {
						break;
					}
#endif
					const auto& data = ev.key;
					lastMod          = (SDL_Keymod)data.keysym.mod;
					signalKeyDown( (SDL_KeyCode)data.keysym.sym, lastMod );
					break;
				}
				case SDL_KEYUP:
				{
#ifdef _DEBUG
					if (io.WantCaptureKeyboard) {
						break;
					}
#endif
					const auto& data = ev.key;
					lastMod          = (SDL_Keymod)data.keysym.mod;
					signalKeyUp( (SDL_KeyCode)data.keysym.sym, lastMod );
					break;
				}
				case SDL_MOUSEBUTTONDOWN:
				{
#ifdef _DEBUG
					if (io.WantCaptureMouse) {
						break;
					}
#endif
					const auto& data = ev.button;
					signalMouseDown( data.button, data.x, data.y );
					switch ( data.button )
					{
						case SDL_BUTTON_LEFT:
						{
							signalLeftClick( lastMod & KMOD_SHIFT, lastMod & KMOD_CTRL );
							break;
						}
						case SDL_BUTTON_RIGHT:
						{
							signalRightClick();
							break;
						}
					}
					break;
				}
				case SDL_MOUSEBUTTONUP:
				{
#ifdef _DEBUG
					if (io.WantCaptureMouse) {
						break;
					}
#endif
					const auto& data = ev.button;
					signalMouseUp( data.button, data.x, data.y );
					break;
				}
				case SDL_MOUSEMOTION:
				{
#ifdef _DEBUG
					if (io.WantCaptureMouse) {
						break;
					}
#endif
					const auto& data = ev.motion;
					signalMouse( data.x, data.y, lastMod & KMOD_SHIFT, lastMod & KMOD_CTRL );
					break;
				}
				case SDL_MOUSEWHEEL:
				{
					const auto& data = ev.wheel;
					scrollX = data.preciseX;
					scrollY = data.preciseY;
#ifdef _DEBUG
					if (io.WantCaptureMouse) {
						break;
					}
#endif
					int mX, mY;
					SDL_GetMouseState(&mX, &mY);
					// TODO: Take direction into account
					signalMouseWheel(mX, mY, data.preciseX, data.preciseY);
				}
				case SDL_WINDOWEVENT:
				{
					const auto& data = ev.window;
					switch ((SDL_WindowEventID)data.event) {
						case SDL_WINDOWEVENT_RESIZED: {
							onResize( data.data1, data.data2 );
							break;
						}
					}
					break;
				}
			}
		}
#ifdef _DEBUG
		imguiBeginFrame(scrollX, scrollY, m_sdlWindow, 0xFF);

		debugMan.showDebug();

		imguiEndFrame();
#endif

		m_renderer.render();
		m_noesisContext->Frame();

		bgfx::frame();
	}
}

void SDL_MainWindow::onExit()
{
	m_done = true;
}

void SDL_MainWindow::onResize( int w, int h ) {
	SDL_GL_GetDrawableSize( m_sdlWindow, &m_fbWidth, &m_fbHeight );
	m_width = w;
	m_height = h;

	// TODO: Honor initial HIDPI flag
	bgfx::reset( m_fbWidth, m_fbHeight, BGFX_RESET_VSYNC | BGFX_RESET_HIDPI );

	m_renderer.resize(m_fbWidth, m_fbHeight);
	m_noesisContext->Resize( m_fbWidth, m_fbHeight );
}

void SDL_MainWindow::noesisInit()
{
	spdlog::debug( "noesisInit()" );

	Noesis::GUI::DisableInspector();

	m_noesisContext = new AppGUI::Context( this, guiViewId );

	this->signalMouse.connect( &AppGUI::Events::OnMousePositionCallback );
	this->signalKeyDown.connect( &AppGUI::Events::OnKeyDownCallback );
	this->signalKeyUp.connect( &AppGUI::Events::OnKeyUpCallback );
	this->signalMouseDown.connect( &AppGUI::Events::OnMouseButtonDownCallback );
	this->signalMouseUp.connect( &AppGUI::Events::OnMouseButtonUpCallback );
	this->signalMouseWheel.connect( &AppGUI::Events::OnMouseWheelCallback );
	// Not fired yet
	this->signalChar.connect( &AppGUI::Events::OnCharCallback );

	noesisInstallResourceProviders();

	noesisRegisterComponents();

	m_noesisContext->CreateViewFromFile( "Main.xaml" );

	signalWindowSize( m_width, m_height );
}

void SDL_MainWindow::noesisInstallResourceProviders()
{
	const auto contentPath = Global::cfg->get<std::string>( "dataPath" ) + "/xaml/";
	Noesis::GUI::SetXamlProvider( Noesis::MakePtr<NoesisApp::LocalXamlProvider>( contentPath.c_str() ) );
	Noesis::GUI::SetTextureProvider( Noesis::MakePtr<NoesisApp::LocalTextureProvider>( contentPath.c_str() ) );
	Noesis::GUI::SetFontProvider( Noesis::MakePtr<NoesisApp::LocalFontProvider>( contentPath.c_str() ) );
}

void SDL_MainWindow::noesisRegisterComponents()
{
	NoesisApp::Launcher::RegisterAppComponents();

	Noesis::RegisterComponent<IngnomiaGUI::Main>();
	Noesis::RegisterComponent<IngnomiaGUI::ViewModel>();

	Noesis::RegisterComponent<IngnomiaGUI::MainMenu>();
	Noesis::RegisterComponent<IngnomiaGUI::MainPage>();
	Noesis::RegisterComponent<IngnomiaGUI::SettingsPage>();
	Noesis::RegisterComponent<IngnomiaGUI::SettingsModel>();
	Noesis::RegisterComponent<IngnomiaGUI::LoadGamePage>();
	Noesis::RegisterComponent<IngnomiaGUI::LoadGameModel>();
	Noesis::RegisterComponent<IngnomiaGUI::NewGamePage>();
	Noesis::RegisterComponent<IngnomiaGUI::NewGameModel>();
	Noesis::RegisterComponent<IngnomiaGUI::WaitPage>();
	Noesis::RegisterComponent<IngnomiaGUI::IngamePage>();
	Noesis::RegisterComponent<IngnomiaGUI::GameModel>();
	Noesis::RegisterComponent<IngnomiaGUI::TileInfo>();
	Noesis::RegisterComponent<IngnomiaGUI::TileInfoModel>();
	Noesis::RegisterComponent<IngnomiaGUI::StockpileGui>();
	Noesis::RegisterComponent<IngnomiaGUI::StockpileModel>();
	Noesis::RegisterComponent<IngnomiaGUI::WorkshopGui>();
	Noesis::RegisterComponent<IngnomiaGUI::WorkshopModel>();
	Noesis::RegisterComponent<IngnomiaGUI::Agriculture>();
	Noesis::RegisterComponent<IngnomiaGUI::AgricultureModel>();
	Noesis::RegisterComponent<IngnomiaGUI::PopulationWindow>();
	Noesis::RegisterComponent<IngnomiaGUI::PopulationModel>();
	Noesis::RegisterComponent<IngnomiaGUI::CreatureInfo>();
	Noesis::RegisterComponent<IngnomiaGUI::CreatureInfoModel>();
	Noesis::RegisterComponent<IngnomiaGUI::DebugGui>();
	Noesis::RegisterComponent<IngnomiaGUI::DebugModel>();
	Noesis::RegisterComponent<IngnomiaGUI::NeighborsGui>();
	Noesis::RegisterComponent<IngnomiaGUI::NeighborsModel>();
	Noesis::RegisterComponent<IngnomiaGUI::MilitaryGui>();
	Noesis::RegisterComponent<IngnomiaGUI::MilitaryModel>();
	Noesis::RegisterComponent<IngnomiaGUI::InventoryGui>();
	Noesis::RegisterComponent<IngnomiaGUI::InventoryModel>();
	Noesis::RegisterComponent<IngnomiaGUI::SelectionGui>();
	Noesis::RegisterComponent<IngnomiaGUI::SelectionModel>();

	Noesis::RegisterComponent<Noesis::EnumConverter<IngnomiaGUI::State>>();
	Noesis::RegisterComponent<IngnomiaGUI::ColorToBrushConverter>();
	Noesis::RegisterComponent<IngnomiaGUI::ColorToBrushConverterDark>();

	Noesis::RegisterComponent<IngnomiaGUI::GameGui>();
}

int SDL_MainWindow::getWidth() const
{
	return m_width;
}

int SDL_MainWindow::getHeight() const
{
	return m_height;
}

int SDL_MainWindow::getFBWidth() const
{
	return m_fbWidth;
}

int SDL_MainWindow::getFBHeight() const
{
	return m_fbHeight;
}
