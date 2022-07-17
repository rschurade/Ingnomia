#pragma once

#include "Renderer/RenderDevice.hpp"

#include <NoesisPCH.h>

class SDL_MainWindow;

namespace AppGUI
{
struct Context final
{
	Context( const SDL_MainWindow* window, bgfx::ViewId guiViewId  );
	~Context();

	auto Frame() -> void;
	auto Resize( std::uint16_t width, std::uint16_t height ) -> void;
	auto CreateViewFromFile( const char* fileName ) -> void;

private:
	const SDL_MainWindow* m_window;
	Noesis::Ptr<RenderDevice> RenderDevice_ {};
	Noesis::Ptr<Noesis::Grid> Root_ {};
	Noesis::Ptr<Noesis::IView> View_ {};
};
} // namespace AppGUI
