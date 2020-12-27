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

#include "../game/gnome.h"
#include "../game/job.h"

class GnomeManager;

class Automaton : public Gnome
{
public:
	Automaton( Position pos, unsigned int automatonItem, Game* game );
	Automaton( QVariantMap& in, Game* game );
	~Automaton();

	virtual void init();

	virtual void serialize( QVariantMap& out );

	virtual void updateSprite();

	virtual CreatureTickResult onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	void installCore( unsigned int itemID );
	unsigned int coreItem();

	unsigned int automatonItem()
	{
		return m_automatonItem;
	}

	bool getRefuelFlag();
	void setRefuelFlag( bool flag );

	void setCoreType( QString coreSID );
	QString coreType();

	void uninstallCore( bool uninstall );
	bool uninstallFlag();

	void setMaintenanceJob( QSharedPointer<Job> job );
	unsigned int maintenanceJobID();

	bool maintenanceJobChanged();

	int getFuelLevel();
	void fillUp( int burnValue );

protected:
	unsigned int m_automatonItem = 0;
	unsigned int m_core          = 0;
	float m_fuel                 = 0;
	bool m_refuel                = true;
	QString m_coreType;
	bool m_uninstallCore = false;

	QWeakPointer<Job> m_maintenaceJob;
	bool m_maintJobChanged       = false;
};
