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

struct LanguageEntry : public Noesis::BaseComponent
{
public:
	LanguageEntry( QString name );

	Noesis::String m_name;

    const char* getName() const;
	
	NS_DECLARE_REFLECTION( LanguageEntry, BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class SettingsModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	SettingsModel();

    void updateSettings( const GuiSettings& settings );

private:
    SettingsProxy* m_proxy = nullptr;

    Noesis::ObservableCollection<LanguageEntry>* getLanguages() const;
	void setLanguage( LanguageEntry* item );
	LanguageEntry* getLanguage() const;

    int getScale() const;
	void setScale( int value );

    bool getFullScreen() const;
	void setFullScreen( bool value );


    Noesis::Ptr<Noesis::ObservableCollection<LanguageEntry>> m_languages;
    LanguageEntry* m_selectedLanguage = nullptr;;

    int m_scale = 10;
    bool m_fullScreen = false;

	NS_DECLARE_REFLECTION( SettingsModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
