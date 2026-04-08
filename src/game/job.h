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

#include "../base/position.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

struct Position;

enum class JobPhase : uint8_t
{
	PENDING,      // requirements known, items not yet claimed
	HAULING,      // items claimed atomically, haul sub-jobs active
	READY,        // all items at work site, waiting for skilled worker
	IN_PROGRESS,  // being worked, progress tracked
	SUSPENDED,    // worker(s) left, progress preserved
	COMPLETE      // done
};

struct RequiredItem
{
	int count = 0;
	QString itemSID;
	QString materialSID;
	QStringList materialRestriction;
	bool requireSame = false;

	// only reset when 'requiredItemsExist()' is called
	// only used for GUI info display
	bool available = false;
};

struct RequiredTool
{
	QString type;
	quint8 level = 0;

	// only used for GUI info display
	bool available = false;
};

class Job
{
	Q_DISABLE_COPY_MOVE( Job )
private:
	unsigned int m_id = 0;
	QString m_type;
	QString m_requiredSkill;
	QString m_description;
	quint8 m_rotation  = 0;
	bool m_noJobSprite = false;
	quint8 m_priority  = 0;

	bool m_canceled = false;
	bool m_aborted  = false;
	// is used in various places to reset job status
	bool m_componentMissing = false;
	bool m_mayTrap          = false;
	bool m_destroyOnAbort   = false;

	bool m_jobIsWorked      = false;
	unsigned int m_workedBy = 0;

	// Job phase and progress
	JobPhase m_phase = JobPhase::PENDING;
	float m_totalWorkTicks = 0.0f;
	float m_accumulatedTicks = 0.0f;
	int m_currentTaskIndex = 0;
	QList<QVariantMap> m_taskList; // moved from CanWork

	// Multi-worker support
	QSet<unsigned int> m_activeWorkers;
	unsigned int m_maxWorkers = 1;
	QHash<unsigned int, Position> m_workerPositions;

	// Hauling sub-jobs
	unsigned int m_parentJobID = 0;
	QList<unsigned int> m_haulSubJobs;
	int m_itemsDelivered = 0;
	int m_itemsRequired = 0;

	RequiredTool m_requiredTool;
	QList<RequiredItem> m_requiredItems;

	Position m_position;
	Position m_posItemInput;
	Position m_posItemOutput;
	Position m_toolPosition;
	Position m_workPosition;
	QList<Position> m_possibleWorkPositions;
	QList<Position> m_origWorkPosOffsets;

	int m_amount = 1;
	QString m_item;
	QString m_material;
	QString m_craftID;
	QVariantMap m_craft;
	QString m_conversionMaterial;
	unsigned int m_craftJobID = 0;

	unsigned int m_stockpile = 0;
	unsigned int m_animal    = 0;
	unsigned int m_automaton = 0;
	unsigned int m_mechanism = 0;

	QList<unsigned int> m_itemsToHaul;
	QString m_spell;

public:
	Job();
	Job( QVariantMap in );

	//Job( const Job& other );

	~Job();

	QVariant serialize() const;

	unsigned int id() const;

	QString type() const;
	void setType( QString type );

	QString requiredSkill() const;
	void setRequiredSkill( QString skill );

	void setDescription( QString desc );
	QString description() const;

	// position of the tile being worked
	Position pos() const;
	void setPos( const Position& pos );

	Position posItemInput() const;
	void setPosItemInput( const Position& pos );

	Position posItemOutput() const;
	void setPosItemOutput( const Position& pos );

	Position toolPosition() const;
	void setToolPosition( const Position& pos );

	QList<Position> possibleWorkPositions();
	void addPossibleWorkPosition( const Position& stage );

	Position workPos() const;
	void setWorkPos( const Position& pos );

	void clearPossibleWorkPositions();

	QList<Position> origWorkPosOffsets();
	void setOrigWorkPosOffsets( QString offsets );
	void setOrigWorkPosOffsets( QList<Position> );

	bool isWorked() const;
	void setIsWorked( bool v );

	unsigned char rotation() const;
	void setRotation( unsigned char rot );

	QList<unsigned int> itemsToHaul() const;
	void addItemToHaul( unsigned int );

	QString item() const;
	void setItem( QString item );

	unsigned int stockpile() const;
	void setStockpile( unsigned int sp );

	unsigned int animal() const;
	void setAnimal( unsigned int animal );

	unsigned int automaton() const;
	void setAutomaton( unsigned int automaton );

	unsigned int mechanism() const;
	void setMechanism( unsigned int animal );

	QString material() const;
	void setMaterial( QString material );

	void setComponentMissing( bool v );
	bool componentMissing() const;

	QList<RequiredItem> requiredItems() const;
	void addRequiredItem( int count, QString item, QString material, QStringList materialRestriction, bool requireSame = false );

	int distanceSquare( Position& pos, int zWeight = 1 ) const;

	bool noJobSprite() const;
	void setNoJobSprite( bool v );

	QVariantMap craft() const;
	void setCraft( QVariantMap craft );

	void setCanceled();
	bool isCanceled() const;

	void setAborted( bool v );
	bool isAborted() const;

	void setWorkedBy( unsigned int gnomeID );
	unsigned int workedBy() const;

	RequiredTool requiredTool() const;
	void setRequiredTool( QString toolID, quint8 level );
	void setRequiredToolAvailable( bool avail );

	void setConversionMaterial( QString material );
	QString conversionMaterial() const;

	void setAmount( int amount );
	int amount() const;

	void setSpell( QString spell );
	QString spell() const;

	void setMayTrap( bool value );
	bool mayTrap() const;

	void setDestroyOnAbort( bool value );
	bool destroyOnAbort() const;

	void setCraftID( QString craftID );
	QString craftID() const;

	void setCraftJobID( unsigned int craftJobID );
	unsigned int craftJobID() const;

	void raisePrio();
	void lowerPrio();
	int priority() const;
	void setPrio( int prio );

	// Phase
	JobPhase phase() const { return m_phase; }
	void setPhase( JobPhase phase ) { m_phase = phase; }

	// Progress
	float progress() const { return m_totalWorkTicks > 0 ? m_accumulatedTicks / m_totalWorkTicks : 0.0f; }
	float totalWorkTicks() const { return m_totalWorkTicks; }
	void setTotalWorkTicks( float ticks ) { m_totalWorkTicks = ticks; }
	float accumulatedTicks() const { return m_accumulatedTicks; }
	void addWork( float ticks ) { m_accumulatedTicks += ticks; }

	// Task tracking (moved from CanWork)
	int currentTaskIndex() const { return m_currentTaskIndex; }
	void setCurrentTaskIndex( int index ) { m_currentTaskIndex = index; }
	const QList<QVariantMap>& taskList() const { return m_taskList; }
	void setTaskList( const QList<QVariantMap>& list ) { m_taskList = list; }

	// Multi-worker
	const QSet<unsigned int>& activeWorkers() const { return m_activeWorkers; }
	bool addWorker( unsigned int gnomeID, Position workPos );
	void removeWorker( unsigned int gnomeID );
	unsigned int maxWorkers() const { return m_maxWorkers; }
	void setMaxWorkers( unsigned int max ) { m_maxWorkers = max; }
	bool canAcceptWorker() const { return m_activeWorkers.size() < m_maxWorkers; }

	// Hauling sub-jobs
	unsigned int parentJobID() const { return m_parentJobID; }
	void setParentJobID( unsigned int id ) { m_parentJobID = id; }
	const QList<unsigned int>& haulSubJobs() const { return m_haulSubJobs; }
	void addHaulSubJob( unsigned int jobID ) { m_haulSubJobs.append( jobID ); }
	int itemsDelivered() const { return m_itemsDelivered; }
	void incrementItemsDelivered() { ++m_itemsDelivered; }
	int itemsRequired() const { return m_itemsRequired; }
	void setItemsRequired( int count ) { m_itemsRequired = count; }
};
