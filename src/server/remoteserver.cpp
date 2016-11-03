// Copyright � 2016 Ilya Chernetsov. All rights reserved. Contacts: <chernecoff@gmail.com>
// License: http://opensource.org/licenses/MIT

#include "global.h"
#include "remoteserver.h"
#include "remoteconnection.h"
#include "settings.h"

#include <QDebug>

RemoteServer::RemoteServer(QObject *parent) : QTcpServer(parent)
{
	clientCount = 0;
	m_server = nullptr;
	bHaveAdmin = false;
}

void RemoteServer::run()
{
	if (CreateServer())
		qInfo() << "Remote server started on" << gEnv->pSettings->GetVariable("remote_server_ip").toString();
	else
		qCritical() << "Failed start remote server! Reason = " << m_server->errorString();
}

bool RemoteServer::CreateServer()
{
	m_server = new QTcpServer(this);
	return QTcpServer::listen(QHostAddress(gEnv->pSettings->GetVariable("remote_server_ip").toString()), 
		gEnv->pSettings->GetVariable("remote_server_port").toInt());
}

void RemoteServer::incomingConnection(qintptr socketDescriptor)
{
	qInfo() << "New incomining connection to remote server. Try accept...";
	
	RemoteConnection* m_remoteConnection = new RemoteConnection();
	connect(this, &QTcpServer::close, m_remoteConnection, &RemoteConnection::close);

	m_remoteConnection->accept(socketDescriptor);
}

void RemoteServer::sendMessageToRemoteClient(QSslSocket * socket, QByteArray data)
{
	qDebug() << "Send message to remote client. Original size = " << data.size();
	socket->write(data);
	socket->waitForBytesWritten(3);
}