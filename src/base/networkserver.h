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

#ifndef NetworkServer_H_
#define NetworkMServer_H_

#include <QMutex>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

struct Client
{
	QTcpSocket* socket;
	QByteArray buffer;
};

class NetworkServer : public QObject
{
	Q_OBJECT

public:
	NetworkServer();
	~NetworkServer();

	bool init();
	void reset();
	void update();

private:
	QMutex m_mutex;
	QTimer* m_timer;

	bool m_allowNetwork;

	QTcpServer* m_tcpServer;
	QList<Client> m_clients;

	void sendCommandAll( QString command );
	void sendCommand( QString command, Client& client );
	void executeCommand( QString command, Client& client );

	void sendWorld( Client& client );
	void sendJSON( Client& client );
	void sendJSONArray( QString key, QJsonArray& data, Client& client );
	void sendChangeSet();

	void processChangeCommand( QString& command );

private slots:
	void onNewConnection();
	void onClientDisconnected();
};

#endif /* NetworkServer_H_ */
