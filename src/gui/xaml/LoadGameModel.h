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
/** @file LoadGameModel.h
 *  @brief View model for the Load Game page. Hosts two observable collections — kingdoms
 *         and per-kingdom save files — and a load command bound to the page button.
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
/// @brief One row in the kingdom or save-file list. Pure data row exposing the path,
///        version, modification date, and compatibility flag for binding.
struct SaveItem : public Noesis::BaseComponent
{
public:
	SaveItem( QString name, QString path, QString dir, QString version, QString date, bool compatible = true );

	Noesis::String _name;     ///< Display name (kingdom name or save filename).
	Noesis::String _path;     ///< Absolute folder path on disk.
	Noesis::String _dir;      ///< Folder basename for the save.
	Noesis::String _version;  ///< Version string read from the save file.
	Noesis::String _date;     ///< Last-modified date as a localised string.
	bool _compatible;         ///< False when the save was produced by an incompatible build.

	NS_DECLARE_REFLECTION( SaveItem, BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Load Game window view model. Lists kingdoms in the left pane and saves in the
///        right pane, with a Load button that loads the selected save.
class LoadGameModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	LoadGameModel();

	/// @brief Replaces the kingdom list with @p kingdoms.
	void updateSavedKingdoms( const QList<GuiSaveInfo>& kingdoms );
	/// @brief Replaces the save-file list with @p kingdoms (saves for the selected kingdom).
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
