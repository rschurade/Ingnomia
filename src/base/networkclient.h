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

#ifndef NetworkClient_H_
#define NetworkClient_H_

#include <QMutex>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

class NetworkClient : public QObject
{
	Q_OBJECT
public:
	NetworkClient();
	~NetworkClient();

	bool init();

	void reset();

	bool join();

	void update();

private:
	QMutex m_mutex;

	QTimer* m_timer;

	bool m_allowNetwork;

	QTcpSocket* m_tcpSocket;

	QByteArray m_buffer;
	QString m_command;

	bool m_worldReceiveMode;
	int m_worldBufferSize;
	int m_worldBufferSizeCompressed;

	void sendCommand( QString& command );
	void executeCommand( QString& command );

	void uncompressWorld();
	void processJSONCommand( QString& command );
	void processChangeCommand( QString& command );

private slots:
	void networkError( QAbstractSocket::SocketError socketError );
	void onConnected();

signals:
	void joinFinished();
};

#endif /* NetworkClient_H_ */
