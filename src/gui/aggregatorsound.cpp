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

#include "../base/config.h"
#include "../base/containersHelper.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/position.h"
#include "../game/game.h"
#include "../game/soundmanager.h"
#include "../game/world.h"
#include "../gui/eventconnector.h"
#include "../gui/mainwindowrenderer.h"
#include "spdlog/spdlog.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QVariantMap>

AggregatorSound::AggregatorSound( EventConnector* parent ) :
	QObject( parent )
{
	//sf::SoundBuffer buffer;
	parent->signalCameraPosition.connect( &AggregatorSound::onCameraPosition, this );
	auto device    = std::make_shared<AL::Device>();
	m_audioContext = std::make_shared<AL::Context>( device );
	AL::Context::Lock lock( m_audioContext );
	m_audioListener = std::make_shared<AL::Listener>( m_audioContext );
	try
	{
		for ( size_t i = 0; i < maxOcclusion; ++i )
		{
			m_occlusionFilter.push_back( std::make_shared<AL::Filter>( m_audioContext ) );
			m_occlusionFilter.back()->setGain( pow( 0.75, i ), pow( 0.75, i ), pow( 0.5, i ) );
		}
	}
	catch ( const std::exception& e )
	{
		spdlog::info("No OpenAL filter support - falling back to pure volume hacks: {}", e.what());
	}
}

AggregatorSound::~AggregatorSound()
{
	AL::Context::Lock lock( m_audioContext );
	m_activeEffects.clear();
	m_audioListener.reset();
	m_occlusionFilter.clear();
	m_buffers.clear();
}

void AggregatorSound::init( Game* game )
{
	g = game;

	AL::Context::Lock lock( m_audioContext );
	m_audioListener->setVolume( Global::cfg->get_or_default<double>( "AudioMasterVolume" , 1.0 ) );
	QList<QVariantMap> soundList = DB::selectRows( "Sounds" );
	for ( auto& sound : soundList )
	{
		QString soundID  = sound.value( "ID" ).toString() + "." + sound.value( "Material" ).toString();
		auto filename = sound.value( "SoundFile" ).toString().toStdString();
		const auto& exePath  = Global::exePath;
		filename         = (exePath / "content" / "audio" / filename).string();

		try
		{
			m_buffers.insert_or_assign( soundID, std::make_shared<AL::Buffer>( m_audioContext, filename ) );
			spdlog::debug( "loaded sound {} {}", soundID.toStdString(), filename );
		}
		catch ( ... )
		{
			spdlog::debug( "unable to load sound {} {}", soundID.toStdString(), filename );
		}
	}
}

void AggregatorSound::onPlayEffect( const SoundEffect& effectRequest )
{
	AL::Context::Lock lock( m_audioContext );

	QString soundID       = effectRequest.type + ".";
	QString soundMaterial = effectRequest.material;
	if ( m_buffers.contains( soundID ) || m_buffers.contains( soundID + soundMaterial ) )
	{
		if ( m_buffers.contains( soundID + soundMaterial ) )
		{
			soundID = soundID + soundMaterial;
		}
		else
		{
			QString mat = soundID + soundMaterial;
			if ( Global::debugSound )
			{
				spdlog::debug( "Unknown sound material  {}", mat.toStdString() );
			}
		}
		m_activeEffects.append( ActiveEffect {
			false,
			effectRequest.origin,
			std::make_shared<AL::Source>( m_audioContext, m_buffers[soundID] ) } );
		auto& sound = m_activeEffects.back().sound;
		sound->setPosition( effectRequest.origin.x, effectRequest.origin.y, effectRequest.origin.z );
		sound->setRelativeToListener( false );
		auto pitchVariation = ( ( static_cast<float>( rand() ) / RAND_MAX ) - 0.5 ) * 2;
		//TODO Factor 1.2 should be configurable per sound effect
		sound->setPitch( pow( 1.2, pitchVariation ) );
		if ( rebalanceSound( m_activeEffects.back() ) )
		{
			sound->play();
		}
		else
		{
			m_activeEffects.pop_back();
		}
	}
	garbageCollection();
}

void AggregatorSound::onPlayNotify( const SoundEffect& effectRequest )
{
	AL::Context::Lock lock( m_audioContext );

	QString soundID       = effectRequest.type + ".";
	QString soundMaterial = effectRequest.material;
	if ( m_buffers.contains( soundID ) || m_buffers.contains( soundID + soundMaterial ) )
	{
		if ( m_buffers.contains( soundID + soundMaterial ) )
		{
			soundID = soundID + soundMaterial;
		}
		else
		{

			if ( Global::debugSound )
			{
				QString mat = soundID + soundMaterial;
				spdlog::debug( "Unknown sound material  {}", mat.toStdString() );
			}
		}
		m_activeEffects.append( ActiveEffect {
			true,
			Position(),
			std::make_shared<AL::Source>( m_audioContext, m_buffers[soundID] ) } );
		auto& sound = m_activeEffects.back().sound;
		sound->setPosition( 0, 0, 0 );
		sound->setRelativeToListener( true );
		sound->setPitch( 1 );
		if ( rebalanceSound( m_activeEffects.back() ) )
		{
			sound->play();
		}
		else
		{
			m_activeEffects.pop_back();
		}
	}
	garbageCollection();
}

bool AggregatorSound::rebalanceSound( ActiveEffect& effect )
{
	//TODO Check if we can get OpenAL band pass and reverb filters somehow working
	// Need them to properly do obstructed sound sources and "stone" environments
	effect.sound->setRelativeToListener( effect.isAbsolute );
	if ( !effect.isAbsolute )
	{
		if ( m_viewDirection.z == 0 )
		{
			//TODO Camera parameters are not known until changed at least once
			return true;
		}
		// Simple line-of-sight check from viewing position
		Position trace = effect.pos;
		// Half loudness per floor, and even less per wall
		short occlusion = 0;
		// Always tone down sounds out of view
		if ( trace.z > m_viewLevel )
		{
			occlusion += 1;
		}
		while ( trace.valid() && trace.z != m_viewLevel )
		{
			if ( trace.z < m_viewLevel )
			{
				trace = trace - m_viewDirection;
			}
			else
			{
				trace = trace + m_viewDirection;
			}
			if ( g->w()->wallType( trace ) & WT_SOLIDWALL )
			{
				occlusion += 2;
			}
			if ( g->w()->floorType( trace ) & FT_SOLIDFLOOR )
			{
				occlusion += 1;
			}
		}
		if ( m_occlusionFilter.empty() && occlusion < maxOcclusion )
		{
			// Without filter support, apply plain loudness reduction
			effect.sound->setVolume( std::pow( 0.75, occlusion ) );
		}
		else if ( occlusion < m_occlusionFilter.size() )
		{
			effect.sound->setDryPathFilter( m_occlusionFilter[occlusion] );
		}
		else
		{
			return false;
		}
	}
	return true;
}

void AggregatorSound::garbageCollection()
{
	//TODO Get the slot fed externally!
	m_audioListener->setVolume( Global::cfg->get_or_default<double>( "AudioMasterVolume" , 1.0 ) );

	for ( auto it = m_activeEffects.begin(); it != m_activeEffects.end(); )
	{
		if ( it->sound->getPlayState() == AL::Source::STOPPED )
		{
			it = m_activeEffects.erase( it );
		}
		else
		{
			++it;
		}
	}
}

void AggregatorSound::onCameraPosition( float x, float y, float z, int r, float scale )
{
	AL::Context::Lock lock( m_audioContext );
	garbageCollection();
	m_viewLevel = z;

	// Compute new listener position
	static constexpr QVector3D up = {
		0.f,
		0.f,
		1.f
	};
	float angle = 0;
	float x_rotated;
	float y_rotated;
	switch ( r )
	{
		case 0:
		{
			angle           = ( ( -45. ) * 3.1415 ) / 180.;
			m_viewDirection = { 1, 1, -1 };
			break;
		}
		case 1:
		{
			angle           = ( ( -45. - 90 ) * 3.1415 ) / 180.;
			m_viewDirection = { 1, -1, -1 };
			break;
		}
		case 2:
		{
			angle           = ( ( -45. - 90 - 90 ) * 3.1415 ) / 180.;
			m_viewDirection = { -1, -1, -1 };
			break;
		}
		case 3:
		{
			angle           = ( ( -45. - 90 - 90 - 90 ) * 3.1415 ) / 180.;
			m_viewDirection = { -1, 1, -1 };
			break;
		}
	}
	QVector3D direction = { static_cast<float>( m_viewDirection.x ), static_cast<float>( m_viewDirection.y ), static_cast<float>( m_viewDirection.z ) };
	direction.normalize();

	auto right = QVector3D::crossProduct( up, direction );
	auto relUp = -QVector3D::crossProduct( right, direction ).normalized();

	const float up3f[3]        = { relUp.x(), relUp.y(), relUp.z() };
	const float direction3f[3] = { direction.x(), direction.y(), direction.z() };
	m_audioListener->setOrientation( direction3f, up3f );

	x = -x / 32 + Global::dimX / 2;
	y = -y / 16;
	x = x - Global::dimX / 2;
	y = y - Global::dimY / 2;
	x = x * 1.41421; // sqrt(2)
	y = y * 1.41421;

	float s = sin( angle );
	float c = cos( angle );

	x_rotated = x;
	y_rotated = y;

	float xnew = x_rotated * c - y_rotated * s;
	float ynew = x_rotated * s + y_rotated * c;

	x_rotated = xnew + Global::dimX / 2;
	y_rotated = ynew + Global::dimY / 2;

	// Center of view on the currently active top-most layer
	QVector3D centerOfView = { x_rotated, y_rotated, z };
	// Virtual listener is actually floating above
	QVector3D listener = centerOfView - direction * 100.f / scale;

	const float position3f[3] = { listener.x(), listener.y(), listener.z() };
	m_audioListener->setPosition( position3f );

	// Cull sounds according to new view
	auto newEnd = std::remove_if(
		m_activeEffects.begin(), m_activeEffects.end(),
		[this]( ActiveEffect& effect )
		{ return !rebalanceSound( effect ); } );
	m_activeEffects.erase( newEnd, m_activeEffects.end() );
}
