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
/** @file militarymanager.cpp
 *  @brief Military system: roles, uniforms, squads, gnome assignment, target priorities, and alarm coordination.
 */
#include "militarymanager.h"
#include "game.h"

#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/io.h"
#include "../base/util.h"
#include "../game/gnome.h"
#include "../game/creaturemanager.h"
#include "../game/inventory.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

/// @brief Serialises this uniform item slot (type, item SID, material SID, quality) into a QVariantMap.
/// @return Map with keys Type, Item, Material, Quality.
QVariantMap UniformItem::serialize()
{
	QVariantMap out;
	out.insert( "Type", type );
	out.insert( "Item", item );
	out.insert( "Material", material );
	out.insert( "Quality", (int)quality );

	return out;
}

/// @brief Deserialising constructor — restores a uniform slot from a saved map.
/// @param in Map produced by UniformItem::serialize().
UniformItem::UniformItem( const QVariantMap& in )
{
	type     = in.value( "Type" ).toString();
	item     = in.value( "Item" ).toString();
	material = in.value( "Material" ).toString();
	quality  = (UniformItemQuality)in.value( "Quality" ).toInt();
}

/// @brief Serialises the full uniform (name, ID, and all nine equipment slots) into a QVariantMap.
/// @return Map with keys Name, ID, Items (nested slot maps).
QVariantMap Uniform::serialize()
{
	QVariantMap vUni;
	vUni.insert( "Name", name );
	vUni.insert( "ID", id );
	QVariantMap items;

	for( const auto& key : parts.keys() )
	{
		items.insert( key, parts[key].serialize() );
	}
	vUni.insert( "Items", items );

	return vUni;
}

/// @brief Deserialising constructor — restores a uniform from a saved map.
/// @param in Map produced by Uniform::serialize().
Uniform::Uniform( const QVariantMap& in )
{
	name          = in.value( "Name" ).toString();
	id            = in.value( "ID" ).toUInt();
	auto vmItems  = in.value( "Items" ).toMap();

	parts.insert( "HeadArmor", UniformItem( vmItems.value( "HeadArmor" ).toMap() ) );
	parts.insert( "ChestArmor", UniformItem( vmItems.value( "ChestArmor" ).toMap() ) );
	parts.insert( "ArmArmor", UniformItem( vmItems.value( "ArmArmor" ).toMap() ) );
	parts.insert( "HandArmor", UniformItem( vmItems.value( "HandArmor" ).toMap() ) );
	parts.insert( "LegArmor", UniformItem( vmItems.value( "LegArmor" ).toMap() ) );
	parts.insert( "FootArmor", UniformItem( vmItems.value( "FootArmor" ).toMap() ) );
	parts.insert( "LeftHandHeld", UniformItem( vmItems.value( "LeftHandHeld" ).toMap() ) );
	parts.insert( "RightHandHeld", UniformItem( vmItems.value( "RightHandHeld" ).toMap() ) );
	parts.insert( "Back", UniformItem( vmItems.value( "Back" ).toMap() ) );
}

/// @brief Default constructor — creates a uniform with all nine slots empty.
Uniform::Uniform()
{
	parts.insert( "HeadArmor", UniformItem() );
	parts.insert( "ChestArmor", UniformItem() );
	parts.insert( "ArmArmor", UniformItem() );
	parts.insert( "HandArmor", UniformItem() );
	parts.insert( "LegArmor", UniformItem( ) );
	parts.insert( "FootArmor", UniformItem() );
	parts.insert( "LeftHandHeld", UniformItem() );
	parts.insert( "RightHandHeld", UniformItem() );
	parts.insert( "Back", UniformItem() );
}


/// @brief Serialises the military role (name, ID, uniform, combat flags) into a QVariantMap.
/// @return Map with keys Name, ID, Uniform, MaintainDist, RetreatBleeding, IsCivilian.
QVariantMap MilitaryRole::serialize()
{
	QVariantMap out;
	out.insert( "Name", name );
	out.insert( "ID", id );
	out.insert( "Uniform", uniform.serialize() );
	out.insert( "MaintainDist", maintainDist );
	out.insert( "RetreatBleeding", retreatBleeding );
	out.insert( "IsCivilian", isCivilian );

	return out;
}

/// @brief Deserialising constructor — restores a military role from a saved map.
/// @param in Map produced by MilitaryRole::serialize().
MilitaryRole::MilitaryRole( const QVariantMap& in )
{
	name            = in.value( "Name" ).toString();
	id              = in.value( "ID" ).toUInt();
	uniform         = Uniform( in.value( "Uniform" ).toMap() );
	isCivilian      = in.value( "IsCivilian" ).toBool();
	maintainDist    = in.value( "MaintainDist" ).toBool();
	retreatBleeding = in.value( "RetreatBleeding" ).toBool();
}

/// @brief Serialises the squad (name, ID, gnome list, and target priorities) into a QVariantMap.
/// @return Map with keys Name, ID, Gnomes, Priorities.
QVariantMap Squad::serialize()
{
	QVariantMap out;
	out.insert( "Name", name );
	out.insert( "ID", id );
	out.insert( "Gnomes", Global::util->uintList2Variant( gnomes ) );

	QVariantList vl;
	for( auto prio : priorities )
	{
		QVariantMap vm;
		vm.insert( "Type", prio.type );
		vm.insert( "Attitude", (int)prio.attitude );
		vl.append( vm );
	}
	out.insert( "Priorities", vl );

	return out;
}

/// @brief Deserialising constructor — restores a squad from a saved map.
///        Adds any creature types missing from the saved priorities with FLEE attitude.
/// @param tps All known creature type SIDs (from CreatureManager::types()).
/// @param in  Map produced by Squad::serialize().
Squad::Squad( QList<QString> tps, const QVariantMap& in ) :
	types( tps )
{
	name       = in.value( "Name" ).toString();
	id         = in.value( "ID" ).toUInt();
	gnomes     = Global::util->variantList2UInt( in.value( "Gnomes" ).toList() );

	if( in.contains( "Priorities" ) )
	{
		QSet<QString> typeSet;
		auto vl = in.value( "Priorities" ).toList();
		for( auto vEntry : vl )
		{
			auto vm = vEntry.toMap();
			auto type = vm.value( "Type" ).toString();
			typeSet.insert( type );
			TargetPriority tp { type, (MilAttitude)vm.value( "Attitude" ).toInt() };
			priorities.append( tp );
		}
		for( auto type : types )
		{
			if( !typeSet.contains( type ) )
			{
				TargetPriority tp { type, MilAttitude::FLEE };
				priorities.append( tp );
			}
		}
	}
	else
	{
		for( auto type : types )
		{
			TargetPriority tp { type, MilAttitude::FLEE };
			priorities.append( tp );
		}
	}
}

/// @brief Constructs the military manager.
/// @param parent Owning Game instance.
MilitaryManager::MilitaryManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
}

/// @brief Destructor.
MilitaryManager::~MilitaryManager()
{
}

/// @brief Loads roles and squads from GameState or from the military.json settings file.
///        Creates a default role and default squad if none exist.
void MilitaryManager::init()
{
	auto mm = GameState::military;
	QVariantList pl;
	if ( mm.contains( "Roles" ) )
	{
		pl = mm.value( "Roles" ).toList();
	}
	else
	{
		//load from file
		QJsonDocument jd;
		IO::loadFile( IO::getDataFolder() + "/settings/military.json", jd );
		QVariantMap vm = jd.toVariant().toMap();
		if ( vm.contains( "Roles" ) )
		{
			pl = vm.value( "Roles" ).toList();
		}
	}
	for ( auto entry : pl )
	{
		MilitaryRole pos( entry.toMap() );
		m_roles.insert( pos.id, pos );
	}
	if ( m_roles.empty() )
	{
		addRole();
	}

	
	if ( mm.contains( "Squads" ) )
	{
		auto sl = mm.value( "Squads" ).toList();
		for ( auto entry : sl )
		{
			Squad squad( g->m_creatureManager->types(), entry.toMap() );
			m_squads.insert( squad.id, squad );

			for( auto gnome : squad.gnomes )
			{
				m_gnome2Squad.insert( gnome, squad.id );
			}
		}
	}
	if ( m_squads.empty() )
	{
		addSquad();
	}
}

/// @brief Persists role definitions (without squad data) to military.json in the data folder.
void MilitaryManager::save()
{
	auto vm = serialize();
	vm.remove( "Squads" );
	QJsonObject jo = QJsonObject::fromVariantMap( vm );
	IO::saveFile( IO::getDataFolder() + "/settings/military.json", jo );
}

/// @brief Serialises all roles and squads into a QVariantMap and caches squads in GameState.
/// @return Map with keys Roles and Squads.
QVariantMap MilitaryManager::serialize()
{
	QVariantMap military;

	QVariantList vRoles;
	for ( auto pos : m_roles )
	{
		vRoles.append( pos.serialize() );
	}
	military.insert( "Roles", vRoles );

	QVariantList vSquads;
	for ( auto squad : m_squads )
	{
		vSquads.append( squad.serialize() );
	}
	GameState::squads = vSquads;
	military.insert( "Squads", vSquads );
	return military;
}

/// @brief Per-tick update hook (currently a no-op stub).
/// @param tickNumber    Current game tick.
/// @param seasonChanged Unused.
/// @param dayChanged    Unused.
/// @param hourChanged   Unused.
/// @param minuteChanged Unused.
void MilitaryManager::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	QElapsedTimer timer;
	timer.start();
}

/// @brief Creates a new default military role and inserts it into the role map.
/// @return UID of the newly created role.
unsigned int MilitaryManager::addRole()
{
	MilitaryRole pos;

	

	m_roles.insert( pos.id, pos );

	return pos.id;
}

/// @brief Removes the military role with the given UID.
/// @param id Role UID to remove.
/// @return true if the role existed and was removed, false otherwise.
bool MilitaryManager::removeRole( unsigned int id )
{
	if ( m_roles.contains( id ) )
	{
		m_roles.remove( id );
		return true;
	}
	return false;
}


/// @brief Renames the military role with the given UID.
/// @param id   Role UID.
/// @param text New name.
void MilitaryManager::renameRole( unsigned int id, QString text )
{
	if ( m_roles.contains( id ) )
	{
		m_roles[id].name = text;
	}
}

/// @brief Returns a pointer to the military role with the given UID, or nullptr if absent.
/// @param id Role UID.
/// @return Pointer to the MilitaryRole, or nullptr.
MilitaryRole* MilitaryManager::role( unsigned int id )
{
	if ( m_roles.contains( id ) )
	{
		return &m_roles[id];
	}
	return nullptr;
}

/// @brief Returns the display name of the role with the given UID.
/// @param id Role UID.
/// @return Role name, or "Position doesn't exist." if not found.
QString MilitaryManager::roleName( unsigned int id )
{
	if ( m_roles.contains( id ) )
	{
		return m_roles[id].name;
	}
	return "Position doesn't exist.";
}

/// @brief Sets the armor type and material for a specific slot in a role's uniform.
///        Looks up the item SID from the Uniform_Slots database table.
/// @param roleID   Role UID to modify.
/// @param slot     Equipment slot key (e.g. "HeadArmor").
/// @param type     Armor type string (e.g. "LeatherHelm").
/// @param material Material SID.
void MilitaryManager::setArmorType( unsigned int roleID, QString slot, QString type, QString material )
{
	if ( m_roles.contains( roleID ) )
	{
		auto& item = m_roles[roleID].uniform.parts[slot];
		item.type = type;
		item.item = DB::select3( "ItemID", "Uniform_Slots", "ID", slot, "Type", type ).toString();
		item.material = material;
	}
}

/// @brief Returns a mutable pointer to the uniform for the given role UID, or nullptr if absent.
/// @param roleID Role UID.
/// @return Pointer to the Uniform, or nullptr.
Uniform* MilitaryManager::uniform( unsigned int roleID )
{
	if ( m_roles.contains( roleID ) )
	{
		return &m_roles[roleID].uniform;
	}
	return nullptr;
}

/// @brief Returns a copy of the uniform for the given role UID, or a default Uniform if absent.
/// @param roleID Role UID.
/// @return Copy of the Uniform.
Uniform MilitaryManager::uniformCopy( unsigned int roleID )
{
	if ( m_roles.contains( roleID ) )
	{
		return m_roles[roleID].uniform;
	}
	return Uniform();
}














/// @brief Returns a pointer to the squad with the given UID, or nullptr if not found.
/// @param id Squad UID.
/// @return Pointer to the Squad, or nullptr.
Squad* MilitaryManager::squad( unsigned int id )
{
	for( int i = 0; i < m_squads.size(); ++i )
	{
		if( m_squads[i].id == id )
		{
			return &m_squads[i];
		}
	}
	return nullptr;
}

/// @brief Creates a new squad with DEFEND attitude for all known creature types.
/// @return UID of the newly created squad.
unsigned int MilitaryManager::addSquad()
{
	Squad squad( g->m_creatureManager->types() );

	auto types = g->m_creatureManager->types();
	for( auto type : types )
	{
		TargetPriority tp { type, MilAttitude::DEFEND };
		squad.priorities.append( tp );
	}

	m_squads.insert( squad.id, squad );
	return squad.id;
}

/// @brief Removes the squad with the given UID.
/// @param id Squad UID to remove.
/// @return true if found and removed, false otherwise.
bool MilitaryManager::removeSquad( unsigned int id )
{
	for( int i = 0; i < m_squads.size(); ++i )
	{
		if( m_squads[i].id == id )
		{
			m_squads.removeAt( i );
			return true;
		}
	}
	return false;
}

/// @brief Moves the squad with the given UID one position earlier in the squad list.
/// @param id Squad UID to move.
void MilitaryManager::moveSquadUp( unsigned int id )
{
	for( int i = 1; i < m_squads.size(); ++i )
	{
		if( m_squads[i].id == id )
		{
			m_squads.move( i, i - 1 );
			return;
		}
	}
}

/// @brief Moves the squad with the given UID one position later in the squad list.
/// @param id Squad UID to move.
void MilitaryManager::moveSquadDown( unsigned int id )
{
	for( int i = 0; i < m_squads.size() - 1; ++i )
	{
		if( m_squads[i].id == id )
		{
			m_squads.move( i, i + 1 );
			return;
		}
	}
}

/// @brief Removes @p gnomeID from every squad except @p squadID (used when reassigning a gnome).
/// @param squadID Squad UID the gnome is being moved into (excluded from removal).
/// @param gnomeID UID of the gnome to remove from all other squads.
void MilitaryManager::removeGnomeFromOtherSquad( unsigned int squadID, unsigned int gnomeID )
{
	if ( gnomeID )
	{
		for ( auto& squad : m_squads )
		{
			if ( squad.id != squadID )
			{
				squad.gnomes.removeAll( gnomeID );
			}
		}
	}
}

/// @brief Renames the squad with the given UID.
/// @param id   Squad UID.
/// @param text New name.
void MilitaryManager::renameSquad( unsigned int id, QString text )
{
	for( int i = 0; i < m_squads.size(); ++i )
	{
		if( m_squads[i].id == id )
		{
			m_squads[i].name = text;
		}
	}
}

/// @brief Stub: intended to add a specific target to a squad's target list (currently unimplemented).
/// @param squadID Squad UID.
/// @param target  Target creature UID.
void MilitaryManager::addSquadTarget( unsigned int squadID, unsigned int target )
{
	/*
	if ( m_squads.contains( squadID ) )
	{
		Squad& squad = m_squads[squadID];
		squad.targetList.insert( target );

		auto targets = squad.targetList.values();

	}
	*/
}

/// @brief Stub: intended to remove a specific target from a squad's target list (currently unimplemented).
/// @param squadID Squad UID.
/// @param target  Target creature UID.
void MilitaryManager::removeSquadTarget( unsigned int squadID, unsigned int target )
{
	/*
	if ( m_squads.contains( squadID ) )
	{
		Squad& squad = m_squads[squadID];
		squad.targetList.remove( target );

		auto targets = squad.targetList.values();

	}
	*/
}

/// @brief Stub: returns the target list for a squad (currently always returns an empty list).
/// @param id Squad UID.
/// @return Empty list (target list logic not yet implemented).
QList<unsigned int> MilitaryManager::squadTargets( unsigned int id )
{
	/*
	if ( m_squads.contains( id ) )
	{
		return m_squads[id].targetList.values();
	}
	*/
	return QList<unsigned int>();
}

/// @brief Returns the squad that contains @p gnomeID, or nullptr if the gnome is unassigned.
/// @param gnomeID UID of the gnome to look up.
/// @return Pointer to the gnome's Squad, or nullptr.
Squad* MilitaryManager::getSquadForGnome( unsigned int gnomeID )
{
	if ( m_gnome2Squad.contains( gnomeID ) )
	{
		unsigned int squadID = m_gnome2Squad.value( gnomeID );
		for( int i = 0; i < m_squads.size(); ++i )
		{
			if( m_squads[i].id == squadID )
			{
				return &m_squads[i];
			}
		}
	}
	return nullptr;
}

/// @brief Returns the uniform assigned to @p gnomeID within the given @p squad (stub — always nullptr).
/// @param gnomeID UID of the gnome.
/// @param squad   Squad to search within.
/// @return Always nullptr (uniform per-gnome logic not yet implemented).
Uniform* MilitaryManager::getGnomeUniform( unsigned int gnomeID, Squad& squad )
{

	return nullptr;
}

/// @brief Searches all squads to find the uniform assigned to @p gnomeID.
/// @param gnomeID UID of the gnome.
/// @return Pointer to the gnome's Uniform, or nullptr if not found.
Uniform* MilitaryManager::getGnomeUniform( unsigned int gnomeID )
{
	for ( auto& squad : m_squads )
	{
		auto uni = getGnomeUniform( gnomeID, squad );
		if ( uni )
		{
			return uni;
		}
	}

	return nullptr;
}

/// @brief Hook called when a gnome's equipment or role changes (stub — currently empty).
/// @param gnomeID UID of the gnome to update.
void MilitaryManager::updateGnome( unsigned int gnomeID )
{

}

/// @brief Notification that a gnome died; intended for cleanup (currently a no-op stub).
/// @param id UID of the dead gnome.
void MilitaryManager::onGnomeDeath( unsigned int id )
{
}

/// @brief Notification that a monster died; intended for target-list cleanup (currently a no-op stub).
/// @param id UID of the dead monster.
void MilitaryManager::onMonsterDeath( unsigned int id )
{
}

/// @brief Notification that an animal died; intended for target-list cleanup (currently a no-op stub).
/// @param id UID of the dead animal.
void MilitaryManager::onAnimalDeath( unsigned int id )
{
}

/// @brief Removes @p gnomeID from their current squad and clears the gnome→squad mapping.
/// @param gnomeID UID of the gnome to remove.
/// @return true if the gnome was found and removed, false if they were not in any squad.
bool MilitaryManager::removeGnome( unsigned int gnomeID )
{
	auto squad = getSquadForGnome( gnomeID );
	if( squad )
	{
		squad->gnomes.removeAll( gnomeID );
		m_gnome2Squad.remove( gnomeID );
		return true;
	}
	return false;
}
	
/// @brief Moves @p gnomeID from their current squad to the previous squad in the ordered list.
/// @param gnomeID UID of the gnome to move.
/// @return true if the gnome was moved, false if they were already in the first squad or unassigned.
bool MilitaryManager::moveGnomeUp( unsigned int gnomeID )
{
	auto squad = getSquadForGnome( gnomeID );
	if( squad )
	{
		for( int i = 1; i < m_squads.size(); ++i )
		{
			if( m_squads[i].id == squad->id )
			{
				m_squads[i].gnomes.removeAll( gnomeID );
				m_squads[i-1].gnomes.append( gnomeID );
				m_gnome2Squad.insert( gnomeID, m_squads[i-1].id );
				return true;
			}
		}
	}

	return false;
}

/// @brief Moves @p gnomeID from their current squad to the next squad in the ordered list.
///        If the gnome is unassigned, places them in the first squad.
/// @param gnomeID UID of the gnome to move.
/// @return true if the gnome was moved or placed, false if there are no squads.
bool MilitaryManager::moveGnomeDown( unsigned int gnomeID )
{
	auto squad = getSquadForGnome( gnomeID );
	if( squad )
	{
		for( int i = 0; i < m_squads.size()-1; ++i )
		{
			if( m_squads[i].id == squad->id )
			{
				m_squads[i].gnomes.removeAll( gnomeID );
				m_squads[i+1].gnomes.append( gnomeID );
				m_gnome2Squad.insert( gnomeID, m_squads[i+1].id );
				return true;
			}
		}
	}
	if( m_squads.size() )
	{
		m_squads[0].gnomes.append( gnomeID );
		m_gnome2Squad.insert( gnomeID, m_squads[0].id );
		return true;
	}
	return false;
}

/// @brief Sets the combat attitude for a given creature type in the specified squad's priority list.
/// @param squadID  Squad UID.
/// @param type     Creature type SID.
/// @param attitude New MilAttitude (FLEE, DEFEND, ATTACK, HUNT).
void MilitaryManager::onSetAttitude( unsigned int squadID, QString type, MilAttitude attitude )
{
	for( auto& squad : m_squads )
	{
		if( squad.id == squadID )
		{
			for( auto& prio : squad.priorities )
			{
				if( prio.type == type )
				{
					prio.attitude = attitude;
					return;
				}
			}
		}
	}
}

/// @brief Moves the target priority for @p type one position earlier in @p squadID's priority list.
/// @param squadID Squad UID.
/// @param type    Creature type SID whose priority should be increased.
/// @return true if the entry was found and moved, false otherwise.
bool MilitaryManager::movePrioUp( unsigned int squadID, QString type )
{
	for( auto& squad : m_squads )
	{
		if( squad.id == squadID )
		{
			for( int i = 1; i < squad.priorities.size(); ++i )
			{
				if( squad.priorities[i].type == type )
				{
					squad.priorities.move( i, i - 1 );
					return true;
				}
			}
		}
	}
	return false;
}

/// @brief Moves the target priority for @p type one position later in @p squadID's priority list.
/// @param squadID Squad UID.
/// @param type    Creature type SID whose priority should be decreased.
/// @return true if the entry was found and moved, false otherwise.
bool MilitaryManager::movePrioDown( unsigned int squadID, QString type )
{
	for( auto& squad : m_squads )
	{
		if( squad.id == squadID )
		{
			for( int i = 0; i < squad.priorities.size() - 1; ++i )
			{
				if( squad.priorities[i].type == type )
				{
					squad.priorities.move( i, i + 1 );
					return true;
				}
			}
		}
	}
	return false;
}

/// @brief Marks or unmarks a role as civilian (civilians do not use military BT branches).
/// @param roleID Role UID.
/// @param value  true to mark as civilian, false for military.
void MilitaryManager::setRoleCivilian( unsigned int roleID, bool value )
{
	if ( m_roles.contains( roleID ) )
	{
		m_roles[roleID].isCivilian = value;
	}
}

/// @brief Returns whether the given role is classified as civilian.
/// @param roleID Role UID.
/// @return true if the role is civilian, false if military or not found.
bool MilitaryManager::roleIsCivilian( unsigned int roleID )
{
	if ( m_roles.contains( roleID ) )
	{
		return m_roles[roleID].isCivilian;
	}
	return false;
}
