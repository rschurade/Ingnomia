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
#include "../../game/gamemanager.h"
#include "../eventconnector.h"
#include "PopulationModel.h"

#include <QDebug>
#include <QPainter>

PopulationProxy::PopulationProxy( QObject* parent ) :
	QObject( parent )
{
	connect( Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::signalPopulationUpdate, this, &PopulationProxy::onUpdateInfo, Qt::QueuedConnection );
	connect( Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::signalProfessionList, this, &PopulationProxy::onProfessionList, Qt::QueuedConnection );
	connect( Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::signalProfessionSkills, this, &PopulationProxy::onProfessionSkills, Qt::QueuedConnection );
	connect( Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::signalUpdateSingleGnome, this, &PopulationProxy::onUpdateSingleGnome, Qt::QueuedConnection );

	connect( this, &PopulationProxy::signalSetSkillActive, Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::onSetSkillActive, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalSetAllSkills, Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::onSetAllSkills, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalSetAllGnomes, Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::onSetAllGnomes, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalSetProfession, Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::onSetProfession, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalSortGnomes, Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::onSortGnomes, Qt::QueuedConnection );

	connect( this, &PopulationProxy::signalRequestSchedules, Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::onRequestSchedules, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalSetSchedule, Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::onSetSchedule, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalSetAllHours, Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::onSetAllHours, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalSetHourForAll, Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::onSetHourForAll, Qt::QueuedConnection );

	connect( Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::signalScheduleUpdate, this, &PopulationProxy::onUpdateSchedules, Qt::QueuedConnection );
	connect( Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::signalScheduleUpdateSingleGnome, this, &PopulationProxy::onScheduleUpdateSingleGnome, Qt::QueuedConnection );
	connect( Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::signalSelectEditProfession, this, &PopulationProxy::onSelectEditProfession, Qt::QueuedConnection );
	
	connect( this, &PopulationProxy::signalRequestProfessions, Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::onRequestProfessions, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalRequestSkills, Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::onRequestSkills, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalUpdateProfession, Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::onUpdateProfession, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalNewProfession, Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::onNewProfession, Qt::QueuedConnection );
	connect( this, &PopulationProxy::signalDeleteProfession, Global::gameManager->eventConnector()->aggregatorPopulation(), &AggregatorPopulation::onDeleteProfession, Qt::QueuedConnection );
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
