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
#ifndef __MENU3D_LOADGAMEMODEL_H__
#define __MENU3D_LOADGAMEMODEL_H__

#include "../aggregatorloadgame.h"

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>

#include <QString>

class LoadGameProxy;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

////////////////////////////////////////////////////////////////////////////////////////////////////
struct SaveItem : public Noesis::BaseComponent
{
public:
	SaveItem( QString name, QString path, QString dir, QString version, QString date, bool compatible = true );

	Noesis::String _name;
	Noesis::String _path;
	Noesis::String _dir;
	Noesis::String _version;
	Noesis::String _date;
	bool _compatible;

	NS_DECLARE_REFLECTION( SaveItem, BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class LoadGameModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	LoadGameModel();

	void updateSavedKingdoms( const QList<GuiSaveInfo>& kingdoms );
	void updateSaveGames( const QList<GuiSaveInfo>& kingdoms );

private:
	LoadGameProxy* m_proxy = nullptr;

	const NoesisApp::DelegateCommand* GetLoadGame() const;

	void OnLoadGame( BaseComponent* param );

	Noesis::ObservableCollection<SaveItem>* GetSavedKingdoms() const;
	SaveItem* _selectedKingdom;

	Noesis::ObservableCollection<SaveItem>* GetSavedGames() const;
	SaveItem* _selectedGame;

	void SetSelectedKingdom( SaveItem* item );
	SaveItem* GetSelectedKingdom() const;
	void SetSelectedGame( SaveItem* item );
	SaveItem* GetSelectedGame() const;

	NoesisApp::DelegateCommand _loadGame;

	Noesis::Ptr<Noesis::ObservableCollection<SaveItem>> _savedKingdoms;
	Noesis::Ptr<Noesis::ObservableCollection<SaveItem>> _savedGames;

	NS_DECLARE_REFLECTION( LoadGameModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
