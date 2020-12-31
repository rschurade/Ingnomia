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

#include "aggregatortileinfo.h"

#include "../base/counter.h"
#include "../base/db.h"
#include "../base/dbhelper.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../game/game.h"
#include "../game/creaturemanager.h"
#include "../game/farmingmanager.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/job.h"
#include "../game/jobmanager.h"
#include "../game/mechanismmanager.h"
#include "../game/plant.h"
#include "../game/stockpilemanager.h"
#include "../game/workshopmanager.h"
#include "../game/world.h"
#include "../gui/strings.h"

AggregatorTileInfo::AggregatorTileInfo( QObject* parent ) :
	QObject(parent)
{
	qRegisterMetaType<GuiTileInfo>();
}

AggregatorTileInfo::~AggregatorTileInfo()
{
}

void AggregatorTileInfo::init( Game* game )
{
	g = game;
}

void AggregatorTileInfo::onShowTileInfo( unsigned int tileID )
{
	if( !g ) return;
	if ( m_currentTileID != tileID )
	{
		m_currentTileID = tileID;
		m_tileInfoDirty = true;
	}
	emit signalShowTileInfo( tileID );
	onUpdateTileInfo( tileID );
}

void AggregatorTileInfo::onUpdateAnyTileInfo( const QSet<unsigned int>& changeSet )
{
	if( !g ) return;
	if ( changeSet.contains( m_currentTileID ) )
	{
		onUpdateTileInfo( m_currentTileID );
	}
}

void AggregatorTileInfo::onUpdateTileInfo( unsigned int tileID )
{
	if( !g ) return;
	if ( m_currentTileID == tileID )
	{
		Position pos( tileID );
		m_tileInfo.tileID      = tileID;
		m_tileInfo.numGnomes   = g->gm()->gnomesAtPosition( pos ).size();
		m_tileInfo.numAnimals  = g->cm()->animalsAtPosition( pos ).size();
		m_tileInfo.numMonsters = g->cm()->monstersAtPosition( pos ).size();
		m_tileInfo.numItems    = g->inv()->countItemsAtPos( pos );

		Tile& tile   = g->w()->getTile( pos );

		m_tileInfo.flags = tile.flags;

		m_tileInfo.possibleTennants.clear();

		m_tileInfo.wall        = "";
		m_tileInfo.floor       = "";
		m_tileInfo.embedded    = "";
		m_tileInfo.plant       = "";
		m_tileInfo.water       = "";
		m_tileInfo.constructed = "";

		if ( (bool)( tile.wallType & WT_SOLIDWALL ) && (bool)tile.wallMaterial )
		{
			QString wallSID = DBH::materialSID( tile.wallMaterial );
			QString wType   = DB::select( "Type", "Materials", wallSID ).toString();
			m_tileInfo.wall = "Wall: " + S::s( "$MaterialName_" + wallSID ) + " " + S::s( "$GroupName_" + wType ).toLower();
		}

		if ( tile.embeddedMaterial )
		{
			QString embeddedSID = DBH::materialSID( tile.embeddedMaterial );
			QString eType       = DB::select( "Type", "Materials", embeddedSID ).toString();
			m_tileInfo.embedded = "Embedded: " + S::s( "$MaterialName_" + embeddedSID ) + " " + S::s( "$GroupName_" + eType ).toLower();
		}

		if ( tile.floorMaterial )
		{
			QString floorSID = DBH::materialSID( tile.floorMaterial );
			QString fType    = DB::select( "Type", "Materials", floorSID ).toString();
			m_tileInfo.floor = "Floor: " + S::s( "$MaterialName_" + floorSID ) + " " + S::s( "$GroupName_" + fType ).toLower();
		}

		if ( g->w()->plants().contains( pos.toInt() ) )
		{
			Plant& plant                  = g->w()->plants()[pos.toInt()];
			m_tileInfo.plant              = "Plant: " + plant.getDesignation();
			m_tileInfo.plantIsTree        = plant.isTree();
			m_tileInfo.plantIsHarvestable = plant.harvestable();
		}

		if ( tile.fluidLevel )
		{
			m_tileInfo.water = "Water: " + QString::number( ( 100 / 10 ) * tile.fluidLevel ) + "% ";
		}

		m_tileInfo.creatures.clear();
		m_tileInfo.items.clear();

		if ( m_tileInfo.numItems )
		{
			PositionEntry pe;

			g->inv()->getObjectsAtPosition( pos, pe );

			Counter<QString> counter;
			for ( auto item : pe )
			{
				QString itext = "";
				QString info = S::s( "$MaterialName_" + g->inv()->materialSID( item ) ) + " " + S::s( "$ItemName_" + g->inv()->itemSID( item ) );
				
				if( g->inv()->isConstructed( item ) )
				{
					if( g->mcm()->hasMechanism( pos ) )
					{
						unsigned int id = g->mcm()->mechanismID( pos );
						m_tileInfo.mechInfo = g->mcm()->mechanismData( id );
						m_tileInfo.mechInfo.name = info;
						if( item == id )
						{
							continue;
						}
					}
					else
					{
						m_tileInfo.mechInfo.itemID = 0;
					}
					itext += "b";
				}
				if( g->inv()->isInStockpile( item ) )
				{
					itext += "s";
				}
				if( g->inv()->isInContainer( item ) )
				{
					itext += "c";
				}
				if( itext.size() )
				{
					info += "(";
					info += itext;
					info += ")";
				}
				counter.add( info );
			}
			for ( auto key : counter.keys() )
			{
				GuiItemInfo git;
				git.text = QString::number( counter.count( key ) ) + "x " + key;
				//git.id = item;
				m_tileInfo.items.append( git );
			}
		}
		if ( ( m_tileInfo.numAnimals + m_tileInfo.numGnomes + m_tileInfo.numMonsters ) > 0 )
		{
			if ( m_tileInfo.numGnomes )
			{
				for ( auto gnome : g->gm()->gnomesAtPosition( pos ) )
				{
					GuiTICreatureInfo gct;
					gct.text = "Gnome: " + gnome->name();
					gct.id   = gnome->id();
					gct.type = gnome->type();
					if( gct.type == CreatureType::AUTOMATON )
					{
						auto automat = dynamic_cast<Automaton*>( gnome );
						gct.text = "Automaton: " + gnome->name();
						gct.refuel = automat->getRefuelFlag();
						gct.coreItem = automat->coreType();
					}

					m_tileInfo.creatures.append( gct );
				}
			}
			if ( m_tileInfo.numAnimals )
			{
				for ( auto animal : g->cm()->animalsAtPosition( pos ) )
				{
					GuiTICreatureInfo gct;
					gct.text = "Animal: " + S::s( "$CreatureName_" + animal->name() );
					gct.id   = animal->id();
					m_tileInfo.creatures.append( gct );
				}
			}
			if ( m_tileInfo.numMonsters )
			{
				for ( auto monster : g->cm()->monstersAtPosition( pos ) )
				{
					GuiTICreatureInfo gct;
					gct.text = "Monster: " + S::s( "$CreatureName_" + monster->name() );
					gct.id   = monster->id();
					m_tileInfo.creatures.append( gct );
				}
			}
		}
		// wrap job into a sub-object?
		auto job                         = g->jm()->getJobAtPos( pos );
		m_tileInfo.jobName               = "";
		m_tileInfo.jobWorker             = "";
		m_tileInfo.jobPriority           = "";
		m_tileInfo.requiredTool          = "";
		m_tileInfo.requiredToolAvailable = ""; // currently just 'exists'
		m_tileInfo.requiredItems.clear();      // exists & is reachable
		m_tileInfo.workPositions = "";
		if ( job )
		{
			m_tileInfo.jobName = job->type();
			auto gnome         = g->gm()->gnome( job->workedBy() );
			if ( gnome )
			{
				m_tileInfo.jobWorker = gnome->name();
			}

			auto rt = job->requiredTool();
			if ( !rt.type.isEmpty() )
			{
				m_tileInfo.requiredTool = rt.type + " level " + QString::number( rt.level );
			}

			m_tileInfo.jobPriority           = QString::number( job->priority() );
			m_tileInfo.requiredSkill         = job->requiredSkill();
			m_tileInfo.requiredToolAvailable = rt.available ? "Yes" : "No";

			for ( auto rim : job->requiredItems() )
			{
				GuiItemInfo git;
				git.text     = rim.itemSID;
				git.count    = rim.count;
				git.material = rim.materialSID;

				m_tileInfo.requiredItems.append( git );
			}

			// gnome available
			// use the CanWork?
			// use the gnomemanager to query whether a gnome exists with that skill enabled?

			m_tileInfo.workPositions = QVariant( job->possibleWorkPositions().size() > 0 ).toString();
		}

		m_tileInfo.designationID   = 0;
		m_tileInfo.designationName = "";
		m_tileInfo.designationFlag = m_tileInfo.flags - ~( TileFlag::TF_WORKSHOP + TileFlag::TF_STOCKPILE + TileFlag::TF_GROVE + TileFlag::TF_FARM + TileFlag::TF_PASTURE + TileFlag::TF_ROOM );
		switch ( m_tileInfo.designationFlag )
		{
			case TileFlag::TF_WORKSHOP:
			{
				auto ws = g->wsm()->workshopAt( pos );
				if ( ws )
				{
					m_tileInfo.designationID   = ws->id();
					m_tileInfo.designationName = ws->name();
				}
			}
			break;
			case TileFlag::TF_STOCKPILE:
			{
				auto sp = g->spm()->getStockpileAtPos( pos );
				if ( sp )
				{
					m_tileInfo.designationID   = sp->id();
					m_tileInfo.designationName = sp->name();
				}
			}
			break;
			case TileFlag::TF_GROVE:
			{
				auto gr = g->fm()->getGroveAtPos( pos );
				if ( gr )
				{
					m_tileInfo.designationID   = gr->id();
					m_tileInfo.designationName = gr->name();
				}
			}
			break;
			case TileFlag::TF_FARM:
			{
				auto fa = g->fm()->getFarmAtPos( pos );
				if ( fa )
				{
					m_tileInfo.designationID   = fa->id();
					m_tileInfo.designationName = fa->name();
				}
			}
			break;
			case TileFlag::TF_PASTURE:
			{
				auto pa = g->fm()->getPastureAtPos( pos );
				if ( pa )
				{
					m_tileInfo.designationID   = pa->id();
					m_tileInfo.designationName = pa->name();
				}
			}
			break;
			case TileFlag::TF_ROOM:
			{
				auto ro = g->rm()->getRoomAtPos( pos );
				if ( ro )
				{
					m_tileInfo.designationID   = ro->id();
					m_tileInfo.designationName = ro->name();
					m_tileInfo.roomType        = ro->type();
					m_tileInfo.tennant         = ro->owner();
					ro->checkEnclosed();
					m_tileInfo.isEnclosed = ro->enclosed();
					ro->checkRoofed();
					m_tileInfo.hasRoof      = ro->roofed();
					m_tileInfo.hasAlarmBell = ro->hasAlarmBell();
					m_tileInfo.alarm        = GameState::alarm;
					m_tileInfo.roomValue    = ro->value();              

					QList<unsigned int> beds = ro->beds();
					int countFree            = beds.size();
					for ( auto b : beds )
					{
						if ( g->inv()->isInJob( b ) )
						{
							--countFree;
						}
					}
					m_tileInfo.beds = QString::number( countFree ) + " / " + QString::number( beds.size() );

					for ( auto gnome : g->gm()->gnomes() )
					{
						GuiTICreatureInfo gci { gnome->name(), gnome->id(), gnome->type(), false, "" };
						if ( gnome->ownedRoom() )
						{
							if ( gnome->ownedRoom() == ro->id() )
							{
								m_tileInfo.tennant = gnome->id();
							}
							gci.text += " (R)";
						}
						m_tileInfo.possibleTennants.append( gci );
					}
				}
				break;
			}
		}

		emit signalUpdateTileInfo( m_tileInfo );
	}
}

void AggregatorTileInfo::onRequestStockpileItems( unsigned int tileID )
{
	if( !g ) return;
	auto sp = g->spm()->getStockpileAtTileID( tileID );

	if ( sp )
	{
		m_spInfo.stockpileID       = sp->id();
		m_spInfo.name              = sp->name();
		m_spInfo.priority          = sp->priority();
		m_spInfo.maxPriority       = g->spm()->maxPriority();
		m_spInfo.suspended         = !sp->active();
		m_spInfo.allowPullFromHere = sp->allowsPull();
		m_spInfo.pullFromOthers    = sp->pullsOthers();
		m_spInfo.capacity          = sp->capacity( tileID );
		m_spInfo.itemCount         = sp->itemCount( tileID );
		m_spInfo.reserved          = sp->reserved( tileID );

		m_spInfo.filter = sp->filter();

		m_spInfo.summary.clear();

		auto active = m_spInfo.filter.getActive();
		for ( auto entry : active )
		{
			int count = sp->count( entry.first, entry.second );
			if ( count > 0 )
			{
				//QIcon icon( Global::util->smallPixmap( Global::sf().createSprite( entry.first, { entry.second } ), season, 0 ) );
				ItemsSummary is;
				is.itemName     = S::s( "$ItemName_" + entry.first );
				is.materialName = S::s( "$MaterialName_" + entry.second );
				is.count        = count;

				m_spInfo.summary.append( is );
			}
		}
		emit signalUpdateSPInfo( m_spInfo );
	}
}

void AggregatorTileInfo::onSetTennant( unsigned int designationID, unsigned int gnomeID )
{
	if( !g ) return;
	auto room = g->rm()->getRoom( designationID );
	if ( room )
	{
		auto oldOwner = room->owner();

		room->setOwner( gnomeID );
		auto gnome = g->gm()->gnome( gnomeID );
		if ( gnome )
		{
			gnome->setOwnedRoom( designationID );
		}

		auto oldGnome = g->gm()->gnome( oldOwner );
		if ( oldGnome )
		{
			oldGnome->setOwnedRoom( 0 );
		}
	}
}

void AggregatorTileInfo::onSetAlarm( unsigned int designationID, bool value )
{
	if( !g ) return;
	switch ( GameState::alarm )
	{
		case 0:
			// create alarm job
			if ( g->rm()->createAlarmJob( designationID ) )
			{
				GameState::alarm = 1;
			}
			break;
		case 1:
			//cancel alarm job;
			g->rm()->cancelAlarmJob( designationID );
		case 2:
			GameState::alarmRoomID = 0;
			GameState::alarm       = 0;
			break;
	}
}

void AggregatorTileInfo::onToggleMechActive( unsigned int id )
{
	g->mcm()->changeActive( id );
}
	
void AggregatorTileInfo::onToggleMechInvert( unsigned int id )
{
	g->mcm()->changeInverted( id );
}

void AggregatorTileInfo::onSetAutomatonRefuel( unsigned int id, bool refuel )
{
	auto automat = g->gm()->automaton( id );
	if( automat )
	{
		automat->setRefuelFlag( refuel );
	}
}
	
void AggregatorTileInfo::onSetAutomatonCore( unsigned int id, QString core )
{
	auto automat = g->gm()->automaton( id );
	if( automat )
	{
		automat->setCoreType( core );
	}
}