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

#include <range/v3/view.hpp>

#include <sigslot/signal.hpp>

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
	absl::btree_map<unsigned int, Gnome*> m_gnomesByID;
	absl::btree_map<std::string, std::vector<std::string>> m_profs;

	QList<Gnome*> m_specialGnomes;
	QList<Automaton*> m_automatons;

	int m_startIndex = 0;

public:
	GnomeManager( Game* parent );
	~GnomeManager();

	void loadProfessions();
	void saveProfessions();
	std::vector<std::string> professions()
	{
		const auto keysView = m_profs | ranges::views::keys;
		std::vector<std::string> keys(keysView.begin(), keysView.end());
		return keys;
	}

	const std::vector<std::string>& professionSkills( const std::string& profession );
	std::string addProfession();
	void addProfession( const std::string& name, const std::vector<std::string>& skills );
	void removeProfession( const std::string& name );
	void modifyProfession( const std::string& name, const std::string& newName, const std::vector<std::string>& skills );

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

	void createJobs();

	void setInMission( unsigned int gnomeID, unsigned int missionID );

	QString name( unsigned int gnomeID );
	
	unsigned int roleID( unsigned int gnomeID );
	void setRoleID( unsigned int gnomeID, unsigned int roleID );

	int numGnomes();

private:
	void getRefuelJob( Automaton* a );
	void getInstallJob( Automaton* a );
	void getUninstallJob( Automaton* a );

public: // signals:
	sigslot::signal<unsigned int /*id*/, const std::string& /*skillID*/> signalGnomeActivity;
	sigslot::signal<unsigned int /*id*/> signalGnomeDeath;

#ifdef _DEBUG
public:
	void showDebug();
#endif
};