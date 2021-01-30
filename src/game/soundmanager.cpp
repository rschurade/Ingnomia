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
#include "soundmanager.h"

#include "game.h"

#include "../base/gamestate.h"
#include "../base/position.h"
#include "../base/db.h"
#include "../base/config.h"



#include <QDebug>
#include <QJsonDocument>

SoundManager::SoundManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
	
}

SoundManager::~SoundManager()
{
}

void SoundManager::onTick( quint64 tick )
{
	
}

void SoundManager::playEffect( QString type, Position& pos, QString material)
{
	playEffect(type, pos, material, 0);
}

void SoundManager::playEffect( QString type, Position& pos, QString material, int duration)
{

	int distance = abs(pos.z - m_viewLevel);
	if (distance < SOUND_FALLOFF) 
	{

		float zvolume = 1.0-((float)distance/(float)SOUND_FALLOFF);
		QVariantMap newEffect;
		newEffect.insert( "ID", type );
		newEffect.insert( "zvolume", zvolume );
		newEffect.insert( "Material", material );
		//m_playQue.append( newEffect );
		emit signalPlayEffect( newEffect );
		//printf("play effect z%d %s material:%s volume %f distance:%d \n", pos.z, type.toStdString().c_str(), material.toStdString().c_str(), zvolume, distance);

	}

	return;
}


void SoundManager::changeViewLevel( int input) 
{
	m_viewLevel = input;
}
