#include "HoverTest.hpp"

#include <SDL_mouse.h>

using namespace Noesis;

namespace AppGUI
{
auto HoverTest( const Visual* const root ) -> bool
{
	Visual* const rootElem { VisualTreeHelper::GetRoot( root ) };
	Pointi cursorPos;
	SDL_GetMouseState(&cursorPos.x, &cursorPos.y);
	const Point point { static_cast<float>(cursorPos.x), static_cast<float>(cursorPos.y) };
	const HitTestResult hit { VisualTreeHelper::HitTest( rootElem, point ) };
	return hit.visualHit != nullptr;
}
} // namespace AppGUI
