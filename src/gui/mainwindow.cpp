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

#include "mainwindow.h"

#include "../base/config.h"
#include "../base/global.h"
#include "../base/io.h"
#include "../base/selection.h"
#include "../base/tile.h"
#include "../game/gamemanager.h"
#include "../game/world.h"
#include "../gui/eventconnector.h"
#include "license.h"
#include "mainwindowrenderer.h"
#include "xaml/GameGui.xaml.h"
#include "xaml/GameModel.h"
#include "xaml/IngamePage.xaml.h"
#include "xaml/LoadGameModel.h"
#include "xaml/LoadGamePage.xaml.h"
#include "xaml/Main.xaml.h"
#include "xaml/MainMenu.xaml.h"
#include "xaml/MainPage.xaml.h"
#include "xaml/NewGameModel.h"
#include "xaml/NewGamePage.xaml.h"
#include "xaml/Population.xaml.h"
#include "xaml/PopulationModel.h"
#include "xaml/SettingsModel.h"
#include "xaml/SettingsPage.xaml.h"
#include "xaml/StockpileModel.h"
#include "xaml/TileInfo.xaml.h"
#include "xaml/TileInfoModel.h"
#include "xaml/ViewModel.h"
#include "xaml/WaitPage.xaml.h"
#include "xaml/agriculture.xaml.h"
#include "xaml/agriculturemodel.h"
#include "xaml/creatureinfo.xaml.h"
#include "xaml/creatureinfomodel.h"
#include "xaml/debug.xaml.h"
#include "xaml/debugmodel.h"
#include "xaml/military.xaml.h"
#include "xaml/militarymodel.h"
#include "xaml/neighbors.xaml.h"
#include "xaml/neighborsmodel.h"
#include "xaml/stockpilegui.xaml.h"
#include "xaml/workshopgui.xaml.h"
#include "xaml/workshopmodel.h"

#include <NsApp/Launcher.h>
#include <NsApp/LocalFontProvider.h>
#include <NsApp/LocalTextureProvider.h>
#include <NsApp/LocalXamlProvider.h>
#include <NsCore/EnumConverter.h>
#include <NsCore/RegisterComponent.h>
#include <NsGui/Grid.h>
#include <NsGui/IRenderer.h>
#include <NsGui/IntegrationAPI.h>
#include <NsRender/GLFactory.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QFile>
#include <QImage>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTimer>

#include <string>

static MainWindow* instance;

static QSet<QString> m_noesisMessages;

MainWindow::MainWindow( QWidget* parent ) :
	QOpenGLWindow()
{
	qDebug() << "Create main window.";
	connect( &EventConnector::getInstance(), &EventConnector::signalExit, this, &MainWindow::onExit );
	connect( this, &MainWindow::signalWindowSize, &EventConnector::getInstance(), &EventConnector::onWindowSize );
	connect( this, &MainWindow::signalViewLevel, &EventConnector::getInstance(), &EventConnector::onViewLevel );
	connect( this, &MainWindow::signalSelectTile, EventConnector::getInstance().aggregatorTileInfo(), &AggregatorTileInfo::onShowTileInfo );
	instance = this;
}

MainWindow::~MainWindow()
{
	qDebug() << "MainWindow destructor";

	Config::getInstance().set( "WindowWidth", this->width() );
	Config::getInstance().set( "WindowHeight", this->height() );
	Config::getInstance().set( "WindowPosX", this->position().x() );
	Config::getInstance().set( "WindowPosY", this->position().y() );

	IO::saveConfig();
	instance = nullptr;
}

MainWindow& MainWindow::getInstance()
{
	return *instance;
}

void MainWindow::onExit()
{
	this->close();
}

void MainWindow::toggleFullScreen()
{
	QOpenGLWindow* w = this;
	if ( !m_isFullScreen )
	{
		qDebug() << "go to fullscreen";
		w->showFullScreen();
		m_isFullScreen = true;
	}
	else
	{
		// Reset from fullscreen:
		qDebug() << "back from fullscreen";
		w->showNormal();
		m_isFullScreen = false;
	}
	m_renderer->onRenderParamsChanged();
}

void MainWindow::keyPressEvent( QKeyEvent* event )
{
	auto noesisKey = Global::keyConvert( (Qt::Key)event->key() );
	//qDebug() << "keyPressEvent" << event->key() << " " << event->text() << noesisKey;

	bool ret = m_view->KeyDown( noesisKey );

	if ( event->key() > 32 && event->key() < 256 )
	{
		if ( event->text().size() == 1 )
		{
			QChar c = event->text().at( 0 );
			ret |= m_view->Char( c.unicode() );
		}
	}

	if ( ret )
	{
		noesisTick();
	}

	if ( !ret )
	{
		switch ( event->key() )
		{
			case Qt::Key_H:
				Global::wallsLowered = !Global::wallsLowered;
				m_renderer->onRenderParamsChanged();
				break;
			case Qt::Key_K:
				break;
			case Qt::Key_D:
				if ( event->modifiers() & Qt::ControlModifier )
				{
					Global::debugMode = !Global::debugMode;
				}
				else
				{
					auto& config = Config::getInstance();
					config.set( "overlay", !config.get( "overlay" ).toBool() );
				}
				m_renderer->onRenderParamsChanged();
				break;
			case Qt::Key_F:
				toggleFullScreen();
				break;
			case Qt::Key_R:
				Selection::getInstance().rotate();
				Selection::getInstance().updateSelection( m_cursorPos, false, false );
				redraw();
				break;
			case Qt::Key_Comma:
				m_renderer->rotate( 1 );
				break;
			case Qt::Key_Period:
				m_renderer->rotate( -1 );
				break;
		}
	}
}

void MainWindow::keyReleaseEvent( QKeyEvent* event )
{
	auto noesisKey = Global::keyConvert( (Qt::Key)event->key() );
	//qDebug() << "keyReleaseEvent" << event->key() << " " << event->text() << noesisKey;
	if ( m_view->KeyUp( noesisKey ) )
	{
		noesisTick();
	}
}

bool MainWindow::isOverGui( int x, int y )
{
	auto root     = Noesis::VisualTreeHelper::GetRoot( m_view->GetContent() );
	auto mousePos = Noesis::Point( x, y );
	auto hit      = Noesis::VisualTreeHelper::HitTest( root, mousePos );

	return hit.visualHit;
}

void MainWindow::mouseMoveEvent( QMouseEvent* event )
{
	if ( m_view )
	{
		// Mouse movement needs to be handled by both Qt and Noesis, to correctly handle ongoing mouse gestures
		if ( m_view->MouseMove( event->x(), event->y() ) )
		{
			noesisTick();
		}

		if ( event->buttons() & Qt::LeftButton && m_leftDown )
		{
			if ( ( abs( event->x() - m_clickX ) > 5 || abs( event->y() - m_clickY ) > 5 ) && !m_isMove )
			{
				m_isMove = true;
				m_moveX  = m_clickX;
				m_moveY  = m_clickY;
			}

			if ( m_isMove )
			{
				m_renderer->move( event->x() - m_moveX, event->y() - m_moveY );

				m_moveX = event->x();
				m_moveY = event->y();
			}
		}
		else
		{
			//qDebug() << "huhu";
		}
	}

	QPoint gp = this->mapFromGlobal( event->globalPos() );
	m_mouseX  = gp.x();
	m_mouseY  = gp.y();

	if ( Selection::getInstance().hasAction() )
	{
		m_cursorPos = m_renderer->calcCursor( m_mouseX, m_mouseY, event->modifiers() & Qt::ShiftModifier );
		Selection::getInstance().updateSelection( m_cursorPos, event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );
		Selection::getInstance().setControlActive( event->modifiers() & Qt::ControlModifier );
		redraw();
	}
}

void MainWindow::mousePressEvent( QMouseEvent* event )
{
	//qDebug() << "mousePressEvent";
	if ( event->button() & Qt::LeftButton )
	{
		if ( m_view )
		{
			if ( isOverGui( event->x(), event->y() ) )
			{
				if ( m_view->MouseButtonDown( event->x(), event->y(), Noesis::MouseButton::MouseButton_Left ) )
				{
					noesisTick();
				}
			}
			else
			{
				m_clickX   = event->x();
				m_clickY   = event->y();
				m_isMove   = false;
				m_leftDown = true;
			}
		}
	}
	else if ( event->button() & Qt::RightButton )
	{
		if ( m_view )
		{
			if ( isOverGui( event->x(), event->y() ) )
			{
				if ( m_view->MouseButtonDown( event->x(), event->y(), Noesis::MouseButton::MouseButton_Right ) )
				{
					noesisTick();
				}
			}
			else
			{
				m_rightDown = true;
			}
		}
	}
}

void MainWindow::mouseReleaseEvent( QMouseEvent* event )
{
	//qDebug() << "mouseReleaseEvent";
	if ( event->button() & Qt::LeftButton )
	{
		if ( m_view )
		{
			if ( isOverGui( event->x(), event->y() ) || !m_leftDown )
			{
				if ( m_view->MouseButtonUp( event->x(), event->y(), Noesis::MouseButton::MouseButton_Left ) )
				{
					noesisTick();
				}
			}
			else
			{
				if ( !m_isMove && m_leftDown )
				{
					m_cursorPos = m_renderer->calcCursor( m_mouseX, m_mouseY, event->modifiers() & Qt::ShiftModifier );
					if ( Selection::getInstance().hasAction() )
					{
						if ( Selection::getInstance().leftClick( m_cursorPos, event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier ) )
						{
							// open info windows after creating something
						}
						redraw();
					}
					else
					{
						//open tile info or do other game related stuff
						if ( m_cursorPos.x < 0 || m_cursorPos.y < 0 || m_cursorPos.x > Global::dimX - 1 || m_cursorPos.y > Global::dimX - 1 || m_cursorPos.z < 0 || m_cursorPos.z > Global::dimZ - 1 )
						{
							//return;
						}
						unsigned int tileID = m_cursorPos.toInt();
						emit signalSelectTile( tileID );
					}
				}
			}
			m_isMove   = false;
			m_leftDown = false;
		}
	}
	else if ( event->button() & Qt::RightButton )
	{
		if ( m_view )
		{
			if ( isOverGui( event->x(), event->y() ) || !m_rightDown )
			{
				if ( m_view->MouseButtonUp( event->x(), event->y(), Noesis::MouseButton::MouseButton_Right ) )
				{
					noesisTick();
				}
			}
			else
			{
				if ( Selection::getInstance().hasAction() )
				{
					m_cursorPos = m_renderer->calcCursor( m_mouseX, m_mouseY, event->modifiers() & Qt::ShiftModifier );
					Selection::getInstance().rightClick( m_cursorPos );
					m_selectedAction = Selection::getInstance().action();
					redraw();
				}
				m_rightDown = false;
			}
		}
	}
}

void MainWindow::wheelEvent( QWheelEvent* event )
{
	QWheelEvent* wEvent = event; //dynamic_cast<QWheelEvent*>( event );
	if ( m_view )
	{
		if ( isOverGui( event->x(), event->y() ) )
		{
			if ( m_view->MouseWheel( event->x(), event->y(), wEvent->delta() ) )
			{
				noesisTick();
			}
		}
		else
		{
			if ( wEvent->modifiers() & Qt::ControlModifier )
			{
				// Scale the view / do the zoom
				auto delta = wEvent->delta();
				m_renderer->scale( pow( 1.002, delta ) );
			}
			else
			{
				if ( wEvent->delta() > 0 )
				{
					keyboardZPlus( event->modifiers() & Qt::ShiftModifier );
				}
				else
				{
					keyboardZMinus( event->modifiers() & Qt::ShiftModifier );
				}
			}
		}
	}
}

void MainWindow::focusInEvent( QFocusEvent* e )
{
	if ( m_view )
	{
		m_view->Activate();
		noesisTick();
	}
}

void MainWindow::focusOutEvent( QFocusEvent* e )
{
	if ( m_view )
	{
		m_view->Deactivate();
		noesisTick();
	}
}

void MainWindow::keyboardZPlus( bool shift, bool ctrl )
{
	int dimZ      = Global::dimZ - 1;
	int viewLevel = Config::getInstance().get( "viewLevel" ).toInt();
	viewLevel += 1;
	viewLevel = qMax( 0, qMin( dimZ, viewLevel ) );
	Config::getInstance().set( "viewLevel", viewLevel );

	m_renderer->onRenderParamsChanged();
	emit signalViewLevel( viewLevel );

	if ( Selection::getInstance().hasAction() )
	{
		m_cursorPos = m_renderer->calcCursor( m_mouseX, m_mouseY, shift );
		Selection::getInstance().updateSelection( m_cursorPos, shift, ctrl );
		Selection::getInstance().setControlActive( ctrl );
		redraw();
	}
}

void MainWindow::keyboardZMinus( bool shift, bool ctrl )
{
	int dimZ      = Global::dimZ - 1;
	int viewLevel = Config::getInstance().get( "viewLevel" ).toInt();
	viewLevel -= 1;
	viewLevel = qMax( 0, qMin( dimZ, viewLevel ) );
	Config::getInstance().set( "viewLevel", viewLevel );

	m_renderer->onRenderParamsChanged();
	emit signalViewLevel( viewLevel );

	if ( Selection::getInstance().hasAction() )
	{
		m_cursorPos = m_renderer->calcCursor( m_mouseX, m_mouseY, shift );
		Selection::getInstance().updateSelection( m_cursorPos, shift, ctrl );
		Selection::getInstance().setControlActive( ctrl );
		redraw();
	}
}

void MainWindow::noesisInit()
{
	qDebug() << "noesisInit()";
	Noesis::LogHandler logHandler = []( const char*, uint32_t, uint32_t level, const char*, const char* message ) {
		// [TRACE] [DEBUG] [INFO] [WARNING] [ERROR]
		const char* prefixes[] = { "T", "D", "I", "W", "E" };
		//printf("[NOESIS/%s] %s\n", prefixes[level], message);
		if ( m_noesisMessages.contains( message ) )
		{
			return;
		}
		m_noesisMessages.insert( message );

		qDebug() << "[NOESIS]" << prefixes[level] << message;
	};

	Noesis::ErrorHandler errorHandler = []( const char* file, uint32_t line, const char* message, bool fatal ) {
		qDebug() << "[NOESIS]" << message << " in " << file << ":" << line;
		if ( fatal )
		{
			abort();
		}
	};

	Noesis::GUI::SetLogHandler( logHandler );
	Noesis::GUI::SetErrorHandler( errorHandler );

	// Noesis initialization. This must be the first step before using any NoesisGUI functionality
	Noesis::GUI::Init( licenseName, licenseKey );

	installResourceProviders();

	registerComponents();

	Noesis::Ptr<Noesis::FrameworkElement> xaml = Noesis::GUI::LoadXaml<Noesis::FrameworkElement>( "Main.xaml" );

	// View creation to render and interact with the user interface
	// We transfer the ownership to a global pointer instead of a Ptr<> because there is no way
	// in GLUT to do shutdown and we don't want the Ptr<> to be released at global time
	m_view = Noesis::GUI::CreateView( xaml ).GiveOwnership();

	//m_view->SetIsPPAAEnabled( true );

	// Renderer initialization with an OpenGL device
	m_view->GetRenderer()->Init( NoesisApp::GLFactory::CreateDevice( false ) );

	m_view->SetSize( this->width(), this->height() );

	emit signalWindowSize( this->width(), this->height() );
}

bool MainWindow::noesisUpdate()
{
	// Update view (layout, animations, ...)
	static std::chrono::system_clock::time_point appStart = std::chrono::system_clock::now();

	auto timeDiff = std::chrono::duration_cast<std::chrono::duration<double>>( std::chrono::system_clock::now() - appStart );

	// See if anything needs to be animated
	if ( m_view->Update( timeDiff.count() ) )
	{
		// If necessary, actually update
		if ( m_view->GetRenderer()->UpdateRenderTree() )
		{
			return true;
		}
	}
	return false;
}

void MainWindow::noesisTick()
{

	if ( noesisUpdate() && !m_pendingUpdate )
	{
		// Trigger rendering
		m_pendingUpdate = true;
		update();
	}
	else
	{
		// check again later
		m_timer->start( 50 );
	}
}

void MainWindow::paintGL()
{
	// Trigger noesis updates again, to avoid "stuttering UI"
	noesisUpdate();

	// Offscreen rendering phase populates textures needed by the on-screen rendering

	// If you are going to render here with your own engine you need to restore the GPU state
	// because noesis changes it. In this case only framebuffer and viewport need to be restored
	if ( m_view->GetRenderer()->RenderOffscreen() )
	{
		// Restore state managed by QOpenGLWindow
		makeCurrent();
	}

	m_renderer->paintWorld();

	// Rendering is done in the active framebuffer
	m_view->GetRenderer()->Render();

	m_timer->start( 50 );
	m_pendingUpdate = false;
}

void MainWindow::resizeGL( int w, int h )
{
	QOpenGLWindow::resizeGL( w, h );

	Config::getInstance().set( "WindowWidth", w );
	Config::getInstance().set( "WindowHeight", h );

	if ( m_view )
	{
		m_view->SetSize( this->width(), this->height() );
	}
	m_renderer->resize( this->width(), this->height() );

	emit signalWindowSize( this->width(), this->height() );

	update();
}

void MainWindow::redraw()
{
	if ( !m_pendingUpdate )
	{
		// Trigger rendering
		m_pendingUpdate = true;
		update();
	}
}

void MainWindow::initializeGL()
{
	QOpenGLWindow::initializeGL();

	noesisInit();
	m_timer = new QTimer( this );
	connect( m_timer, &QTimer::timeout, this, &MainWindow::noesisTick );

	m_renderer = new MainWindowRenderer( this );
	m_renderer->initializeGL();

	update();
}

MainWindowRenderer* MainWindow::renderer()
{
	return m_renderer;
}

void MainWindow::installResourceProviders()
{
	const std::string contentPath = Config::getInstance().get( "dataPath" ).toString().toStdString() + "/xaml/";
	Noesis::GUI::SetXamlProvider( Noesis::MakePtr<NoesisApp::LocalXamlProvider>( contentPath.c_str() ) );
	Noesis::GUI::SetTextureProvider( Noesis::MakePtr<NoesisApp::LocalTextureProvider>( contentPath.c_str() ) );
	Noesis::GUI::SetFontProvider( Noesis::MakePtr<NoesisApp::LocalFontProvider>( contentPath.c_str() ) );
}

void MainWindow::registerComponents()
{
	NoesisApp::Launcher::RegisterAppComponents();

	Noesis::RegisterComponent<IngnomiaGUI::Main>();
	Noesis::RegisterComponent<IngnomiaGUI::ViewModel>();

	Noesis::RegisterComponent<IngnomiaGUI::MainMenu>();
	Noesis::RegisterComponent<IngnomiaGUI::MainPage>();
	Noesis::RegisterComponent<IngnomiaGUI::SettingsPage>();
	Noesis::RegisterComponent<IngnomiaGUI::SettingsModel>();
	Noesis::RegisterComponent<IngnomiaGUI::LoadGamePage>();
	Noesis::RegisterComponent<IngnomiaGUI::LoadGameModel>();
	Noesis::RegisterComponent<IngnomiaGUI::NewGamePage>();
	Noesis::RegisterComponent<IngnomiaGUI::NewGameModel>();
	Noesis::RegisterComponent<IngnomiaGUI::WaitPage>();
	Noesis::RegisterComponent<IngnomiaGUI::IngamePage>();
	Noesis::RegisterComponent<IngnomiaGUI::GameModel>();
	Noesis::RegisterComponent<IngnomiaGUI::TileInfo>();
	Noesis::RegisterComponent<IngnomiaGUI::TileInfoModel>();
	Noesis::RegisterComponent<IngnomiaGUI::StockpileGui>();
	Noesis::RegisterComponent<IngnomiaGUI::StockpileModel>();
	Noesis::RegisterComponent<IngnomiaGUI::WorkshopGui>();
	Noesis::RegisterComponent<IngnomiaGUI::WorkshopModel>();
	Noesis::RegisterComponent<IngnomiaGUI::Agriculture>();
	Noesis::RegisterComponent<IngnomiaGUI::AgricultureModel>();
	Noesis::RegisterComponent<IngnomiaGUI::PopulationWindow>();
	Noesis::RegisterComponent<IngnomiaGUI::PopulationModel>();
	Noesis::RegisterComponent<IngnomiaGUI::CreatureInfo>();
	Noesis::RegisterComponent<IngnomiaGUI::CreatureInfoModel>();
	Noesis::RegisterComponent<IngnomiaGUI::DebugGui>();
	Noesis::RegisterComponent<IngnomiaGUI::DebugModel>();
	Noesis::RegisterComponent<IngnomiaGUI::NeighborsGui>();
	Noesis::RegisterComponent<IngnomiaGUI::NeighborsModel>();
	Noesis::RegisterComponent<IngnomiaGUI::MilitaryGui>();
	Noesis::RegisterComponent<IngnomiaGUI::MilitaryModel>();

	Noesis::RegisterComponent<Noesis::EnumConverter<IngnomiaGUI::State>>();

	Noesis::RegisterComponent<IngnomiaGUI::GameGui>();
}
