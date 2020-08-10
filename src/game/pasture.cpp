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

void PastureProperties::serialize( QVariantMap& out )
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

Pasture::Pasture( QList<QPair<Position, bool>> tiles ) :
	WorldObject()
{
	m_name = "Pasture";

	for ( auto p : tiles )
	{
		if ( p.second )
		{
			PastureField grofi;
			grofi.pos = p.first;
			m_fields.insert( p.first.toInt(), grofi );
		}
	}
	if ( m_fields.size() )
	{
		for ( auto field : m_fields )
		{
			m_properties.firstPos = field.pos;
			break;
		}
	}
}

Pasture::Pasture( QVariantMap vals ) :
	WorldObject( vals ),
	m_properties( vals )
{
	QVariantList vfl = vals.value( "Fields" ).toList();
	for ( auto vf : vfl )
	{
		PastureField grofi;
		grofi.pos    = Position( vf.toMap().value( "Pos" ).toString() );
		grofi.hasJob = vf.toMap().value( "HasJob" ).toBool();
		grofi.util   = vf.toMap().value( "Util" ).toUInt();
		m_fields.insert( grofi.pos.toInt(), grofi );
	}
	if ( m_fields.size() )
	{
		for ( auto field : m_fields )
		{
			m_properties.firstPos = field.pos;
			break;
		}
	}

	QVariantList vjl = vals.value( "Jobs" ).toList();
	for ( auto vj : vjl )
	{
		Job* job = new Job( vj.toMap() );
		m_jobsOut.insert( job->id(), job );
	}

	QVariantList val = vals.value( "Animals" ).toList();
	for ( auto va : val )
	{
		m_animals.append( va.toUInt() );
	}

	val = vals.value( "AnimalsInJobs" ).toList();
	for ( auto va : val )
	{
		m_animalsInJob.insert( va.toUInt() );
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

QVariant Pasture::serialize()
{
	QVariantMap out;
	WorldObject::serialize( out );
	m_properties.serialize( out );

	QVariantList tiles;
	for ( auto field : m_fields )
	{
		QVariantMap entry;
		entry.insert( "Pos", field.pos.toString() );
		entry.insert( "HasJob", field.hasJob );
		entry.insert( "Util", field.util );
		tiles.append( entry );
	}
	out.insert( "Fields", tiles );

	QVariantList jobs;
	for ( auto job : m_jobsOut )
	{
		jobs.append( job->serialize() );
	}
	out.insert( "Jobs", jobs );

	QVariantList animals;
	for ( auto an : m_animals )
	{
		animals.append( an );
	}
	out.insert( "Animals", animals );
	QVariantList animalsInJobs;
	for ( auto an : m_animalsInJob )
	{
		animalsInJobs.append( an );
	}
	out.insert( "AnimalsInJobs", animalsInJobs );

	return out;
}

Pasture::~Pasture()
{
}

void Pasture::addTile( Position& pos )
{
	PastureField grofi;
	grofi.pos = pos;
	m_fields.insert( pos.toInt(), grofi );

	m_properties.max = m_fields.size() / m_properties.animalSize;
}

// farming manager calls this on hour changed
void Pasture::onTick( quint64 tick )
{
	for ( auto field : m_fields )
	{
		Tile& tile = Global::w().getTile( field.pos );
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

	if ( m_animals.size() >= m_properties.max )
		return;
	int countMale   = 0;
	int countFemale = 0;
	for ( auto id : m_animals )
	{
		Animal* animal = Global::cm().animal( id );
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

	for ( auto& a : Global::cm().animals() )
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
		if ( Global::cm().animal( id )->gender() == Gender::MALE )
		{
			++countMale;
		}
		else
		{
			++countFemale;
		}
	}

	auto a = Global::cm().animal( animalID );

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

unsigned int Pasture::getJob( unsigned int gnomeID, QString skillID )
{
	if ( !m_active )
		return 0;

	if ( Global::gm().gnomeCanReach( gnomeID, m_properties.firstPos ) )
	{

		Job* job = nullptr;
		if ( skillID == "AnimalHusbandry" )
		{
			job = createJob( "AnimalHusbandry" );

			if ( job )
			{
				m_jobsOut.insert( job->id(), job );
				return job->id();
			}
		}
		if ( skillID == "Farming" )
		{
			job = createJob( "Farming" );

			if ( job )
			{
				m_jobsOut.insert( job->id(), job );
				return job->id();
			}
		}
	}

	return 0;
}

bool Pasture::finishJob( unsigned int jobID )
{
	if ( m_jobsOut.contains( jobID ) )
	{
		Job* job = m_jobsOut[jobID];
		m_jobsOut.remove( jobID );
		if ( m_fields.contains( job->pos().toInt() ) )
		{
			m_fields[job->pos().toInt()].hasJob = false;
		}
		m_animalsInJob.remove( job->animal() );

		auto animal = Global::cm().animal( job->animal() );
		if ( animal )
		{
			animal->setInJob( 0 );
		}

		delete job;
		return true;
	}
	return false;
}

bool Pasture::giveBackJob( unsigned int jobID )
{
	if ( m_jobsOut.contains( jobID ) )
	{
		Job* job = m_jobsOut[jobID];
		if ( m_fields.contains( job->pos().toInt() ) )
		{
			m_fields[job->pos().toInt()].hasJob = false;
		}
		m_jobsOut.remove( jobID );
		m_animalsInJob.remove( job->animal() );

		auto animal = Global::cm().animal( job->animal() );
		if ( animal )
		{
			animal->setInJob( 0 );
		}

		delete job;
		return true;
	}
	return false;
}

Job* Pasture::getJob( unsigned int jobID )
{
	if ( m_jobsOut.contains( jobID ) )
	{
		return m_jobsOut[jobID];
	}
	return nullptr;
}

bool Pasture::hasJobID( unsigned int jobID )
{
	return m_jobsOut.contains( jobID );
}

Job* Pasture::createJob( QString skillID )
{
	Job* job = nullptr;

	if ( m_fields.size() > 0 )
	{
		if ( skillID == "AnimalHusbandry" )
		{
			// fill troughs
			// if current food in troughs < capacity - 10
			if ( !m_properties.foodSettings.isEmpty() && m_properties.maxTroughCapacity > 0 )
			{
				if ( m_properties.troughContent < m_properties.maxTroughCapacity - 9 )
				{
					for ( auto foodString : m_properties.foodSettings )
					{
						auto fsl = foodString.split( "_" );
						if ( Global::inv().itemCount( fsl[0], fsl[1] ) > 0 )
						{
							for ( auto& field : m_fields )
							{
								if ( field.util && !field.hasJob )
								{
									if ( Global::inv().itemSID( field.util ) == "Trough" )
									{
										job = new Job();

										job->setType( "FillTrough" );
										job->setRequiredSkill( "AnimalHusbandry" );

										job->setPos( field.pos );
										job->addPossibleWorkPosition( field.pos );
										job->setPosItemInput( field.pos );
										field.hasJob = true;
										job->setNoJobSprite( true );
										job->addRequiredItem( 1, fsl[0], fsl[1], {} );
										return job;
									}
								}
							}
						}
					}
				}
			}

			// lead animal to pasture
			Animal* a = checkAnimalOutsidePasture();
			if ( a )
			{
				job = new Job();

				job->setType( "LeadAnimalToPasture" );
				job->setRequiredSkill( "AnimalHusbandry" );
				job->setPos( a->getPos() );
				job->addPossibleWorkPosition( a->getPos() );
				job->setAnimal( a->id() );
				a->setInJob( job->id() );
				a->setImmobile( true );
				job->setNoJobSprite( true );
				m_animalsInJob.insert( a->id() );
				int random = rand() % m_fields.size();
				for ( auto field : m_fields )
				{
					if ( random == 0 )
					{
						job->setPosItemInput( field.pos );
						break;
					}
					--random;
				}
				return job;
			}

			if ( m_properties.tameWild )
			{
				if ( m_animals.size() < m_properties.max )
				{
					int countMale   = 0;
					int countFemale = 0;
					for ( auto id : m_animals )
					{
						if ( Global::cm().animal( id )->gender() == Gender::MALE )
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
							fieldPos = field.pos;
							break;
						}
						--random;
					}

					auto animals = Global::cm().animalsByDistance( fieldPos, m_properties.animalType );
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
						job = new Job();

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
					}
				}
			}

			if ( m_properties.harvest )
			{
				// harvest producing animal
				a = checkAnimalHarvestReady();
				if ( a )
				{
					job = new Job();

					job->setType( "HarvestAnimal" );
					job->setRequiredSkill( "AnimalHusbandry" );
					job->setPos( a->getPos() );
					job->addPossibleWorkPosition( a->getPos() );
					job->setAnimal( a->id() );
					a->setInJob( job->id() );
					a->setImmobile( true );
					job->setNoJobSprite( true );
					m_animalsInJob.insert( a->id() );
				}
			}
		}
		else if ( skillID == "Farming" )
		{
			if ( m_properties.harvestHay && GameState::season != 3 )
			{
				if ( Global::inv().itemCount( "Hay", "any" ) < (unsigned int)m_properties.maxHay )
				{
					for ( auto& field : m_fields )
					{
						if ( !field.hasJob && Global::w().hasMaxGrass( field.pos ) )
						{
							job = new Job();

							job->setType( "HarvestHay" );
							job->setRequiredSkill( "Farming" );

							job->setPos( field.pos );
							job->addPossibleWorkPosition( field.pos );
							job->setPosItemInput( field.pos );
							field.hasJob = true;
							job->setNoJobSprite( true );
							return job;
						}
					}
				}
			}
		}
	}
	return job;
}

Animal* Pasture::checkAnimalOutsidePasture()
{
	for ( auto id : m_animals )
	{
		Animal* animal = Global::cm().animal( id );
		int posID      = animal->getPos().toInt();
		if ( !m_fields.contains( posID ) && !m_animalsInJob.contains( id ) && !animal->inJob() )
		{
			if ( Global::w().regionMap().checkConnectedRegions( m_fields.first().pos, animal->getPos() ) )
			{
				return animal;
			}
		}
	}

	return nullptr;
}

Animal* Pasture::checkAnimalHarvestReady()
{
	for ( auto id : m_animals )
	{
		Animal* animal = Global::cm().animal( id );
		if ( animal && animal->numProduce() > 0 && !m_animalsInJob.contains( id ) )
		{
			if ( Global::w().regionMap().checkConnectedRegions( m_fields.first().pos, animal->getPos() ) )
			{
				return animal;
			}
		}
	}

	return nullptr;
}

bool Pasture::removeTile( Position& pos )
{
	PastureField ff = m_fields.value( pos.toInt() );

	m_fields.remove( pos.toInt() );

	Global::w().clearTileFlag( pos, TileFlag::TF_PASTURE );

	if ( m_fields.size() )
	{
		m_properties.firstPos = m_fields[0].pos;
	}
	else
	{
		m_properties.firstPos = Position();
	}

	m_properties.max       = m_fields.size() / m_properties.animalSize;
	m_properties.maxMale   = qMin( m_properties.max, m_properties.maxMale );
	m_properties.maxFemale = qMin( m_properties.max, m_properties.maxFemale );

	while ( m_animals.size() && m_properties.max < m_animals.size() )
	{
		auto id        = m_animals.takeLast();
		Animal* animal = Global::cm().animal( id );
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
		Animal* animal = Global::cm().animal( id );
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

void Pasture::setInJob( unsigned int animalID )
{
	m_animalsInJob.insert( animalID );
}

void Pasture::removeAnimal( unsigned int animalID )
{
	Animal* animal = Global::cm().animal( animalID );
	if ( animal )
	{
		animal->setFollowID( 0 );
		animal->setPastureID( 0 );
	}

	m_animals.removeAll( animalID );
	m_animalsInJob.remove( animalID );

	int plots;
	int male;
	int female;
	getInfo( plots, male, female );
}

void Pasture::removeAllAnimals()
{
	for ( auto id : m_animals )
	{
		Animal* animal = Global::cm().animal( id );
		if ( animal )
		{
			animal->setFollowID( 0 );
			animal->setPastureID( 0 );
		}
	}
	m_animals.clear();
	m_animalsInJob.clear();

	int plots;
	int male;
	int female;
	getInfo( plots, male, female );
}

bool Pasture::canDelete()
{
	return m_jobsOut.isEmpty();
}

int Pasture::countTiles()
{
	return m_fields.size();
}

bool Pasture::addUtil( Position pos, unsigned int itemID )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		PastureField& pf = m_fields[pos.toInt()];
		if ( pf.util == 0 )
		{
			pf.util = itemID;

			//if item is trough
			if ( Global::inv().itemSID( itemID ) == "Trough" )
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
		PastureField& pf = m_fields[pos.toInt()];

		if ( pf.util != 0 )
		{
			unsigned int itemID = pf.util;

			pf.util = 0;

			//if item is trough
			if ( Global::inv().itemSID( itemID ) == "Trough" )
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
		PastureField pf = m_fields[pos.toInt()];
		return pf.util;
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
			return field.pos;
		}
		--random;
	}
	return Position();
}

Position Pasture::findShed()
{
	for ( auto field : m_fields )
	{
		if ( field.util )
		{
			if ( Global::inv().itemSID( field.util ) == "Shed" )
			{
				return field.pos;
			}
		}
	}
	return Position();
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
			Animal* animal = Global::cm().animal( id );
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

		m_properties.max       = numPlots / m_properties.animalSize;
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
		Animal* animal = Global::cm().animal( id );
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
		Animal* animal = Global::cm().animal( id );
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