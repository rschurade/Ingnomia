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

#include <QColor>
#include <QList>
#include <QPair>
#include <QString>
#include <QVariantMap>

class Game;

enum enumReqs
{
	SEL_NONE,
	SEL_FLOOR,
	SEL_FLOOR_OR_SCAFFOLD,
	SEL_WALL,
	SEL_MINEABLE_WALL,
	SEL_CONSTRUCTION,
	SEL_JOB,
	SEL_DESIGNATION,
	SEL_STOCKPILE,
	SEL_ROOM,
	SEL_ALLOW_BELL,
	SEL_ALLOW_FURNITURE,
	SEL_PASTURE,
	SEL_WALKABLE,
	SEL_MECHANISM,
	SEL_GEARBOX,

	SEL_STAIRS,
	SEL_STAIRSTOP,
	SEL_RAMP,
	SEL_RAMPTOP,

	SEL_SOIL,
	SEL_TREE,
	SEL_PLANT,
	SEL_TREECLIP,
	SEL_PLANT_WITH_FRUIT,
	SEL_TREE_IN_RANGE,

	SEL_ANYWALL
};

class Selection : public QObject
{
	Q_OBJECT

private:
	QPointer<Game> g;

	int m_rotation;
	Position m_firstClick;
	bool m_firstClicked;
	QString m_action;
	QString m_item;
	QStringList m_materials;
	QList<QPair<Position, bool>> m_selection;
	QPair<int, int> m_selectionSize;
	bool m_debug;
	bool m_isFloor    = false;
	bool m_isMulti    = false;
	bool m_isMultiZ   = false;
	bool m_ctrlActive = false;
	QList<QVariantMap> m_tileCheckList;
	bool m_canRotate = false;

	QMap<QString, int> m_reqMap;

	bool testTileForJobSelection( const Position& pos );
	void onSecondClick( bool shift, bool ctrl );

	bool m_changed = false;

public:
	Selection( Game* game );
	~Selection();

	QVariantMap serialize();

	void clear();
	void rotate();
	bool leftClick( Position& pos, bool shift, bool ctrl );
	void updateSelection( Position& pos, bool shift, bool ctrl );
	void rightClick( Position& pos );
	void setAction( QString action );
	void setItemID( QString item )
	{
		m_item = item;
	}
	void setMaterials( QList<QString> materials )
	{
		m_materials = materials;
	}
	void setMaterial( QString material )
	{
		m_materials.clear();
		m_materials.append( material );
	}
	void setDebug( bool d )
	{
		m_debug = d;
	}

	bool hasAction()
	{
		return !m_action.isEmpty();
	}
	QList<QPair<Position, bool>>& getSelection()
	{
		return m_selection;
	}
	QString& action()
	{
		return m_action;
	}
	QString& itemID()
	{
		return m_item;
	}
	QStringList& material()
	{
		return m_materials;
	}
	QPair<int, int>& size()
	{
		return m_selectionSize;
	}
	int rotation();
	bool isFloor()
	{
		return m_isFloor;
	}
	void setControlActive( bool active )
	{
		m_ctrlActive = active;
	}

	bool changed();
	void updateGui();

signals:
	void signalActionChanged( const QString action );
	void signalFirstClick( const QString firstClick );
	void signalSize( const QString size );
};
