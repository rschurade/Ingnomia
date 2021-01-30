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
#include "aggregatorsound.h"

#include "../game/game.h"

#include "../base/gamestate.h"
#include "../base/position.h"
#include "../base/db.h"
#include "../base/config.h"



#include <QDebug>
#include <QJsonDocument>

AggregatorSound::AggregatorSound( QObject* parent ) :
	QObject( parent )
{

	m_effects.clear();

}

AggregatorSound::~AggregatorSound()
{
}

void AggregatorSound::init( Game* game )
{
	g = game;
	printf("new sound loaded\n");
	m_volume = Global::cfg->get( "AudioMasterVolume" ).toFloat();
	m_effects.clear();
	QList<QVariantMap> soundList = DB::selectRows( "Sounds" );
	for ( auto& sound : soundList )
	{
		
		QString soundID = sound.value( "ID" ).toString()+"."+sound.value( "Material" ).toString();
		
		QSoundEffect *effect = new QSoundEffect(this);
		QString filename = sound.value( "SoundFile" ).toString();
		filename = "content/audio/" + filename;
		effect->setSource(QUrl::fromLocalFile(filename));
		effect->setLoopCount(1);
		effect->setVolume(0.5f);
		qDebug() << "loaded sound " << soundID << " " << filename;
		printf("new sound loaded %s \n", soundID.toStdString().c_str());
		m_effects.insert(soundID, effect);
	}

}
void AggregatorSound::onPlayEffect( QVariantMap effect)
{
	m_volume = Global::cfg->get( "AudioMasterVolume" ).toFloat() /100.0;
	QString soundID = effect.value("ID").toString()+".";
	QString soundMaterial = effect.value("Material").toString();
	if( m_effects.contains( soundID ) || m_effects.contains( soundID+soundMaterial ))
	{
		if (m_effects.contains( soundID+soundMaterial ))
		{
			soundID = soundID + soundMaterial;
		}
		else {
			QString mat = soundID+soundMaterial;
			printf("Unknown sound material %s \n", mat.toStdString().c_str());
		}
		if (!m_effects[soundID]->isPlaying() )
		{
			float volume = effect.value("zvolume").toFloat()*m_volume;
			//printf("playing sound with volume %f\n", volume);
			m_effects[soundID]->setVolume(volume);
			m_effects[soundID]->play();
		}

	}
	else 
	{
		printf("Unknown sound %s \n", soundID.toStdString().c_str());
	}
}


void AggregatorSound::setVolume( float newvol )
{
	m_volume = newvol;
}

float AggregatorSound::getVolume( )
{
	
	return m_volume;
}
void AggregatorSound::changeViewLevel( int input) 
{
	m_viewLevel = input;
}
