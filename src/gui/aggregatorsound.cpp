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

#include "../gui/mainwindowrenderer.h"
#include "../gui/eventconnector.h"

#include <SFML/Audio.hpp>

#include <QDebug>
#include <QJsonDocument>

AggregatorSound::AggregatorSound( QObject* parent ) :
	QObject( parent )
{

	m_effects.clear();
	//sf::SoundBuffer buffer;
	buffer.loadFromFile("content/audio/wood1.wav");
	sound.setBuffer(buffer);
	sound.play();
	
	connect( Global::eventConnector, &EventConnector::signalCameraPosition, this, &AggregatorSound::onCameraPosition );

}

AggregatorSound::~AggregatorSound()
{
}

void AggregatorSound::init( Game* game )
{
	g = game;
	
	m_volume = Global::cfg->get( "AudioMasterVolume" ).toFloat();
	m_effects.clear();
	QList<QVariantMap> soundList = DB::selectRows( "Sounds" );
	for ( auto& sound : soundList )
	{
		
		QString soundID = sound.value( "ID" ).toString()+"."+sound.value( "Material" ).toString();
		
		QString filename = sound.value( "SoundFile" ).toString();
		sf::SoundBuffer* buffer = new sf::SoundBuffer();
		filename = "content/audio/" + filename;
		if (!buffer->loadFromFile(filename.toStdString()))
		{
			qDebug() << "unable to load sound" << soundID << " " << filename;
		}
		else{
			sf::Sound *effect = new sf::Sound();
			effect->setBuffer(*buffer);
			qDebug() << "loaded sound " << soundID << " " << filename;
			m_effects.insert(soundID, effect);
			m_buffers.insert(soundID, buffer);
		}

	}

}
void AggregatorSound::onPlayEffect( QVariantMap effect)
{
	m_volume = Global::cfg->get( "AudioMasterVolume" ).toFloat();
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
			if (Global::debugSound)
			{
				qDebug() << "Unknown sound material " << mat;
			}
		}
		if (m_effects[soundID]->getStatus() != sf::SoundSource::Status::Playing)
		{
			float volume = effect.value("zvolume").toFloat()*m_volume;
			m_effects[soundID]->setVolume(volume);
			m_effects[soundID]->setPosition(effect.value("x").toFloat(), effect.value("y").toFloat(), effect.value("z").toFloat()*m_zAttenuation);
			m_effects[soundID]->setRelativeToListener(false);
			m_effects[soundID]->setMinDistance(10.f);
			m_effects[soundID]->setAttenuation(0.05f);
			m_effects[soundID]->play();
			if (Global::debugSound)
			{
				qDebug() << "playing sound " << soundID << " v " << volume;
			}
		}

	}
	else 
	{
		if (Global::debugSound)
		{
			qDebug() << "Unknown sound " << soundID;
		}
	}
}

void AggregatorSound::onPlayNotify( QVariantMap effect)
{
	QString soundID = effect.value("ID").toString()+".";
	QString soundMaterial = effect.value("Material").toString();
	if( m_effects.contains( soundID ) || m_effects.contains( soundID+soundMaterial ))
	{
		if (m_effects.contains( soundID+soundMaterial ))
		{
			soundID = soundID + soundMaterial;
		}
		else {
			
			if (Global::debugSound)
			{
				QString mat = soundID+soundMaterial;
				qDebug() << "Unknown sound material " << mat;
			}
		}
		
		m_effects[soundID]->setVolume(1.0);
		m_effects[soundID]->setPosition(0, 0, 0);
		m_effects[soundID]->setRelativeToListener(true);
		m_effects[soundID]->play();
		if (Global::debugSound)
		{
			qDebug() << "playing sound " << soundID << " v " << volume;
		}

	}
	else 
	{
		if (Global::debugSound)
		{
			qDebug() << "Unknown sound " << soundID;
		}
	}
}


void AggregatorSound::setVolume( float newvol )
{
	m_volume = newvol;
	sf::Listener::setGlobalVolume(newvol);
}

float AggregatorSound::getVolume( )
{
	
	return m_volume;
}

void AggregatorSound::changeViewPosition() 
{
	
	qDebug() << "changeViewPosition x" << GameState::moveX << " y" << GameState::moveY ;
}
void AggregatorSound::onCameraPosition(float x, float y, float z, int r)
{
	
	sf::Listener::setUpVector(0.f, 0.f, 1.f);
	float angle = 0;
	float x_rotated;
	float y_rotated;
	switch ( r )
	{
		case 0:
			{
				angle = ( (-45.) * 3.1415 ) / 180. ;
				sf::Listener::setDirection(1.f, 1.f, 0.f);
				break;
			}
		case 1:
			{
				angle = ( (-45.-90) * 3.1415 ) / 180. ;
				sf::Listener::setDirection(1.f, -1.f, 0.f);
				break;
			}
		case 2:
			{
				angle = ( (-45.-90-90) * 3.1415 ) / 180. ;
				sf::Listener::setDirection(-1.f, -1.f, 0.f);
				break;
			}
		case 3:
			{
				angle = ( (-45.-90-90-90) * 3.1415 ) / 180. ;
				sf::Listener::setDirection(-1.f, 1.f, 0.f);
				break;
			}
	}
	
	x = -x / 32 + Global::dimX/2;
	y = -y / 16;
	x = x -Global::dimX/2;
	y = y -Global::dimY/2;
	x = x*1.41421; // sqrt(2)
	y = y*1.41421;
	
	float s = sin(angle);
	float c = cos(angle);

	x_rotated = x;
	y_rotated = y;

	float xnew = x_rotated * c - y_rotated * s;
	float ynew = x_rotated * s + y_rotated * c;

	x_rotated = xnew + Global::dimX/2;
	y_rotated = ynew + Global::dimY/2;
	qDebug() << "changeViewPosition x" << x << x_rotated << " y" << y << y_rotated << "r" << r;
	sf::Listener::setPosition(x_rotated, y_rotated, z*m_zAttenuation);
	
}
