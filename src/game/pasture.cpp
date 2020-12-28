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

	foodSettings = in.value( "Food" ).toStringList().toSet();

	animalSize = DB::select( "PastureSize", "Animals", animalType ).toInt();
}

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
		grofi->util   = vf.toMap().value( "Util" ).toUInt();
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
		entry.insert( "Util", field->util );
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

Pasture::~Pasture()
{
	for ( const auto& field : m_fields )
	{
		delete field;
	}
}

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

// farming manager calls this on hour changed
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
						if ( field->util && !field->job )
						{
							if ( g->inv()->itemSID( field->util ) == "Trough" )
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

bool Pasture::canDelete()
{
	return true;
}

int Pasture::countTiles()
{
	return m_fields.size();
}

bool Pasture::addUtil( Position pos, unsigned int itemID )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		PastureField* pf = m_fields[pos.toInt()];
		if ( pf->util == 0 )
		{
			pf->util = itemID;

			//if item is trough
			if ( g->inv()->itemSID( itemID ) == "Trough" )
			{
				m_properties.maxTroughCapacity += 20;
			}

			return true;
		}
	}
	return false;
}

bool Pasture::removeUtil( Position pos )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		PastureField* pf = m_fields[pos.toInt()];

		if ( pf->util != 0 )
		{
			unsigned int itemID = pf->util;

			pf->util = 0;

			//if item is trough
			if ( g->inv()->itemSID( itemID ) == "Trough" )
			{
				m_properties.maxTroughCapacity -= 20;
			}

			return true;
		}
	}
	return false;
}

unsigned int Pasture::util( Position pos )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		PastureField* pf = m_fields[pos.toInt()];
		return pf->util;
	}
	return 0;
}

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

Position Pasture::findShed()
{
	for ( auto field : m_fields )
	{
		if ( field->util )
		{
			if ( g->inv()->itemSID( field->util ) == "Shed" )
			{
				return field->pos;
			}
		}
	}
	return Position();
}

QSet<QString>& Pasture::foodSettings()
{
	return m_properties.foodSettings;
}

void Pasture::addFoodSetting( QString itemSID, QString materialSID )
{
	m_properties.foodSettings.insert( itemSID + "_" + materialSID );
}

void Pasture::removeFoodSetting( QString itemSID, QString materialSID )
{
	m_properties.foodSettings.remove( itemSID + "_" + materialSID );
}

void Pasture::addFood( unsigned int itemID )
{
	m_properties.troughContent += 10;
}

int Pasture::maxHay()
{
	return m_properties.maxHay;
}

void Pasture::setMaxHay( int value )
{
	m_properties.maxHay = value;
}

int Pasture::foodLevel()
{
	return m_properties.troughContent;
}

int Pasture::maxFoodLevel()
{
	return m_properties.maxTroughCapacity;
}


bool Pasture::eatFromTrough()
{
	if ( m_properties.troughContent > 0 )
	{
		m_properties.troughContent -= 1;
		return true;
	}

	return false;
}

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

QString Pasture::animalType()
{
	return m_properties.animalType;
}

bool Pasture::harvest()
{
	return m_properties.harvest;
}

void Pasture::setHarvest( bool harvest )
{
	m_properties.harvest = harvest;
}

bool Pasture::harvestHay()
{
	return m_properties.harvestHay;
}

void Pasture::setHarvestHay( bool harvest )
{
	m_properties.harvestHay = harvest;
}

bool Pasture::tameWild()
{
	return m_properties.tameWild;
}
	
void Pasture::setTameWild( bool value )
{
	m_properties.tameWild = value;
}

int Pasture::maxNumber()
{
	return m_properties.max;
}

int Pasture::animalSize()
{
	return m_properties.animalSize;
}

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

Position Pasture::firstPos()
{
	if ( m_fields.size() )
	{
		return m_properties.firstPos;
	}
	return Position();
}