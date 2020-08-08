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
#include "networkserver.h"

#include "../base/config.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/io.h"
#include "../base/selection.h"
#include "../game/workshopmanager.h"
#include "../game/world.h"
#include "../gfx/spritefactory.h"

#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

NetworkServer::NetworkServer() :
	m_allowNetwork( false ),
	m_tcpServer( 0 )
{
	init();
}

NetworkServer::~NetworkServer()
{
	reset();
}

void NetworkServer::reset()
{
	if ( m_tcpServer )
	{
		m_tcpServer->close();
		delete m_tcpServer;
		m_tcpServer = 0;
	}
	m_clients.clear();
}

bool NetworkServer::init()
{
	m_allowNetwork = Config::getInstance().get( "allowNetwork" ).toBool();

	if ( m_allowNetwork )
	{
		for ( auto client : m_clients )
		{
			client.socket->disconnectFromHost();
			client.socket->deleteLater();
		}
		m_clients.clear();
		if ( m_tcpServer )
		{
			reset();
		}
		m_tcpServer = new QTcpServer();

		unsigned short port = Config::getInstance().get( "port" ).toUInt();

		if ( !m_tcpServer->listen( QHostAddress::Any, port ) )
		{
			return false;
		}

		connect( m_tcpServer, &QTcpServer::newConnection, this, &NetworkServer::onNewConnection );
	}

	m_timer = new QTimer( this );
	connect( m_timer, &QTimer::timeout, this, &NetworkServer::update );
	m_timer->start( 40 );

	return true;
}

void NetworkServer::onNewConnection()
{
	qDebug() << "new tcp connection";

	Client client;

	client.socket = m_tcpServer->nextPendingConnection();
	connect( client.socket, &QAbstractSocket::disconnected, this, &NetworkServer::onClientDisconnected );

	m_clients.push_back( client );

	GameState::setAcceptChangeSets( true );
}

void NetworkServer::onClientDisconnected()
{
	for ( int i = 0; i < m_clients.size(); ++i )
	{
		if ( !m_clients[i].socket->isValid() )
		{
			m_clients[i].socket->deleteLater();
			m_clients.removeAt( i );
			break;
		}
	}
	if ( m_clients.empty() )
	{
		GameState::setAcceptChangeSets( false );
	}
}

void NetworkServer::sendCommandAll( QString command )
{
	//qDebug() << "Send command: " << command;
	QByteArray block;
	QDataStream out( &block, QIODevice::WriteOnly );
	out.setVersion( QDataStream::Qt_5_9 );
	out << command.toLocal8Bit();
	for ( auto client : m_clients )
	{
		client.socket->write( block );
	}
}

void NetworkServer::sendCommand( QString command, Client& client )
{
	//qDebug() << "Send command: " << command;
	QByteArray block;
	QDataStream out( &block, QIODevice::WriteOnly );
	out.setVersion( QDataStream::Qt_5_9 );
	out << command.toLocal8Bit();
	client.socket->write( block );
}

void NetworkServer::update()
{
	if ( m_allowNetwork )
	{
		for ( auto&& client : m_clients )
		{
			if ( client.socket->bytesAvailable() > 0 )
			{
				client.buffer.append( client.socket->readAll() );
			}
			while ( client.buffer.size() && client.buffer.contains( '!' ) )
			{
				int index = client.buffer.indexOf( "!" );
				if ( index == 0 )
				{
					client.buffer.remove( 0, 1 );
				}
				else
				{
					QByteArray left = client.buffer.left( index );
					client.buffer.remove( 0, index + 1 );
					QString command( left );
					executeCommand( command, client );
					break;
				}
			}
		}
	}
	sendChangeSet();
}

void NetworkServer::executeCommand( QString command, Client& client )
{
	//qDebug() << "Execute network command: " << command;
	if ( command == "RequestWorld" )
	{
		sendWorld( client );
	}
	else if ( command == "RequestJSON" )
	{
		sendJSON( client );
	}
	else if ( command.startsWith( "Change" ) )
	{
		processChangeCommand( command );
	}
}

void NetworkServer::sendWorld( Client& client )
{
	Config::getInstance().set( "Pause", true );
	Config::getInstance().set( "FastSpeed", false );

	QJsonArray ja = IO::jsonArraySprites();
	sendJSONArray( "Sprites", ja, client );

	QByteArray buffer;

	QDataStream out( &buffer, QIODevice::WriteOnly );
	//out << int(0); // placeholder for info about bytes for the binary data

	std::vector<Tile>& world = Global::w().world();

	for ( auto tile : world )
	{
		out << (quint32)tile.flags;
		out << (quint16)tile.wallType;
		out << (quint16)tile.wallMaterial;
		out << (quint16)tile.floorType;
		out << (quint16)tile.floorMaterial;
		out << (quint8)tile.wallRotation;
		out << (quint8)tile.floorRotation;
		out << (quint8)tile.fluidLevel;
#ifdef SAVEREGIONINFO
		out << (quint32)tile.region;
#endif
	}

	//out.device()->seek(0); // go back to start
	//out << buffer.size(); // info about bytes for the size value (int) and binary image data (image)

	QByteArray compressed = qCompress( buffer );

	int dimX = Global::dimX;
	int dimY = Global::dimY;
	int dimZ = Global::dimZ;
	QString cmd( "!World|" );
	cmd += QString::number( dimX );
	cmd += "|";
	cmd += QString::number( dimY );
	cmd += "|";
	cmd += QString::number( dimZ );
	cmd += "|";
	cmd += QString::number( buffer.size() );
	cmd += "|";
	cmd += QString::number( compressed.size() );
	cmd += "!";
	sendCommand( cmd, client );

	if ( client.socket->write( compressed ) < compressed.size() )
	{
		qWarning( "transmit error!" );
	}
}

void NetworkServer::sendJSON( Client& client )
{
	/*
	QJsonArray ja;

	ja = IO::jsonArrayGame();
	sendJSONArray( "Game", ja, client );

	ja = IO::jsonArrayWallConstructions();
	sendJSONArray( "WallConstructions", ja, client );

	ja = IO::jsonArrayFloorConstructions();
	sendJSONArray( "FloorConstructions", ja, client );

	ja = IO::jsonArrayItems();
	sendJSONArray( "Items", ja, client );

	ja = IO::jsonArrayStockpiles();
	sendJSONArray( "Stockpiles", ja, client );

	ja = IO::jsonArrayJobs();
	sendJSONArray( "Jobs", ja, client );

	ja = IO::jsonArrayGnomes();
	sendJSONArray( "Gnomes", ja, client );

	ja = IO::jsonArrayPlants();
	sendJSONArray( "Plants", ja, client );

	ja = IO::jsonArrayAnimals();
	sendJSONArray( "Animals", ja, client );

	ja = IO::jsonArrayFarms();
	sendJSONArray( "Farms", ja, client );

	ja = IO::jsonArrayWorkshops();
	sendJSONArray( "Workshops", ja, client );
	*/
}

void NetworkServer::sendJSONArray( QString key, QJsonArray& ja, Client& client )
{
	QJsonDocument doc( ja );
	QString data( doc.toJson() );

	QString cmd( "!JSON|" );
	cmd += key;
	cmd += "|";
	cmd += data;
	cmd += "!";
	sendCommand( cmd, client );
}

void NetworkServer::sendChangeSet()
{
	/*
	QString cs = GameState::getInstance().getChangeSet();
	if ( cs.size() > 0 )
	{
		QString cmd = "!Change";
		cmd += cs;
		cmd += "!";
		sendCommandAll( cmd );
	}
	*/
}

void NetworkServer::processChangeCommand( QString& command )
{
	QStringList cl = command.split( "|" );
	if ( cl.size() > 1 )
	{
		World& world = Global::w();
		for ( int i = 1; i < cl.size(); ++i )
		{
			QStringList args = cl[i].split( ";" );
			{
				if ( args.size() > 0 )
				{
					NetworkCommand cmd = (NetworkCommand)args[0].toInt();

					switch ( cmd )
					{
						case NetworkCommand::SELECTION:
						{
							Selection::getInstance().setNetworkCommand( QJsonDocument::fromJson( args[1].toLocal8Bit() ).toVariant().toMap() );
							break;
						}
						case NetworkCommand::CREATESPRITE:
						{
							QString itemSID          = args[1];
							QStringList materialSIDs = args[2].split( "_" );
							Global::sf().createSprite( itemSID, materialSIDs );
							break;
						}
						case NetworkCommand::CREATESPRITERANDOM:
						{
							QString itemSID          = args[1];
							QStringList materialSIDs = args[2].split( "_" );
							QMap<int, int> random;
							QString randomString = args[3];
							if ( randomString.endsWith( "_" ) )
							{
								randomString.chop( 1 );
							}
							QStringList randomSList = randomString.split( "_" );
							for ( int i = 0; i < randomSList.size(); i += 2 )
							{
								random.insert( randomSList[i].toInt(), randomSList[i + 1].toInt() );
							}
							Global::sf().createSprite( itemSID, materialSIDs, random );
							break;
						}
						case NetworkCommand::WORKSHOPINFO:
						{
							//Global::wsm().setJobQueueJson( args[1].toUInt(), QJsonDocument::fromJson( args[2].toLocal8Bit() ).toVariant().toMap() );
							break;
						}
					}
				}
			}
		}
	}
}