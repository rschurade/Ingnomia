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
#include "../base/tile.h"

#include <NsGui/IView.h>

#include <QElapsedTimer>
#include <QOpenGLWindow>
#include <QTimer>

struct Position;
class QOpenGLTexture;
class MainWindowRenderer;

class MainWindow : public QOpenGLWindow
{
	Q_OBJECT

public:
	MainWindow( QWidget* parent = Q_NULLPTR );
	~MainWindow();
	static MainWindow& getInstance();

	bool noesisUpdate();

	MainWindowRenderer* renderer();

protected:
	void paintGL() override;
	void resizeGL( int w, int h ) override;
	void initializeGL() override;

	void keyPressEvent( QKeyEvent* event ) override;
	void keyReleaseEvent( QKeyEvent* event ) override;
	void mouseMoveEvent( QMouseEvent* event ) override;
	void mousePressEvent( QMouseEvent* event ) override;
	void mouseReleaseEvent( QMouseEvent* event ) override;
	void wheelEvent( QWheelEvent* event ) override;
	void focusInEvent( QFocusEvent* e ) override;
	void focusOutEvent( QFocusEvent* e ) override;

private:
	void noesisInit();

	void keyboardZPlus( bool shift = false, bool ctrl = false );
	void keyboardZMinus( bool shift = false, bool ctrl = false );
	bool isOverGui( int x, int y );

	void installResourceProviders();
	void registerComponents();

	void toggleFullScreen();
	bool m_isFullScreen = false;

	void onExit();

	Position m_cursorPos;
	QString m_selectedAction = "";

	QTimer* m_timer = nullptr;

	Noesis::IView* m_view          = nullptr;
	MainWindowRenderer* m_renderer = nullptr;

	// Global position of last mouse click
	int m_clickX = 0;
	int m_clickY = 0;
	// Local position of last mouse click
	int m_mouseX = 0;
	int m_mouseY = 0;
	// Global position of last mouse event during drag
	int m_moveX = 0;
	int m_moveY = 0;

	bool m_leftDown  = false;
	bool m_rightDown = false;
	bool m_isMove    = false;

	bool m_pendingUpdate = false;

public slots:
	void redraw();
	void noesisTick();
signals:
	void signalWindowSize( int w, int h );
	void signalViewLevel( int level );
	void signalSelectTile( unsigned int );
};
