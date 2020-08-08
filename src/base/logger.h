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
#pragma once

#include <QMutex>
#include <QString>

#include <vector>

enum class LogType
{
	DEBUG,
	JOB,
	CRAFT,
	COMBAT
};

struct LogMessage
{
	quint64 tick;
	QString dateTime;
	LogType type;
	QString message;
	unsigned int source;
};

class Logger
{
public:
	Logger();
	~Logger();

	void reset();

	void log( LogType lt, QString msg, unsigned int sourceEntity );

	std::vector<LogMessage>& messages()
	{
		return m_messages;
	}

private:
	QMutex m_mutex;

	std::vector<LogMessage> m_messages;
};
