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
#include "agricultureproxy.h"

#include "../../base/gamestate.h"
#include "../../gfx/sprite.h"
#include "../../gfx/spritefactory.h"
#include "../../base/global.h"
#include "../../gui/eventconnector.h"

#include <QDebug>
#include <QPainter>

AgricultureProxy::AgricultureProxy( QObject* parent ) :
	QObject( parent )
{
	connect( EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::signalUpdateFarm, this, &AgricultureProxy::onUpdateFarm, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::signalUpdatePasture, this, &AgricultureProxy::onUpdatePasture, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::signalUpdateGrove, this, &AgricultureProxy::onUpdateGrove, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::signalGlobalPlantInfo, this, &AgricultureProxy::onUpdateGlobalPlants, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::signalGlobalAnimalInfo, this, &AgricultureProxy::onUpdateGlobalAnimals, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::signalGlobalTreeInfo, this, &AgricultureProxy::onUpdateGlobalTrees, Qt::QueuedConnection );

	connect( this, &AgricultureProxy::signalSetBasicOptions, EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::onSetBasicOptions, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalSelectProduct, EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::onSelectProduct, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalSetHarvestOptions, EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::onSetHarvestOptions, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalSetGroveOptions, EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::onSetGroveOptions, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalRequestGlobalPlantInfo, EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::onRequestGlobalPlantInfo, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalRequestGlobalAnimalInfo, EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::onRequestGlobalAnimalInfo, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalRequestGlobalTreeInfo, EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::onRequestGlobalTreeInfo, Qt::QueuedConnection );

	connect( this, &AgricultureProxy::signalSetMaxMale, EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::onSetMaxMale, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalSetMaxFemale, EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::onSetMaxFemale, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalSetButchering, EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::onSetButchering, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalRequestPastureAnimalInfo, EventConnector::getInstance().aggregatorAgri(), &AggregatorAgri::onRequestPastureAnimalInfo, Qt::QueuedConnection );
}

AgricultureProxy::~AgricultureProxy()
{
}

void AgricultureProxy::setParent( IngnomiaGUI::AgricultureModel* parent )
{
	m_parent = parent;
}

void AgricultureProxy::onUpdateFarm( const GuiFarmInfo& info )
{
	qDebug() << "AgricultureProxy::onUpdateFarm";
	if ( m_parent )
	{
		m_AgricultureID = info.ID;
		m_type          = AgriType::Farm;
		m_parent->updateFarmInfo( info );
	}
}

void AgricultureProxy::onUpdatePasture( const GuiPastureInfo& info )
{
	qDebug() << "AgricultureProxy::onUpdatePasture";
	if ( m_parent )
	{
		m_AgricultureID = info.ID;
		m_type          = AgriType::Pasture;
		m_parent->updatePastureInfo( info );
	}
}

void AgricultureProxy::onUpdateGrove( const GuiGroveInfo& info )
{
	qDebug() << "AgricultureProxy::onUpdateGrove";
	if ( m_parent )
	{
		m_AgricultureID = info.ID;
		m_type          = AgriType::Grove;
		m_parent->updateGroveInfo( info );
	}
}

void AgricultureProxy::setBasicOptions( unsigned int agricultureID, QString name, int priority, bool suspended )
{
	qDebug() << "AgricultureProxy::setBasicOptions";
	emit signalSetBasicOptions( m_type, agricultureID, name, priority, suspended );
}

void AgricultureProxy::selectProduct( unsigned int agricultureID, QString product )
{
	qDebug() << "AgricultureProxy::selectProduct";
	emit signalSelectProduct( m_type, agricultureID, product );
}

void AgricultureProxy::setHarvestOptions( unsigned int agricultureID, bool harvest, bool harvestHay, bool tame )
{
	qDebug() << "AgricultureProxy::setHarvestOptions";
	emit signalSetHarvestOptions( m_type, agricultureID, harvest, harvestHay, tame );
}

void AgricultureProxy::setGroveOptions( unsigned int designationID, bool pick, bool plant, bool fell )
{
	qDebug() << "AgricultureProxy::setGroveOptions";
	emit signalSetGroveOptions( designationID, pick, plant, fell );
}

void AgricultureProxy::onUpdateGlobalPlants( const QList<GuiPlant>& plants )
{
	qDebug() << "AgricultureProxy::onUpdateGlobalPlants";
	if ( m_parent )
	{
		m_parent->updateGlobalPlantInfo( plants );
	}
}

void AgricultureProxy::onUpdateGlobalAnimals( const QList<GuiAnimal>& animals )
{
	qDebug() << "AgricultureProxy::onUpdateGlobalAnimals";
	if ( m_parent )
	{
		m_parent->updateGlobalAnimalInfo( animals );
	}
}

void AgricultureProxy::onUpdateGlobalTrees( const QList<GuiPlant>& trees )
{
	qDebug() << "AgricultureProxy::onUpdateGlobalTrees";
	if ( m_parent )
	{
		m_parent->updateGlobalTreeInfo( trees );
	}
}

void AgricultureProxy::requestGlobalPlantInfo()
{
	qDebug() << "AgricultureProxy::requestGlobalPlantInfo";
	emit signalRequestGlobalPlantInfo();
}

void AgricultureProxy::requestGlobalAnimalInfo()
{
	qDebug() << "AgricultureProxy::requestGlobalAnimalInfo";
	emit signalRequestGlobalAnimalInfo();
}

void AgricultureProxy::requestGlobalTreeInfo()
{
	qDebug() << "AgricultureProxy::requestGlobalTreeInfo";
	emit signalRequestGlobalTreeInfo();
}

void AgricultureProxy::setMaxMale( unsigned int designationID, int max )
{
	emit signalSetMaxMale( designationID, max );
}
	
void AgricultureProxy::setMaxFemale( unsigned int designationID, int max )
{
	emit signalSetMaxFemale( designationID, max );
}

void AgricultureProxy::setButchering( unsigned int animalId, bool value )
{
	emit signalSetButchering( animalId, value );
}

void AgricultureProxy::requestPastureAnimalInfo()
{
	emit signalRequestPastureAnimalInfo( m_AgricultureID );
}
