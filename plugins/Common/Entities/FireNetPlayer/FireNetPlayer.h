// Copyright (C) 2016-2017 Ilya Chernetsov. All rights reserved. Contacts: <chernecoff@gmail.com>
// License: https://github.com/afrostalin/EasyShooter/blob/master/LICENCE.md

#pragma once

#include "Entities/ISimpleActor.h"

#include <CryMath/Cry_Camera.h>

class CFireNetPlayerInput;
class CFireNetPlayerMovement;
class CFireNetPlayerView;
class CFireNetPlayerAnimations;

class CFireNetPlayer;

class CFireNetSpawnPoint;

struct ISimpleWeapon;

class CFireNetPlayer : public CGameObjectExtensionHelper<CFireNetPlayer, ISimpleActor>
{
public:
	enum EGeometrySlots
	{
		eGeometry_Default = 0,
	};

	struct SExternalCVars
	{
		float m_mass;
		float m_moveSpeed;

		float m_jumpHeightMultiplier;

		float m_viewOffsetY;
		float m_viewOffsetZ;

		float m_rotationSpeedYaw;
		float m_rotationSpeedPitch;

		float m_rotationLimitsMinPitch;
		float m_rotationLimitsMaxPitch;

		float m_playerEyeHeight;

		float m_cameraOffsetY;
		float m_cameraOffsetZ;

		ICVar *m_pGeometry;
		ICVar *m_pCameraJointName;

		ICVar *m_pMannequinContext;
		ICVar *m_pAnimationDatabase;
		ICVar *m_pControllerDefinition;
	};

public:
	CFireNetPlayer();
	virtual ~CFireNetPlayer();

	// ISimpleActor
	virtual bool              Init(IGameObject* pGameObject) override;
	virtual void              PostInit(IGameObject* pGameObject) override;
	virtual void              ProcessEvent(SEntityEvent& event) override;
	virtual void              Update(SEntityUpdateContext& ctx, int updateSlot) override;
	virtual void              SetHealth(float health) override;
	virtual float             GetHealth() const override { return m_bAlive ? GetMaxHealth() : 0.f; }
	// ~ISimpleActor

	CFireNetPlayerInput*      GetInput() const { return m_pInput; }
	CFireNetPlayerMovement*   GetMovement() const { return m_pMovement; }
	ISimpleWeapon*            GetCurrentWeapon() const { return m_pCurrentWeapon; }

	bool                      IsThirdPerson() const { return m_bThirdPerson; }
	void                      SetThirdPerson(bool mode) { m_bThirdPerson = mode; }

	const SExternalCVars&     GetCVars() const;
protected:
	void                      SelectSpawnPoint();
	void                      SetPlayerModel();
	void                      CreateWeapon(const char *name);
protected:
	CFireNetPlayerInput*      m_pInput;
	CFireNetPlayerMovement*   m_pMovement;
	CFireNetPlayerView*       m_pView;
	CFireNetPlayerAnimations* m_pAnimations;
	ISimpleWeapon*            m_pCurrentWeapon;

	bool                      m_bAlive;
	bool                      m_bThirdPerson;
};