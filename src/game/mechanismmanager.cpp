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
/** @file mechanismmanager.cpp
 *  @brief Mechanism system: axles, gears, levers, engines, pumps, pressure plates, and power networks.
 */
#include "mechanismmanager.h"
#include "game.h"

#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/gnomemanager.h"
#include "../game/fluidmanager.h"
#include "../game/inventory.h"
#include "../game/world.h"
#include "../gfx/spritefactory.h"
#include "../gui/strings.h"

#include <QDebug>
#include <QQueue>

/** @brief Serializes this mechanism's data into a QVariantMap for save/load.
 *  @return QVariantMap containing all mechanism properties.
 */
QVariantMap MechanismData::serialize() const
{
	QVariantMap out;
	out.insert( "Type", type );
	out.insert( "ItemID", itemID );
	out.insert( "ItemSID", itemSID );
	out.insert( "MaterialSID", materialSID );
	out.insert( "SpriteID", spriteID );
	out.insert( "Pos", pos.toString() );
	out.insert( "Rot", rot );
	out.insert( "Gui", gui );
	out.insert( "Active", active );
	out.insert( "HasPower", hasPower );
	out.insert( "ChangeActive", changeActive );
	out.insert( "Produce", producePower );
	out.insert( "Consume", consumePower );
	out.insert( "Fuel", fuel );
	out.insert( "MaxFuel", maxFuel );
	out.insert( "RFThreshold", refuelThreshold );
	out.insert( "ConnectsTo", Global::util->positionList2Variant( connectsTo ) );
	out.insert( "IsInvertable", isInvertable );
	out.insert( "Inverted", inverted );
	out.insert( "ChangeInverted", changeInverted );
	out.insert( "Anim", anim );
	return out;
}

/// @brief Restores all mechanism fields from a previously serialised QVariantMap.
/// @param in Map produced by MechanismData::serialize().
void MechanismData::deserialize( QVariantMap in )
{
	type            = (MechanismType)in.value( "Type" ).toInt();
	itemID          = in.value( "ItemID" ).toUInt();
	itemSID         = in.value( "ItemSID" ).toString();
	materialSID     = in.value( "MaterialSID" ).toString();
	spriteID        = in.value( "SpriteID" ).toUInt();
	pos             = Position( in.value( "Pos" ) );
	rot             = in.value( "Rot" ).toInt();
	gui             = in.value( "Gui" ).toString();
	active          = in.value( "Active" ).toBool();
	hasPower        = in.value( "HasPower" ).toBool();
	changeActive    = in.value( "ChangeActive" ).toBool();
	producePower    = in.value( "Produce" ).toInt();
	consumePower    = in.value( "Consume" ).toInt();
	fuel            = in.value( "Fuel" ).toInt();
	maxFuel         = in.value( "MaxFuel" ).toInt();
	refuelThreshold = in.value( "RFThreshold" ).toInt();
	connectsTo      = Global::util->variantList2Position( in.value( "ConnectsTo" ).toList() );
	isInvertable    = in.value( "IsInvertable" ).toBool();
	inverted        = in.value( "Inverted" ).toBool();
	changeInverted  = in.value( "ChangeInverted" ).toBool();
	anim			= in.value( "Anim" ).toBool();
}

/// @brief Constructs the mechanism manager and populates the SID-to-type lookup table.
/// @param parent Owning Game instance.
MechanismManager::MechanismManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
	m_string2Type.insert( "None", MT_NONE );
	m_string2Type.insert( "Axle", MT_AXLE );
	m_string2Type.insert( "Lever", MT_LEVER );
	m_string2Type.insert( "VerticalAxle", MT_VERTICALAXLE );
	m_string2Type.insert( "GearBox", MT_GEARBOX );
	m_string2Type.insert( "SteamEngine", MT_ENGINE );
	m_string2Type.insert( "Pump", MT_PUMP );
	m_string2Type.insert( "MechanicalWall", MT_WALL );
	m_string2Type.insert( "PressurePlate", MT_PRESSUREPLATE );
}

/// @brief Destructor.
MechanismManager::~MechanismManager()
{
}

/// @brief Deserialises and installs all mechanisms from saved data, then rebuilds power networks.
/// @param data List of QVariantMaps, each produced by MechanismData::serialize().
void MechanismManager::loadMechanisms( QVariantList data )
{
	for ( auto vdata : data )
	{
		MechanismData md;
		auto vmd = vdata.toMap();

		md.deserialize( vmd );

		installItem( md );
	}
	updateNetWorks();
}

/// @brief Per-tick update: burns engine fuel, tallies network power balance, propagates hasPower to consumers,
///        and creates SwitchMechanism/InvertMechanism/Refuel jobs as needed.
/// @param tickNumber     Current game tick.
/// @param seasonChanged  Unused.
/// @param dayChanged     Unused.
/// @param hourChanged    Unused.
/// @param minuteChanged  Unused.
void MechanismManager::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	int tickDiff = 1;
	if ( m_lastTick != 0 )
	{
		tickDiff = tickNumber - m_lastTick;
	}
	m_lastTick = tickNumber;

	if ( m_needNetworkUpdate )
	{
		updateNetWorks();
	}

	for ( auto& network : m_networks )
	{
		network.produce = 0;
		network.consume = 0;
		for ( auto itemID : network.producers )
		{
			if ( m_mechanisms.contains( itemID ) )
			{
				auto& md = m_mechanisms[itemID];
				if ( md.active && md.fuel > 0 )
				{
					md.fuel -= tickDiff;
					if ( md.fuel > 0 )
					{
						network.produce += md.producePower;
					}
					else
					{
						g->m_world->setWallSpriteAnim( m_mechanisms[itemID].pos, false );
						m_needNetworkUpdate = true;
					}
				}
			}
		}
		for ( auto itemID : network.consumers )
		{
			if ( m_mechanisms.contains( itemID ) )
			{
				auto& md = m_mechanisms[itemID];
				if ( md.active )
				{
					network.consume += md.consumePower;
				}
			}
		}
		bool enoughPower = ( network.produce >= network.consume );
		//qDebug() << "Network power consume/produce" << network.consume << network.produce;
		for ( auto itemID : network.consumers )
		{
			if ( m_mechanisms.contains( itemID ) )
			{
				auto& md = m_mechanisms[itemID];
				if ( md.hasPower != enoughPower )
				{
					md.hasPower = enoughPower;

					updateSpritesAndFlags( md, md.active && enoughPower );
				}
			}
		}
	}
	for( auto& md : m_mechanisms )
	{
		if( !md.job )
		{
			if( md.changeActive )
			{
				auto jobID = g->jm()->addJob( "SwitchMechanism", md.pos, 0, false );
				auto job = g->jm()->getJob( jobID );
				if( job )
				{
					job->setMechanism( md.itemID );
					md.job = job;
				}
			}
			else if( md.changeInverted )
			{
				auto jobID = g->jm()->addJob( "InvertMechanism", md.pos, 0, false );
				auto job = g->jm()->getJob( jobID );
				if( job )
				{
					job->setMechanism( md.itemID );
					md.job = job;
				}
			}
			else if ( md.active && md.fuel < ( md.maxFuel * md.refuelThreshold / 100 ) )
			{
				auto jobID = g->jm()->addJob( "Refuel", md.pos, 0, false );
				auto job = g->jm()->getJob( jobID );
				if( job )
				{
					job->setMechanism( md.itemID );
					job->addRequiredItem( 1, "RawCoal", "any", {} );
					md.job = job;
				}
			}
		}
	}
}

/// @brief Returns true if axle animation state changed since the last call, then resets the flag.
/// @return true if axles changed, false otherwise.
bool MechanismManager::axlesChanged()
{
	bool out       = m_axlesChanged;
	m_axlesChanged = false;
	return out;
}

/// @brief Returns a copy of the axle data map (position UID → AxleData) for rendering.
/// @return Copy of the axle data hash.
QHash<unsigned int, AxleData> MechanismManager::axleData()
{
	return m_axleData;
}

/// @brief Returns true if a mechanism (floor or wall) is installed at @p pos.
/// @param pos World position to query.
/// @return true if a mechanism exists there.
bool MechanismManager::hasMechanism( Position pos )
{
	return m_floorPositions.contains( pos.toInt() ) || m_wallPositions.contains( pos.toInt() );
}

/// @brief Returns true if the floor mechanism at @p pos is a GearBox.
/// @param pos World position to query.
/// @return true if a GearBox is installed on the floor there.
bool MechanismManager::hasGearBox( Position pos )
{
	if ( m_floorPositions.contains( pos.toInt() ) )
	{
		auto itemID = m_floorPositions[pos.toInt()];
		return ( m_mechanisms[itemID].itemSID == "GearBox" );
	}
	return false;
}

/// @brief Returns true if the wall mechanism at @p pos is currently powered.
/// @param pos World position to query.
/// @return true if the mechanism has power.
bool MechanismManager::hasPower( Position pos )
{
	if ( m_wallPositions.contains( pos.toInt() ) )
	{
		auto itemID = m_wallPositions[pos.toInt()];
		if ( m_mechanisms.contains( itemID ) )
		{
			return m_mechanisms[itemID].hasPower;
		}
	}
	return false;
}

/// @brief Returns the localised display name of the mechanism at @p pos.
/// @param pos World position to query.
/// @return Localised item name, or "No mechanism" if none exists there.
QString MechanismManager::mechanismName( Position pos )
{
	unsigned itemID = mechanismID( pos );
	if ( itemID )
	{
		return S::s( "$ItemName_" + m_mechanisms[itemID].itemSID );
	}
	return "No mechanism";
}

/// @brief Returns the item UID of the mechanism at @p pos (wall slot checked first, then floor).
/// @param pos World position to query.
/// @return Mechanism item UID, or 0 if none.
unsigned int MechanismManager::mechanismID( Position pos )
{
	if ( m_wallPositions.contains( pos.toInt() ) )
	{
		auto itemID = m_wallPositions[pos.toInt()];
		return itemID;
	}
	else if ( m_floorPositions.contains( pos.toInt() ) )
	{
		auto itemID = m_floorPositions[pos.toInt()];
		return itemID;
	}
	return 0;
}

/// @brief Returns true if the mechanism identified by @p itemID has a GUI defined in the database.
/// @param itemID Mechanism item UID.
/// @return true if a GUI string is present in the Mechanism table.
bool MechanismManager::hasGUI( unsigned int itemID )
{
	QString itemSID = m_mechanisms[itemID].itemSID;
	auto row        = DB::selectRow( "Mechanism", itemSID );
	if ( !row.value( "GUI" ).toString().isEmpty() )
	{
		return true;
	}
	return false;
}

/// @brief Returns true if the mechanism at @p pos has a GUI defined in the database.
/// @param pos World position to query.
/// @return true if a GUI string is present in the Mechanism table.
bool MechanismManager::hasGUI( Position pos )
{
	unsigned itemID = mechanismID( pos );
	if ( itemID )
	{
		QString itemSID = m_mechanisms[itemID].itemSID;
		auto row        = DB::selectRow( "Mechanism", itemSID );
		if ( !row.value( "GUI" ).toString().isEmpty() )
		{
			return true;
		}
	}
	return false;
}

/// @brief Returns the GUI identifier string for the mechanism, used to open the correct panel.
/// @param itemID Mechanism item UID.
/// @return GUI string from the Mechanism database table, or empty if none.
QString MechanismManager::gui( unsigned int itemID )
{
	QString itemSID = m_mechanisms[itemID].itemSID;
	auto row        = DB::selectRow( "Mechanism", itemSID );
	return row.value( "GUI" ).toString();
}

/// @brief Registers a previously deserialised MechanismData into the position maps and axle table.
///        Used during save-game loading; does not re-read DB values.
/// @param md Fully populated MechanismData to install.
void MechanismManager::installItem( MechanismData md )
{
	if ( md.itemSID == "Axle" )
	{
		m_floorPositions.insert( md.pos.toInt(), md.itemID );

		AxleData ad;
		ad.itemID   = md.itemID;
		ad.anim     = false;
		ad.localRot = md.rot;
		ad.pos      = md.pos;
		ad.spriteID = md.spriteID;

		m_axleData.insert( md.pos.toInt(), ad );
	}
	else if ( md.itemSID == "GearBox" )
	{
		m_floorPositions.insert( md.pos.toInt(), md.itemID );
	}
	else if ( md.itemSID == "SteamEngine" )
	{
		m_wallPositions.insert( md.pos.toInt(), md.itemID );
		g->m_world->setWallSpriteAnim( md.pos, md.active );
	}
	else if ( md.itemSID == "Lever" )
	{
		m_wallPositions.insert( md.pos.toInt(), md.itemID );
	}
	else if ( md.itemSID == "PressurePlate" )
	{
		m_floorPositions.insert( md.pos.toInt(), md.itemID );
	}
	else if ( md.itemSID == "VerticalAxle" )
	{
		m_wallPositions.insert( md.pos.toInt(), md.itemID );

		AxleData ad;
		ad.itemID     = md.itemID;
		ad.anim       = false;
		ad.localRot   = md.rot;
		ad.pos        = md.pos;
		ad.isVertical = true;
		ad.spriteID   = md.spriteID;

		m_axleData.insert( md.pos.toInt(), ad );
	}
	else if ( md.itemSID == "MechanicalWall" )
	{
		m_wallPositions.insert( md.pos.toInt(), md.itemID );
	}
	else if ( md.itemSID == "Pump" )
	{
		m_wallPositions.insert( md.pos.toInt(), md.itemID );
		g->flm()->addInput( md.pos, md.itemID );
	}

	m_mechanisms.insert( md.itemID, md );
}

/// @brief Installs a freshly placed mechanism item: reads DB values, sets up connectivity and
///        position maps for the item type, and schedules a network rebuild.
/// @param itemID Item UID of the mechanism being placed.
/// @param pos    World position to install at.
/// @param rot    Rotation (0–3) used by axles to determine east/west vs north/south alignment.
void MechanismManager::installItem( unsigned int itemID, Position pos, int rot )
{
	MechanismData md;
	md.itemID      = itemID;
	md.itemSID     = g->m_inv->itemSID( itemID );
	md.materialSID = g->m_inv->materialSID( itemID );
	md.spriteID    = g->m_inv->spriteID( itemID );
	md.pos         = pos;
	md.rot         = rot;

	auto row = DB::selectRow( "Mechanism", md.itemSID );
	md.gui   = row.value( "GUI" ).toString();

	md.fuel         = 0;
	md.producePower = row.value( "ProducePower" ).toInt();
	md.maxFuel      = row.value( "MaxFuel" ).toInt();
	md.consumePower = row.value( "ConsumePower" ).toInt();

	md.anim = row.value( "Anim" ).toBool();

	md.type = m_string2Type.value( md.itemSID );

	switch ( md.type )
	{
		case MT_AXLE:
		{
			if ( rot == 0 || rot == 2 )
			{
				md.connectsTo.append( pos.eastOf() );
				md.connectsTo.append( pos.westOf() );
			}
			else if ( rot == 1 || rot == 3 )
			{
				md.connectsTo.append( pos.northOf() );
				md.connectsTo.append( pos.southOf() );
			}

			m_floorPositions.insert( pos.toInt(), itemID );

			AxleData ad;
			ad.itemID   = itemID;
			ad.anim     = false;
			ad.localRot = rot;
			ad.pos      = pos;
			ad.spriteID = md.spriteID;

			m_axleData.insert( pos.toInt(), ad );
		}
		break;
		case MT_GEARBOX:
		{
			md.connectsTo.append( pos.eastOf() );
			md.connectsTo.append( pos.westOf() );
			md.connectsTo.append( pos.northOf() );
			md.connectsTo.append( pos.southOf() );
			md.connectsTo.append( pos.belowOf() );
			md.connectsTo.append( pos );

			m_floorPositions.insert( pos.toInt(), itemID );
		}
		break;
		case MT_ENGINE:
		{
			md.connectsTo.append( pos.eastOf() );
			md.connectsTo.append( pos.westOf() );
			md.connectsTo.append( pos.northOf() );
			md.connectsTo.append( pos.southOf() );

			m_wallPositions.insert( pos.toInt(), itemID );
		}
		break;
		case MT_LEVER:
		{
			md.active = false;

			m_wallPositions.insert( pos.toInt(), itemID );
		}
		break;
		case MT_PRESSUREPLATE:
		{
			md.active       = true;
			md.isInvertable = true;

			m_floorPositions.insert( pos.toInt(), itemID );
		}
		break;
		case MT_PUMP:
		{
			g->flm()->addInput( pos, itemID );
			m_wallPositions.insert( pos.toInt(), itemID );

			md.connectsTo.append( md.pos.eastOf() );
			md.connectsTo.append( md.pos.westOf() );
			md.connectsTo.append( md.pos.northOf() );
			md.connectsTo.append( md.pos.southOf() );
			md.active = true;
		}
		break;
		case MT_VERTICALAXLE:
		{
			md.connectsTo.append( pos );
			md.connectsTo.append( pos.aboveOf() );

			m_wallPositions.insert( pos.toInt(), itemID );

			AxleData ad;
			ad.itemID     = itemID;
			ad.anim       = false;
			ad.localRot   = rot;
			ad.pos        = pos;
			ad.isVertical = true;
			ad.spriteID   = md.spriteID;

			m_axleData.insert( pos.toInt(), ad );
		}
		break;
		case MT_WALL:
		{
			md.connectsTo.append( pos.eastOf() );
			md.connectsTo.append( pos.westOf() );
			md.connectsTo.append( pos.northOf() );
			md.connectsTo.append( pos.southOf() );
			md.active       = true;
			md.isInvertable = true;
			m_wallPositions.insert( pos.toInt(), itemID );
		}
		break;
	}

	m_mechanisms.insert( itemID, md );

	g->m_world->setWallSpriteAnim( md.pos, false );

	m_needNetworkUpdate = true;
}

/// @brief Removes a mechanism from all position maps and the mechanism store,
///        restores walkability for wall-type mechanisms, and schedules a network rebuild.
/// @param itemID Item UID of the mechanism to remove.
void MechanismManager::uninstallItem( unsigned int itemID )
{
	if ( m_mechanisms.contains( itemID ) )
	{
		auto md = m_mechanisms[itemID];

		unsigned int posID = md.pos.toInt();
		m_wallPositions.remove( posID );
		m_floorPositions.remove( posID );
		m_axleData.remove( posID );

		m_mechanisms.remove( itemID );

		switch ( md.type )
		{
			case MT_WALL:
			{
				g->m_world->setWalkable( md.pos, true );
				Tile& tile    = g->m_world->getTile( md.pos );
				tile.wallType = WT_NOWALL;
			}
		}

		m_needNetworkUpdate = true;
	}
}

/// @brief Returns a copy of the MechanismData for @p itemID, or a default-constructed instance if absent.
/// @param itemID Mechanism item UID.
/// @return Copy of the MechanismData.
MechanismData MechanismManager::mechanismData( unsigned int itemID )
{
	if ( m_mechanisms.contains( itemID ) )
	{
		return m_mechanisms[itemID];
	}
	return MechanismData();
}

/// @brief Toggles the changeActive flag on @p itemID, queuing a SwitchMechanism job next tick.
/// @param itemID Mechanism item UID.
/// @return New value of changeActive, or false if the mechanism doesn't exist.
bool MechanismManager::changeActive( unsigned int itemID )
{
	if ( m_mechanisms.contains( itemID ) )
	{
		bool out                          = !m_mechanisms[itemID].changeActive;
		m_mechanisms[itemID].changeActive = out;
		return out;
	}
	return false;
}

/// @brief Sets the active state of @p itemID, updates its connectivity and sprites,
///        and schedules a power network rebuild.
/// @param itemID Mechanism item UID.
/// @param active New active state.
void MechanismManager::setActive( unsigned int itemID, bool active )
{
	qDebug() << "setActive";
	if ( m_mechanisms.contains( itemID ) )
	{
		auto& md = m_mechanisms[itemID];

		md.active = active;

		setConnectsTo( md );

		if( md.maxFuel > 0 )
		{
			updateSpritesAndFlags( md, md.active && ( md.fuel > 0 ) );
		}
		else
		{
			updateSpritesAndFlags( md, md.active );
		}

		m_needNetworkUpdate = true;
	}
}

/// @brief Toggles the changeInverted flag on @p itemID, queuing an InvertMechanism job next tick.
/// @param itemID Mechanism item UID.
/// @return New value of changeInverted, or false if the mechanism doesn't exist.
bool MechanismManager::changeInverted( unsigned int itemID )
{
	if ( m_mechanisms.contains( itemID ) )
	{
		bool out                            = !m_mechanisms[itemID].changeInverted;
		m_mechanisms[itemID].changeInverted = out;
		return out;
	}
	return false;
}

/// @brief Sets the inverted state of @p itemID, updates connectivity, and schedules a network rebuild.
/// @param itemID Mechanism item UID.
/// @param inv    New inverted state.
void MechanismManager::setInverted( unsigned int itemID, bool inv )
{
	if ( m_mechanisms.contains( itemID ) )
	{
		auto& md = m_mechanisms[itemID];

		md.inverted = inv;

		setConnectsTo( md );

		m_needNetworkUpdate = true;
	}
}

/// @brief Recomputes the connectsTo list for levers and pressure plates based on active/inverted state.
///        Active (non-inverted) or inactive (inverted) states add the four cardinal neighbours;
///        otherwise the list is cleared.
/// @param md Mechanism whose connectivity should be updated in-place.
void MechanismManager::setConnectsTo( MechanismData& md )
{
	switch( md.type )
	{
		case MT_LEVER:
		case MT_PRESSUREPLATE:
		md.connectsTo.clear();
			if ( md.inverted )
			{
				if ( !md.active )
				{
					md.connectsTo.append( md.pos.eastOf() );
					md.connectsTo.append( md.pos.westOf() );
					md.connectsTo.append( md.pos.northOf() );
					md.connectsTo.append( md.pos.southOf() );
				}
			}
			else
			{
				if ( md.active )
				{
					md.connectsTo.append( md.pos.eastOf() );
					md.connectsTo.append( md.pos.westOf() );
					md.connectsTo.append( md.pos.northOf() );
					md.connectsTo.append( md.pos.southOf() );
				}
			}
			break;
	}
}

/// @brief Immediately flips the active state of @p itemID and clears the pending changeActive flag.
/// @param itemID Mechanism item UID.
void MechanismManager::toggleActive( unsigned int itemID )
{
	if ( m_mechanisms.contains( itemID ) )
	{
		auto& md = m_mechanisms[itemID];
		setActive( itemID, !md.active );
		md.changeActive = false;
	}
}

/// @brief Immediately flips the inverted state of @p itemID and clears the pending changeInverted flag.
/// @param itemID Mechanism item UID.
void MechanismManager::toggleInvert( unsigned int itemID )
{
	if ( m_mechanisms.contains( itemID ) )
	{
		auto& md = m_mechanisms[itemID];
		setInverted( itemID, !md.inverted );
		md.changeInverted = false;
	}
}

/// @brief Adds @p burnValue fuel to @p itemID, clamped to maxFuel.
///        If the engine was previously empty and now has fuel, triggers a network update and restores animation.
/// @param itemID    Mechanism item UID of the engine to refuel.
/// @param burnValue Amount of fuel ticks to add.
void MechanismManager::refuel( unsigned int itemID, int burnValue )
{
	if ( m_mechanisms.contains( itemID ) )
	{
		int currentFuel = m_mechanisms[itemID].fuel;

		int newFuel = qMin( m_mechanisms[itemID].maxFuel, currentFuel + burnValue );

		m_mechanisms[itemID].fuel = newFuel;

		if ( currentFuel == 0 )
		{
			m_needNetworkUpdate = true;

			if ( newFuel > 0 )
			{
				g->m_world->setWallSpriteAnim( m_mechanisms[itemID].pos, m_mechanisms[itemID].active && ( m_mechanisms[itemID].fuel > 0 ) );
			}
		}
	}
}

/// @brief Sets the fuel percentage at which a refuel job is automatically created.
/// @param itemID  Mechanism item UID.
/// @param percent Threshold percentage (0–100).
void MechanismManager::setRefuelThreshold( unsigned int itemID, int percent )
{
	if ( m_mechanisms.contains( itemID ) )
	{
		m_mechanisms[itemID].refuelThreshold = percent;
	}
}

/// @brief Rebuilds all power networks using BFS from each active engine.
///        Assigns networkIDs, tallies producers/consumers per network, updates axle animation states,
///        and clears m_needNetworkUpdate.
void MechanismManager::updateNetWorks()
{
	QQueue<MechanismData> workQueue;
	m_networks.clear();

	for ( auto& md : m_mechanisms )
	{
		md.networkID = 0;
		md.hasPower  = false;

		if ( md.consumePower > 0 )
		{
			updateSpritesAndFlags( md, false );
		}
	}

	for ( auto md : m_mechanisms )
	{
		if ( md.producePower > 0 && md.networkID == 0 )
		{
			md.networkID = md.itemID;
			workQueue.enqueue( md );

			MechanismNetwork mn;
			mn.id = md.itemID;
			while ( !workQueue.empty() )
			{
				auto md2 = workQueue.dequeue();
				if ( md2.active && md2.fuel > 0 && md2.producePower > 0 )
				{
					mn.produce += md2.producePower;
					mn.producers.insert( md2.itemID );
				}
				if ( md2.active && md2.consumePower > 0 )
				{
					mn.consume += md2.consumePower;
					mn.consumers.insert( md2.itemID );
				}

				for ( auto toPos : md2.connectsTo )
				{
					if ( m_floorPositions.contains( toPos.toInt() ) )
					{
						auto& neighbormd = m_mechanisms[m_floorPositions[toPos.toInt()]];
						if ( neighbormd.networkID == 0 && neighbormd.connectsTo.contains( md2.pos ) )
						{
							neighbormd.networkID = md2.networkID;
							workQueue.enqueue( neighbormd );
						}
					}
					if ( m_wallPositions.contains( toPos.toInt() ) )
					{
						auto& neighbormd = m_mechanisms[m_wallPositions[toPos.toInt()]];
						if ( neighbormd.networkID == 0 && neighbormd.connectsTo.contains( md2.pos ) )
						{
							neighbormd.networkID = md2.networkID;
							workQueue.enqueue( neighbormd );
						}
					}
				}
			}
			m_networks.insert( mn.id, mn );
		}
	}

	for ( auto& ad : m_axleData )
	{
		unsigned int networkID = m_mechanisms[ad.itemID].networkID;
		if ( m_networks[networkID].produce > 0 && m_networks[networkID].produce >= m_networks[networkID].consume )
		{
			ad.anim = true;
		}
		else
		{
			ad.anim = false;
		}

		if ( ad.isVertical )
		{
			g->m_world->setWallSpriteAnim( ad.pos, ad.anim );
		}
	}

	m_axlesChanged      = true;
	m_needNetworkUpdate = false;
}

/// @brief Updates wall/floor sprites and world effects (Wall/Floor) for @p md based on @p isOn.
///        Respects the inverted flag, swaps On/Off sprites from the Mechanism DB table,
///        and calls addEffect/removeEffect for "Wall" and "Floor" effect strings.
/// @param md   Mechanism whose visual state should be updated.
/// @param isOn Desired on/off state (before inversion).
void MechanismManager::updateSpritesAndFlags( MechanismData& md, bool isOn )
{
	QString itemSID = md.itemSID;
	auto row        = DB::selectRow( "Mechanism", itemSID );
	if( md.inverted )
	{
		isOn  = !isOn;
	}

	if ( md.anim )
	{
		g->m_world->setWallSpriteAnim( md.pos, isOn );
	}
	else
	{
		if ( md.anim )
		{
			g->m_world->setWallSpriteAnim( md.pos, isOn );
		}
		else
		{
			if ( isOn )
			{
				QString wallSpriteOn  = row.value( "WallSpriteOn" ).toString();
				QString floorSpriteOn = row.value( "FloorSpriteOn" ).toString();
				if ( !wallSpriteOn.isEmpty() )
				{
					g->m_world->setWallSprite( md.pos, g->m_sf->createSprite( wallSpriteOn, { md.materialSID } )->uID );
					g->m_world->addToUpdateList( md.pos );
				}
				if ( !floorSpriteOn.isEmpty() )
				{
					g->m_world->setFloorSprite( md.pos, g->m_sf->createSprite( floorSpriteOn, { md.materialSID } )->uID );
					g->m_world->addToUpdateList( md.pos );
				}
			}
			else
			{
				QString wallSpriteOff  = row.value( "WallSpriteOff" ).toString();
				QString floorSpriteOff = row.value( "FloorSpriteOff" ).toString();

				if ( !wallSpriteOff.isEmpty() )
				{
					g->m_world->setWallSprite( md.pos, g->m_sf->createSprite( wallSpriteOff, { md.materialSID } )->uID );
					g->m_world->addToUpdateList( md.pos );
				}
				if ( !floorSpriteOff.isEmpty() )
				{
					g->m_world->setFloorSprite( md.pos, g->m_sf->createSprite( floorSpriteOff, { md.materialSID } )->uID );
					g->m_world->addToUpdateList( md.pos );
				}
			}
		}
	}

	QString effectOn  = row.value( "EffectOn" ).toString();
	QString effectOff = row.value( "EffectOff" ).toString();

	if ( isOn )
	{
		removeEffect( md.pos, effectOff );
		addEffect( md.pos, effectOn );
	}
	else
	{
		removeEffect( md.pos, effectOn );
		addEffect( md.pos, effectOff );
	}
}

/// @brief Applies a named world effect at @p pos ("Wall" makes tile solid and unwalkable; "Floor" makes it walkable).
/// @param pos    World position to affect.
/// @param effect Effect string ID from the Mechanism DB table.
void MechanismManager::addEffect( Position pos, QString effect )
{
	if ( effect == "Wall" )
	{
		g->m_world->setWalkable( pos, false );
		Tile& tile    = g->m_world->getTile( pos );
		tile.wallType = WT_SOLIDWALL;

		// TODO if floor above crush tile inhabitants
	}
	else if ( effect == "Floor" )
	{
		g->m_world->setWalkable( pos, true );
	}
}

/// @brief Reverses a named world effect at @p pos ("Wall" restores walkability and removes solid wall;
///        "Floor" makes the tile unwalkable again).
/// @param pos    World position to affect.
/// @param effect Effect string ID to reverse.
void MechanismManager::removeEffect( Position pos, QString effect )
{
	if ( effect == "Wall" )
	{
		g->m_world->setWalkable( pos, true );
		Tile& tile    = g->m_world->getTile( pos );
		tile.wallType = WT_NOWALL;
	}
	else if ( effect == "Floor" )
	{
		g->m_world->setWalkable( pos, false );
	}
}

/// @brief Notifies the mechanism at @p pos of the creature count standing on it.
///        Activates pressure plates when numCreatures > 0 and deactivates them when empty.
/// @param pos          World position of the mechanism.
/// @param numCreatures Number of creatures currently at this tile.
void MechanismManager::updateCreaturesAtPos( Position pos, int numCreatures )
{
	unsigned int id = mechanismID( pos );
	if ( id )
	{
		auto& md = m_mechanisms[id];
		if ( md.type == MT_PRESSUREPLATE )
		{
			if ( numCreatures > 0 )
			{
				setActive( id, true );
			}
			else
			{
				setActive( id, false );
			}
		}
	}
}