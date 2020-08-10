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

#include "../aggregatorstockpile.h"
#include "StockpileModel.h"

#include <QObject>

class StockpileProxy : public QObject
{
	Q_OBJECT

public:
	StockpileProxy( QObject* parent = nullptr );
	~StockpileProxy();

	void setParent( IngnomiaGUI::StockpileModel* parent );

	void setBasicOptions( unsigned int stockpileID, QString name, int priority, bool suspended, bool pull, bool allowPull );

	void setActive( unsigned int stockpileID, bool active, QString category, QString group = "", QString item = "", QString material = "" );

private:
	IngnomiaGUI::StockpileModel* m_parent = nullptr;

private slots:
	void onUpdateInfo( const GuiStockpileInfo& info );
	void onUpdateContent( const GuiStockpileInfo& info );

signals:
	void signalSetBasicOptions( unsigned int stockpileID, QString name, int priority, bool suspended, bool pull, bool allowPull );
	void signalSetActive( unsigned int stockpileID, bool active, QString category, QString group, QString item, QString material );
};
