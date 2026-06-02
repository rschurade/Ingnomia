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

/** @file logger.cpp
 *  @brief Implementation of Logger, a thread-safe game event message log.
 */

#include "logger.h"

#include "../base/gamestate.h"

/** @brief Constructs an empty Logger instance. */
Logger::Logger()
{
}

/** @brief Destructor. */
Logger::~Logger()
{
}

/** @brief Clears all stored log messages. */
void Logger::reset()
{
	m_messages.clear();
}

/** @brief Adds a timestamped log message to the message list.
 *
 *  Creates a LogMessage stamped with the current game tick and appends
 *  it to the internal message buffer. Access is mutex-protected for
 *  thread safety.
 *
 *  @param lt            The log type/category of the message.
 *  @param msg           The human-readable log message text.
 *  @param sourceEntity  The ID of the entity that generated the message (0 if none).
 */
void Logger::log( LogType lt, QString msg, unsigned int sourceEntity )
{
	LogMessage lm { GameState::tick, "", lt, msg, sourceEntity };
	QMutexLocker lock( &m_mutex );
	m_messages.push_back( lm );
}
