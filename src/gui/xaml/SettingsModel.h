/*	
	This file is part of Ingnomia https://github.com/rschurade/Ingnomia
    Copyright (C) 2017-2020  Ralph Schurade, Ingnomia Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/** @file SettingsModel.h
 *  @brief View model and helper component types for the in-game Settings page. Exposes
 *         language, UI scale, fullscreen, keyboard speed, light min, mouse wheel mode,
 *         and audio master volume.
 */
#ifndef __SETTINGSMODEL_H__
#define __SETTINGSMODEL_H__

#include "../aggregatorsettings.h"

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>

#include <QString>

class SettingsProxy;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

/// @brief One language entry in the language dropdown (display name only).
struct LanguageEntry : public Noesis::BaseComponent
{
public:
	LanguageEntry( QString name );

	Noesis::String m_name; ///< Display name (e.g. "English", "Deutsch").

    const char* getName() const;

	NS_DECLARE_REFLECTION( LanguageEntry, BaseComponent )
};

/// @brief One UI-scale entry in the scale dropdown (display name + numeric scale factor).
struct ScaleEntry : public Noesis::BaseComponent
{
public:
	ScaleEntry( QString name, float scale );

	Noesis::String m_name; ///< Display name (e.g. "100%", "125%").
    float m_scale;         ///< Scale factor applied to the GUI (1.0 = 100%).

    const char* getName() const;
	
	NS_DECLARE_REFLECTION( ScaleEntry, BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Settings page view model. Exposes language, UI scale, fullscreen, keyboard speed,
///        minimum light level, mouse wheel behaviour, and audio master volume to XAML and
///        forwards changes to the game side via SettingsProxy.
class SettingsModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	SettingsModel();

	/// @brief Applies a fresh GuiSettings snapshot from the game side to the bindable fields.
    void updateSettings( const GuiSettings& settings );

private:
    SettingsProxy* m_proxy = nullptr; ///< Qt-side proxy for signal/slot bridge to the game.

    Noesis::ObservableCollection<LanguageEntry>* getLanguages() const;
	void setLanguage( LanguageEntry* item );
	LanguageEntry* getLanguage() const;

    Noesis::ObservableCollection<ScaleEntry>* getScales() const;
	void setScale( ScaleEntry* item );
	ScaleEntry* getScale() const;

    bool getFullScreen() const;
	void setFullScreen( bool value );

    int getKeyboardSpeed() const;
	void setKeyboardSpeed( int value );

    int getLightMin() const;
	void setLightMin( int value );

    bool getMouseWheelBehavior() const;
	void setMouseWheelBehavior( bool value );
	
	float getAudioMasterVolume() const;
	void setAudioMasterVolume( float value );

    Noesis::Ptr<Noesis::ObservableCollection<LanguageEntry>> m_languages; ///< Available languages bound to the language dropdown.
    LanguageEntry* m_selectedLanguage = nullptr;;                         ///< Currently selected language entry.

    Noesis::Ptr<Noesis::ObservableCollection<ScaleEntry>> m_scales; ///< Available UI scales bound to the scale dropdown.
    ScaleEntry* m_selectedScale = nullptr;;                         ///< Currently selected UI scale entry.

    float m_scale = 1.0f;              ///< Current UI scale factor.
    bool m_fullScreen = false;         ///< Fullscreen window mode.
    bool m_toggleMouseWheel = false;   ///< Mouse wheel behaviour (true = zoom, false = z-level).
    int m_keyboardSpeed = 20;          ///< Keyboard camera pan speed.
    int m_lightMin = 30;               ///< Minimum ambient light level at night.
	 float m_audioMasterVolume = 50.0f; ///< Master audio volume (0-100).

	NS_DECLARE_REFLECTION( SettingsModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
