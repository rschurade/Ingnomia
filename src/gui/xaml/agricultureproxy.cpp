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
	connect( Global::eventConnector->aggregatorAgri(), &AggregatorAgri::signalUpdateFarm, this, &AgricultureProxy::onUpdateFarm, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorAgri(), &AggregatorAgri::signalUpdatePasture, this, &AgricultureProxy::onUpdatePasture, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorAgri(), &AggregatorAgri::signalUpdateGrove, this, &AgricultureProxy::onUpdateGrove, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorAgri(), &AggregatorAgri::signalGlobalPlantInfo, this, &AgricultureProxy::onUpdateGlobalPlants, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorAgri(), &AggregatorAgri::signalGlobalAnimalInfo, this, &AgricultureProxy::onUpdateGlobalAnimals, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorAgri(), &AggregatorAgri::signalGlobalTreeInfo, this, &AgricultureProxy::onUpdateGlobalTrees, Qt::QueuedConnection );

	connect( this, &AgricultureProxy::signalSetBasicOptions, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onSetBasicOptions, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalSelectProduct, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onSelectProduct, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalSetHarvestOptions, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onSetHarvestOptions, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalSetGroveOptions, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onSetGroveOptions, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalRequestGlobalPlantInfo, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onRequestGlobalPlantInfo, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalRequestGlobalAnimalInfo, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onRequestGlobalAnimalInfo, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalRequestGlobalTreeInfo, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onRequestGlobalTreeInfo, Qt::QueuedConnection );

	connect( this, &AgricultureProxy::signalSetMaxMale, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onSetMaxMale, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalSetMaxFemale, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onSetMaxFemale, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalSetButchering, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onSetButchering, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalRequestPastureAnimalInfo, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onRequestPastureAnimalInfo, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalRequestPastureFoodInfo, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onRequestPastureFoodInfo, Qt::QueuedConnection );
	connect( this, &AgricultureProxy::signalSetFoodItemChecked, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onSetFoodItemChecked, Qt::QueuedConnection );
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
	if ( m_parent )
	{
		m_AgricultureID = info.ID;
		m_type          = AgriType::Farm;
		m_parent->updateFarmInfo( info );
	}
}

void AgricultureProxy::onUpdatePasture( const GuiPastureInfo& info )
{
	if ( m_parent )
	{
		m_AgricultureID = info.ID;
		m_type          = AgriType::Pasture;
		m_parent->updatePastureInfo( info );
	}
}

void AgricultureProxy::onUpdateGrove( const GuiGroveInfo& info )
{
	if ( m_parent )
	{
		m_AgricultureID = info.ID;
		m_type          = AgriType::Grove;
		m_parent->updateGroveInfo( info );
	}
}

void AgricultureProxy::setBasicOptions( unsigned int agricultureID, QString name, int priority, bool suspended )
{
	emit signalSetBasicOptions( m_type, agricultureID, name, priority, suspended );
}

void AgricultureProxy::selectProduct( unsigned int agricultureID, QString product )
{
	emit signalSelectProduct( m_type, agricultureID, product );
}

void AgricultureProxy::setHarvestOptions( unsigned int agricultureID, bool harvest, bool harvestHay, bool tame )
{
	emit signalSetHarvestOptions( m_type, agricultureID, harvest, harvestHay, tame );
}

void AgricultureProxy::setGroveOptions( unsigned int designationID, bool pick, bool plant, bool fell )
{
	emit signalSetGroveOptions( designationID, pick, plant, fell );
}

void AgricultureProxy::onUpdateGlobalPlants( const QList<GuiPlant>& plants )
{
	if ( m_parent )
	{
		m_parent->updateGlobalPlantInfo( plants );
	}
}

void AgricultureProxy::onUpdateGlobalAnimals( const QList<GuiAnimal>& animals )
{
	if ( m_parent )
	{
		m_parent->updateGlobalAnimalInfo( animals );
	}
}

void AgricultureProxy::onUpdateGlobalTrees( const QList<GuiPlant>& trees )
{
	if ( m_parent )
	{
		m_parent->updateGlobalTreeInfo( trees );
	}
}

void AgricultureProxy::requestGlobalPlantInfo()
{
	emit signalRequestGlobalPlantInfo();
}

void AgricultureProxy::requestGlobalAnimalInfo()
{
	emit signalRequestGlobalAnimalInfo();
}

void AgricultureProxy::requestGlobalTreeInfo()
{
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

void AgricultureProxy::requestPastureFoodInfo()
{
	emit signalRequestPastureFoodInfo( m_AgricultureID );
}

void AgricultureProxy::setFoodItemChecked( QString itemSID, QString materialSID, bool checked )
{
	emit signalSetFoodItemChecked( m_AgricultureID, itemSID, materialSID, checked );
}
