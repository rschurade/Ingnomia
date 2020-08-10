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
#include "populationproxy.h"

#include "../../base/db.h"
#include "../../base/gamestate.h"
#include "../../gfx/sprite.h"
#include "../../gfx/spritefactory.h"
#include "../../base/global.h"
#include "../eventconnector.h"
#include "PopulationModel.h"

#include <QDebug>
#include <QPainter>

PopulationProxy::PopulationProxy( QObject* parent ) :
	QObject( parent )
{
	connect( EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::signalPopulationUpdate, this, &PopulationProxy::onUpdateInfo, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::signalProfessionList, this, &PopulationProxy::onProfessionList, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::signalProfessionSkills, this, &PopulationProxy::onProfessionSkills, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::signalUpdateSingleGnome, this, &PopulationProxy::onUpdateSingleGnome, Qt::QueuedConnection );

	connect( this, &PopulationProxy::signalSetSkillActive, EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::onSetSkillActive, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalSetAllSkills, EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::onSetAllSkills, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalSetAllGnomes, EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::onSetAllGnomes, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalSetProfession, EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::onSetProfession, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalSortGnomes, EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::onSortGnomes, Qt::QueuedConnection );

	connect( this, &PopulationProxy::signalRequestSchedules, EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::onRequestSchedules, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalSetSchedule, EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::onSetSchedule, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalSetAllHours, EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::onSetAllHours, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalSetHourForAll, EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::onSetHourForAll, Qt::QueuedConnection );

	connect( EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::signalScheduleUpdate, this, &PopulationProxy::onUpdateSchedules, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::signalScheduleUpdateSingleGnome, this, &PopulationProxy::onScheduleUpdateSingleGnome, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::signalSelectEditProfession, this, &PopulationProxy::onSelectEditProfession, Qt::QueuedConnection );
	
	connect( this, &PopulationProxy::signalRequestProfessions, EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::onRequestProfessions, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalRequestSkills, EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::onRequestSkills, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalUpdateProfession, EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::onUpdateProfession, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalNewProfession, EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::onNewProfession, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalDeleteProfession, EventConnector::getInstance().aggregatorPopulation(), &AggregatorPopulation::onDeleteProfession, Qt::QueuedConnection );
}

PopulationProxy::~PopulationProxy()
{
}

void PopulationProxy::setParent( IngnomiaGUI::PopulationModel* parent )
{
	m_parent = parent;
}

void PopulationProxy::onUpdateInfo( const GuiPopulationInfo& info )
{
	if ( m_parent )
	{
		m_parent->updateInfo( info );
	}
}

void PopulationProxy::setSkillActive( unsigned int gnomeID, QString skillID, bool value )
{
	emit signalSetSkillActive( gnomeID, skillID, value );
}

void PopulationProxy::setAllSkillsForGnome( unsigned int gnomeID, bool value )
{
	emit signalSetAllSkills( gnomeID, value );
}
	
void PopulationProxy::setSkillForAllGnomes( QString skillID, bool value )
{
	emit signalSetAllGnomes( skillID, value );
}

void PopulationProxy::onProfessionList( const QStringList& professions )
{
	if( m_parent )
	{
		m_parent->updateProfessionList( professions );
	}
}

void PopulationProxy::onProfessionSkills( const QString profession, const QList<GuiSkillInfo>& skills )
{
	if( m_parent )
	{
		m_parent->updateProfessionSkills( profession, skills );
	}
}

void PopulationProxy::setProfession( unsigned int gnomeID, QString profession )
{
	emit signalSetProfession( gnomeID, profession );
}

void PopulationProxy::onUpdateSingleGnome( const GuiGnomeInfo& gnome )
{
	if( m_parent )
	{
		m_parent->updateSingleGnome( gnome );
	}
}

void PopulationProxy::sortGnomes( QString mode )
{ 
	emit signalSortGnomes( mode );
}

void PopulationProxy::requestSchedules()
{
	emit signalRequestSchedules();
}
	
void PopulationProxy::setSchedule( unsigned int gnomeID, int hour, ScheduleActivity activity )
{
	emit signalSetSchedule( gnomeID, hour, m_currentActivitiy );
}

void PopulationProxy::setAllHours( unsigned int gnomeID, ScheduleActivity activitiy )
{
	emit signalSetAllHours( gnomeID, activitiy );
}
	
void PopulationProxy::setHourForAll( int hour, ScheduleActivity activity )
{
	emit signalSetHourForAll( hour, activity );
}

void PopulationProxy::onUpdateSchedules( const GuiScheduleInfo& info )
{
	if( m_parent )
	{
		m_parent->updateSchedules( info );
	}
}

void PopulationProxy::onScheduleUpdateSingleGnome( const GuiGnomeScheduleInfo& info )
{
	if( m_parent )
	{
		m_parent->updateScheduleSingleGnome( info );
	}
}

void PopulationProxy::setCurrentActivity( ScheduleActivity activity )
{
	m_currentActivitiy = activity;
}

void PopulationProxy::requestProfessions()
{
	emit signalRequestProfessions();
}
	
void PopulationProxy::requestSkills( QString profession )
{
	emit signalRequestSkills( profession );
}

void PopulationProxy::updateProfession( QString name, QString newName, QStringList skills )
{
	emit signalUpdateProfession( name, newName, skills );
}

void PopulationProxy::deleteProfession( QString name )
{
	emit signalDeleteProfession( name );
}

void PopulationProxy::newProfession()
{
	emit signalNewProfession();
}

void PopulationProxy::onSelectEditProfession( const QString name )
{
	if( m_parent )
	{
		m_parent->selectEditProfession( name );
	}
}
