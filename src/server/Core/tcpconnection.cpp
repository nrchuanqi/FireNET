// Copyright (C) 2014-2017 Ilya Chernetsov. All rights reserved. Contacts: <chernecoff@gmail.com>
// License: https://github.com/afrostalin/FireNET/blob/master/LICENSE

#include "global.h"
#include "tcpconnection.h"
#include "tcpserver.h"
#include "tcppacket.h"

#include "Workers/Packets/clientquerys.h"
#include "Workers/Databases/mysqlconnector.h"
#include "Workers/Databases/dbworker.h"
#include "Tools/settings.h"

TcpConnection::TcpConnection(QObject *parent) : QObject(parent),
	pQuery(nullptr),
	m_Socket(nullptr),
	bConnected(false),
	bIsQuiting(false),
	bLastMsgSended(true)
{
	Q_UNUSED(parent);

	m_maxPacketSize = gEnv->pSettings->GetVariable("net_max_packet_read_size").toInt();
	m_maxBadPacketsCount = gEnv->pSettings->GetVariable("net_max_bad_packets_count").toInt();
	m_BadPacketsCount = 0;

	m_Time = QTime::currentTime();
	m_InputPacketsCount = 0;
	m_PacketsSpeed = 0;
	m_maxPacketSpeed = gEnv->pSettings->GetVariable("net_max_packets_speed").toInt();
}

TcpConnection::~TcpConnection()
{
	qDebug() << "~TcpConnection";
	SAFE_RELEASE(m_Socket);
	SAFE_RELEASE(pQuery);
}

void TcpConnection::Update()
{
	if (!m_Packets.empty() && m_Socket && bConnected && bLastMsgSended)
	{
		bLastMsgSended = false;
		CTcpPacket packet = m_Packets.front();
		m_Packets.pop();
		m_Socket->write(packet.toString());
	}

	if (m_Time.elapsed() >= 1000)
	{
		m_Time = QTime::currentTime();
		CalculateStatistic();
	}
}

void TcpConnection::SendMessage(CTcpPacket& packet)
{
	m_Packets.push(packet);
}

void TcpConnection::quit()
{
	qDebug() << "Quit called, closing client";

	bIsQuiting = true;
	m_Socket->close();
}

void TcpConnection::accept(qint64 socketDescriptor)
{
	qDebug() << "Accepting new client...";

	m_Socket = CreateSocket();

	if (!m_Socket)
	{
		qDebug() << "Could not find created socket!";
		return;
	}

	if (!m_Socket->setSocketDescriptor(socketDescriptor))
	{
		qDebug() << "Can't accept socket!";
		return;
	}

	m_Socket->setLocalCertificate("key.pem");
	m_Socket->setPrivateKey("key.key");
	m_Socket->startServerEncryption();

	int timeout = gEnv->pSettings->GetVariable("net_encryption_timeout").toInt();

	if (!m_Socket->waitForEncrypted(timeout * 1000))
	{
		qDebug() << "Can't accept socket! Encryption timeout!";
		emit quit();
		return;
	}

	qDebug() << "Client accepted. Socket " << m_Socket;
}

void TcpConnection::connected()
{
    if(!m_Socket)
		return;

	m_Client.socket = m_Socket;
	m_Client.profile = nullptr;
	m_Client.status = 0;	

	// Add client to server client list
	gEnv->pServer->AddNewClient(m_Client);

	// Create client querys worker
	pQuery = new ClientQuerys(this);
	// Set socket for client querys worker
	pQuery->SetSocket(m_Socket);
	// Set client
	pQuery->SetClient(&m_Client);
	// Set connection
	pQuery->SetConnection(this);

	bConnected = true;

	qInfo() << "Client" << m_Socket << "connected.";

	emit opened();
}

void TcpConnection::disconnected()
{
	if (!m_Socket || !bConnected)
	{
		emit closed();
		return;
	}

	// Remove client from server client list
	gEnv->pServer->RemoveClient(m_Client);

	qInfo() << "Client" << m_Socket << "disconnected.";

	emit closed();
}

void TcpConnection::readyRead()
{
    if(!m_Socket || bIsQuiting)
		return;

	m_InputPacketsCount++;

	emit received();

	// If client send a lot bad packet we need disconnect him
	if (m_BadPacketsCount >= m_maxBadPacketsCount)
	{
		qWarning() << "Exceeded the number of bad packets from a client. Connection will be closed" << m_Socket;
		quit();
		return;
	}

	// STACK OVERFLOW HERE !!!
    //qDebug() << "Read message from client" << m_Socket;
 	//qDebug() << "Available bytes for read" << m_Socket->bytesAvailable();

	// Check bytes count before reading
	if (m_Socket->bytesAvailable() > m_maxPacketSize)
	{
		qWarning() << "Very big packet from client" << m_Socket;
		m_BadPacketsCount++;
		return;
	}
	else if (m_Socket->bytesAvailable() <= 0)
	{
		qDebug() << "Very small packet from client" << m_Socket;
		m_BadPacketsCount++;
		return;
	}

	CTcpPacket packet(m_Socket->readAll());

	if(packet.getType() == EFireNetTcpPacketType::Query)
	{
		switch (packet.ReadQuery())
		{
		case EFireNetTcpQuery::Login :
		{
			pQuery->onLogin(packet);
			break;
		}
		case EFireNetTcpQuery::Register :
		{
			pQuery->onRegister(packet);
			break;
		}
		case EFireNetTcpQuery::CreateProfile :
		{
			pQuery->onCreateProfile(packet);
			break;
		}
		case EFireNetTcpQuery::GetProfile :
		{
			pQuery->onGetProfile();
			break;
		}
		case EFireNetTcpQuery::GetShop :
		{
			pQuery->onGetShopItems();
			break;
		}
		case EFireNetTcpQuery::BuyItem :
		{
			pQuery->onBuyItem(packet);
			break;
		}
		case EFireNetTcpQuery::RemoveItem :
		{
			pQuery->onRemoveItem(packet);
			break;
		}		
		case EFireNetTcpQuery::SendInvite :
		{
			pQuery->onInvite(packet);
			break;
		}
		case EFireNetTcpQuery::DeclineInvite :
		{
			pQuery->onDeclineInvite(packet);
			break;
		}
		case EFireNetTcpQuery::AcceptInvite :
		{
			break;
		}
		case EFireNetTcpQuery::RemoveFriend :
		{
			pQuery->onRemoveFriend(packet);
			break;
		}
		case EFireNetTcpQuery::SendChatMsg :
		{
			pQuery->onChatMessage(packet);
			break;
		}
		case EFireNetTcpQuery::GetServer :
		{
			pQuery->onGetGameServer(packet);
			break;
		}

		default:
		{
			qCritical() << "Error reading query. Can't get query type!";
			m_BadPacketsCount++;
			break;
		}
		}
	}
	else
	{
		qCritical() << "Error reading packet. Can't get packet type!";
		m_BadPacketsCount++;
	}
}

void TcpConnection::bytesWritten(qint64 bytes)
{
    if(!m_Socket)
		return;

	emit sended();

	bLastMsgSended = true;

    qDebug() << "Message to client" << m_Socket << "sended! Size =" << bytes;
}

void TcpConnection::stateChanged(QAbstractSocket::SocketState socketState)
{
    if(!m_Socket)
		return;

    qDebug() << "Client" << m_Socket << "changed socket state to " << socketState;
}

void TcpConnection::socketError(QAbstractSocket::SocketError error)
{
	if (error == QAbstractSocket::SocketError::RemoteHostClosedError)
		quit();
	else
	{
		qCritical() << "Client" << m_Socket << "return socket error" << error;
		quit();
	}
}

QSslSocket * TcpConnection::CreateSocket()
{
	qDebug() << "Creating socket for client";

	QSslSocket *socket = new QSslSocket(this);
	connect(socket, &QSslSocket::encrypted, this, &TcpConnection::connected, Qt::QueuedConnection);
	connect(socket, &QSslSocket::disconnected, this, &TcpConnection::disconnected, Qt::QueuedConnection);
	connect(socket, &QSslSocket::readyRead, this, &TcpConnection::readyRead, Qt::QueuedConnection);
	connect(socket, &QSslSocket::bytesWritten, this, &TcpConnection::bytesWritten, Qt::QueuedConnection);
	connect(socket, &QSslSocket::stateChanged, this, &TcpConnection::stateChanged, Qt::QueuedConnection);
	connect(socket, static_cast<void (QSslSocket::*)(QAbstractSocket::SocketError)>(&QSslSocket::error), this, &TcpConnection::socketError, Qt::QueuedConnection);

	return socket;
}

void TcpConnection::CalculateStatistic()
{
	m_PacketsSpeed = m_InputPacketsCount;

	if (m_PacketsSpeed >= m_maxPacketSpeed)
	{
		qWarning() << "Client" << m_Socket << "exceeded the limit of the number of packets per second. Speed:" << m_PacketsSpeed << ". Maximum speed :" << m_maxPacketSpeed;
		quit();
	}

	m_InputPacketsCount = 0;
}