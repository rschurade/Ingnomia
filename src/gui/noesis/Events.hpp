#pragma once

#include <NoesisPCH.h>

#include <SDL_keycode.h>

class SDL_MainWindow;

namespace AppGUI::Events
{
extern auto RegisterAll() -> void;

inline constinit Noesis::IView* EventProxy;

extern auto OnKeyDownCallback( SDL_KeyCode key, SDL_Keymod mod ) -> void;
extern auto OnKeyUpCallback( SDL_KeyCode key, SDL_Keymod mod ) -> void;
extern auto OnCharCallback( unsigned codePoint ) -> void;
extern auto OnMousePositionCallback( const int posX, const int posY, bool shift, bool control ) -> void;
extern auto OnMouseButtonDownCallback( const signed button, const int32_t x, const int32_t y ) -> void;
extern auto OnMouseButtonUpCallback( const signed button, const int32_t x, const int32_t y ) -> void;
extern auto OnMouseWheelCallback(const int posX, const int posY, const float rotX, const float rotY) -> void;

	[[nodiscard]] extern auto TranslateKey( SDL_KeyCode key ) noexcept -> Noesis::Key;
[[nodiscard]] extern auto TranslateMouseButton( int button ) noexcept -> Noesis::MouseButton;
} // namespace AppGUI::Events
