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
/** @file militarymanager.h
 *  @brief Squad management, military roles/uniforms, target priorities, and combat attitude configuration.
 */
#pragma once


#include "../base/gamestate.h"

#include <QObject>
#include <QRandomGenerator>
#include <QSet>
#include <QVariantMap>

class Game;

/** @brief Quality filter for uniform item selection. */
enum class UniformItemQuality : unsigned char
{
	ANY,
	LOWEST,
	BEST
};

/** @brief A single equipment slot in a uniform, specifying item type, material, and quality preference. */
struct UniformItem
{
	QString type               = "none";
	QString item               = "none";
	QString material           = "any";
	UniformItemQuality quality = UniformItemQuality::ANY;

	QVariantMap serialize();
	UniformItem( const QVariantMap& in );

	UniformItem() {};
};

/** @brief A complete equipment loadout containing armor and weapon slots for a military role. */
struct Uniform
{
	QString name = "new uniform";
	quint32 id   = GameState::createID();

	QMap<QString, UniformItem> parts;

	QVariantMap serialize();
	Uniform( const QVariantMap& in );
	Uniform();
};

/** @brief A military role defining a uniform and combat behavior flags (maintain distance, retreat when bleeding). */
struct MilitaryRole
{
	QString name = "new role";
	quint32 id   = GameState::createID();

	Uniform uniform;

	bool isCivilian = false;

	bool maintainDist    = false;
	bool retreatBleeding = false;

	QVariantMap serialize();
	MilitaryRole( const QVariantMap& in );

	MilitaryRole() {};
};

/** @brief Combat attitude toward a creature type. */
enum class MilAttitude {
	FLEE,
	DEFEND,
	ATTACK,
	HUNT
};
Q_DECLARE_METATYPE( MilAttitude )

/** @brief A target creature type paired with the squad's attitude toward it. */
struct TargetPriority {
	QString type;
	MilAttitude attitude;
};

/** @brief A military squad containing gnome members and an ordered list of target priorities. */
struct Squad
{
	QString name = "new squad";
	quint32 id   = GameState::createID();
	QList<QString> types;

	QList<unsigned int> gnomes;

	QVariantMap serialize();
	Squad( QList<QString> tps, const QVariantMap& in );

	QList<TargetPriority> priorities;

	Squad( QList<QString> tps ) : types( tps ) {};
	Squad() {};
};
Q_DECLARE_METATYPE( Squad )







/** @brief Manages military roles, uniforms, squads, gnome assignments, and target priority ordering.
 *
 *  Provides CRUD operations for roles and squads, handles gnome-to-squad assignment,
 *  serialization/deserialization of military state, and responds to death events
 *  and attitude changes from the GUI.
 */
class MilitaryManager : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( MilitaryManager )
public:
	MilitaryManager() = delete;
	MilitaryManager( Game* parent );
	~MilitaryManager();

	void init();

	void onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	QVariantMap serialize();

	Uniform* uniform( unsigned int roleID );
	Uniform uniformCopy( unsigned int roleID );

	MilitaryRole* role( unsigned int id );

	Squad* squad( unsigned int id );
	Squad* getSquadForGnome( unsigned int gnomeID );

	QHash<unsigned int, MilitaryRole>& roles()
	{
		return m_roles;
	}
	const QList<Squad>& squads()
	{
		return m_squads;
	}

	QString roleName( unsigned int id );

	Uniform* getGnomeUniform( unsigned int gnomeID );
	Uniform* getGnomeUniform( unsigned int gnomeID, Squad& squad );

	void updateGnome( unsigned int gnomeID );

	void save();

	bool roleIsCivilian( unsigned int roleID );

private:
	QPointer<Game> g;

	int m_startIndex = 0;

	QHash<unsigned int, MilitaryRole> m_roles;
	QList<Squad> m_squads;

	QHash<unsigned int, unsigned int> m_gnome2Squad;

	void removeGnomeFromOtherSquad( unsigned int squadID, unsigned int gnomeID );

signals:
	void sendOverlayMessage( int id, QString text );

public slots:
	unsigned int addRole();
	bool removeRole( unsigned int id );
	void renameRole( unsigned int id, QString text );

	void setArmorType( unsigned int roleID, QString slot, QString type, QString material );


	unsigned int addSquad();
	bool removeSquad( unsigned int id );
	void renameSquad( unsigned int id, QString text );
	void addSquadTarget( unsigned int squadID, unsigned int target );
	void removeSquadTarget( unsigned int squadID, unsigned int target );
	QList<unsigned int> squadTargets( unsigned int squadID );
	void moveSquadUp( unsigned int id );
	void moveSquadDown( unsigned int id );

	bool removeGnome( unsigned int gnomeID );
	bool moveGnomeUp( unsigned int gnomeID );
	bool moveGnomeDown( unsigned int gnomeID );


	void onGnomeDeath( unsigned int id );
	void onMonsterDeath( unsigned int id );
	void onAnimalDeath( unsigned int id );

	void onSetAttitude( unsigned int squadID, QString type, MilAttitude attitude );

	bool movePrioUp( unsigned int squadID, QString type );
	bool movePrioDown( unsigned int squadID, QString type );

	void setRoleCivilian( unsigned int roleID, bool value );
};
