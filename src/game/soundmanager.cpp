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
/** @file soundmanager.cpp
 *  @brief Sound effect dispatch: routes in-game events to the audio subsystem via signals.
 */
#include "soundmanager.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/position.h"
#include "../gui/eventconnector.h"
#include "game.h"

#include <QDebug>
#include <QJsonDocument>

/// @brief Constructs the SoundManager.
/// @param parent Owning Game instance.
SoundManager::SoundManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
}

/// @brief Destructor.
SoundManager::~SoundManager()
{
}

/// @brief Per-tick update hook. Currently a no-op.
/// @param tick Current game tick.
void SoundManager::onTick( quint64 tick )
{
}

/// @brief Plays a sound effect at the given world position (zero duration overload).
/// @param type     Sound effect type key.
/// @param pos      World position of the sound source.
/// @param material Material string for material-specific sound variants.
void SoundManager::playEffect( QString type, Position& pos, QString material )
{
	playEffect( type, pos, material, 0 );
}

/// @brief Plays a sound effect at the given world position.
///        Emits signalPlayEffect to the audio subsystem.
/// @param type     Sound effect type key.
/// @param pos      World position of the sound source.
/// @param material Material string for material-specific sound variants.
/// @param duration Intended playback duration in ticks (currently unused).
void SoundManager::playEffect( QString type, Position& pos, QString material, int duration )
{
	emit signalPlayEffect( SoundEffect { type, material, pos } );
}
