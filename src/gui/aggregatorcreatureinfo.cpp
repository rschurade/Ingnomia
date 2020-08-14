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
#include "aggregatorcreatureinfo.h"

#include "../base/db.h"
#include "../base/dbhelper.h"
#include "../base/global.h"
#include "../game/creaturemanager.h"
#include "../game/gnomemanager.h"
#include "../game/militarymanager.h"
#include "../gui/strings.h"

AggregatorCreatureInfo::AggregatorCreatureInfo( QObject* parent )
{
}

void AggregatorCreatureInfo::update()
{
	if( m_currentID != 0 )
	{
		onRequestCreatureUpdate( m_currentID );
	}
}

void AggregatorCreatureInfo::onRequestCreatureUpdate( unsigned int id )
{
	m_currentID = id;
	auto gnome = Global::gm().gnome( id );
	if( gnome )
	{
		m_info.name = gnome->name();
		m_info.id = id;
		m_info.profession = gnome->profession();

		m_info.str = gnome->attribute( "Str" );
		m_info.con = gnome->attribute( "Con" );
		m_info.dex = gnome->attribute( "Dex" );
		m_info.intel = gnome->attribute( "Int" );
		m_info.wis = gnome->attribute( "Wis" );
		m_info.cha = gnome->attribute( "Cha" );

		m_info.hunger = gnome->need( "Hunger" );
		m_info.thirst = gnome->need( "Thirst" );
		m_info.sleep = gnome->need( "Sleep" );
		m_info.happiness = gnome->need( "Happiness" );

		m_info.activity = "Doing something. tbi";

		if( gnome->roleID() )
		{
			m_info.uniform = Global::mil().uniformCopy( gnome->roleID() );
			m_info.equipment = gnome->equipment();
		}

		emit signalCreatureUpdate( m_info );
		return;
	}
	else
	{
		auto monster = Global::cm().monster( id );
		if( monster )
		{
			m_info.name = monster->name();
			m_info.id = id;
			//m_info.profession = monster->profession();

			m_info.str = monster->attribute( "Str" );
			m_info.con = monster->attribute( "Con" );
			m_info.dex = monster->attribute( "Dex" );
			m_info.intel = monster->attribute( "Int" );
			m_info.wis = monster->attribute( "Wis" );
			m_info.cha = monster->attribute( "Cha" );

			m_info.hunger = 100; //monster->need( "Hunger" );
			m_info.thirst = 100; //monster->need( "Thirst" );
			m_info.sleep = 100; //monster->need( "Sleep" );
			m_info.happiness = 100; //monster->need( "Happiness" );
			
			m_info.activity = "Doing something. tbi";
			emit signalCreatureUpdate( m_info );
			return;
		}
		else
		{
			auto animal = Global::cm().animal( id );
			if( animal )
			{
				m_info.name = animal->name();
				m_info.id = id;
				//m_info.profession = animal->profession();

				m_info.str = animal->attribute( "Str" );
				m_info.con = animal->attribute( "Con" );
				m_info.dex = animal->attribute( "Dex" );
				m_info.intel = animal->attribute( "Int" );
				m_info.wis = animal->attribute( "Wis" );
				m_info.cha = animal->attribute( "Cha" );

				m_info.hunger = animal->hunger();
				m_info.thirst = 100; //animal->need( "Thirst" );
				m_info.sleep = 100; //animal->need( "Sleep" );
				m_info.happiness = 100; //animal->need( "Happiness" );
			
				m_info.activity = "Doing something. tbi";
				emit signalCreatureUpdate( m_info );
				return;
			}
		}
		
	}
	m_currentID = 0;
}


void AggregatorCreatureInfo::onRequestProfessionList()
{
	emit signalProfessionList( Global::gm().professions() );
}

void AggregatorCreatureInfo::onSetProfession( unsigned int gnomeID, QString profession )
{
	auto gnome = Global::gm().gnome( gnomeID );
	if( gnome )
	{
		QString oldProf = gnome->profession();
		if( oldProf != profession )
		{
			gnome->selectProfession( profession );
			//onUpdateSingleGnome( gnomeID );
		}
	}
}