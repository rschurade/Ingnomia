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

#include <QPainter>

AgricultureProxy::AgricultureProxy( QObject* parent ) :
	QObject( parent )
{
	Global::eventConnector->aggregatorAgri()->signalUpdateFarm.connect(&AgricultureProxy::onUpdateFarm, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorAgri()->signalUpdatePasture.connect(&AgricultureProxy::onUpdatePasture, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorAgri()->signalUpdateGrove.connect(&AgricultureProxy::onUpdateGrove, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorAgri()->signalGlobalPlantInfo.connect(&AgricultureProxy::onUpdateGlobalPlants, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorAgri()->signalGlobalAnimalInfo.connect(&AgricultureProxy::onUpdateGlobalAnimals, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorAgri()->signalGlobalTreeInfo.connect(&AgricultureProxy::onUpdateGlobalTrees, this); // TODO: Qt::QueuedConnection

	this->signalSetBasicOptions.connect(&AggregatorAgri::onSetBasicOptions, Global::eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection
	this->signalSelectProduct.connect(&AggregatorAgri::onSelectProduct, Global::eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection
	this->signalSetHarvestOptions.connect(&AggregatorAgri::onSetHarvestOptions, Global::eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection
	this->signalSetGroveOptions.connect(&AggregatorAgri::onSetGroveOptions, Global::eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection
	this->signalRequestGlobalPlantInfo.connect(&AggregatorAgri::onRequestGlobalPlantInfo, Global::eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection
	this->signalRequestGlobalAnimalInfo.connect(&AggregatorAgri::onRequestGlobalAnimalInfo, Global::eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection
	this->signalRequestGlobalTreeInfo.connect(&AggregatorAgri::onRequestGlobalTreeInfo, Global::eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection

	this->signalSetMaxMale.connect(&AggregatorAgri::onSetMaxMale, Global::eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection
	this->signalSetMaxFemale.connect(&AggregatorAgri::onSetMaxFemale, Global::eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection
	this->signalSetButchering.connect(&AggregatorAgri::onSetButchering, Global::eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection
	this->signalRequestPastureAnimalInfo.connect(&AggregatorAgri::onRequestPastureAnimalInfo, Global::eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection
	this->signalRequestPastureFoodInfo.connect(&AggregatorAgri::onRequestPastureFoodInfo, Global::eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection
	this->signalSetFoodItemChecked.connect(&AggregatorAgri::onSetFoodItemChecked, Global::eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection
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
	signalSetBasicOptions( m_type, agricultureID, name, priority, suspended );
}

void AgricultureProxy::selectProduct( unsigned int agricultureID, QString product )
{
	signalSelectProduct( m_type, agricultureID, product );
}

void AgricultureProxy::setHarvestOptions( unsigned int agricultureID, bool harvest, bool harvestHay, bool tame )
{
	signalSetHarvestOptions( m_type, agricultureID, harvest, harvestHay, tame );
}

void AgricultureProxy::setGroveOptions( unsigned int designationID, bool pick, bool plant, bool fell )
{
	signalSetGroveOptions( designationID, pick, plant, fell );
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
	signalRequestGlobalPlantInfo();
}

void AgricultureProxy::requestGlobalAnimalInfo()
{
	signalRequestGlobalAnimalInfo();
}

void AgricultureProxy::requestGlobalTreeInfo()
{
	signalRequestGlobalTreeInfo();
}

void AgricultureProxy::setMaxMale( unsigned int designationID, int max )
{
	signalSetMaxMale( designationID, max );
}
	
void AgricultureProxy::setMaxFemale( unsigned int designationID, int max )
{
	signalSetMaxFemale( designationID, max );
}

void AgricultureProxy::setButchering( unsigned int animalId, bool value )
{
	signalSetButchering( animalId, value );
}

void AgricultureProxy::requestPastureAnimalInfo()
{
	signalRequestPastureAnimalInfo( m_AgricultureID );
}

void AgricultureProxy::requestPastureFoodInfo()
{
	signalRequestPastureFoodInfo( m_AgricultureID );
}

void AgricultureProxy::setFoodItemChecked( QString itemSID, QString materialSID, bool checked )
{
	signalSetFoodItemChecked( m_AgricultureID, itemSID, materialSID, checked );
}
