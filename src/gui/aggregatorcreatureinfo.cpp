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
			m_info.uniform = Global::mil().uniform( gnome->roleID() );
		}
		else
		{
			m_info.uniform = nullptr;
		}

		emit signalCreatureUpdate( m_info );
	}
	else
	{
		m_currentID = 0;
	}
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