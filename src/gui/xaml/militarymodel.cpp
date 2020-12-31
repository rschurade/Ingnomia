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
#include "militarymodel.h"
#include "militaryproxy.h"

#include <NsApp/Application.h>
#include <NsCore/Log.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/ReflectionImplementEnum.h>
#include <NsGui/ObservableCollection.h>
#include <NsGui/UIElement.h>

#include <QDebug>

using namespace IngnomiaGUI;
using namespace Noesis;
using namespace NoesisApp;

////////////////////////////////////////////////////////////////////////////////////////////////////
SquadPriority::SquadPriority( const GuiTargetPriority& prio, unsigned int squadID, MilitaryProxy* proxy ) :
	m_name( prio.name.toStdString().c_str() ),
	m_idString( prio.id.toStdString().c_str() ),
	m_attitude( prio.attitude ),
	m_squadID( squadID ),
	m_proxy( proxy )
{
	m_moveUpCmd.SetExecuteFunc( MakeDelegate( this, &SquadPriority::onMoveUpCmd ) );
	m_moveDownCmd.SetExecuteFunc( MakeDelegate( this, &SquadPriority::onMoveDownCmd ) );
}


void SquadPriority::setDefend( bool value )
{
	if( value && m_attitude != MilAttitude::DEFEND )
	{
		m_attitude = MilAttitude::DEFEND;
		m_proxy->setAttitude( m_squadID, m_idString.Str(), m_attitude );
		OnPropertyChanged( "Flee" );
		OnPropertyChanged( "Defend" );
		OnPropertyChanged( "Attack" );
		OnPropertyChanged( "Hunt" );
	}
}

void SquadPriority::setFlee( bool value )
{
	if( value && m_attitude != MilAttitude::FLEE )
	{
		m_attitude = MilAttitude::FLEE;
		m_proxy->setAttitude( m_squadID, m_idString.Str(), m_attitude );
		OnPropertyChanged( "Flee" );
		OnPropertyChanged( "Defend" );
		OnPropertyChanged( "Attack" );
		OnPropertyChanged( "Hunt" );
	}
}

void SquadPriority::setAttack( bool value )
{
	if( value && m_attitude != MilAttitude::ATTACK )
	{
		m_attitude = MilAttitude::ATTACK;
		m_proxy->setAttitude( m_squadID, m_idString.Str(), m_attitude );
		OnPropertyChanged( "Flee" );
		OnPropertyChanged( "Defend" );
		OnPropertyChanged( "Attack" );
		OnPropertyChanged( "Hunt" );
	}
}

void SquadPriority::setHunt( bool value )
{
	if( value && m_attitude != MilAttitude::HUNT )
	{
		m_attitude = MilAttitude::HUNT;
		m_proxy->setAttitude( m_squadID, m_idString.Str(), m_attitude );
		OnPropertyChanged( "Flee" );
		OnPropertyChanged( "Defend" );
		OnPropertyChanged( "Attack" );
		OnPropertyChanged( "Hunt" );
	}
}

void SquadPriority::onMoveUpCmd( BaseComponent* param )
{
	if( param )
	{
		m_proxy->movePrioUp( m_squadID, param->ToString().Str() );
	}
}

void SquadPriority::onMoveDownCmd( BaseComponent* param )
{
	if( param )
	{
		m_proxy->movePrioDown( m_squadID, param->ToString().Str() );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
SquadGnome::SquadGnome( const GuiSquadGnome& gnome, bool showLeft, bool showRight, bool showX, MilitaryProxy* proxy ) :
	m_id( gnome.id ),
	m_idString( QString::number( gnome.id ).toStdString().c_str() ),
	m_name( gnome.name.toStdString().c_str() ),
	m_showLeftArrow( showLeft ),
	m_showRightArrow( showRight ),
	m_showX( showX ),
	m_proxy( proxy ),
	m_roleID( gnome.roleID )
{
}

RoleItem* SquadGnome::getRole() const
{
	return m_role;
}
	
void SquadGnome::setRole( RoleItem* role )
{
	if( m_role != role )
	{
		m_role = role;
		m_proxy->setRole( m_id, role->getID() );
		OnPropertyChanged( "Role" );
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
SquadItem::SquadItem( const GuiSquad& squad, MilitaryProxy* proxy ) :
	m_id( squad.id ),
	m_idString( QString::number( squad.id ).toStdString().c_str() ),
	m_name( squad.name.toStdString().c_str() ),
	m_showLeftArrow( squad.showLeftArrow ),
	m_showRightArrow( squad.showRightArrow ),
	m_proxy( proxy )
{
	m_gnomes = *new ObservableCollection<SquadGnome>();
	for( const auto& gnome: squad.gnomes )
	{
		m_gnomes->Add( MakePtr<SquadGnome>( gnome, m_showLeftArrow, m_showRightArrow || ( m_id == 0 ), ( m_id != 0 ),  m_proxy ) );
	}

	m_priorities = *new ObservableCollection<SquadPriority>();
	for( const auto& prio : squad.priorities )
	{
		m_priorities->Add( MakePtr<SquadPriority>( prio, squad.id, proxy ) );
	}

	m_showConfigCmd.SetExecuteFunc( MakeDelegate( this, &SquadItem::onShowConfigCmd ) );
}

void SquadItem::setName( const char* value )
{
	m_name = value;
	m_proxy->renameSquad( m_id, m_name.Str() );
	//OnPropertyChanged( "Name" );
}

void SquadItem::onShowConfigCmd( BaseComponent* param )
{
	m_showConfig = !m_showConfig;
	OnPropertyChanged( "ShowConfig" );
}

void SquadItem::updatePriorities( const QList<GuiTargetPriority>& prios )
{
	m_priorities->Clear();
	for( const auto& prio : prios )
	{
		m_priorities->Add( MakePtr<SquadPriority>( prio, m_id, m_proxy ) );
	}
	OnPropertyChanged( "Priorities" );
}

UniformModelMaterial::UniformModelMaterial( QString sid, QString name ) :
	m_name( name.toStdString().c_str() ),
	m_sid( sid.toStdString().c_str() )
{

}



UniformModelType::UniformModelType( QString sid, QString name ) :
	m_name( name.toStdString().c_str() ),
	m_sid( sid.toStdString().c_str() )
{

}

UniformModelItem::UniformModelItem( const GuiUniformItem& gui, unsigned int roleID, MilitaryProxy* proxy ) :
	m_name( gui.slotName.toStdString().c_str() ),
	m_sid( gui.slotName.toStdString().c_str() ),
	m_mat( gui.material ),
	m_roleID( roleID ),
	m_proxy( proxy )
{
	m_availableTypes = *new ObservableCollection<UniformModelType>();
	m_availableMats = *new ObservableCollection<UniformModelMaterial>();

	int selectedIndex = -1;
	int i = -1;
	for( auto type : gui.possibleTypesForSlot )
	{
		++i;
		if( type == gui.armorType )
		{
			selectedIndex = i;
		}
		m_availableTypes->Add( MakePtr<UniformModelType>( type, type ) );
	}
	if( selectedIndex != -1 )
	{
		setSelectedType( m_availableTypes->Get( selectedIndex ) );
		m_mat = gui.material;
	}
}

void UniformModelItem::setSelectedType( UniformModelType* type )
{
	if ( m_selectedType != type && type != nullptr )
	{
		m_selectedType = type;
		m_mat = "any";
		m_proxy->setArmorType( m_roleID, m_sid.Str(), type->sid(), "any" );
		
		//OnPropertyChanged( "SelectedType" );
	}
}

void UniformModelItem::setSelectedMaterial( UniformModelMaterial* mat )
{
	if ( m_selectedMat != mat && mat != nullptr )
	{
		m_selectedMat = mat;
		m_mat = mat->sid();
		if( m_selectedType )
		{
			m_proxy->setArmorType( m_roleID, m_sid.Str(), m_selectedType->sid(), mat->sid() );
		}
		//OnPropertyChanged( "SelectedType" );
	}
}

void UniformModelItem::setPossibleMats( QStringList mats )
{
	m_availableMats->Clear();
	int selectedMat = -1;
	
	for( int i = 0; i < mats.size(); ++i )
	{
		QString mat = mats[i];
		m_availableMats->Add( MakePtr<UniformModelMaterial>( mat, mat ) );
		if( mat == m_mat )
		{
			selectedMat = i;
		}
	}
	if( selectedMat != -1 )
	{
		m_selectedMat = m_availableMats->Get( selectedMat );
	}
	OnPropertyChanged( "AvailableMaterials" );
	OnPropertyChanged( "SelectedMaterial" );
}


////////////////////////////////////////////////////////////////////////////////////////////////////
RoleItem::RoleItem( const GuiMilRole& role, MilitaryProxy* proxy ) :
	m_id( role.id ),
	m_idString( QString::number( role.id ).toStdString().c_str() ),
	m_name( role.name.toStdString().c_str() ),
	m_showLeftArrow( role.showLeftArrow ),
	m_showRightArrow( role.showRightArrow ),
	m_proxy( proxy )
{
	m_showConfigCmd.SetExecuteFunc( MakeDelegate( this, &RoleItem::onShowConfigCmd ) );

	m_uniformItems = *new ObservableCollection<UniformModelItem>();

	for( const auto& gui : role.uniform )
	{
		m_uniformItems->Add( MakePtr<UniformModelItem>( gui, m_id, m_proxy ) );
	}

}

void RoleItem::setName( const char* value )
{
	m_name = value;
	m_proxy->renameRole( m_id, m_name.Str() );
	//OnPropertyChanged( "Name" );
}

void RoleItem::onShowConfigCmd( BaseComponent* param )
{
	m_showConfig = !m_showConfig;
	OnPropertyChanged( "ShowConfig" );
}

void RoleItem::updatePossibleMaterials( QString slot, QStringList mats )
{
	for( int i = 0; i < m_uniformItems->Count(); ++i )
	{
		auto item = m_uniformItems->Get( i );
		if( item->sid() == slot )
		{
			item->setPossibleMats( mats );
			break;
		}
	}
}

bool RoleItem::GetCivilian() const
{
	return m_civilian;
}
	
void RoleItem::SetCivilian( bool value )
{
	if( m_civilian != value )
	{
		m_civilian = value;
		m_proxy->setRoleCivilian( m_id, value );
	}
}




////////////////////////////////////////////////////////////////////////////////////////////////////
MilitaryModel::MilitaryModel()
{
	m_proxy = new MilitaryProxy;
	m_proxy->setParent( this );

	m_pageCmd.SetExecuteFunc( MakeDelegate( this, &MilitaryModel::onPageCmd ) );
	m_addSquadCmd.SetExecuteFunc( MakeDelegate( this, &MilitaryModel::onAddSquadCmd ) );
	m_removeSquadCmd.SetExecuteFunc( MakeDelegate( this, &MilitaryModel::onRemoveSquadCmd ) );
	m_moveSquadLeftCmd.SetExecuteFunc( MakeDelegate( this, &MilitaryModel::onMoveSquadLeftCmd ) );
	m_moveSquadRightCmd.SetExecuteFunc( MakeDelegate( this, &MilitaryModel::onMoveSquadRightCmd ) );

	m_removeGnomeFromSquadCmd.SetExecuteFunc( MakeDelegate( this, &MilitaryModel::onRemoveGnomeFromSquadCmd ) );
	m_moveGnomeLeftCmd.SetExecuteFunc( MakeDelegate( this, &MilitaryModel::onMoveGnomeLeftCmd ) );
	m_moveGnomeRightCmd.SetExecuteFunc( MakeDelegate( this, &MilitaryModel::onMoveGnomeRightCmd ) );
	
	m_addRoleCmd.SetExecuteFunc( MakeDelegate( this, &MilitaryModel::onAddRoleCmd ) );
	m_removeRoleCmd.SetExecuteFunc( MakeDelegate( this, &MilitaryModel::onRemoveRoleCmd ) );


	m_squads = *new ObservableCollection<SquadItem>();
	m_roles = *new ObservableCollection<RoleItem>();
}

const char* MilitaryModel::getShowFirst() const
{
	if ( m_page == MilitaryPage::First )
	{
		return "Visible";
	}
	return "Hidden";
}
	
const char* MilitaryModel::getShowSecond() const
{
	if ( m_page == MilitaryPage::Second )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* MilitaryModel::getShowThird() const
{
	if ( m_page == MilitaryPage::Third )
	{
		return "Visible";
	}
	return "Hidden";
}

void MilitaryModel::onPageCmd( BaseComponent* param )
{
	if( param->ToString() == "First" )
	{
		m_page = MilitaryPage::First;
	}
	else if( param->ToString() == "Second" )
	{
		m_proxy->requestRoles();
		m_page = MilitaryPage::Second;
	}
	else
	{
		m_page = MilitaryPage::Third;
	}
	
	OnPropertyChanged( "ShowFirst" );
	OnPropertyChanged( "ShowSecond" );
	OnPropertyChanged( "ShowThird" );
}

void MilitaryModel::updateSquads( const QList<GuiSquad>& squads )
{
	m_squads->Clear();

	for( const auto& squad : squads )
	{
		m_squads->Add( MakePtr<SquadItem>( squad, m_proxy ) );
	}
	m_proxy->requestRoles();

	OnPropertyChanged( "SquadList" );
}

void MilitaryModel::updateRoles( const QList<GuiMilRole>& roles )
{
	for( int r = 0; r < m_roles->Count(); ++r )
	{
		auto roleItem = m_roles->Get( r );

		for( int i = 0; i < m_squads->Count(); ++i )
		{
			auto squad = m_squads->Get( i );
			auto gnomes = squad->getGnomes();
			for( int k = 0; k < gnomes->Count(); ++k )
			{
				auto gnome = gnomes->Get( k );
				gnome->nullifyRole();
			}
		}
	}

	m_roles->Clear();

	for( const auto& role : roles )
	{
		m_roles->Add( MakePtr<RoleItem>( role, m_proxy ) );
	}
	
	for( int r = 0; r < m_roles->Count(); ++r )
	{
		auto roleItem = m_roles->Get( r );

		for( int i = 0; i < m_squads->Count(); ++i )
		{
			auto squad = m_squads->Get( i );
			auto gnomes = squad->getGnomes();
			for( int k = 0; k < gnomes->Count(); ++k )
			{
				auto gnome = gnomes->Get( k );
				if( gnome->roleID() == roleItem->getID() )
				{
					gnome->setRole( roleItem );
				}
			}
		}
	}
	OnPropertyChanged( "RoleList" );
}

void MilitaryModel::onAddSquadCmd( BaseComponent* param )
{
	m_proxy->addSquad();
}

void MilitaryModel::onRemoveSquadCmd( BaseComponent* param )
{
	if( param )
	{
		QString qID( param->ToString().Str() );
		unsigned int id = qID.toUInt();
		m_proxy->removeSquad( id );
	}
}

void MilitaryModel::onMoveSquadLeftCmd( BaseComponent* param )
{
	if( param )
	{
		QString qID( param->ToString().Str() );
		unsigned int id = qID.toUInt();
		m_proxy->moveSquadLeft( id );
	}
}

void MilitaryModel::onMoveSquadRightCmd( BaseComponent* param )
{
	if( param )
	{
		QString qID( param->ToString().Str() );
		unsigned int id = qID.toUInt();
		m_proxy->moveSquadRight( id );
	}
}

void MilitaryModel::onRemoveGnomeFromSquadCmd( BaseComponent* param )
{
	if( param )
	{
		QString qID( param->ToString().Str() );
		unsigned int id = qID.toUInt();
		m_proxy->removeGnomeFromSquad( id );
	}
}

void MilitaryModel::onMoveGnomeLeftCmd( BaseComponent* param )
{
	if( param )
	{
		QString qID( param->ToString().Str() );
		unsigned int id = qID.toUInt();
		m_proxy->moveGnomeLeft( id );
	}
}

void MilitaryModel::onMoveGnomeRightCmd( BaseComponent* param )
{
	if( param )
	{
		QString qID( param->ToString().Str() );
		unsigned int id = qID.toUInt();
		m_proxy->moveGnomeRight( id );
	}
}

void MilitaryModel::updatePriorities( unsigned int squadID, const QList<GuiTargetPriority>& priorities )
{
	for( int i = 0; i < m_squads->Count(); ++i )
	{
		auto squad = m_squads->Get( i );
		if( squad->getID() == squadID )
		{
			squad->updatePriorities( priorities );
			break;
		}
	}
}


void MilitaryModel::onAddRoleCmd( BaseComponent* param )
{
	m_proxy->addRole();
}

void MilitaryModel::onRemoveRoleCmd( BaseComponent* param )
{
	if( param )
	{
		QString qID( param->ToString().Str() );
		unsigned int id = qID.toUInt();
		m_proxy->removeRole( id );
	}
}

void MilitaryModel::updatePossibleMaterials( unsigned int roleID, const QString slot, const QStringList mats )
{
	for( int i = 0; i < m_roles->Count(); ++i )
	{
		if( m_roles->Get( i )->getID() == roleID )
		{
			m_roles->Get( i )->updatePossibleMaterials( slot, mats );

			break;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( MilitaryModel, "IngnomiaGUI.MilitaryModel" )
{
	NsProp( "PageCmd", &MilitaryModel::getPageCmd );
	NsProp( "ShowFirst", &MilitaryModel::getShowFirst );
	NsProp( "ShowSecond", &MilitaryModel::getShowSecond );
	NsProp( "ShowThird", &MilitaryModel::getShowThird );

	NsProp( "SquadList", &MilitaryModel::getSquads );
	NsProp( "AddSquadCmd", &MilitaryModel::getAddSquadCmd );
	NsProp( "RemoveSquadCmd", &MilitaryModel::getRemoveSquadCmd );
	NsProp( "MoveSquadLeftCmd", &MilitaryModel::getMoveSquadLeftCmd );
	NsProp( "MoveSquadRightCmd", &MilitaryModel::getMoveSquadRightCmd );
	NsProp( "RemoveGnomeFromSquadCmd", &MilitaryModel::getRemoveGnomeFromSquadCmd );
	NsProp( "MoveGnomeLeftCmd", &MilitaryModel::getMoveGnomeLeftCmd );
	NsProp( "MoveGnomeRightCmd", &MilitaryModel::getMoveGnomeRightCmd );

	NsProp( "RoleList", &MilitaryModel::getRoles );
	NsProp( "AddRoleCmd", &MilitaryModel::getAddRoleCmd );
	NsProp( "RemoveRoleCmd", &MilitaryModel::getRemoveRoleCmd );
}

NS_IMPLEMENT_REFLECTION( SquadItem, "IngnomiaGUI.SquadItem" )
{
	NsProp( "ID", &SquadItem::getIDString );
	NsProp( "Name", &SquadItem::getName, &SquadItem::setName );
	NsProp( "Gnomes", &SquadItem::getGnomes );
	NsProp( "Priorities", &SquadItem::getPriorities );
	NsProp( "ShowLeftArrow", &SquadItem::getShowLeftArrow );
	NsProp( "ShowRightArrow", &SquadItem::getShowRightArrow );
	NsProp( "ShowX", &SquadItem::getShowX );
	NsProp( "ConfigureSquadCmd", &SquadItem::getShowConfigCmd );
	NsProp( "ShowConfig", &SquadItem::getShowConfig );
}

NS_IMPLEMENT_REFLECTION( SquadGnome, "IngnomiaGUI.SquadGnome" )
{
	NsProp( "ID", &SquadGnome::getIDString );
	NsProp( "Name", &SquadGnome::getName );
	
	NsProp( "ShowLeftArrow", &SquadGnome::getShowLeftArrow );
	NsProp( "ShowRightArrow", &SquadGnome::getShowRightArrow );
	NsProp( "ShowX", &SquadGnome::getShowX );
	NsProp( "Role", &SquadGnome::getRole, &SquadGnome::setRole );
}

NS_IMPLEMENT_REFLECTION( SquadPriority, "IngnomiaGUI.SquadPriority" )
{
	NsProp( "Name", &SquadPriority::getName );
	NsProp( "ID", &SquadPriority::getIDString );
	NsProp( "Flee", &SquadPriority::getFlee, &SquadPriority::setFlee );
	NsProp( "Defend", &SquadPriority::getDefend, &SquadPriority::setDefend );
	NsProp( "Attack", &SquadPriority::getAttack, &SquadPriority::setAttack );
	NsProp( "Hunt", &SquadPriority::getHunt, &SquadPriority::setHunt );
	NsProp( "MoveUpCmd", &SquadPriority::getMoveUpCmd );
	NsProp( "MoveDownCmd", &SquadPriority::getMoveDownCmd );
	
}

NS_IMPLEMENT_REFLECTION( RoleItem, "IngnomiaGUI.RoleItem" )
{
	NsProp( "ID", &RoleItem::getIDString );
	NsProp( "Name", &RoleItem::getName, &RoleItem::setName );
	NsProp( "ShowLeftArrow", &RoleItem::getShowLeftArrow );
	NsProp( "ShowRightArrow", &RoleItem::getShowRightArrow );
	NsProp( "ShowX", &RoleItem::getShowX );
	NsProp( "ConfigureUniformCmd", &RoleItem::getShowConfigCmd );
	NsProp( "ShowConfig", &RoleItem::getShowConfig );
	NsProp( "UniformItems", &RoleItem::getUniformItems );
	NsProp( "Civilian", &RoleItem::GetCivilian, &RoleItem::SetCivilian );
}

NS_IMPLEMENT_REFLECTION( UniformModelItem, "IngnomiaGUI.UniformModelItem" )
{
	NsProp( "ID", &UniformModelItem::sid );
	NsProp( "Name", &UniformModelItem::getName );
	NsProp( "SelectedType", &UniformModelItem::getSelectedType, &UniformModelItem::setSelectedType );
	NsProp( "SelectedMaterial", &UniformModelItem::getSelectedMaterial, &UniformModelItem::setSelectedMaterial );
	NsProp( "AvailableTypes", &UniformModelItem::availableTypes );
	NsProp( "AvailableMaterials", &UniformModelItem::availableMaterials );
}

NS_IMPLEMENT_REFLECTION( UniformModelType, "IngnomiaGUI.UniformModelType" )
{
	NsProp( "ID", &UniformModelType::sid );
	NsProp( "Name", &UniformModelType::getName );
}

NS_IMPLEMENT_REFLECTION( UniformModelMaterial, "IngnomiaGUI.UniformModelMaterial" )
{
	NsProp( "ID", &UniformModelMaterial::sid );
	NsProp( "Name", &UniformModelMaterial::getName );
}