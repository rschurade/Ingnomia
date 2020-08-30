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

#include "../aggregatoragri.h"
#include "agriculturemodel.h"

#include <QObject>

class AgricultureProxy : public QObject
{
	Q_OBJECT

public:
	AgricultureProxy( QObject* parent = nullptr );
	~AgricultureProxy();

	void setParent( IngnomiaGUI::AgricultureModel* parent );

	void setBasicOptions( unsigned int designationID, QString name, int priority, bool suspended );
	void setHarvestOptions( unsigned int designationID, bool farmHarvest, bool harvestHay, bool tame );
	void setGroveOptions( unsigned int designationID, bool pick, bool plant, bool fell );

	void selectProduct( unsigned int designationID, QString product );

	void setMaxMale( unsigned int designationID, int max );
	void setMaxFemale( unsigned int designationID, int max );

	void requestGlobalPlantInfo();
	void requestGlobalAnimalInfo();
	void requestGlobalTreeInfo();

	void requestPastureAnimalInfo();
	void requestPastureFoodInfo();

	void setButchering( unsigned int animalId, bool value );

	void setFoodItemChecked( QString itemSID, QString materialSID, bool checked );

private:
	IngnomiaGUI::AgricultureModel* m_parent = nullptr;

	unsigned int m_AgricultureID = 0;
	AgriType m_type              = AgriType::Farm;

private slots:
	void onUpdateFarm( const GuiFarmInfo& info );
	void onUpdatePasture( const GuiPastureInfo& info );
	void onUpdateGrove( const GuiGroveInfo& info );

	void onUpdateGlobalPlants( const QList<GuiPlant>& plants );
	void onUpdateGlobalAnimals( const QList<GuiAnimal>& animals );
	void onUpdateGlobalTrees( const QList<GuiPlant>& trees );

signals:
	void signalSetBasicOptions( AgriType type, unsigned int designationID, QString name, int priority, bool suspended );
	void signalSelectProduct( AgriType type, unsigned int designationID, QString product );
	void signalSetHarvestOptions( AgriType type, unsigned int designationID, bool harvest, bool harvestHay, bool tame );
	void signalSetGroveOptions( unsigned int designationID, bool pick, bool plant, bool fell );
	void signalRequestGlobalPlantInfo();
	void signalRequestGlobalAnimalInfo();
	void signalRequestGlobalTreeInfo();
	void signalSetMaxMale( unsigned int designationID, int max );
	void signalSetMaxFemale( unsigned int designationID, int max );
	void signalSetButchering( unsigned int animalId, bool value );
	void signalRequestPastureAnimalInfo( unsigned int pastureID );
	void signalRequestPastureFoodInfo( unsigned int pastureID );
	void signalSetFoodItemChecked( unsigned int pastureID, QString itemSID, QString materialSID, bool checked );
};
