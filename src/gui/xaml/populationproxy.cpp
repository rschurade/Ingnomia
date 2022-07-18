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

PopulationProxy::PopulationProxy( QObject* parent ) :
	QObject( parent )
{
	Global::eventConnector->aggregatorPopulation()->signalPopulationUpdate.connect(&PopulationProxy::onUpdateInfo, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorPopulation()->signalProfessionList.connect(&PopulationProxy::onProfessionList, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorPopulation()->signalProfessionSkills.connect(&PopulationProxy::onProfessionSkills, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorPopulation()->signalUpdateSingleGnome.connect(&PopulationProxy::onUpdateSingleGnome, this); // TODO: Qt::QueuedConnection

	this->signalSetSkillActive.connect(&AggregatorPopulation::onSetSkillActive, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection
	this->signalSetAllSkills.connect(&AggregatorPopulation::onSetAllSkills, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection
	this->signalSetAllGnomes.connect(&AggregatorPopulation::onSetAllGnomes, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection
	this->signalSetProfession.connect(&AggregatorPopulation::onSetProfession, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection
	this->signalSortGnomes.connect(&AggregatorPopulation::onSortGnomes, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection

	this->signalRequestSchedules.connect(&AggregatorPopulation::onRequestSchedules, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection
	this->signalSetSchedule.connect(&AggregatorPopulation::onSetSchedule, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection
	this->signalSetAllHours.connect(&AggregatorPopulation::onSetAllHours, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection
	this->signalSetHourForAll.connect(&AggregatorPopulation::onSetHourForAll, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection

	Global::eventConnector->aggregatorPopulation()->signalScheduleUpdate.connect(&PopulationProxy::onUpdateSchedules, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorPopulation()->signalScheduleUpdateSingleGnome.connect(&PopulationProxy::onScheduleUpdateSingleGnome, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorPopulation()->signalSelectEditProfession.connect(&PopulationProxy::onSelectEditProfession, this); // TODO: Qt::QueuedConnection
	
	this->signalRequestProfessions.connect(&AggregatorPopulation::onRequestProfessions, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection
	this->signalRequestSkills.connect(&AggregatorPopulation::onRequestSkills, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection
	this->signalUpdateProfession.connect(&AggregatorPopulation::onUpdateProfession, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection
	this->signalNewProfession.connect(&AggregatorPopulation::onNewProfession, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection
	this->signalDeleteProfession.connect(&AggregatorPopulation::onDeleteProfession, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection
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

void PopulationProxy::setSkillActive( unsigned int gnomeID, const std::string& skillID, bool value )
{
	signalSetSkillActive( gnomeID, skillID, value );
}

void PopulationProxy::setAllSkillsForGnome( unsigned int gnomeID, bool value )
{
	signalSetAllSkills( gnomeID, value );
}
	
void PopulationProxy::setSkillForAllGnomes( const std::string& skillID, bool value )
{
	signalSetAllGnomes( skillID, value );
}

void PopulationProxy::onProfessionList( const std::vector<std::string>& professions )
{
	if( m_parent )
	{
		m_parent->updateProfessionList( professions );
	}
}

void PopulationProxy::onProfessionSkills( const std::string& profession, const QList<GuiSkillInfo>& skills )
{
	if( m_parent )
	{
		m_parent->updateProfessionSkills( profession, skills );
	}
}

void PopulationProxy::setProfession( unsigned int gnomeID, const std::string& profession )
{
	signalSetProfession( gnomeID, profession );
}

void PopulationProxy::onUpdateSingleGnome( const GuiGnomeInfo& gnome )
{
	if( m_parent )
	{
		m_parent->updateSingleGnome( gnome );
	}
}

void PopulationProxy::sortGnomes( const std::string& mode )
{ 
	signalSortGnomes( mode );
}

void PopulationProxy::requestSchedules()
{
	signalRequestSchedules();
}
	
void PopulationProxy::setSchedule( unsigned int gnomeID, int hour, ScheduleActivity activity )
{
	signalSetSchedule( gnomeID, hour, m_currentActivitiy );
}

void PopulationProxy::setAllHours( unsigned int gnomeID, ScheduleActivity activitiy )
{
	signalSetAllHours( gnomeID, activitiy );
}
	
void PopulationProxy::setHourForAll( int hour, ScheduleActivity activity )
{
	signalSetHourForAll( hour, activity );
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
	signalRequestProfessions();
}
	
void PopulationProxy::requestSkills( const std::string& profession )
{
	signalRequestSkills( profession );
}

void PopulationProxy::updateProfession( const std::string& name, const std::string& newName, const std::vector<std::string>& skills )
{
	signalUpdateProfession( name, newName, skills );
}

void PopulationProxy::deleteProfession( const std::string& name )
{
	signalDeleteProfession( name );
}

void PopulationProxy::newProfession()
{
	signalNewProfession();
}

void PopulationProxy::onSelectEditProfession( const std::string& name )
{
	if( m_parent )
	{
		m_parent->selectEditProfession( QString::fromStdString(name) );
	}
}
