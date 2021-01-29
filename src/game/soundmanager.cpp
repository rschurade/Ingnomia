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



#include <QDebug>
#include <QJsonDocument>

SoundManager::SoundManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
	m_volume = 1.0;
	m_effects.clear();
	QList<QVariantMap> soundList = DB::selectRows( "Sounds" );
	for ( auto& sound : soundList )
	{
		QString soundID = sound.value( "ID" ).toString();
		QSoundEffect *effect = new QSoundEffect(this);
		QString filename = sound.value( "SoundFile" ).toString();
		filename = "content/audio/" + filename;
		effect->setSource(QUrl::fromLocalFile(filename));
		effect->setLoopCount(1);
		effect->setVolume(0.5f);
		printf("loaded sound%s %s\n", soundID.toStdString().c_str(), filename.toStdString().c_str());
		m_effects.insert(soundID, effect);
	}

	
}

SoundManager::~SoundManager()
{
}

void SoundManager::onTick( quint64 tick )
{
	while (!m_playQue.isEmpty()) {
		QVariantMap playEffect = m_playQue.takeFirst();
		if( m_effects.contains(playEffect.value("ID").toString()) )
		{
			if (!m_effects[playEffect.value("ID").toString()]->isPlaying() )
			{
				float volume = playEffect.value("zvolume").toFloat()*m_volume;
				printf("playing sound with volume %f\n", volume);
				m_effects[playEffect.value("ID").toString()]->setVolume(volume);
				m_effects[playEffect.value("ID").toString()]->play();
			}else 
			{
				printf("sound playing already %s\n", playEffect.value("ID").toString().toStdString().c_str());
			}
			
		}
		playEffect.clear();
	}
	
}

void SoundManager::playEffect( QString type, Position& pos)
{
	if ( m_playQue.size() < SOUNDS_MAX )
	{
		
		int distance = abs(pos.z - m_viewLevel);
		if (distance < SOUND_FALLOFF) 
		{
			// EffectStruct newEffect;
			// newEffect.id = type;
			// float zvolume = 1.0-(distance/SOUND_FALLOFF);
			// 
			float zvolume = 1.0-((float)distance/(float)SOUND_FALLOFF);
			QVariantMap newEffect;
			newEffect.insert( "ID", type );
			newEffect.insert( "zvolume", zvolume );
			m_playQue.append( newEffect );
			printf("play effect z%d %s volume %f distance:%d\n", pos.z, type.toStdString().c_str(), zvolume, distance);
			//m_playQue.append(QVariant(newEffect));
		}
	}
	
	return;
}

void SoundManager::setVolume( float newvol )
{
	m_volume = newvol;
}

float SoundManager::getVolume( )
{
	
	return m_volume;
}
void SoundManager::changeViewLevel( int input) 
{
	//printf("viewlevel %d\n", input);
	m_viewLevel = input;
}
