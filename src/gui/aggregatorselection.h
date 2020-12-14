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

#include "aggregatorrenderer.h"

#include "../base/position.h"

#include <QObject>

class AggregatorSelection : public QObject
{
	Q_OBJECT

public:
	AggregatorSelection( QObject* parent = nullptr );
	~AggregatorSelection();

private:
	Position calcCursor( int mouseX, int mouseY, bool isFloor, bool useViewLevel ) const;
    void updateSelection();
    unsigned int posToInt( Position pos, quint8 rotation );

    int m_width = 0;
    int m_height = 0;
    int m_moveX = 0;
    int m_moveY = 0;
    float m_scale = 1.0;
    int m_rotation = 0;

    Position m_cursorPos;

    QMap<unsigned int, SelectionData> m_selectionData;

public slots:
    void onActionChanged( const QString action );
    void onUpdateCursorPos( const QString pos );
    void onUpdateFirstClick( const QString pos );
    void onUpdateSize( const QString size );

    void onRenderParams( int width, int height, int moveX, int moveY, float scale, int rotation );
    void onMouse( int mouseX, int mouseY, bool shift, bool ctrl );
    void onLeftClick( bool shift, bool ctrl );
    void onRightClick();
    void onRotateSelection();

signals:
    void signalAction( const QString action );
    void signalCursorPos( const QString pos );
    void signalFirstClick( const QString pos );
    void signalSize( const QString size );

    void signalSelectTile( unsigned int );

    void signalUpdateSelection( const QMap<unsigned int, SelectionData>& data, bool noDepthTest );
};
