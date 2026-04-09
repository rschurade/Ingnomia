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
/** @file pasture.cpp
 *  @brief Animal pasture implementation: animal assignment, capacity management, feeding,
 *         hay harvesting, trough filling, and animal type configuration.
 */
#include "pasture.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/game.h"
#include "../game/creaturemanager.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/jobmanager.h"
#include "../game/world.h"

#include <QDebug>

/// @brief Deserialising constructor — restores pasture configuration from a saved variant map.
/// @param in Map produced by PastureProperties::serialize().
PastureProperties::PastureProperties( QVariantMap& in )
{
	animalType = in.value( "AnimalType" ).toString();
	harvest    = in.value( "Harvest" ).toBool();
	harvestHay = in.value( "HarvestHay" ).toBool();
	maxHay     = in.value( "MaxHay" ).toInt();

	max           = in.value( "Max" ).toInt();
	maxMale       = in.value( "MaxMale" ).toInt();
	maxFemale     = in.value( "MaxFemale" ).toInt();
	tameWild      = in.value( "TameWild" ).toBool();
	collectEggs   = in.value( "CollectEggs" ).toBool();
	onlyAvailable = in.value( "OnlyAvailable" ).toBool();

	maxTroughCapacity = in.value( "MaxTroughCapacity" ).toInt();
	troughContent     = in.value( "TroughContent" ).toInt();

	{
		auto list = in.value( "Food" ).toStringList();
		foodSettings = QSet<QString>( list.begin(), list.end() );
	}

	animalSize = DB::select( "PastureSize", "Animals", animalType ).toInt();
}

/// @brief Serialises pasture configuration into an existing variant map.
/// @param out Map to write keys into (Type, AnimalType, harvest/hay flags, capacity limits, food settings).
void PastureProperties::serialize( QVariantMap& out ) const
{
	out.insert( "Type", "pasture" );
	out.insert( "AnimalType", animalType );
	out.insert( "Harvest", harvest );
	out.insert( "HarvestHay", harvestHay );
	out.insert( "MaxHay", maxHay );

	out.insert( "Max", max );
	out.insert( "MaxMale", maxMale );
	out.insert( "MaxFemale", maxFemale );
	out.insert( "TameWild", tameWild );
	out.insert( "CollectEggs", collectEggs );
	out.insert( "OnlyAvailable", onlyAvailable );

	out.insert( "MaxTroughCapacity", maxTroughCapacity );
	out.insert( "TroughContent", troughContent );

	out.insert( "Food", QStringList( foodSettings.values() ) );
}

/// @brief Constructs a new pasture from a list of tile positions.
/// @param tiles List of (position, included) pairs; only tiles with second==true are registered.
/// @param game  Owning game instance.
Pasture::Pasture( QList<QPair<Position, bool>> tiles, Game* game ) :
	WorldObject( game )
{
	m_name = "Pasture";

	for ( auto p : tiles )
	{
		if ( p.second )
		{
			PastureField* grofi = new PastureField;
			grofi->pos = p.first;
			m_fields.insert( p.first.toInt(), grofi );
		}
	}
	if ( m_fields.size() )
	{
		for ( auto field : m_fields )
		{
			m_properties.firstPos = field->pos;
			break;
		}
	}
}

/// @brief Deserialising constructor — restores a pasture from a saved variant map.
/// @param vals Map produced by Pasture::serialize().
/// @param game Owning game instance.
Pasture::Pasture( QVariantMap vals, Game* game ) :
	WorldObject( vals, game ),
	m_properties( vals )
{
	QVariantList vfl = vals.value( "Fields" ).toList();
	for ( auto vf : vfl )
	{
		PastureField* grofi = new PastureField;
		auto vfm = vf.toMap();
		grofi->pos = Position( vfm.value( "Pos" ).toString() );
		if( vfm.contains( "Job" ) )
		{
			grofi->job = g->jm()->getJob( vfm.value( "Job" ).toUInt() );
		}
		grofi->utilSID = vf.toMap().value( "UtilSID" ).toString();
		m_fields.insert( grofi->pos.toInt(), grofi );
	}
	if ( m_fields.size() )
	{
		for ( auto field : m_fields )
		{
			m_properties.firstPos = field->pos;
			break;
		}
	}

	QVariantList val = vals.value( "Animals" ).toList();
	for ( auto va : val )
	{
		m_animals.append( va.toUInt() );
	}
}
/*
Pasture::Pasture( const Pasture& other )
{
	m_properties = other.m_properties;
	m_fields = other.m_fields;
	m_animals = other.m_animals;
	m_animalsInJob = other.m_animalsInJob;
	m_jobsOut = other.m_jobsOut;
}
*/

/// @brief Serialises the pasture (WorldObject base, properties, fields, and animal ID list).
/// @return QVariant wrapping a QVariantMap suitable for saving.
QVariant Pasture::serialize() const
{
	QVariantMap out;
	WorldObject::serialize( out );
	m_properties.serialize( out );

	QVariantList tiles;
	for ( auto field : m_fields )
	{
		QVariantMap entry;
		entry.insert( "Pos", field->pos.toString() );
		if( field->job )
		{
			QSharedPointer<Job> spJob = field->job.toStrongRef();
			entry.insert( "Job", spJob->id() );
		}
		entry.insert( "UtilSID", field->utilSID );
		tiles.append( entry );
	}
	out.insert( "Fields", tiles );

	QVariantList animals;
	for ( auto an : m_animals )
	{
		animals.append( an );
	}
	out.insert( "Animals", animals );

	return out;
}

/// @brief Destructor — frees all PastureField allocations.
Pasture::~Pasture()
{
	for ( const auto& field : m_fields )
	{
		delete field;
	}
}

/// @brief Adds a tile to the pasture, sets its TF_PASTURE flag, and recalculates max animal capacity.
/// @param pos World position of the tile to add.
void Pasture::addTile( const Position & pos )
{
	PastureField* grofi = new PastureField;
	grofi->pos = pos;
	m_fields.insert( pos.toInt(), grofi );

	if( m_properties.animalSize != 0 )
	{
		m_properties.max = m_fields.size() / m_properties.animalSize;
	}
	else
	{
		m_properties.max = 0;
	}

	g->w()->setTileFlag( pos, TileFlag::TF_PASTURE );
}

/// @brief Per-tick update called by the farming manager.
///        Regrows vegetation on grass tiles, auto-assigns tame animals to open slots,
///        leads straying animals back, creates HarvestAnimal/FillTrough/HarvestHay jobs as needed.
/// @param tick   Current game tick.
/// @param count  Output: total number of animals currently assigned to this pasture.
void Pasture::onTick( quint64 tick, int& count )
{
	for ( auto field : m_fields )
	{
		Tile& tile = g->w()->getTile( field->pos );
		if( GameState::season != 3 )
		{
			if ( tile.flags & TileFlag::TF_GRASS )
			{
				if( tick % 500 == 0 )
				{
					tile.vegetationLevel = qMin( 100, tile.vegetationLevel + 1 );
				}
			}
		}
	}
	if ( !m_active )
		return;

	int countMale   = 0;
	int countFemale = 0;
	for ( auto id : m_animals )
	{
		Animal* animal = g->cm()->animal( id );
		if ( animal )
		{
			if ( animal->gender() == Gender::MALE )
			{
				++countMale;
			}
			else
			{
				++countFemale;
			}
		}
	}
	count = countMale + countFemale;

	for ( auto& a : g->cm()->animals() )
	{
		if ( a->species() == m_properties.animalType && a->isTame() && a->pastureID() == 0 && !a->toDestroy() && !a->isDead() )
		{
			if ( a->gender() == Gender::MALE && countMale < m_properties.maxMale )
			{
				m_animals.append( a->id() );
				a->setPastureID( id() );
				++countMale;
				if ( m_animals.size() >= m_properties.max )
					return;
			}
			else if ( a->gender() == Gender::FEMALE && countFemale < m_properties.maxFemale )
			{
				m_animals.append( a->id() );
				a->setPastureID( id() );
				++countFemale;
				if ( m_animals.size() >= m_properties.max )
					return;
			}
		}
	}

	// lead animals back to pasture
	for ( auto id : m_animals )
	{
		Animal* animal = g->cm()->animal( id );
		int posID      = animal->getPos().toInt();
		if ( !m_fields.contains( posID ) && !animal->inJob() )
		{
			if ( g->w()->regionMap().checkConnectedRegions( m_fields.first()->pos, animal->getPos() ) )
			{
				// find a free field first
				QList<Position>freeFields;
				for ( auto& field : m_fields )
				{
					if( !field->job )
					{
						freeFields.append( field->pos );
					}
				}
				if( freeFields.size() )
				{
					int random = rand() % freeFields.size();
					auto targetPos = freeFields[random];

					auto jobID = g->jm()->addJob( "LeadAnimalToPasture", animal->getPos(), 0, false );
					auto job = g->jm()->getJob( jobID );
					if( job )
					{
						job->setAnimal( animal->id() );
						animal->setInJob( job->id() );
						animal->setImmobile( true );
						job->setPosItemInput( targetPos );
						m_fields[targetPos.toInt()]->job = job;
					}
				}
				else
				{
					break; // no reason to check other animals
				}
			}
		}
	}
	// harvest animals
	for ( auto id : m_animals )
	{
		Animal* animal = g->cm()->animal( id );
		if ( animal && animal->numProduce() > 0 && !animal->inJob() )
		{
			if ( m_fields.contains( animal->getPos().toInt() ) )
			{
				if( !m_fields[animal->getPos().toInt()]->job )
				{
					auto jobID = g->jm()->addJob( "HarvestAnimal", animal->getPos(), 0, false );
					auto job = g->jm()->getJob( jobID );
					if( job )
					{
						job->setAnimal( animal->id() );
						animal->setInJob( job->id() );
						animal->setImmobile( true );
						m_fields[animal->getPos().toInt()]->job = job;
					}
				}
			}
		}
	}
	//fill troughs
	if ( !m_properties.foodSettings.isEmpty() && m_properties.maxTroughCapacity > 0 )
	{
		if ( m_properties.troughContent < m_properties.maxTroughCapacity - 9 )
		{
			for ( auto foodString : m_properties.foodSettings )
			{
				auto fsl = foodString.split( "_" );
				if ( g->inv()->itemCount( fsl[0], fsl[1] ) > 0 )
				{
					for ( auto& field : m_fields )
					{
						if ( !field->utilSID.isEmpty() && !field->job )
						{
							if ( field->utilSID == "Trough" )
							{
								auto jobID = g->jm()->addJob( "FillTrough", field->pos, 0, false );
								auto job = g->jm()->getJob( jobID );
								if( job )
								{
									job->setPosItemInput( field->pos );
									job->addRequiredItem( 1, fsl[0], fsl[1], {} );
									field->job = job;
								}
							}
						}
					}
				}
			}
		}
	}
	//harvest hay
	if ( m_properties.harvestHay && GameState::season != 3 )
	{
		if ( g->inv()->itemCount( "Hay", "any" ) < (unsigned int)m_properties.maxHay )
		{
			for ( auto& field : m_fields )
			{
				if ( !field->job && g->w()->hasMaxGrass( field->pos ) )
				{
					auto jobID = g->jm()->addJob( "HarvestHay", field->pos, 0, false );
					auto job = g->jm()->getJob( jobID );
					if( job )
					{
						field->job = job;
					}
				}
			}
		}
	}
}

/// @brief Attempts to assign a single tame animal of the correct type and gender to this pasture.
///        Does nothing if the pasture is inactive, already full, or the animal does not match.
/// @param animalID UID of the animal to assign.
void Pasture::addAnimal( unsigned int animalID )
{
	if ( !m_active )
		return;

	if ( m_animals.size() >= m_properties.max )
		return;
	int countMale   = 0;
	int countFemale = 0;
	for ( auto id : m_animals )
	{
		if ( g->cm()->animal( id )->gender() == Gender::MALE )
		{
			++countMale;
		}
		else
		{
			++countFemale;
		}
	}

	auto a = g->cm()->animal( animalID );

	if ( a && a->species() == m_properties.animalType && a->isTame() && a->pastureID() == 0 && !a->toDestroy() && !a->isDead() )
	{
		if ( a->gender() == Gender::MALE && countMale < m_properties.maxMale )
		{
			m_animals.append( a->id() );
			a->setPastureID( id() );
			++countMale;
			if ( m_animals.size() >= m_properties.max )
				return;
		}
		else if ( a->gender() == Gender::FEMALE && countFemale < m_properties.maxFemale )
		{
			m_animals.append( a->id() );
			a->setPastureID( id() );
			++countFemale;
			if ( m_animals.size() >= m_properties.max )
				return;
		}
	}
}



/*
			if ( m_properties.tameWild )
			{
				if ( m_animals.size() < m_properties.max )
				{
					int countMale   = 0;
					int countFemale = 0;
					for ( auto id : m_animals )
					{
						if ( g->cm()->animal( id )->gender() == Gender::MALE )
						{
							++countMale;
						}
						else
						{
							++countFemale;
						}
					}
					int random = rand() % m_fields.size();
					Position fieldPos;
					for ( auto field : m_fields )
					{
						if ( random == 0 )
						{
							fieldPos = field->pos;
							break;
						}
						--random;
					}

					auto animals = g->cm()->animalsByDistance( fieldPos, m_properties.animalType );
					Animal* a    = nullptr;
					while ( !animals.empty() )
					{
						a = animals.get();
						if ( !a->isTame() && a->pastureID() == 0 && !a->toDestroy() && !a->isDead() )
						{
							if ( a->gender() == Gender::MALE && countMale < m_properties.maxMale )
							{
								break;
							}
							else if ( a->gender() == Gender::FEMALE && countFemale < m_properties.maxFemale )
							{
								break;
							}
						}
						a = nullptr;
					}
					if ( a )
					{
						QSharedPointer<Job> job( new Job() );

						job->setType( "TameAnimal" );
						job->setRequiredSkill( "AnimalHusbandry" );
						job->setPos( a->getPos() );
						job->addPossibleWorkPosition( a->getPos() );
						job->setAnimal( a->id() );
						job->setPosItemInput( fieldPos );
						a->setInJob( job->id() );
						a->setImmobile( true );
						job->setNoJobSprite( true );
						m_animalsInJob.insert( a->id() );

						return job;
					}
				}
			}
*/

/// @brief Removes a tile from the pasture, recalculates capacity, and evicts excess animals.
/// @param pos Position of the tile to remove.
/// @return true if this was the last tile and the pasture should be deleted, false otherwise.
bool Pasture::removeTile( const Position & pos )
{
	PastureField* ff = m_fields.value( pos.toInt() );
	m_fields.remove( pos.toInt() );
	delete ff;

	g->w()->clearTileFlag( pos, TileFlag::TF_PASTURE );


	if ( m_fields.size() )
	{
		for ( auto field : m_fields )
		{
			m_properties.firstPos = field->pos;
			break;
		}
	}
	else
	{
		m_properties.firstPos = Position();
	}

	if( m_properties.animalSize != 0 )
	{
		m_properties.max = m_fields.size() / m_properties.animalSize;
	}
	else
	{
		m_properties.max = 0;
	}
	m_properties.maxMale   = qMin( m_properties.max, m_properties.maxMale );
	m_properties.maxFemale = qMin( m_properties.max, m_properties.maxFemale );

	while ( m_animals.size() && m_properties.max < m_animals.size() )
	{
		auto id        = m_animals.takeLast();
		Animal* animal = g->cm()->animal( id );
		if ( animal )
		{
			animal->setPastureID( 0 );
		}
	}

	// if last tile deleted return true
	return m_fields.empty();
}

/// @brief Fills out plot and animal gender counts for UI display.
/// @param numPlots  Output: total number of field tiles.
/// @param numMale   Output: number of male animals currently assigned.
/// @param numFemale Output: number of female animals currently assigned.
void Pasture::getInfo( int& numPlots, int& numMale, int& numFemale )
{
	numPlots  = m_fields.size();
	numMale   = 0;
	numFemale = 0;

	for ( auto id : m_animals )
	{
		Animal* animal = g->cm()->animal( id );
		if( animal )
		{
			if ( animal->gender() == Gender::MALE )
			{
				++numMale;
			}
			else if ( animal->gender() == Gender::FEMALE )
			{
				++numFemale;
			}
		}
	}
}

/// @brief Removes a single animal from the pasture assignment list and clears its pasture reference.
/// @param animalID UID of the animal to remove.
void Pasture::removeAnimal( unsigned int animalID )
{
	Animal* animal = g->cm()->animal( animalID );
	if ( animal )
	{
		animal->setFollowID( 0 );
		animal->setPastureID( 0 );
	}

	m_animals.removeAll( animalID );

	int plots;
	int male;
	int female;
	getInfo( plots, male, female );
}

/// @brief Removes all animals from the pasture, clearing their pasture reference and the internal list.
void Pasture::removeAllAnimals()
{
	for ( auto id : m_animals )
	{
		Animal* animal = g->cm()->animal( id );
		if ( animal )
		{
			animal->setFollowID( 0 );
			animal->setPastureID( 0 );
		}
	}
	m_animals.clear();

	int plots;
	int male;
	int female;
	getInfo( plots, male, female );
}

/// @brief Returns true; pastures can always be deleted by the player.
/// @return Always true.
bool Pasture::canDelete()
{
	return true;
}

/// @brief Returns the total number of field tiles in this pasture.
/// @return Number of fields.
int Pasture::countTiles()
{
	return m_fields.size();
}

/// @brief Places a utility object (e.g. Trough, Shed) on the given pasture field tile.
///        Increases maxTroughCapacity by 20 for each Trough added.
/// @param pos     Position of the field tile.
/// @param utilSID String ID of the utility to place.
/// @return true if the tile exists and was not already occupied, false otherwise.
bool Pasture::addUtil( Position pos, const QString& utilSID )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		PastureField* pf = m_fields[pos.toInt()];
		if ( pf->utilSID.isEmpty() )
		{
			pf->utilSID = utilSID;

			if ( utilSID == "Trough" )
			{
				m_properties.maxTroughCapacity += 20;
			}

			return true;
		}
	}
	return false;
}

/// @brief Removes the utility object from the given pasture field tile.
///        Decreases maxTroughCapacity by 20 if the removed utility was a Trough.
/// @param pos Position of the field tile.
/// @return true if the tile existed and held a utility, false otherwise.
bool Pasture::removeUtil( Position pos )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		PastureField* pf = m_fields[pos.toInt()];

		if ( !pf->utilSID.isEmpty() )
		{
			if ( pf->utilSID == "Trough" )
			{
				m_properties.maxTroughCapacity -= 20;
			}

			pf->utilSID.clear();
			return true;
		}
	}
	return false;
}

/// @brief Returns the utility SID placed on the field at @p pos, or an empty string if none.
/// @param pos Position of the field tile to query.
/// @return Utility string ID, or empty if the tile is absent or has no utility.
QString Pasture::utilSID( Position pos )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		PastureField* pf = m_fields[pos.toInt()];
		return pf->utilSID;
	}
	return QString();
}

/// @brief Returns the position of a randomly selected field tile, or an invalid Position if empty.
/// @return Random field position.
Position Pasture::randomFieldPos()
{
	int random = rand() % m_fields.size();
	for ( auto field : m_fields )
	{
		if ( random == 0 )
		{
			return field->pos;
		}
		--random;
	}
	return Position();
}

/// @brief Searches field tiles for a "Shed" utility and returns its position.
/// @return Position of the first shed tile found, or an invalid Position if none exists.
Position Pasture::findShed()
{
	for ( auto field : m_fields )
	{
		if ( !field->utilSID.isEmpty() )
		{
			if ( field->utilSID == "Shed" )
			{
				return field->pos;
			}
		}
	}
	return Position();
}

/// @brief Returns a mutable reference to the set of allowed food item+material strings.
/// @return Reference to the food settings set (entries formatted as "ItemSID_MaterialSID").
QSet<QString>& Pasture::foodSettings()
{
	return m_properties.foodSettings;
}

/// @brief Adds an allowed food type to the pasture trough settings.
/// @param itemSID     Item string ID (e.g. "Grain").
/// @param materialSID Material string ID (e.g. "Wheat").
void Pasture::addFoodSetting( QString itemSID, QString materialSID )
{
	m_properties.foodSettings.insert( itemSID + "_" + materialSID );
}

/// @brief Removes a food type from the pasture trough settings.
/// @param itemSID     Item string ID to remove.
/// @param materialSID Material string ID to remove.
void Pasture::removeFoodSetting( QString itemSID, QString materialSID )
{
	m_properties.foodSettings.remove( itemSID + "_" + materialSID );
}

/// @brief Credits one food item delivery to the trough (+10 units of content).
/// @param itemID UID of the delivered food item (currently unused; content is incremented unconditionally).
void Pasture::addFood( unsigned int itemID )
{
	m_properties.troughContent += 10;
}

/// @brief Returns the maximum hay inventory target for the auto-harvesting threshold.
/// @return Max hay count.
int Pasture::maxHay()
{
	return m_properties.maxHay;
}

/// @brief Sets the maximum hay inventory target for the auto-harvesting threshold.
/// @param value New max hay count.
void Pasture::setMaxHay( int value )
{
	m_properties.maxHay = value;
}

/// @brief Returns the current food content of all troughs combined.
/// @return Current trough content units.
int Pasture::foodLevel()
{
	return m_properties.troughContent;
}

/// @brief Returns the total trough capacity across all installed troughs.
/// @return Maximum trough content units.
int Pasture::maxFoodLevel()
{
	return m_properties.maxTroughCapacity;
}


/// @brief Consumes one unit of food from the trough when an animal eats.
/// @return true if food was available and consumed, false if the trough was empty.
bool Pasture::eatFromTrough()
{
	if ( m_properties.troughContent > 0 )
	{
		m_properties.troughContent -= 1;
		return true;
	}

	return false;
}

/// @brief Changes the animal type assigned to this pasture, evicting all current animals if the type changes.
///        Recalculates per-gender capacity limits from the new type's PastureSize DB entry.
/// @param type New animal type string ID.
void Pasture::setAnimalType( QString type )
{
	if ( m_properties.animalType != type )
	{
		for ( auto id : m_animals )
		{
			Animal* animal = g->cm()->animal( id );
			if ( animal )
			{
				animal->setPastureID( 0 );
			}
		}
		m_animals.clear();
	}

	m_properties.animalType = type;

	m_properties.animalSize = DB::select( "PastureSize", "Animals", type ).toInt();
	if ( m_properties.animalSize != 0 )
	{
		int numPlots = m_fields.size();

		if( m_properties.animalSize != 0 )
		{
			m_properties.max = m_fields.size() / m_properties.animalSize;
		}
		else
		{
			m_properties.max = 0;
		}
		m_properties.maxMale   = m_properties.max;
		m_properties.maxFemale = m_properties.max;
	}
	else
	{
		m_properties.max       = 0;
		m_properties.maxMale   = 0;
		m_properties.maxFemale = 0;
	}
}

/// @brief Returns the string ID of the animal type this pasture is configured for.
/// @return Animal type SID.
QString Pasture::animalType()
{
	return m_properties.animalType;
}

/// @brief Returns whether animal product harvesting (wool, milk, etc.) is enabled.
/// @return true if harvest is enabled.
bool Pasture::harvest()
{
	return m_properties.harvest;
}

/// @brief Enables or disables animal product harvesting.
/// @param harvest true to enable, false to disable.
void Pasture::setHarvest( bool harvest )
{
	m_properties.harvest = harvest;
}

/// @brief Returns whether automatic hay harvesting from pasture grass is enabled.
/// @return true if hay harvesting is enabled.
bool Pasture::harvestHay()
{
	return m_properties.harvestHay;
}

/// @brief Enables or disables automatic hay harvesting.
/// @param harvest true to enable, false to disable.
void Pasture::setHarvestHay( bool harvest )
{
	m_properties.harvestHay = harvest;
}

/// @brief Returns whether taming wild animals of the pasture's type is enabled.
/// @return true if tame-wild mode is active.
bool Pasture::tameWild()
{
	return m_properties.tameWild;
}
	
/// @brief Enables or disables taming wild animals for this pasture.
/// @param value true to enable, false to disable.
void Pasture::setTameWild( bool value )
{
	m_properties.tameWild = value;
}

/// @brief Returns the total animal capacity of this pasture (fields / animalSize).
/// @return Maximum number of animals.
int Pasture::maxNumber()
{
	return m_properties.max;
}

/// @brief Returns the number of field tiles one animal of this type requires.
/// @return PastureSize value from the Animals database table.
int Pasture::animalSize()
{
	return m_properties.animalSize;
}

/// @brief Sets the maximum allowed male animal count, evicting excess males.
/// @param max New maximum male count.
void Pasture::setMaxMale( int max )
{
	m_properties.maxMale = max;

	int count = 0;
	QList<unsigned int> newAnimals;
	for ( auto id : m_animals )
	{
		Animal* animal = g->cm()->animal( id );
		if ( animal )
		{
			if ( animal->gender() == Gender::MALE )
			{
				if ( count < max )
				{
					newAnimals.append( id );
				}
				else
				{
					animal->setPastureID( 0 );
				}
				++count;
			}
			else
			{
				newAnimals.append( id );
			}
		}
	}
	m_animals = newAnimals;
}

/// @brief Sets the maximum allowed female animal count, evicting excess females.
/// @param max New maximum female count.
void Pasture::setMaxFemale( int max )
{
	m_properties.maxFemale = max;

	int count = 0;
	QList<unsigned int> newAnimals;
	for ( auto id : m_animals )
	{
		Animal* animal = g->cm()->animal( id );
		if ( animal )
		{
			if ( animal->gender() == Gender::FEMALE )
			{
				if ( count < max )
				{
					newAnimals.append( id );
				}
				else
				{
					animal->setPastureID( 0 );
				}
				++count;
			}
			else
			{
				newAnimals.append( id );
			}
		}
	}
	m_animals = newAnimals;
}

/// @brief Returns the cached position of the first registered field tile.
/// @return First field position, or an invalid Position if the pasture has no tiles.
Position Pasture::firstPos()
{
	if ( m_fields.size() )
	{
		return m_properties.firstPos;
	}
	return Position();
}