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
#include "aggregatormilitary.h"

#include "../base/db.h"
#include "../base/global.h"
#include "../game/gnomemanager.h"

#include "../gui/strings.h"

AggregatorMilitary::AggregatorMilitary( QObject* parent ) :
	QObject(parent)
{
}

AggregatorMilitary::~AggregatorMilitary()
{
}

void AggregatorMilitary::sendSquadUpdate()
{
	m_squads.clear();

	GuiSquad ngs;
	ngs.name = "no squad";
	ngs.id = 0;
	ngs.showLeftArrow = false;
	ngs.showRightArrow = false;

	for( auto gnome : Global::gm().gnomes() )
	{
		if( !Global::mil().getSquadForGnome( gnome->id() ) )
		{
			ngs.gnomes.append( { gnome->id(), gnome->name(), gnome->roleID() } );
		}
	}
	m_squads.append( ngs );

	for( const auto& squad : Global::mil().squads() )
	{
		GuiSquad gs;
		gs.name = squad.name;
		gs.id = squad.id;

		for( const auto& prio : squad.priorities )
		{
			GuiTargetPriority gtp;
			gtp.id = prio.type;
			gtp.name = S::s( "$CreatureName_" + prio.type );
			gtp.attitude = prio.attitude;
			gs.priorities.append( gtp );
		}
		
		for( auto gnomeID : squad.gnomes )
		{
			GuiSquadGnome gsg;
			gsg.id = gnomeID;
			gsg.name = Global::gm().name( gnomeID );
			gsg.roleID = Global::gm().roleID( gnomeID );

			gs.gnomes.append( gsg );
		}
		m_squads.append( gs );
	}

	if ( m_squads.size() > 1 )
	{
		m_squads[1].showLeftArrow = false;
		m_squads.last().showRightArrow = false;

	}

	emit signalSquads( m_squads );
}

void AggregatorMilitary::onRequestRoles()
{
	sendRoleUpdate();
}

void AggregatorMilitary::sendRoleUpdate()
{
	m_roles.clear();

	auto uniformSlots = DB::ids( "Uniform" );

	for( auto& role : Global::mil().roles() )
	{
		GuiMilRole gmr;
		gmr.name = role.name;
		gmr.id = role.id;
		gmr.isCivilian = role.isCivilian;

		auto uniVM = role.uniform.serialize();
		auto items = uniVM.value( "Items" ).toMap();

		for( auto slot : uniformSlots )
		{
			gmr.uniform.append( createUniformItem( slot, items.value( slot ).toMap() ) );
		}
		

		m_roles.append( gmr );
	}
	emit signalRoles( m_roles );
}

GuiUniformItem AggregatorMilitary::createUniformItem( QString slot, QVariantMap uniVM )
{
	GuiUniformItem gui;
	gui.slotName = slot;
	gui.armorType = uniVM.value( "Type" ).toString();
	gui.material = uniVM.value( "Material" ).toString();

	gui.possibleTypesForSlot.append( "none" );

	auto rows = DB::selectRows( "Uniform_Slots", gui.slotName );

	for( auto row : rows )
	{
		gui.possibleTypesForSlot.append( row.value( "Type" ).toString() );
	}

	return gui;
}

void AggregatorMilitary::sendPriorityUpdate( unsigned int squadID )
{
	auto squad = Global::mil().squad( squadID );
	if( squad )
	{
		m_tmpPriorities.clear();

		for( const auto& prio : squad->priorities )
		{
			GuiTargetPriority gtp;
			gtp.id = prio.type;
			gtp.name = S::s( "$CreatureName_" + prio.type );
			gtp.attitude = prio.attitude;
			m_tmpPriorities.append( gtp );
		}

		emit signalPriorities( squadID, m_tmpPriorities );
	}
}

void AggregatorMilitary::onRequestMilitary()
{
	sendSquadUpdate();
}

void AggregatorMilitary::onAddSquad()
{
	Global::mil().addSquad();
	sendSquadUpdate();
}

void AggregatorMilitary::onRemoveSquad( unsigned int id )
{
	Global::mil().removeSquad( id );
	sendSquadUpdate();
}
	
void AggregatorMilitary::onRenameSquad( unsigned int id, QString newName )
{
	Global::mil().renameSquad( id, newName );
}

void AggregatorMilitary::onMoveSquadLeft( unsigned int id )
{
	Global::mil().moveSquadUp( id );
	sendSquadUpdate();
}
	
void AggregatorMilitary::onMoveSquadRight( unsigned int id )
{
	Global::mil().moveSquadDown( id );
	sendSquadUpdate();
}

void AggregatorMilitary::onRemoveGnomeFromSquad( unsigned int gnomeID )
{
	if( Global::mil().removeGnome( gnomeID ) )
	{
		sendSquadUpdate();
	}
}
	
void AggregatorMilitary::onMoveGnomeLeft( unsigned int gnomeID )
{
	if( Global::mil().moveGnomeUp( gnomeID ) )
	{
		sendSquadUpdate();
	}
}

void AggregatorMilitary::onMoveGnomeRight( unsigned int gnomeID )
{
	if( Global::mil().moveGnomeDown( gnomeID ) )
	{
		sendSquadUpdate();
	}
}

	
void AggregatorMilitary::onMovePrioUp( unsigned int squadID, QString type )
{
	if( Global::mil().movePrioUp( squadID, type ) )
	{
		sendPriorityUpdate( squadID );
	}
}

void AggregatorMilitary::onMovePrioDown( unsigned int squadID, QString type )
{
	if( Global::mil().movePrioDown( squadID, type ) )
	{
		sendPriorityUpdate( squadID );
	}
}

void AggregatorMilitary::onAddRole()
{
	Global::mil().addRole();
	sendRoleUpdate();
}
	
void AggregatorMilitary::onRemoveRole( unsigned int id )
{
	Global::mil().removeRole( id );
	sendRoleUpdate();
}

void AggregatorMilitary::onRenameRole( unsigned int id, QString newName )
{
	Global::mil().renameRole( id, newName );
}

void AggregatorMilitary::onSetArmorType( unsigned int roleID, QString slot, QString type, QString material )
{
	Global::mil().setArmorType( roleID, slot, type, material );
	QStringList mats;
	mats.append( "any" );

	auto matTypes = DB::select2( "MaterialType", "Uniform_Slots", "Type", type );
	if( matTypes.size() )
	{
		auto mt = matTypes.first().toString();
		auto ids = DB::select2( "ID", "Materials", "Type", mt );
		for( auto id : ids  )
		{
			mats.append( id.toString() );
		}
	}

	emit signalPossibleMaterials( roleID, slot, mats );
}

void AggregatorMilitary::onSetRole( unsigned int gnomeID, unsigned int roleID )
{
	Global::gm().setRoleID( gnomeID, roleID );
}

void AggregatorMilitary::onSetRoleCivilian( unsigned int roleID, bool value )
{
	Global::mil().setRoleCivilian( roleID, value );
}
