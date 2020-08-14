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
#include "militarymanager.h"

#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/io.h"
#include "../base/util.h"
#include "../game/gnome.h"
#include "../game/creaturemanager.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

QVariantMap UniformItem::serialize()
{
	QVariantMap out;
	out.insert( "Type", type );
	out.insert( "Item", item );
	out.insert( "Material", material );
	out.insert( "Quality", (int)quality );

	return out;
}

UniformItem::UniformItem( const QVariantMap& in )
{
	type     = in.value( "Type" ).toString();
	item     = in.value( "Item" ).toString();
	material = in.value( "Material" ).toString();
	quality  = (UniformItemQuality)in.value( "Quality" ).toInt();
}

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

MilitaryRole::MilitaryRole( const QVariantMap& in )
{
	name            = in.value( "Name" ).toString();
	id              = in.value( "ID" ).toUInt();
	uniform         = Uniform( in.value( "Uniform" ).toMap() );
	isCivilian      = in.value( "IsCivilian" ).toBool();
	maintainDist    = in.value( "MaintainDist" ).toBool();
	retreatBleeding = in.value( "RetreatBleeding" ).toBool();
}

QVariantMap Squad::serialize()
{
	QVariantMap out;
	out.insert( "Name", name );
	out.insert( "ID", id );
	out.insert( "Gnomes", Util::uintList2Variant( gnomes ) );

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

Squad::Squad( const QVariantMap& in )
{
	name       = in.value( "Name" ).toString();
	id         = in.value( "ID" ).toUInt();
	gnomes     = Util::variantList2UInt( in.value( "Gnomes" ).toList() );

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
		auto types = Global::cm().types();
		for( auto type : types )
		{
			if( !typeSet.contains( type ) )
			{
				TargetPriority tp { type, MilAttitude::_IGNORE };
				priorities.append( tp );
			}
		}
	}
	else
	{
		auto types = Global::cm().types();
		for( auto type : types )
		{
			TargetPriority tp { type, MilAttitude::_IGNORE };
			priorities.append( tp );
		}
	}
}

MilitaryManager::MilitaryManager()
{
}
MilitaryManager::~MilitaryManager()
{
}

void MilitaryManager::reset()
{
	m_roles.clear();
	m_squads.clear();
	m_gnome2Squad.clear();
}

void MilitaryManager::init()
{
	//load from file
	QJsonDocument jd;
	IO::loadFile( QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/settings/military.json", jd );

	QVariantMap vm = jd.toVariant().toMap();

	if ( vm.contains( "Roles" ) )
	{
		auto pl = vm.value( "Roles" ).toList();
		for ( auto entry : pl )
		{
			MilitaryRole pos( entry.toMap() );
			m_roles.insert( pos.id, pos );
		}
	}
	if ( m_roles.empty() )
	{
		addRole();
	}

	auto mm = GameState::military;

	if ( mm.contains( "Squads" ) )
	{
		auto sl = mm.value( "Squads" ).toList();
		for ( auto entry : sl )
		{
			Squad squad( entry.toMap() );
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

void MilitaryManager::save()
{
	auto vm = serialize();
	vm.remove( "Squads" );
	QJsonObject jo = QJsonObject::fromVariantMap( vm );
	IO::saveFile( QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/" + "settings/military.json", jo );
}

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

void MilitaryManager::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	QElapsedTimer timer;
	timer.start();
}

unsigned int MilitaryManager::addRole()
{
	MilitaryRole pos;

	

	m_roles.insert( pos.id, pos );

	return pos.id;
}

bool MilitaryManager::removeRole( unsigned int id )
{
	if ( m_roles.contains( id ) )
	{
		m_roles.remove( id );
		return true;
	}
	return false;
}


void MilitaryManager::renameRole( unsigned int id, QString text )
{
	if ( m_roles.contains( id ) )
	{
		m_roles[id].name = text;
	}
}

MilitaryRole* MilitaryManager::role( unsigned int id )
{
	if ( m_roles.contains( id ) )
	{
		return &m_roles[id];
	}
	return nullptr;
}

QString MilitaryManager::roleName( unsigned int id )
{
	if ( m_roles.contains( id ) )
	{
		return m_roles[id].name;
	}
	return "Position doesn't exist.";
}

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

Uniform* MilitaryManager::uniform( unsigned int roleID )
{
	if ( m_roles.contains( roleID ) )
	{
		return &m_roles[roleID].uniform;
	}
	return nullptr;
}

Uniform MilitaryManager::uniformCopy( unsigned int roleID )
{
	if ( m_roles.contains( roleID ) )
	{
		return m_roles[roleID].uniform;
	}
	return Uniform();
}














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

unsigned int MilitaryManager::addSquad()
{
	Squad squad;

	auto types = Global::cm().types();
	for( auto type : types )
	{
		TargetPriority tp { type, MilAttitude::_IGNORE };
		squad.priorities.append( tp );
	}

	m_squads.insert( squad.id, squad );
	return squad.id;
}

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

void MilitaryManager::moveSquadUp( unsigned int id )
{
	for( int i = 1; i < m_squads.size(); ++i )
	{
		if( m_squads[i].id == id )
		{
			m_squads.swapItemsAt( i, i - 1 );
			return;
		}
	}
}

void MilitaryManager::moveSquadDown( unsigned int id )
{
	for( int i = 0; i < m_squads.size() - 1; ++i )
	{
		if( m_squads[i].id == id )
		{
			m_squads.swapItemsAt( i, i + 1 );
			return;
		}
	}
}

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

Uniform* MilitaryManager::getGnomeUniform( unsigned int gnomeID, Squad& squad )
{
	
	return nullptr;
}

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

void MilitaryManager::updateGnome( unsigned int gnomeID )
{
	
}

void MilitaryManager::onGnomeDeath( unsigned int id )
{
}

void MilitaryManager::onMonsterDeath( unsigned int id )
{
}

void MilitaryManager::onAnimalDeath( unsigned int id )
{
}

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
					squad.priorities.swapItemsAt( i - 1, i );
					return true;
				}
			}
		}
	}
	return false;
}

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
					squad.priorities.swapItemsAt( i, i + 1 );
					return true;
				}
			}
		}
	}
	return false;
}
