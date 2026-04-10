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
/** @file aggregatormilitary.cpp
 *  @brief AggregatorMilitary implementation: builds squad, role, and priority payloads for
 *         the Military XAML window, and forwards edits (add/remove/reorder, armor setup,
 *         attitude changes) to MilitaryManager.
 */
#include "aggregatormilitary.h"

#include "../base/db.h"
#include "../base/global.h"
#include "../game/gnomemanager.h"
#include "../game/game.h"

#include "../gui/strings.h"

/// @brief Constructs the AggregatorMilitary.
/// @param parent Qt parent object.
AggregatorMilitary::AggregatorMilitary( QObject* parent ) :
	QObject(parent)
{
}

/// @brief Destructor.
AggregatorMilitary::~AggregatorMilitary()
{
}

/// @brief Binds the aggregator to a Game instance.
/// @param game Game to bind to.
void AggregatorMilitary::init( Game* game )
{
	g = game;
}

/// @brief Rebuilds the cached squad list: prepends a synthetic "no squad" bucket containing
///        every unassigned gnome, appends every real squad with its priorities and roster,
///        flags the first/last real squad so the GUI hides unusable reorder arrows, then
///        emits signalSquads.
void AggregatorMilitary::sendSquadUpdate()
{
	if( !g ) return;
	m_squads.clear();

	GuiSquad ngs;
	ngs.name = "no squad";
	ngs.id = 0;
	ngs.showLeftArrow = false;
	ngs.showRightArrow = false;

	for( auto gnome : g->gm()->gnomes() )
	{
		if( !g->mil()->getSquadForGnome( gnome->id() ) )
		{
			ngs.gnomes.append( { gnome->id(), gnome->name(), gnome->roleID() } );
		}
	}
	m_squads.append( ngs );

	for( const auto& squad : g->mil()->squads() )
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
			gsg.name = g->gm()->name( gnomeID );
			gsg.roleID = g->gm()->roleID( gnomeID );

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

/// @brief Forwards a role-list refresh request to sendRoleUpdate().
void AggregatorMilitary::onRequestRoles()
{
	sendRoleUpdate();
}

/// @brief Rebuilds the cached role list with full per-slot uniform info and emits signalRoles.
void AggregatorMilitary::sendRoleUpdate()
{
	if( !g ) return;
	m_roles.clear();

	auto uniformSlots = DB::ids( "Uniform" );

	for( auto& role : g->mil()->roles() )
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

/// @brief Builds a GuiUniformItem for one slot, merging the current role's uniform value
///        with the DB-defined list of allowed armor types for that slot (prepended with "none").
/// @param slot   Slot name (HeadArmor, ChestArmor, …).
/// @param uniVM  Serialised role uniform entry for this slot.
/// @return Fully populated GuiUniformItem.
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

/// @brief Emits the current engagement priority list for one squad via signalPriorities.
/// @param squadID Squad UID whose priorities to send.
void AggregatorMilitary::sendPriorityUpdate( unsigned int squadID )
{
	if( !g ) return;
	auto squad = g->mil()->squad( squadID );
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

/// @brief Triggers a full squad list refresh when the GUI opens the Military window.
void AggregatorMilitary::onRequestMilitary()
{
	sendSquadUpdate();
}

/// @brief Creates a new empty squad and refreshes the GUI.
void AggregatorMilitary::onAddSquad()
{
	if( !g ) return;
	g->mil()->addSquad();
	sendSquadUpdate();
}

/// @brief Removes a squad and refreshes the GUI.
/// @param id Squad UID to remove.
void AggregatorMilitary::onRemoveSquad( unsigned int id )
{
	if( !g ) return;
	g->mil()->removeSquad( id );
	sendSquadUpdate();
}

/// @brief Renames a squad (no full refresh; GUI updates its own label).
/// @param id      Squad UID.
/// @param newName New display name.
void AggregatorMilitary::onRenameSquad( unsigned int id, QString newName )
{
	if( !g ) return;
	g->mil()->renameSquad( id, newName );
}

/// @brief Moves a squad one position earlier in the order and refreshes the GUI.
/// @param id Squad UID.
void AggregatorMilitary::onMoveSquadLeft( unsigned int id )
{
	if( !g ) return;
	g->mil()->moveSquadUp( id );
	sendSquadUpdate();
}

/// @brief Moves a squad one position later in the order and refreshes the GUI.
/// @param id Squad UID.
void AggregatorMilitary::onMoveSquadRight( unsigned int id )
{
	if( !g ) return;
	g->mil()->moveSquadDown( id );
	sendSquadUpdate();
}

/// @brief Removes a gnome from whichever squad it's in and refreshes the GUI.
/// @param gnomeID Creature UID.
void AggregatorMilitary::onRemoveGnomeFromSquad( unsigned int gnomeID )
{
	if( !g ) return;
	if( g->mil()->removeGnome( gnomeID ) )
	{
		sendSquadUpdate();
	}
}

/// @brief Moves a gnome one slot up in its squad order and refreshes the GUI.
/// @param gnomeID Creature UID.
void AggregatorMilitary::onMoveGnomeLeft( unsigned int gnomeID )
{
	if( !g ) return;
	if( g->mil()->moveGnomeUp( gnomeID ) )
	{
		sendSquadUpdate();
	}
}

/// @brief Moves a gnome one slot down in its squad order and refreshes the GUI.
/// @param gnomeID Creature UID.
void AggregatorMilitary::onMoveGnomeRight( unsigned int gnomeID )
{
	if( !g ) return;
	if( g->mil()->moveGnomeDown( gnomeID ) )
	{
		sendSquadUpdate();
	}
}


/// @brief Moves an engagement priority row one slot up and refreshes the GUI.
/// @param squadID Squad UID.
/// @param type    Creature type key identifying the priority row.
void AggregatorMilitary::onMovePrioUp( unsigned int squadID, QString type )
{
	if( !g ) return;
	if( g->mil()->movePrioUp( squadID, type ) )
	{
		sendPriorityUpdate( squadID );
	}
}

/// @brief Moves an engagement priority row one slot down and refreshes the GUI.
/// @param squadID Squad UID.
/// @param type    Creature type key identifying the priority row.
void AggregatorMilitary::onMovePrioDown( unsigned int squadID, QString type )
{
	if( !g ) return;
	if( g->mil()->movePrioDown( squadID, type ) )
	{
		sendPriorityUpdate( squadID );
	}
}

/// @brief Creates a new military role and refreshes the role list GUI.
void AggregatorMilitary::onAddRole()
{
	if( !g ) return;
	g->mil()->addRole();
	sendRoleUpdate();
}

/// @brief Removes a military role and refreshes the GUI.
/// @param id Role UID.
void AggregatorMilitary::onRemoveRole( unsigned int id )
{
	if( !g ) return;
	g->mil()->removeRole( id );
	sendRoleUpdate();
}

/// @brief Renames a military role without full refresh (GUI updates label locally).
/// @param id      Role UID.
/// @param newName New display name.
void AggregatorMilitary::onRenameRole( unsigned int id, QString newName )
{
	if( !g ) return;
	g->mil()->renameRole( id, newName );
}

/// @brief Applies an armor type/material change to one slot of a role, then emits the
///        refreshed list of allowed materials for that slot so the GUI dropdown updates.
/// @param roleID   Role UID.
/// @param slot     Uniform slot name.
/// @param type     New armor type.
/// @param material New material.
void AggregatorMilitary::onSetArmorType( unsigned int roleID, QString slot, QString type, QString material )
{
	if( !g ) return;
	g->mil()->setArmorType( roleID, slot, type, material );
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

/// @brief Assigns a military role to a gnome.
/// @param gnomeID Creature UID.
/// @param roleID  Role UID.
void AggregatorMilitary::onSetRole( unsigned int gnomeID, unsigned int roleID )
{
	if( !g ) return;
	g->gm()->setRoleID( gnomeID, roleID );
}

/// @brief Flips a role's civilian flag (civilians are exempt from combat orders).
/// @param roleID Role UID.
/// @param value  True to mark as civilian.
void AggregatorMilitary::onSetRoleCivilian( unsigned int roleID, bool value )
{
	if( !g ) return;
	g->mil()->setRoleCivilian( roleID, value );
}

/// @brief Sets the engagement attitude for a specific creature type in a specific squad.
/// @param squadID  Squad UID.
/// @param type     Creature type key.
/// @param attitude New attitude (attack/defend/flee/ignore).
void AggregatorMilitary::onSetAttitude( unsigned int squadID, QString type, MilAttitude attitude )
{
	if( !g ) return;
	g->mil()->onSetAttitude( squadID, type, attitude );
}