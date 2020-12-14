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

enum class KeyboardMove : unsigned char
{
	None  = 0,
	Up    = 0x01,
	Down  = 0x02,
	Left  = 0x04,
	Right = 0x08
};

inline KeyboardMove operator+( KeyboardMove a, KeyboardMove b )
{
	return static_cast<KeyboardMove>( static_cast<unsigned char>( a ) | static_cast<unsigned char>( b ) );
}

inline KeyboardMove operator&( KeyboardMove a, KeyboardMove b )
{
	return static_cast<KeyboardMove>( static_cast<unsigned char>( a ) & static_cast<unsigned char>( b ) );
}

inline KeyboardMove operator-( KeyboardMove a )
{
	return static_cast<KeyboardMove>( ~static_cast<unsigned char>( a ) );
}

inline KeyboardMove& operator+=( KeyboardMove& a, KeyboardMove b )
{
	return a = a + b;
}

inline KeyboardMove& operator-=( KeyboardMove& a, KeyboardMove b )
{
	return a = a & -b;
}

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

	QTimer* m_timer = nullptr;
	QElapsedTimer m_keyboardMovementTimer;

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

	KeyboardMove m_keyboardMove = KeyboardMove::None;

public slots:
	void redraw();
	void idleRenderTick();
	void onFullScreen( bool value );
	void keyboardMove();

	void onSetWindowSize( int width, int height );

	void onInitViewAfterLoad();

signals:
	void signalWindowSize( int w, int h );
	void signalViewLevel( int level );
	

	void signalKeyPress( int key );
	void signalUpdateRenderOptions();
	void signalTogglePause();

	void signalRenderParams( int width, int height, int moveX, int moveY, float scale, int rotation );
	void signalRotateSelection();
	void signalMouse( int mouseX, int mouseY, bool shift, bool ctrl );
	void signalLeftClick( bool shift, bool ctrl );
	void signalRightClick();
};
