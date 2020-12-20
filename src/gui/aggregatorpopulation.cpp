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
#include "aggregatorpopulation.h"

#include "../base/counter.h"
#include "../base/db.h"
#include "../base/dbhelper.h"
#include "../base/global.h"
#include "../game/game.h"
#include "../game/creaturemanager.h"
#include "../game/gnomemanager.h"
#include "../gui/strings.h"

#include <QDebug>

AggregatorPopulation::AggregatorPopulation( QObject* parent ) :
	QObject(parent)
{
	//qRegisterMetaType<GuiGnomeScheduleInfo>();
	qRegisterMetaType<ScheduleActivity>();

	for( auto group : DB::selectRows( "SkillGroups" ) )
	{
		for( auto skillID : group.value( "SkillID" ).toString().split( "|" ) )
		{
			GuiSkillInfo gsi;
			gsi.sid = skillID;
			gsi.name = S::s( "$SkillName_" + skillID );
			gsi.group = group.value( "ID" ).toString();
			gsi.color = group.value( "Color" ).toString();
			
			m_skillIds.push_back( gsi );
		}
	}
}

void AggregatorPopulation::init( Game* game )
{
	g = game;
}

void AggregatorPopulation::onRequestPopulationUpdate()
{
	if( !g ) return;

	emit signalProfessionList( g->gm()->professions() );

	m_populationInfo.gnomes.clear();

	//for( int i = 0; i < 100; ++i )
	{
		for( auto gnome : g->gm()->gnomes() )
		{
			GuiGnomeInfo ggi;

			ggi.name = gnome->name();
			ggi.id = gnome->id();
			ggi.profession = gnome->profession();

			for( auto skill : m_skillIds )
			{
				GuiSkillInfo gsi = skill;
				
				gsi.level = gnome->getSkillLevel( skill.sid );
				gsi.active = gnome->getSkillActive( skill.sid );
				gsi.xpValue = gnome->getSkillXP( skill.sid );

				ggi.skills.append( gsi );
			}

			m_populationInfo.gnomes.append( ggi );
		}
	}
	if( m_sortMode == "Name" )
	{
		std::sort( m_populationInfo.gnomes.begin(), m_populationInfo.gnomes.end(), []( const GuiGnomeInfo& lhs, const GuiGnomeInfo& rhs ){ return lhs.name < rhs.name; } );
	}
	else if( m_sortMode == "Prof" )
	{
		std::sort( m_populationInfo.gnomes.begin(), m_populationInfo.gnomes.end(), []( const GuiGnomeInfo& lhs, const GuiGnomeInfo& rhs ){ return lhs.profession < rhs.profession; } );
	}
	else
	{
		std::sort( m_populationInfo.gnomes.begin(), m_populationInfo.gnomes.end(), [=]( const GuiGnomeInfo& lhs, const GuiGnomeInfo& rhs ){ 
			auto lg = g->gm()->gnome( lhs.id );
			if( lg )
			{
				auto rg = g->gm()->gnome( rhs.id );
				if( rg )
				{
					if( m_revertSort )
					{
						return lg->getSkillLevel( m_sortMode ) < rg->getSkillLevel( m_sortMode );
					}
					else
					{
						return lg->getSkillLevel( m_sortMode ) > rg->getSkillLevel( m_sortMode );
					}
				}
			}
			return false;
		} );
	}

	emit signalPopulationUpdate( m_populationInfo );
}

void AggregatorPopulation::onUpdateSingleGnome( unsigned int gnomeID )
{
	if( !g ) return;
	auto gnome = g->gm()->gnome( gnomeID );
	if( gnome )
	{
	
		GuiGnomeInfo ggi;

		ggi.name = gnome->name();
		ggi.id = gnome->id();
		ggi.profession = gnome->profession();

		for( auto skill : m_skillIds )
		{
			GuiSkillInfo gsi = skill;
				
			gsi.level = gnome->getSkillLevel( skill.sid );
			gsi.active = gnome->getSkillActive( skill.sid );
			gsi.xpValue = gnome->getSkillXP( skill.sid );

			ggi.skills.append( gsi );
		}
		emit signalUpdateSingleGnome( ggi );
	}
}

void AggregatorPopulation::onSetSkillActive( unsigned int gnomeID, QString skillID, bool value )
{
	if( !g ) return;
	auto gnome = g->gm()->gnome( gnomeID );
	if( gnome )
	{
		gnome->setSkillActive( skillID, value );
	}
}

void AggregatorPopulation::onSetAllSkills( unsigned int gnomeID, bool value )
{
	if( !g ) return;
	auto gnome = g->gm()->gnome( gnomeID );
	if( gnome )
	{
		for( const auto& skill : m_skillIds )
		{
			gnome->setSkillActive( skill.sid, value );
		}
		onUpdateSingleGnome( gnomeID );
	}
}
	
void AggregatorPopulation::onSetAllGnomes( QString skillID, bool value )
{
	if( !g ) return;
	for( auto gnome : g->gm()->gnomes() )
	{
		gnome->setSkillActive( skillID, value );
	}
	onRequestPopulationUpdate();
}

void AggregatorPopulation::onSetProfession( unsigned int gnomeID, QString profession )
{
	if( !g ) return;
	auto gnome = g->gm()->gnome( gnomeID );
	if( gnome )
	{
		QString oldProf = gnome->profession();
		if( oldProf != profession )
		{
			gnome->selectProfession( profession );
			onUpdateSingleGnome( gnomeID );
		}
	}
}

void AggregatorPopulation::onSortGnomes( QString mode )
{
	if( !g ) return;
	if( m_sortMode != mode )
	{
		m_revertSort = false;
		m_sortMode = mode;
	}
	else
	{
		m_revertSort = !m_revertSort;
	}
	onRequestPopulationUpdate();
}

void AggregatorPopulation::onRequestSchedules()
{
	if( !g ) return;
	m_scheduleInfo.schedules.clear();
	for( auto gnome : g->gm()->gnomes() )
	{
		GuiGnomeScheduleInfo ggs;
		ggs.id = gnome->id();
		ggs.name = gnome->name();
		ggs.schedule = gnome->schedule();

		m_scheduleInfo.schedules.append( ggs );
	}

	emit signalScheduleUpdate( m_scheduleInfo );
}
	
void AggregatorPopulation::onSetSchedule( unsigned int gnomeID, int hour, ScheduleActivity activity )
{
	if( !g ) return;
	auto gnome = g->gm()->gnome( gnomeID );
	if( gnome )
	{
		gnome->setSchedule( hour, activity );

		GuiGnomeScheduleInfo ggs;
		ggs.id = gnome->id();
		ggs.name = gnome->name();
		ggs.schedule = gnome->schedule();

		emit signalScheduleUpdateSingleGnome( ggs );
	}
}

void AggregatorPopulation::onSetAllHours( unsigned int gnomeID, ScheduleActivity activity )
{
	if( !g ) return;
	auto gnome = g->gm()->gnome( gnomeID );
	if( gnome )
	{
		for( int i = 0; i < 24; ++i )
		{
			gnome->setSchedule( i, activity );
		}

		GuiGnomeScheduleInfo ggs;
		ggs.id = gnome->id();
		ggs.name = gnome->name();
		ggs.schedule = gnome->schedule();

		emit signalScheduleUpdateSingleGnome( ggs );
	}
}
	
void AggregatorPopulation::onSetHourForAll( int hour, ScheduleActivity activity )
{
	if( !g ) return;
	for( auto gnome : g->gm()->gnomes() )
	{
		gnome->setSchedule( hour, activity );
	}
	onRequestSchedules();
}

void AggregatorPopulation::onRequestProfessions()
{
	if( !g ) return;
	emit signalProfessionList( g->gm()->professions() );
}
	
void AggregatorPopulation::onRequestSkills( QString profession )
{
	if( !g ) return;
	m_profSkills.clear();

	auto skillIDs = g->gm()->professionSkills( profession );
	for( auto skillID : skillIDs )
	{
		GuiSkillInfo gsi;
		gsi.sid = skillID;
		gsi.name = S::s( "$SkillName_" + skillID );		
		
		m_profSkills.append( gsi );
	}

	emit signalProfessionSkills( profession, m_profSkills );
}

void AggregatorPopulation::onUpdateProfession( QString name, QString newName, QStringList skills )
{
	if( !g ) return;
	g->gm()->modifyProfession( name, newName, skills );
}

void AggregatorPopulation::onDeleteProfession( QString name )
{
	if( !g ) return;
	g->gm()->removeProfession( name );
	emit signalProfessionList( g->gm()->professions() );
}
	
void AggregatorPopulation::onNewProfession()
{
	if( !g ) return;
	QString name = g->gm()->addProfession();
	emit signalProfessionList( g->gm()->professions() );
	emit signalSelectEditProfession( name );
}
