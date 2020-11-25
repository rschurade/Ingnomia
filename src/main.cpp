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
#include "base/config.h"
#include "base/db.h"
#include "gui/mainwindow.h"
#include "gui/strings.h"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDesktopWidget>
#include <QDir>
#include <QFileIconProvider>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QWindow>
#include <QtWidgets/QApplication>

#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
#include "version.h"

QTextStream* out = 0;
bool verbose     = true;

void clearLog()
{
	QString folder   = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/";
	bool ok          = true;
	QString fileName = "log.txt";
	if ( QDir( folder ).exists() )
	{
		fileName = folder + fileName;
	}

	QFile file( fileName );
	file.open( QIODevice::WriteOnly );
	file.close();
}

QPointer<QFile> openLog()
{
	QString folder   = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/";
	bool ok          = true;
	QString fileName = "log.txt";
	if ( QDir( folder ).exists() )
	{
		fileName = folder + fileName;
	}

	QPointer<QFile> outFile(new QFile( fileName ));
	if(outFile->open( QIODevice::WriteOnly | QIODevice::Append ))
	{
		return outFile;
	}
	else
	{
		return nullptr;
	}
}

void logOutput( QtMsgType type, const QMessageLogContext& context, const QString& message )
{
	if ( message.startsWith( "libpng warning:" ) )
		return;

	QString filedate  = QDateTime::currentDateTime().toString( "yyyy.MM.dd hh:mm:ss:zzz" );
	QString debugdate = QDateTime::currentDateTime().toString( "hh:mm:ss:zzz" );

	switch ( type )
	{
		case QtDebugMsg:
			debugdate += " [D]";
			break;
		case QtWarningMsg:
			debugdate += " [W]";
			break;
		case QtCriticalMsg:
			debugdate += " [C]";
			break;
		case QtFatalMsg:
			debugdate += " [F]";
			break;
	}
	if ( verbose )
	{
		//std::cout << OutputDebugStringA( debugdate.toStdString() ) << " " << message.toStdString() << endl;
		QString text    = debugdate + " " + message + "\n";
		std::string str = text.toStdString();

#ifdef _WIN32
		OutputDebugStringA( str.c_str() );
#else
		std::cerr << str;
#endif
	}
	static QPointer<QFile> outFile = openLog();

	if ( outFile )
	{
		QTextStream ts( outFile );
		ts << filedate << " " << message << endl;
	}
}

void noOutput( QtMsgType type, const QMessageLogContext& context, const QString& message )
{
}

int main( int argc, char* argv[] )
{
	clearLog();
	qInstallMessageHandler( &logOutput );
	qInfo() << PROJECT_NAME << "version" << PROJECT_VERSION;
#ifdef GIT_REPO
	qInfo() << "Built from" << GIT_REPO << GIT_REF << "(" << GIT_SHA << ")";
#endif // GIT_REPO

	QApplication a( argc, argv );
	QCoreApplication::addLibraryPath( QCoreApplication::applicationDirPath() );
	QCoreApplication::setOrganizationDomain( PROJECT_HOMEPAGE_URL );
	QCoreApplication::setOrganizationName( "Roest" );
	QCoreApplication::setApplicationName( PROJECT_NAME );
	QCoreApplication::setApplicationVersion( PROJECT_VERSION );

	if ( !Config::getInstance().init() )
	{
		qDebug() << "Failed to init Config.";
		exit( 0 );
	}

	DB::init();

	if ( !S::gi().init() )
	{
		qDebug() << "Failed to init translation.";
		exit( 0 );
	}

	Config::getInstance().set( "CurrentVersion", PROJECT_VERSION );

	QStringList args = a.arguments();

	for ( int i = 1; i < args.size(); ++i )
	{
		if ( args.at( i ) == "-h" || args.at( i ) == "?" )
		{
			qDebug() << "Command line options:";
			qDebug() << "-h : displays this message";
			qDebug() << "-v : toggles verbose mode, warning: this will spam your console with messages";
			qDebug() << "---";
		}
		if ( args.at( i ) == "-v" )
		{
			verbose = true;
		}
	}

	int width  = qMax( 1200, Config::getInstance().get( "WindowWidth" ).toInt() );
	int height = qMax( 675, Config::getInstance().get( "WindowHeight" ).toInt() );

	auto defaultFormat = QSurfaceFormat::defaultFormat();
	defaultFormat.setRenderableType( QSurfaceFormat::OpenGL );
	defaultFormat.setSwapBehavior( QSurfaceFormat::TripleBuffer );
	defaultFormat.setColorSpace( QSurfaceFormat::sRGBColorSpace );
	defaultFormat.setDepthBufferSize( 16 );
	// 0 = unthrottled, 1 = vysnc full FPS, 2 = vsync half FPS
	defaultFormat.setSwapInterval( 0 );
	defaultFormat.setVersion( 4, 3 );
	defaultFormat.setRenderableType( QSurfaceFormat::OpenGL );
	defaultFormat.setProfile( QSurfaceFormat::CoreProfile );
	defaultFormat.setOption( QSurfaceFormat::DebugContext );
	QSurfaceFormat::setDefaultFormat( defaultFormat );

	//MainWindow w;
	MainWindow w;
	
#ifdef _WIN32
	w.setIcon( QFileIconProvider().icon( QFileInfo( QCoreApplication::applicationFilePath() ) ) );
#endif // _WIN32

	w.resize( width, height );
	w.setPosition( Config::getInstance().get( "WindowPosX" ).toInt(), Config::getInstance().get( "WindowPosY" ).toInt() );
	w.show();
	if( Config::getInstance().get( "fullscreen" ).toBool() )
	{
		w.onFullScreen( true );
	}
	return a.exec();
}

#ifdef _WIN32
INT WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow )
{
	return main( 0, nullptr );
	return 0;
}

extern "C"
{
	// Request use of dedicated GPUs for NVidia/AMD/iGPU mixed setups
	__declspec( dllexport ) DWORD NvOptimusEnablement                  = 1;
	__declspec( dllexport ) DWORD AmdPowerXpressRequestHighPerformance = 1;
}
#endif // _WIN32

