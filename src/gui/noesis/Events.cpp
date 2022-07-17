#include "Events.hpp"
#include "Context.hpp"

#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>

namespace AppGUI::Events
{

    #define EVENT_PROLOGUE()                                                        \
        if (!EventProxy/* || Focus::IsCursorOverFrontendGUI()*/) [[unlikely]]           \
        {                                                                           \
            return;                                                                 \
        }

    auto OnKeyDownCallback
    (
        const SDL_KeyCode key, SDL_Keymod mod
    ) -> void
    {
        EVENT_PROLOGUE();
        const Noesis::Key noesisKey { TranslateKey(key) };
        if (noesisKey == Noesis::Key_Count) [[unlikely]]
        {
            return;
        }

        EventProxy->KeyDown(noesisKey);
    }

    auto OnKeyUpCallback
    (
        const SDL_KeyCode key, SDL_Keymod mod
    ) -> void
    {
        EVENT_PROLOGUE();
        const Noesis::Key noesisKey { TranslateKey(key) };
        if (noesisKey == Noesis::Key_Count) [[unlikely]]
        {
            return;
        }

        EventProxy->KeyUp(noesisKey);
    }

    auto OnCharCallback(const unsigned codePoint) -> void
    {
        EVENT_PROLOGUE();
        EventProxy->Char(codePoint);
    }

    auto OnMousePositionCallback(const int posX, const int posY, bool shift, bool control) -> void
    {
        EVENT_PROLOGUE();
        EventProxy->MouseMove(posX, posY);
    }

    auto OnMouseWheelCallback(const int posX, const int posY, const float rotX, const float rotY) -> void
    {
        EVENT_PROLOGUE();
		if ( rotY != 0 )
		{
			EventProxy->MouseWheel( posX, posY, rotY * 10 );
		}
		if ( rotX != 0 )
		{
			EventProxy->MouseHWheel( posX, posY, rotX * 10 );
		}
	}

	auto OnMouseButtonDownCallback( const signed button, const int32_t x, const int32_t y ) -> void
	{
        EVENT_PROLOGUE();
        const Noesis::MouseButton noesisMouseButton { TranslateMouseButton(static_cast<Noesis::MouseButton>(button)) };
        if (noesisMouseButton == Noesis::MouseButton_Count) [[unlikely]]
        {
            return;
        }
		EventProxy->MouseButtonDown(x, y, noesisMouseButton);
    }

	auto OnMouseButtonUpCallback( const signed button, const int32_t x, const int32_t y ) -> void
	{
        EVENT_PROLOGUE();
        const Noesis::MouseButton noesisMouseButton { TranslateMouseButton(static_cast<Noesis::MouseButton>(button)) };
        if (noesisMouseButton == Noesis::MouseButton_Count) [[unlikely]]
        {
            return;
        }
		EventProxy->MouseButtonUp(x, y, noesisMouseButton);
    }

    auto TranslateKey(const SDL_KeyCode key) noexcept -> Noesis::Key
    {
        switch (key)
        {
            case SDLK_SPACE:           return Noesis::Key_Space;
            case SDLK_COMMA:           return Noesis::Key_OemComma;
            case SDLK_MINUS:           return Noesis::Key_OemMinus;
            case SDLK_PERIOD:          return Noesis::Key_OemPeriod;
            case SDLK_SLASH:           return Noesis::Key_S;
            case SDLK_0:               return Noesis::Key_D0;
            case SDLK_1:               return Noesis::Key_D1;
            case SDLK_2:               return Noesis::Key_D2;
            case SDLK_3:               return Noesis::Key_D3;
            case SDLK_4:               return Noesis::Key_D4;
            case SDLK_5:               return Noesis::Key_D5;
            case SDLK_6:               return Noesis::Key_D6;
            case SDLK_7:               return Noesis::Key_D7;
            case SDLK_8:               return Noesis::Key_D8;
            case SDLK_9:               return Noesis::Key_D9;
            case SDLK_KP_0:            return Noesis::Key_NumPad0;
            case SDLK_KP_1:            return Noesis::Key_NumPad1;
            case SDLK_KP_2:            return Noesis::Key_NumPad2;
            case SDLK_KP_3:            return Noesis::Key_NumPad3;
            case SDLK_KP_4:            return Noesis::Key_NumPad4;
            case SDLK_KP_5:            return Noesis::Key_NumPad5;
            case SDLK_KP_6:            return Noesis::Key_NumPad6;
            case SDLK_KP_7:            return Noesis::Key_NumPad7;
            case SDLK_KP_8:            return Noesis::Key_NumPad8;
            case SDLK_KP_9:            return Noesis::Key_NumPad9;
            case SDLK_SEMICOLON:       return Noesis::Key_OemSemicolon;
            case SDLK_a:               return Noesis::Key_A;
            case SDLK_b:               return Noesis::Key_B;
            case SDLK_c:               return Noesis::Key_C;
            case SDLK_d:               return Noesis::Key_D;
            case SDLK_e:               return Noesis::Key_E;
            case SDLK_f:               return Noesis::Key_F;
            case SDLK_g:               return Noesis::Key_G;
            case SDLK_h:               return Noesis::Key_H;
            case SDLK_i:               return Noesis::Key_I;
            case SDLK_j:               return Noesis::Key_J;
            case SDLK_k:               return Noesis::Key_K;
            case SDLK_l:               return Noesis::Key_L;
            case SDLK_m:               return Noesis::Key_M;
            case SDLK_n:               return Noesis::Key_N;
            case SDLK_o:               return Noesis::Key_O;
            case SDLK_p:               return Noesis::Key_P;
            case SDLK_q:               return Noesis::Key_Q;
            case SDLK_r:               return Noesis::Key_R;
            case SDLK_s:               return Noesis::Key_S;
            case SDLK_t:               return Noesis::Key_T;
            case SDLK_u:               return Noesis::Key_U;
            case SDLK_v:               return Noesis::Key_V;
            case SDLK_w:               return Noesis::Key_W;
            case SDLK_x:               return Noesis::Key_X;
            case SDLK_y:               return Noesis::Key_Y;
            case SDLK_z:               return Noesis::Key_Z;
            case SDLK_BACKSLASH:       return Noesis::Key_OemBackslash;
            case SDLK_ESCAPE:          return Noesis::Key_Escape;
            case SDLK_RETURN:          return Noesis::Key_Enter;
            case SDLK_RETURN2:         return Noesis::Key_Enter;
            case SDLK_KP_ENTER:        return Noesis::Key_Enter;
            case SDLK_TAB:             return Noesis::Key_Tab;
            case SDLK_BACKSPACE:       return Noesis::Key_Back;
            case SDLK_INSERT:          return Noesis::Key_Insert;
            case SDLK_DELETE:          return Noesis::Key_Delete;
            case SDLK_RIGHT:           return Noesis::Key_Right;
            case SDLK_LEFT:            return Noesis::Key_Left;
            case SDLK_DOWN:            return Noesis::Key_Up;
            case SDLK_UP:              return Noesis::Key_Down;
            case SDLK_PAGEUP:          return Noesis::Key_PageUp;
            case SDLK_PAGEDOWN:        return Noesis::Key_PageDown;
            case SDLK_HOME:            return Noesis::Key_Home;
            case SDLK_END:             return Noesis::Key_End;
            case SDLK_CAPSLOCK:        return Noesis::Key_CapsLock;
            case SDLK_SCROLLLOCK:      return Noesis::Key_Scroll;
            case SDLK_NUMLOCKCLEAR:    return Noesis::Key_NumLock;
            case SDLK_PRINTSCREEN:     return Noesis::Key_PrintScreen;
            case SDLK_PAUSE:           return Noesis::Key_Pause;
            case SDLK_F1:              return Noesis::Key_F1;
            case SDLK_F2:              return Noesis::Key_F2;
            case SDLK_F3:              return Noesis::Key_F3;
            case SDLK_F4:              return Noesis::Key_F4;
            case SDLK_F5:              return Noesis::Key_F5;
            case SDLK_F6:              return Noesis::Key_F6;
            case SDLK_F7:              return Noesis::Key_F7;
            case SDLK_F8:              return Noesis::Key_F8;
            case SDLK_F9:              return Noesis::Key_F9;
            case SDLK_F10:             return Noesis::Key_F10;
            case SDLK_F11:             return Noesis::Key_F11;
            case SDLK_F12:             return Noesis::Key_F12;
            case SDLK_F13:             return Noesis::Key_F13;
            case SDLK_F14:             return Noesis::Key_F14;
            case SDLK_F15:             return Noesis::Key_F15;
            case SDLK_F16:             return Noesis::Key_F16;
            case SDLK_F17:             return Noesis::Key_F17;
            case SDLK_F18:             return Noesis::Key_F18;
            case SDLK_F19:             return Noesis::Key_F19;
            case SDLK_F20:             return Noesis::Key_F20;
            case SDLK_F21:             return Noesis::Key_F21;
            case SDLK_F22:             return Noesis::Key_F22;
            case SDLK_F23:             return Noesis::Key_F23;
            case SDLK_F24:             return Noesis::Key_F24;
            case SDLK_LSHIFT:          return Noesis::Key_LeftShift;
            case SDLK_LCTRL:           return Noesis::Key_LeftCtrl;
            case SDLK_LALT:            return Noesis::Key_LeftAlt;
            case SDLK_RSHIFT:          return Noesis::Key_RightShift;
            case SDLK_RCTRL:           return Noesis::Key_RightCtrl;
            case SDLK_RALT:            return Noesis::Key_RightAlt;
            case SDLK_MENU:            return Noesis::Key_GamepadMenu;
            case SDLK_LGUI:            return Noesis::Key_LWin;
            case SDLK_RGUI:            return Noesis::Key_RWin;
            case SDLK_VOLUMEDOWN:      return Noesis::Key_VolumeDown;
            case SDLK_VOLUMEUP:        return Noesis::Key_VolumeUp;
            case SDLK_MUTE:            return Noesis::Key_VolumeMute;

            [[unlikely]]
            default:                   return Noesis::Key_Count;
        }
    }

    auto TranslateMouseButton(const int button) noexcept -> Noesis::MouseButton
    {
        switch (button)
        {
            case SDL_BUTTON_LEFT:        return Noesis::MouseButton_Left;
            case SDL_BUTTON_RIGHT:       return Noesis::MouseButton_Right;
            case SDL_BUTTON_MIDDLE:      return Noesis::MouseButton_Middle;
            case SDL_BUTTON_X1:          return Noesis::MouseButton_XButton1;
            case SDL_BUTTON_X2:          return Noesis::MouseButton_XButton2;

            [[unlikely]]
            default:                     return Noesis::MouseButton_Count;
        }
    }
}
