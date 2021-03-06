// Copyright (C) 2014-2017 Ilya Chernetsov. All rights reserved. Contacts: <chernecoff@gmail.com>
// License: https://github.com/afrostalin/FireNET/blob/master/LICENSE

#pragma once

#define TITLE "[FireNet-Client] "

class CGameStateSynchronization;
class CUdpClient;
class CNetworkThread;

struct SFireNetProfile;

struct SPluginEnv
{
	SPluginEnv()
	{
		pNetworkThread = nullptr;
		pUdpClient = nullptr;
		pGameSync = nullptr;

		m_FireNetProfile = nullptr;

		net_ip = nullptr;
		net_port = 0;
		net_timeout = 0;
	}

	//! Pointers
	CUdpClient*                pUdpClient;
	CGameStateSynchronization* pGameSync;
	CNetworkThread*            pNetworkThread;

	//! Local FireNet profile
	SFireNetProfile*           m_FireNetProfile;

	//! CVars
	ICVar*                     net_ip;
	int                        net_port;
	int                        net_timeout;
};

extern SPluginEnv* mEnv;