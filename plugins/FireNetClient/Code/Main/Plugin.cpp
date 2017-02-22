// Copyright (C) 2014-2017 Ilya Chernetsov. All rights reserved. Contacts: <chernecoff@gmail.com>
// License: https://github.com/afrostalin/FireNET/blob/master/LICENSE

#include "StdAfx.h"
#include "Plugin.h"

#include "Network/UdpClient.h"
#include "Network/NetworkThread.h"
#include "Network/SyncGameState.h"
#include "Network/UdpPacket.h"

#include <CryCore/Platform/platform_impl.inl>

IEntityRegistrator *IEntityRegistrator::g_pFirst = nullptr;
IEntityRegistrator *IEntityRegistrator::g_pLast = nullptr;

void TestConnection(IConsoleCmdArgs* argc)
{
	if (mEnv->pNetworkThread)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, TITLE  "FireNet client thread alredy spawned!");
		return;
	}

	if (!gEnv->IsDedicated() && !gEnv->IsEditor())
	{
		mEnv->pNetworkThread = new CNetworkThread();

		if (!gEnv->pThreadManager->SpawnThread(mEnv->pNetworkThread, "FireNetClient_Thread"))
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, TITLE  "Can't spawn FireNet client thread!");

			SAFE_DELETE(mEnv->pNetworkThread);
		}
		else
			CryLog(TITLE "FireNet client thread spawned");
	}
	else
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, TITLE  "Can't spawn FireNet client thread : It's not dedicated server or it's editor");
	}
}

CFireNetClientPlugin::~CFireNetClientPlugin()
{
	// Unregister entities
	IEntityRegistrator* pTemp = IEntityRegistrator::g_pFirst;
	while (pTemp != nullptr)
	{
		pTemp->Unregister();
		pTemp = pTemp->m_pNext;
	}

	// Unregister CVars
	IConsole* pConsole = gEnv->pConsole;
	if (pConsole)
	{
		pConsole->UnregisterVariable("firenet_game_server_ip");
		pConsole->UnregisterVariable("firenet_game_server_port");
		pConsole->UnregisterVariable("firenet_game_server_timeout");
	}

	// Stop and delete network thread if Quit funtion not executed
	if (mEnv->pNetworkThread)
	{
		CryWarning(VALIDATOR_MODULE_NETWORK, VALIDATOR_ERROR, TITLE  "Network thread not deleted! Use Quit function to normal shutdown plugin!");

		mEnv->pNetworkThread->SignalStopWork();

		if (gEnv->pThreadManager)
			gEnv->pThreadManager->JoinThread(mEnv->pNetworkThread, eJM_Join);
	}

	SAFE_DELETE(mEnv->pNetworkThread);

	// Unregister listeners
	if (gEnv->pSystem)
		gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

	// Clear FireNet core pointer
	gEnv->pFireNetClient = nullptr;

	CryLogAlways(TITLE "Unloaded.");
}

bool CFireNetClientPlugin::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	if (initParams.bEditor && !gEnv->IsEditor())
		gEnv->SetIsEditor(true);

	if (initParams.bDedicatedServer && !gEnv->IsDedicated())
		gEnv->SetIsDedicated(true);

	if (!gEnv->IsDedicated())
	{
		gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

		// Init FireNet client pointer
		gEnv->pFireNetClient = this;
	}
	else
	{
		CryWarning(VALIDATOR_MODULE_NETWORK, VALIDATOR_ERROR, TITLE "Can't init FireNet-Client.dll - Dedicated server not support client library!");

		gEnv->pFireNetClient = nullptr;
	}

	return true;
}

void CFireNetClientPlugin::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_GAME_POST_INIT:
	{
		// Register entities
		IEntityRegistrator* pTemp = IEntityRegistrator::g_pFirst;
		while (pTemp != nullptr)
		{
			pTemp->Register();
			pTemp = pTemp->m_pNext;
		}

		// Register CVars
		mEnv->net_ip = REGISTER_STRING("firenet_game_server_ip", "127.0.0.1", VF_NULL, "Sets the FireNet game server ip address");
		REGISTER_CVAR2("firenet_game_server_port", &mEnv->net_port, 64000, VF_CHEAT, "FireNet game server port");
		REGISTER_CVAR2("firenet_game_server_timeout", &mEnv->net_timeout, 10, VF_NULL, "FireNet game server timeout");

		// Test functionality
		REGISTER_COMMAND("net_connect", TestConnection, VF_NULL, "");

		break;
	}
	case ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED:
	{
		if (mEnv->pGameSync && wparam == 0)
			mEnv->pGameSync->Reset();
		break;
	}
	case ESYSTEM_EVENT_GAME_FRAMEWORK_INIT_DONE:
	{
		if (gEnv->pGameFramework)
		{
			gEnv->pGameFramework->RegisterListener(this, "FireNetClient_GameFrameworkListener", FRAMEWORKLISTENERPRIORITY_DEFAULT);
			CryLog(TITLE "Framework listener registered");
		}
		else
			CryWarning(VALIDATOR_MODULE_NETWORK, VALIDATOR_ERROR, TITLE "Can't register framework listener!");

		break;
	}
	break;
	}
}

void CFireNetClientPlugin::OnPostUpdate(float fDeltaTime)
{
	if (mEnv->pNetworkThread && mEnv->pUdpClient)
	{
		mEnv->pUdpClient->Update(fDeltaTime);
	}
}

void CFireNetClientPlugin::ConnectToGameServer()
{
	if (mEnv->pNetworkThread)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, TITLE  "FireNet client thread alredy spawned. Removing...");

		mEnv->pNetworkThread->SignalStopWork();
		gEnv->pThreadManager->JoinThread(mEnv->pNetworkThread, eJM_Join);

		SAFE_DELETE(mEnv->pNetworkThread);
	}

	if (!gEnv->IsDedicated() && !gEnv->IsEditor())
	{
		mEnv->pNetworkThread = new CNetworkThread();

		if (!gEnv->pThreadManager->SpawnThread(mEnv->pNetworkThread, "FireNetClient_Thread"))
		{
			CryWarning(VALIDATOR_MODULE_NETWORK, VALIDATOR_ERROR, TITLE  "Can't spawn FireNet client thread!");

			SAFE_DELETE(mEnv->pNetworkThread);
		}
		else
			CryLog(TITLE "FireNet client thread spawned");
	}
	else
	{
		CryWarning(VALIDATOR_MODULE_NETWORK, VALIDATOR_ERROR, TITLE  "Can't spawn FireNet client thread : It's not dedicated server or it's editor");
	}
}

void CFireNetClientPlugin::DisconnectFromServer()
{
	if (mEnv->pNetworkThread && mEnv->pUdpClient)
	{
		mEnv->pNetworkThread->SignalStopWork();
	}
}

void CFireNetClientPlugin::SendMovementRequest(EFireNetClientActions action, float value)
{
	if (mEnv->pUdpClient && mEnv->pUdpClient->IsConnected())
	{
		CUdpPacket packet(mEnv->m_LastOutPacketNumber, EFireNetUdpPacketType::Request);
		packet.WriteRequest(EFireNetUdpRequest::Action);
		packet.WriteInt(action);
		packet.WriteFloat(value);

		mEnv->pUdpClient->SendNetMessage(packet);
	}
}

bool CFireNetClientPlugin::IsConnected()
{
	return mEnv->pUdpClient ? mEnv->pUdpClient->IsConnected() : false;
}

bool CFireNetClientPlugin::Quit()
{
	CryLogAlways(TITLE "Closing plugin...");

	if (mEnv->pNetworkThread)
	{
		mEnv->pNetworkThread->SignalStopWork();
		gEnv->pThreadManager->JoinThread(mEnv->pNetworkThread, eJM_Join);
	}

	SAFE_DELETE(mEnv->pNetworkThread);

	if (!mEnv->pNetworkThread)
		return true;
	else
		CryWarning(VALIDATOR_MODULE_NETWORK, VALIDATOR_ERROR, TITLE  "Can't normal close plugin - network thread not deleted!");

	return false;
}

CRYREGISTER_SINGLETON_CLASS(CFireNetClientPlugin)