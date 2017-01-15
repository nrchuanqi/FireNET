// Copyright (C) 2014-2017 Ilya Chernetsov. All rights reserved. Contacts: <chernecoff@gmail.com>
// License: https://github.com/afrostalin/FireNET/blob/master/LICENSE

#ifndef HTTPCONNECTOR_H
#define HTTPCONNECTOR_H

#include <QObject>

class QNetworkReply;
class QUrl;
class QTimer;

class HttpConnector : public QObject
{
    Q_OBJECT
public:
    explicit HttpConnector(QObject *parent = 0);
public:
	bool Login(QString login, QString password);
	bool Register(QString login, QString password);
public:
	int GetUID();
	int GetError();
public slots:
	virtual void replyFinished(QNetworkReply* reply);
private:
	void SendPostRequest(QUrl url, QByteArray requestData);
private:
	// Login error types : 0 - Login not found, 1 - Account blocked, 2 - Incorrect password, 3 - Double authorization
	// Register error types : 0 - Login alredy register, 1 - Can't create account, 2 - Double registration
	int m_lastError;
	int m_uid;
	bool bHaveResult;
	bool bSuccessAuth;
	bool bSuccessReg;
};

#endif // HTTPCONNECTOR_H