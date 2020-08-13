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
#include "networkclient.h"

#include "../base/config.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/io.h"
#include "../base/util.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/workshopmanager.h"
#include "../game/world.h"
#include "../gfx/spritefactory.h"

#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>

NetworkClient::NetworkClient() :
	m_allowNetwork( false ),
	m_tcpSocket( 0 ),
	m_worldReceiveMode( false ),
	m_worldBufferSize( 0 ),
	m_worldBufferSizeCompressed( 0 )
{
	init();
}

NetworkClient::~NetworkClient()
{
	reset();
}

void NetworkClient::reset()
{
	if ( m_tcpSocket )
	{
		m_tcpSocket->disconnect();
		m_tcpSocket->deleteLater();
		m_tcpSocket = 0;
	}
}

bool NetworkClient::init()
{
	m_allowNetwork = Config::getInstance().get( "allowNetwork" ).toBool();
	return true;
}

bool NetworkClient::join()
{
	QString host = Config::getInstance().get( "server" ).toString();
	QString port = Config::getInstance().get( "port" ).toString();

	if ( m_tcpSocket )
	{
		reset();
	}
	m_tcpSocket = new QTcpSocket();
	connect( m_tcpSocket, QOverload<QAbstractSocket::SocketError>::of( &QAbstractSocket::error ), this, &NetworkClient::networkError );
	connect( m_tcpSocket, &QTcpSocket::connected, this, &NetworkClient::onConnected );
	m_tcpSocket->abort();
	m_tcpSocket->connectToHost( host, port.toInt() );

	m_timer = new QTimer( this );
	connect( m_timer, &QTimer::timeout, this, &NetworkClient::update );
	m_timer->start( 40 );

	return true;
}

void NetworkClient::onConnected()
{
	QString cmd;
	cmd += "!NAME|";
	cmd += Config::getInstance().get( "playerName" ).toString().toUtf8();
	cmd += "!";
	sendCommand( cmd );
	cmd = "!RequestWorld!";
	sendCommand( cmd );
}

void NetworkClient::sendCommand( QString& command )
{
	QByteArray block;
	QDataStream out( &block, QIODevice::WriteOnly );
	out.setVersion( QDataStream::Qt_5_9 );

	out << command.toLocal8Bit();
	m_tcpSocket->write( block );
}

void NetworkClient::networkError( QAbstractSocket::SocketError socketError )
{
	switch ( socketError )
	{
		case QAbstractSocket::RemoteHostClosedError:
			break;
		case QAbstractSocket::HostNotFoundError:
			qDebug() << "The host was not found. Please check the host name and port settings.";
			break;
		case QAbstractSocket::ConnectionRefusedError:
			qDebug() << "The connection was refused by the peer. Make sure the server is running, and check that the host name and port settings are correct.";
			break;
		default:
			qDebug() << "The following error occurred: " << m_tcpSocket->errorString();
	}
}

void NetworkClient::update()
{
	if ( m_allowNetwork )
	{
		if ( m_tcpSocket->bytesAvailable() > 0 )
		{
			m_buffer.append( m_tcpSocket->readAll() );
		}

		if ( m_worldReceiveMode )
		{
			if ( m_buffer.size() >= m_worldBufferSizeCompressed )
			{
				uncompressWorld();
			}
		}
		else
		{
			while ( m_buffer.size() && m_buffer.contains( '!' ) )
			{
				int index = m_buffer.indexOf( "!" );
				if ( index == 0 )
				{
					m_buffer.remove( 0, 1 );
				}
				else
				{
					QByteArray left = m_buffer.left( index );
					m_buffer.remove( 0, index + 1 );
					QString command( left );
					executeCommand( command );
					if ( m_worldReceiveMode )
						break;
				}
			}
		}
		// send own commands;
		QString cs = GameState::getChangeSet();
		if ( cs.size() > 0 )
		{
			QString cmd = "!Change";
			cmd += cs;
			cmd += "!";
			sendCommand( cmd );
		}
	}
}

void NetworkClient::executeCommand( QString& command )
{
	//qDebug() << "Execute network command: " << command;
	if ( command.startsWith( "World" ) )
	{
		qDebug() << "World incomming" << command;
		QStringList cl     = command.split( "|" );
		m_worldReceiveMode = true;

		Global::dimX = cl[1].toInt();
		Global::dimY = cl[2].toInt();
		Global::dimZ = cl[3].toInt();

		m_worldBufferSize           = cl[4].toInt();
		m_worldBufferSizeCompressed = cl[5].toInt();
	}
	else if ( command.startsWith( "JSON" ) )
	{
		processJSONCommand( command );
	}
	else if ( command.startsWith( "Change" ) )
	{
		processChangeCommand( command );
	}
}

void NetworkClient::uncompressWorld()
{
	QByteArray compressed = m_buffer.left( m_worldBufferSizeCompressed );
	m_buffer.remove( 0, m_worldBufferSizeCompressed );

	QByteArray uncompressed = qUncompress( compressed );
	if ( uncompressed.size() == m_worldBufferSize )
	{
		qDebug() << "Received world buffer with size " << uncompressed.size();
		QDataStream in( uncompressed );
		IO::loadWorld( in );
		Global::w().afterLoad();
	}
	else
	{
		qDebug() << "Received world buffer with size " << uncompressed.size() << " but expected " << m_worldBufferSize << " bytes";
	}

	m_worldReceiveMode = false;
	QString cmd( "!RequestJSON!" );
	sendCommand( cmd );
	emit joinFinished();
}

void NetworkClient::processJSONCommand( QString& command )
{
	/*
	QStringList cl = command.split( "|" );
	if( cl.size() == 3 )
	{
		qDebug() << "process command " << cl[0] << " " << cl[1];
		QJsonDocument jd;
		if( IO::loadString( cl[2], jd ) )
		{
			if( cl[1] == "Game" )
			{
				IO::loadGame( jd );
			}
			if( cl[1] == "Sprites" )
			{
				IO::loadSprites( jd );
			}
			else if( cl[1] == "WallConstructions" ) 
			{
				IO::loadWallConstructions( jd );
			}
			else if( cl[1] == "FloorConstructions" ) 
			{
				IO::loadFloorConstructions( jd );
			}
			else if( cl[1] == "Items" ) 
			{
				IO::loadItems( jd );
			}
			else if( cl[1] == "Stockpiles" ) 
			{
				IO::loadStockpiles( jd );
			}
			else if( cl[1] == "Jobs" ) 
			{
				IO::loadJobs( jd );
			}
			else if( cl[1] == "Gnomes" ) 
			{
				IO::loadGnomes( jd );
			}
			else if( cl[1] == "Plants" ) 
			{
				IO::loadPlants( jd );
			}
			else if( cl[1] == "Animals" ) 
			{
				IO::loadAnimals( jd );
			}
			else if( cl[1] == "Farms" ) 
			{
				IO::loadFarms( jd );
			}
			else if( cl[1] == "Workshops" ) 
			{
				IO::loadWorkshops( jd );
			}
		}
	}
	*/
}

void NetworkClient::processChangeCommand( QString& command )
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
						case NetworkCommand::CREATUREMOVE:
						{
							unsigned int id = args[1].toUInt();
							Position newPos( args[2] );
							int facing = args[3].toInt();
							if ( !Global::gm().setNetworkMove( id, newPos, facing ) )
							{
								world.setNetworkMove( id, newPos, facing );
							}
							break;
						}
						case NetworkCommand::CREATESPRITE:
						{
							QString itemSID          = args[1];
							QStringList materialSIDs = args[2].split( "_" );
							Global::sf().createSpriteNetwork( itemSID, materialSIDs );
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
							Global::sf().createSpriteNetwork( itemSID, materialSIDs, random );
							break;
						}
						case NetworkCommand::SETFLOORSPRITE:
						{
							world.setFloorSprite( args[1].toUInt(), args[2].toUInt() );
							break;
						}
						case NetworkCommand::CLEARFLOORSPRITE:
						{
							world.setFloorSprite( args[1].toUInt(), 0 );
							break;
						}
						case NetworkCommand::SETWALLSPRITE:
						{
							world.setWallSprite( args[1].toUInt(), args[2].toUInt() );
							break;
						}
						case NetworkCommand::CLEARWALLSPRITE:
						{
							world.setWallSprite( args[1].toUInt(), 0 );
							break;
						}
						case NetworkCommand::SETJOBSPRITE:
						{
							unsigned int tileID   = args[1].toUInt();
							unsigned int spriteID = args[2].toUInt();
							unsigned int rot      = args[3].toUInt();
							unsigned int jobID    = args[4].toUInt();
							bool floor            = ( args[4].toUInt() == 1 );
							world.setJobSprite( tileID, spriteID, rot, floor, jobID, false );
							break;
						}
						case NetworkCommand::CLEARJOBSPRITE:
						{
							world.clearJobSprite( Position( args[1] ), (bool)args[2].toInt() );
							break;
						}
						case NetworkCommand::SETTILEFLAGS:
						{
							Tile& tile = world.getTile( args[1].toUInt() );
							Util::string2Tile( tile, args[2] );
							break;
						}
						case NetworkCommand::ITEMCREATE:
						{
							Global::inv().createItem( QJsonDocument::fromJson( args[1].toLocal8Bit() ).toVariant().toMap() );
							break;
						}
						case NetworkCommand::ITEMDESTROY:
						{
							Global::inv().destroyObject( args[1].toUInt() );
							break;
						}
						case NetworkCommand::ITEMMOVE:
						{
							Global::inv().moveItemToPos( args[1].toUInt(), Position( args[2] ) );
							break;
						}
						case NetworkCommand::ITEMPICKUP:
						{
							Global::inv().pickUpItem( args[1].toUInt() );
							break;
						}
						case NetworkCommand::ITEMPUTDOWN:
						{
							Global::inv().putDownItem( args[1].toUInt(), Position( args[2] ) );
							break;
						}
						case NetworkCommand::ITEMSETINSTOCKPILE:
						{
							Global::inv().setInStockpile( args[1].toUInt(), args[2].toUInt() );
							break;
						}
						case NetworkCommand::ITEMSETINJOB:
						{
							Global::inv().setInJob( args[1].toUInt(), args[2].toUInt() );
							break;
						}
						case NetworkCommand::ITEMSETINCONTAINER:
						{
							Global::inv().setInContainer( args[1].toUInt(), args[2].toUInt() );
							break;
						}
						case NetworkCommand::ITEMSETCONSTRUCTED:
						{
							Global::inv().setConstructedOrEquipped( args[1].toUInt(), args[2].toInt() );
							break;
						}
						case NetworkCommand::ITEMSETPOS:
						{
							Global::inv().setItemPos( args[1].toUInt(), Position( args[2] ) );
							break;
						}
						case NetworkCommand::ITEMPUTINCONTAINER:
						{
							Global::inv().putItemInContainer( args[1].toUInt(), args[2].toUInt() );
							break;
						}
						case NetworkCommand::ITEMREMOVEFROMCONTAINER:
						{
							Global::inv().removeItemFromContainer( args[1].toUInt() );
							break;
						}
						case NetworkCommand::WORKSHOPADD:
						{
							Global::wsm().addWorkshop( QJsonDocument::fromJson( args[1].toLocal8Bit() ).toVariant().toMap() );
							break;
						}
						case NetworkCommand::WORKSHOPINFO:
						{
							//Global::wsm().setJobQueueJson( args[1].toUInt(), QJsonDocument::fromJson( args[2].toLocal8Bit() ).toVariant().toMap() );
							break;
						}
						case NetworkCommand::PLANTREMOVE:
						{
							world.removePlant( Position( args[1] ) );
							break;
						}
					}
				}
			}
		}
	}
}