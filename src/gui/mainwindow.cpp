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
#include "../base/io.h"
#include "../gui/eventconnector.h"
#include "../gui/aggregatorselection.h"

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
#include "xaml/inventory.xaml.h"
#include "xaml/inventorymodel.h"
#include "xaml/selection.xaml.h"
#include "xaml/selectionmodel.h"


#include "xaml/military.xaml.h"
#include "xaml/militarymodel.h"
#include "xaml/neighbors.xaml.h"
#include "xaml/neighborsmodel.h"
#include "xaml/stockpilegui.xaml.h"
#include "xaml/workshopgui.xaml.h"
#include "xaml/workshopmodel.h"

#include "xaml/converters.h"

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

#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>

#include <string>

static MainWindow* instance;

static QSet<QString> m_noesisMessages;

MainWindow::MainWindow( QWidget* parent ) :
	QOpenGLWindow()
{
	qDebug() << "Create main window.";
	connect( Global::eventConnector, &EventConnector::signalExit, this, &MainWindow::onExit );
	connect( this, &MainWindow::signalWindowSize, Global::eventConnector, &EventConnector::onWindowSize );
	connect( this, &MainWindow::signalViewLevel, Global::eventConnector, &EventConnector::onViewLevel );
	connect( this, &MainWindow::signalKeyPress, Global::eventConnector, &EventConnector::onKeyPress );
	connect( this, &MainWindow::signalTogglePause, Global::eventConnector, &EventConnector::onTogglePause );
	connect( this, &MainWindow::signalUpdateRenderOptions, Global::eventConnector, &EventConnector::onUpdateRenderOptions );
		
	connect( this, &MainWindow::signalMouse, Global::eventConnector->aggregatorSelection(), &AggregatorSelection::onMouse, Qt::QueuedConnection );
	connect( this, &MainWindow::signalLeftClick, Global::eventConnector->aggregatorSelection(), &AggregatorSelection::onLeftClick, Qt::QueuedConnection );
	connect( this, &MainWindow::signalRightClick, Global::eventConnector->aggregatorSelection(), &AggregatorSelection::onRightClick, Qt::QueuedConnection );
	connect( this, &MainWindow::signalRotateSelection, Global::eventConnector->aggregatorSelection(), &AggregatorSelection::onRotateSelection, Qt::QueuedConnection );
	connect( this, &MainWindow::signalRenderParams, Global::eventConnector->aggregatorSelection(), &AggregatorSelection::onRenderParams, Qt::QueuedConnection );

	connect( Global::eventConnector->aggregatorDebug(), &AggregatorDebug::signalSetWindowSize, this, &MainWindow::onSetWindowSize, Qt::QueuedConnection );

	connect( Global::eventConnector->aggregatorSettings(), &AggregatorSettings::signalFullScreen, this, &MainWindow::onFullScreen, Qt::QueuedConnection );

	connect( Global::eventConnector, &EventConnector::signalInitView, this, &MainWindow::onInitViewAfterLoad, Qt::QueuedConnection );

	instance = this;
}

MainWindow::~MainWindow()
{
	qDebug() << "MainWindow destructor";

	if( !m_isFullScreen )
	{
		Global::cfg->set( "WindowWidth", this->width() );
		Global::cfg->set( "WindowHeight", this->height() );
		Global::cfg->set( "WindowPosX", this->position().x() );
		Global::cfg->set( "WindowPosY", this->position().y() );
	}
	
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
	m_isFullScreen = !m_isFullScreen;
	if ( m_isFullScreen )
	{
		w->showFullScreen();
		Global::cfg->set( "fullscreen", true );
	}
	else
	{
		// Reset from fullscreen:
		w->showNormal();
		Global::cfg->set( "fullscreen", false );
	}
	m_renderer->onRenderParamsChanged();
	emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
}

void MainWindow::onFullScreen( bool value )
{
	QOpenGLWindow* w = this;
	m_isFullScreen = value;
	Global::cfg->set( "fullscreen", value );
	if ( value )
	{
		w->showFullScreen();
	}
	else
	{
		// Reset from fullscreen:
		w->showNormal();
	}
	m_renderer->onRenderParamsChanged();
	emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
}

void MainWindow::keyPressEvent( QKeyEvent* event )
{
	int qtKey = event->key();
	auto noesisKey = Global::keyConvert( (Qt::Key)qtKey );
	//qDebug() << "keyPressEvent" << event->key() << " " << event->text() << noesisKey;

	bool ret = false;
	
	if( qtKey != 32 )
	{
		ret = m_view->KeyDown( noesisKey );
	}

	if ( event->key() > 31 && event->key() < 256 )
	{
		if ( event->text().size() == 1 )
		{
			QChar c = event->text().at( 0 );
			ret |= m_view->Char( c.unicode() );
		}
	}
	
	if ( ret )
	{
		idleRenderTick();
	}

	if ( !ret )
	{
		switch ( event->key() )
		{
			case Qt::Key_H:
				Global::wallsLowered = !Global::wallsLowered;
				emit signalUpdateRenderOptions();
				break;
			case Qt::Key_K:
				break;
			case Qt::Key_O:
				if ( event->modifiers() & Qt::ControlModifier )
				{
					Global::debugMode = !Global::debugMode;
				}
				else
				{
					Global::showDesignations = !Global::showDesignations;
					emit signalUpdateRenderOptions();
				}
				m_renderer->onRenderParamsChanged();
				break;
			case Qt::Key_F:
				//toggleFullScreen();
				break;
			case Qt::Key_R:
				if ( event->modifiers() & Qt::ControlModifier )
				{
					Global::debugOpenGL = !Global::debugOpenGL;
				}
				else
				{
					emit signalRotateSelection();
					redraw();
				}
				break;
			case Qt::Key_Comma:
				m_renderer->rotate( 1 );
				break;
			case Qt::Key_Period:
				m_renderer->rotate( -1 );
				break;
			case Qt::Key_Escape:
				emit signalKeyPress( event->key() );
				break;
			case Qt::Key_Space:
				emit signalTogglePause();
				break;
			case Qt::Key_W:
				keyboardMove();
				m_keyboardMove += KeyboardMove::Up;
				redraw();
				break;
			case Qt::Key_S:
				keyboardMove();
				m_keyboardMove += KeyboardMove::Down;
				redraw();
				break;
			case Qt::Key_A:
				keyboardMove();
				m_keyboardMove += KeyboardMove::Left;
				redraw();
				break;
			case Qt::Key_D:
				keyboardMove();
				m_keyboardMove += KeyboardMove::Right;
				redraw();
				break;
		}
		emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
		emit signalMouse( m_mouseX, m_mouseY, event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );
	}
}

void MainWindow::keyReleaseEvent( QKeyEvent* event )
{
	auto noesisKey = Global::keyConvert( (Qt::Key)event->key() );
	//qDebug() << "keyReleaseEvent" << event->key() << " " << event->text() << noesisKey;
	if ( m_view->KeyUp( noesisKey ) )
	{
		idleRenderTick();
	}

	switch ( event->key() )
	{
		case Qt::Key_W:
			keyboardMove();
			m_keyboardMove -= KeyboardMove::Up;
			redraw();
			break;
		case Qt::Key_S:
			keyboardMove();
			m_keyboardMove -= KeyboardMove::Down;
			redraw();
			break;
		case Qt::Key_A:
			keyboardMove();
			m_keyboardMove -= KeyboardMove::Left;
			redraw();
			break;
		case Qt::Key_D:
			keyboardMove();
			m_keyboardMove -= KeyboardMove::Right;
			redraw();
			break;
	}
	emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
	emit signalMouse( m_mouseX, m_mouseY, event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );
}

bool MainWindow::isOverGui( int x, int y )
{
	auto root     = Noesis::VisualTreeHelper::GetRoot( m_view->GetContent() );
	auto mousePos = Noesis::Point( x, y );
	auto hit      = Noesis::VisualTreeHelper::HitTest( root, mousePos );

	return hit.visualHit;
}

void MainWindow::keyboardMove()
{
	int x = 0;
	int y = 0;
	if( (bool)( m_keyboardMove & KeyboardMove::Up ) ) y -= 1;
	if( (bool)( m_keyboardMove & KeyboardMove::Down ) ) y += 1;
	if( (bool)( m_keyboardMove & KeyboardMove::Left ) ) x -= 1;
	if( (bool)( m_keyboardMove & KeyboardMove::Right ) ) x += 1;

	// Elapsed time in second
	const float elapsedTime = m_keyboardMovementTimer.nsecsElapsed() * 0.000000001f;
	m_keyboardMovementTimer.restart();

	if( m_renderer && (x || y) )
	{
		const float keyboardMoveSpeed = (Global::cfg->get( "keyboardMoveSpeed" ).toFloat() + 50.f) * 4.f;

		float moveX = -x * keyboardMoveSpeed * elapsedTime;
		float moveY = -y * keyboardMoveSpeed * elapsedTime;

		if (x && y)
		{
			moveX /= sqrt( 2 );
			moveY /= sqrt( 2 );
		}

		m_renderer->move( moveX, moveY );
		emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
	}
}

void MainWindow::mouseMoveEvent( QMouseEvent* event )
{
	auto gp = this->mapFromGlobal( event->globalPos() );
	if ( m_view )
	{
		// Mouse movement needs to be handled by both Qt and Noesis, to correctly handle ongoing mouse gestures
		if ( m_view->MouseMove( gp.x(), gp.y() ) )
		{
			idleRenderTick();
		}

		if ( event->buttons() & Qt::LeftButton && m_leftDown )
		{
			if ( ( abs( gp.x() - m_clickX ) > 5 || abs( gp.y() - m_clickY ) > 5 ) && !m_isMove )
			{
				m_isMove = true;
				m_moveX  = m_clickX;
				m_moveY  = m_clickY;
			}

			if ( m_isMove )
			{
				m_renderer->move( gp.x() - m_moveX, gp.y() - m_moveY );

				m_moveX = gp.x();
				m_moveY = gp.y();
			}
			emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
		}
		else
		{
			//qDebug() << "huhu";
		}
	}

	m_mouseX  = gp.x();
	m_mouseY  = gp.y();


	emit signalMouse( m_mouseX, m_mouseY, event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );

	redraw();
}

void MainWindow::onInitViewAfterLoad()
{
	m_renderer->setScale( 1.0 );
	m_moveX = GameState::moveX;
	m_moveY = GameState::moveY;
	m_renderer->move( m_moveX, m_moveY );
	
	m_renderer->setScale( GameState::scale );
	emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
}

void MainWindow::mousePressEvent( QMouseEvent* event )
{
	//qDebug() << "mousePressEvent";
	auto gp = this->mapFromGlobal( event->globalPos() );
	if ( event->button() & Qt::LeftButton )
	{
		if ( m_view )
		{
			if ( isOverGui( gp.x(), gp.y() ) )
			{
				if ( m_view->MouseButtonDown( gp.x(), gp.y(), Noesis::MouseButton::MouseButton_Left ) )
				{
					idleRenderTick();
				}
			}
			else
			{
				m_clickX   = gp.x();
				m_clickY   = gp.y();
				m_isMove   = false;
				m_leftDown = true;
			}
		}
	}
	else if ( event->button() & Qt::RightButton )
	{
		if ( m_view )
		{
			if ( isOverGui( gp.x(), gp.y() ) )
			{
				if ( m_view->MouseButtonDown( gp.x(), gp.y(), Noesis::MouseButton::MouseButton_Right ) )
				{
					idleRenderTick();
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
			auto gp = this->mapFromGlobal( event->globalPos() );
			if ( isOverGui( gp.x(), gp.y() ) || !m_leftDown )
			{
				if ( m_view->MouseButtonUp( gp.x(), gp.y(), Noesis::MouseButton::MouseButton_Left ) )
				{
					idleRenderTick();
				}
			}
			else
			{
				if ( !m_isMove && m_leftDown )
				{
					emit signalMouse( m_mouseX, m_mouseY, event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );
					emit signalLeftClick( event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );
					redraw();
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
			auto gp = this->mapFromGlobal( event->globalPos() );
			if ( isOverGui( gp.x(), gp.y() ) || !m_rightDown )
			{
				if ( m_view->MouseButtonUp( gp.x(), gp.y(), Noesis::MouseButton::MouseButton_Right ) )
				{
					idleRenderTick();
				}
			}
			else
			{
				emit signalMouse( m_mouseX, m_mouseY, event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );
				emit signalRightClick();
				redraw();
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
		auto gp = this->mapFromGlobal( event->globalPos() );
		if ( isOverGui( gp.x(), gp.y() ) )
		{
			if ( m_view->MouseWheel( gp.x(), gp.y(), wEvent->delta() ) )
			{
				idleRenderTick();
			}
		}
		else
		{
			if ( (bool)( wEvent->modifiers() & Qt::ControlModifier ) ^ Global::cfg->get( "toggleMouseWheel" ).toBool() ) 
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
		emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
	}
}

void MainWindow::focusInEvent( QFocusEvent* e )
{
	if ( m_view )
	{
		m_view->Activate();
		idleRenderTick();
	}
}

void MainWindow::focusOutEvent( QFocusEvent* e )
{
	if ( m_view )
	{
		m_view->Deactivate();
		idleRenderTick();
	}
}

void MainWindow::keyboardZPlus( bool shift, bool ctrl )
{
	int dimZ      = Global::dimZ - 1;
	GameState::viewLevel += 1;
	GameState::viewLevel = qMax( 0, qMin( dimZ, GameState::viewLevel ) );

	m_renderer->onRenderParamsChanged();
	emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
	emit signalViewLevel( GameState::viewLevel );

	emit signalMouse( m_mouseX, m_mouseY, shift, ctrl );
	redraw();
}

void MainWindow::keyboardZMinus( bool shift, bool ctrl )
{
	int dimZ      = Global::dimZ - 1;
	GameState::viewLevel -= 1;
	GameState::viewLevel = qMax( 0, qMin( dimZ, GameState::viewLevel ) );

	m_renderer->onRenderParamsChanged();
	emit signalViewLevel( GameState::viewLevel );
	emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
	emit signalMouse( m_mouseX, m_mouseY, shift, ctrl );
	redraw();
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
	Noesis::GUI::DisableInspector();

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
		return true;
	}
	return false;
}

void MainWindow::idleRenderTick()
{
	// Check for ongoing keyboard movement
	keyboardMove();

	// Check if redraw is required
	if ( noesisUpdate() && !m_pendingUpdate )
	{
		// Trigger rendering
		m_pendingUpdate = true;
		update();
	}
	else
	{
		// check again later
		m_timer->start( 20 );
	}
}

void MainWindow::paintGL()
{
	// Apply latest position
	keyboardMove();

	// Get the GPU busy
	m_renderer->paintWorld();

	// Trigger noesis updates again, to avoid "stuttering UI"
	noesisUpdate();

	// Offscreen rendering phase populates textures needed by the on-screen rendering
	// If necessary, actually update
	m_view->GetRenderer()->UpdateRenderTree();

	// If you are going to render here with your own engine you need to restore the GPU state
	// because noesis changes it. In this case only framebuffer and viewport need to be restored
	if ( m_view->GetRenderer()->RenderOffscreen() )
	{
		// Restore state managed by QOpenGLWindow
		makeCurrent();
		context()->functions()->glViewport( 0, 0, this->width() * devicePixelRatioF(), this->height() * devicePixelRatioF() );
	}

	// Rendering is done in the active framebuffer
	m_view->GetRenderer()->Render();

	m_timer->start( 0 );
	m_pendingUpdate = false;
}

void MainWindow::resizeGL( int w, int h )
{
	QOpenGLWindow::resizeGL( w, h );

	if( !m_isFullScreen )
	{
		Global::cfg->set( "WindowWidth", w );
		Global::cfg->set( "WindowHeight", h );
	}

	if ( m_view )
	{
		m_view->SetSize( this->width(), this->height() );
	}
	m_renderer->resize( this->width(), this->height() );

	context()->functions()->glViewport( 0, 0, this->width() * devicePixelRatioF(), this->height() * devicePixelRatioF() );

	emit signalWindowSize( this->width(), this->height() );

	update();
}

void MainWindow::onSetWindowSize( int width, int height )
{
	this->resize( width, height );
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

	m_renderer = new MainWindowRenderer( this );
	m_renderer->initializeGL();

	noesisInit();
	m_timer = new QTimer( this );
	connect( m_timer, &QTimer::timeout, this, &MainWindow::idleRenderTick );

	update();
}

MainWindowRenderer* MainWindow::renderer()
{
	return m_renderer;
}

void MainWindow::installResourceProviders()
{
	const std::string contentPath = Global::cfg->get( "dataPath" ).toString().toStdString() + "/xaml/";
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
	Noesis::RegisterComponent<IngnomiaGUI::InventoryGui>();
	Noesis::RegisterComponent<IngnomiaGUI::InventoryModel>();
	Noesis::RegisterComponent<IngnomiaGUI::SelectionGui>();
	Noesis::RegisterComponent<IngnomiaGUI::SelectionModel>();

	Noesis::RegisterComponent<Noesis::EnumConverter<IngnomiaGUI::State>>();
	Noesis::RegisterComponent<IngnomiaGUI::ColorToBrushConverter>();
	Noesis::RegisterComponent<IngnomiaGUI::ColorToBrushConverterDark>();

	Noesis::RegisterComponent<IngnomiaGUI::GameGui>();
}
