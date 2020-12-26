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


#include "../game/automaton.h"
#include "../game/gnome.h"
#include "../game/gnometrader.h"

#include <QList>

class Game;

class GnomeManager : public QObject
{
	friend class Gnome;
	friend class GnomeTrader;
	friend class Automaton;

	Q_OBJECT
	Q_DISABLE_COPY_MOVE( GnomeManager )
private:
	QPointer<Game> g;

	QList<Gnome*> m_gnomes;
	QList<Gnome*> m_deadGnomes;
	QMap<unsigned int, Gnome*> m_gnomesByID;
	QMap<QString, QStringList> m_profs;

	QList<Gnome*> m_specialGnomes;
	QList<Automaton*> m_automatons;

	int m_startIndex = 0;

	QHash<unsigned int, QSharedPointer<Job>> m_jobs;

public:
	GnomeManager( Game* parent );
	~GnomeManager();

	void loadProfessions();
	void saveProfessions();
	QStringList professions();
	QStringList professionSkills( QString profession );
	QString addProfession();
	void addProfession( QString name, QStringList skills );
	void removeProfession( QString name );
	void modifyProfession( QString name, QString newName, QStringList skills );

	void addGnome( Position pos );
	void addGnome( QVariantMap values );

	unsigned int addTrader( Position pos, unsigned int workshopID, QString type );
	void addTrader( QVariantMap values );

	void addAutomaton( Automaton* automaton );
	void addAutomaton( QVariantMap values );

	void onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	bool contains( unsigned int gnomeID );

	QList<Gnome*>& gnomes()
	{
		return m_gnomes;
	}
	QList<Gnome*> gnomesSorted();
	QList<Gnome*>& specialGnomes()
	{
		return m_specialGnomes;
	}
	QList<Automaton*>& automatons()
	{
		return m_automatons;
	}
	QList<Gnome*>& deadGnomes()
	{
		return m_deadGnomes;
	}

	void forceMoveGnomes( Position from, Position to );

	QList<Gnome*> gnomesAtPosition( Position pos );
	QList<Gnome*> deadGnomesAtPosition( Position pos );
	Gnome* gnome( unsigned int gnomeID );
	GnomeTrader* trader( unsigned int traderID );
	Automaton* automaton( unsigned int automatonID );

	bool gnomeCanReach( unsigned int gnomeID, Position pos );

	unsigned int getJob( unsigned int gnomeID, QString skillID );

	bool finishJob( unsigned int jobID );
	bool giveBackJob( unsigned int jobID );
	QSharedPointer<Job> getJob( unsigned int jobID );
	bool hasJobID( unsigned int jobID ) const;

	void setInMission( unsigned int gnomeID, unsigned int missionID );

	QString name( unsigned int gnomeID );
	
	unsigned int roleID( unsigned int gnomeID );
	void setRoleID( unsigned int gnomeID, unsigned int roleID );

private:
	QSharedPointer<Job> getRefuelJob( Automaton* a );
	QSharedPointer<Job> getInstallJob( Automaton* a );
	QSharedPointer<Job> getUninstallJob( Automaton* a );

signals:
	void signalGnomeActivity( unsigned int id, QString skillID );
	void signalGnomeDeath( unsigned int id );
};