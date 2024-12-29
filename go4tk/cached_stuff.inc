stock int CBasePlayer_hMyWeapons;
stock int CBasePlayer_hActiveWeapon;
stock int CBasePlayer_hLastWeapon;
stock int CBasePlayer_flMaxspeed;

stock int CBasePlayer_bSpotted;
stock int CBasePlayer_3d_fog_end;
stock int CBasePlayer_3d_fog_start;
stock int CBasePlayer_3d_fog_enable;
stock int CBasePlayer_3d_fog_maxdensity;
stock int CBasePlayer_aimPunchAngleVel;
stock int CBasePlayer_aimPunchAngle;
stock int CBasePlayer_iHealth;

stock int CBaseEntity_vecOrigin;
stock int CBaseEntity_angRotation;
stock int CBaseEntity_iName;

stock int CCSPlayer_iPrimaryAddon;
stock int CCSPlayer_iSecondaryAddon;
stock int CCSPlayer_iAddonBits;
stock int CCSPlayer_flNextAttack;
stock int CCSPlayer_bIsScoped;
stock int CCSPlayer_vecVelocity;
stock int CCSPlayer_iNumRoundKills;
stock int CCSPlayer_hObserverTarget;
//stock int CCSPlayer_bGunGameImmunity;

stock int CCSPlayerResource_bConnected;
stock int CCSPlayerResource_bAlive;
stock int CCSPlayerResource_iTeam;

stock int CDynamicProp_iTeamNum;
stock int CDynamicProp_nSequence;

stock int CBaseCombatWeapon_iPrimaryReserveAmmoCount;
stock int CBaseCombatWeapon_iClip1;
stock int CBaseCombatWeapon_hOwner;

stock int CWeaponCSBaseGun_bReloadVisuallyComplete;
stock int CWeaponCSBaseGun_weaponMode;
stock int CWeaponCSBaseGun_flNextPrimaryAttack;
stock int CWeaponCSBaseGun_flNextSecondaryAttack;
stock int CWeaponCSBaseGun_iItemDefinitionIndex;

stock int CFogController_fog_end;
stock int CFogController_fog_start;
stock int CFogController_fog_maxdensity;
stock int CFogController_fog_lerptime;
stock int CFogController_fog_farZ;

stock UserMsg g_msgHudMsg;
stock Handle g_hSwitchWeaponCall = null;

stock Handle g_hStartLagCompensationCall = null;
stock Handle g_hFinishLagCompensationCall = null;

stock Address g_hCLagCompensationManagerInstance;
stock Address g_hCLagCompensationManagerInstance_FinishCall;

bool USE_ARBITRARY_LAGCOMPENSATION = false;

stock int no_z_BeamModelIndex = -1;
int BeamModelIndex = -1;
int g_PlayerModelIndex = -1;
int g_iPunchModel;
int g_iv_BinocularModel;
int CCSPlayerResource_Manager = INVALID_ENT_REFERENCE;

//Handle g_iSvParallelPackentitiesCvar = null;

float SkyCameraOrigin[3];

#define PLAYER_T_MODEL "models/player/custom_player/legacy/tm_leet_variantb.mdl"

//Go4TK Sounds. W/o * CSGO hack because they are embedded in the BSP
enum Go4TK_MapSounds {
	Go4TK_AmbientSound,
	Go4TK_ParachuteLandSound,
	Go4TK_ParachuteFlySound,
	Go4TK_SafezoneShrinkSound,
	Go4TK_InfoSound
}

char map_pakked_sounds[Go4TK_MapSounds][] = {
	"Go4TK/ambientnew.wav",
	"Go4TK/parachute_land.mp3",
	"Go4TK/parachute_fly.mp3",
	"Go4TK/safezone_shrink.mp3",
	"Go4TK/info.mp3"
};

public void LoadNetPropOffsets() {
	//g_iSvParallelPackentitiesCvar = FindConVar("sv_parallel_packentities");

	CBasePlayer_hMyWeapons = FindSendPropInfo("CBasePlayer", "m_hMyWeapons");
	CBasePlayer_hActiveWeapon = FindSendPropInfo("CBasePlayer", "m_hActiveWeapon");
	CBasePlayer_hLastWeapon = FindSendPropInfo("CBasePlayer", "m_hLastWeapon");
	CBasePlayer_flMaxspeed = FindSendPropInfo("CBasePlayer", "m_flMaxspeed");
	
	CBasePlayer_bSpotted = FindSendPropInfo("CBasePlayer", "m_bSpotted");
	CBasePlayer_3d_fog_end = FindSendPropInfo("CBasePlayer", "m_skybox3d.fog.end");
	CBasePlayer_3d_fog_start = FindSendPropInfo("CBasePlayer", "m_skybox3d.fog.start");
	CBasePlayer_3d_fog_enable = FindSendPropInfo("CBasePlayer", "m_skybox3d.fog.enable");
	CBasePlayer_3d_fog_maxdensity = FindSendPropInfo("CBasePlayer", "m_skybox3d.fog.maxdensity");
	CBasePlayer_aimPunchAngleVel = FindSendPropInfo("CBasePlayer", "m_aimPunchAngleVel");
	CBasePlayer_aimPunchAngle = FindSendPropInfo("CBasePlayer", "m_aimPunchAngle");
	CBasePlayer_iHealth = FindSendPropInfo("CBasePlayer", "m_iHealth");
	
	CCSPlayer_iPrimaryAddon = FindSendPropInfo("CCSPlayer", "m_iPrimaryAddon");
	CCSPlayer_iSecondaryAddon = FindSendPropInfo("CCSPlayer", "m_iSecondaryAddon");
	CCSPlayer_iAddonBits = FindSendPropInfo("CCSPlayer", "m_iAddonBits");
	CCSPlayer_flNextAttack = FindSendPropInfo("CCSPlayer", "m_flNextAttack");
	CCSPlayer_bIsScoped = FindSendPropInfo("CCSPlayer", "m_bIsScoped");
	CCSPlayer_vecVelocity = FindSendPropInfo("CCSPlayer", "m_vecVelocity[0]");
	CCSPlayer_iNumRoundKills = FindSendPropInfo("CCSPlayer", "m_iNumRoundKills");
	CCSPlayer_hObserverTarget = FindSendPropInfo("CCSPlayer", "m_hObserverTarget");
	//CCSPlayer_bGunGameImmunity = FindSendPropInfo("CCSPlayer", "m_bGunGameImmunity");
	
	CBaseEntity_vecOrigin = FindSendPropInfo("CBaseEntity", "m_vecOrigin");
	CBaseEntity_angRotation = FindSendPropInfo("CBaseEntity", "m_angRotation");
	CBaseEntity_iName = FindSendPropInfo("CBaseEntity", "m_iName");
	
	CCSPlayerResource_bConnected = FindSendPropInfo("CCSPlayerResource", "m_bConnected");
	CCSPlayerResource_bAlive = FindSendPropInfo("CCSPlayerResource", "m_bAlive");
	CCSPlayerResource_iTeam = FindSendPropInfo("CCSPlayerResource", "m_iTeam");
	
	CDynamicProp_iTeamNum = FindSendPropInfo("CDynamicProp", "m_iTeamNum");
	CDynamicProp_nSequence = FindSendPropInfo("CDynamicProp", "m_nSequence");
	
	CBaseCombatWeapon_iPrimaryReserveAmmoCount = FindSendPropInfo("CBaseCombatWeapon", "m_iPrimaryReserveAmmoCount");
	CBaseCombatWeapon_iClip1 = FindSendPropInfo("CBaseCombatWeapon", "m_iClip1");
	CBaseCombatWeapon_hOwner = FindSendPropInfo("CBaseCombatWeapon", "m_hOwner");
	
	CWeaponCSBaseGun_bReloadVisuallyComplete = FindSendPropInfo("CWeaponCSBaseGun", "m_bReloadVisuallyComplete");
	CWeaponCSBaseGun_weaponMode = FindSendPropInfo("CWeaponCSBaseGun", "m_weaponMode");
	CWeaponCSBaseGun_flNextPrimaryAttack = FindSendPropInfo("CWeaponCSBaseGun", "m_flNextPrimaryAttack");
	CWeaponCSBaseGun_flNextSecondaryAttack = FindSendPropInfo("CWeaponCSBaseGun", "m_flNextSecondaryAttack");
	CWeaponCSBaseGun_iItemDefinitionIndex = FindSendPropInfo("CWeaponCSBaseGun", "m_iItemDefinitionIndex");
	
	CFogController_fog_end = FindSendPropInfo("CFogController", "m_fog.end");
	CFogController_fog_start = FindSendPropInfo("CFogController", "m_fog.start");
	CFogController_fog_maxdensity = FindSendPropInfo("CFogController", "m_fog.maxdensity");
	CFogController_fog_lerptime = FindSendPropInfo("CFogController", "m_fog.lerptime");
	CFogController_fog_farZ = FindSendPropInfo("CFogController", "m_fog.farz");
	
	g_msgHudMsg = GetUserMessageId("HudMsg");
	
	Handle hConfig = LoadGameConfigFile("sdkhooks.games");
	
	StartPrepSDKCall(SDKCall_Player);
	PrepSDKCall_SetFromConf(hConfig, SDKConf_Virtual, "Weapon_Switch");
	PrepSDKCall_AddParameter(SDKType_CBaseEntity, SDKPass_Pointer);
	PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Plain);
	g_hSwitchWeaponCall = EndPrepSDKCall();
	
	delete hConfig;
	
	
	hConfig = LoadGameConfigFile("go4tk");
	
	g_hCLagCompensationManagerInstance = GameConfGetAddress(hConfig, "CBaseEntity::UpdateOnRemove");
	
	if(g_hCLagCompensationManagerInstance == Address_Null)
		USE_ARBITRARY_LAGCOMPENSATION = true;
	
	PrintToServer("GameConfGetAddress UpdateOnRemove+offset: %x", g_hCLagCompensationManagerInstance);
	
	g_hCLagCompensationManagerInstance_FinishCall = g_hCLagCompensationManagerInstance + view_as<Address>(GameConfGetOffset(hConfig, "CBaseEntity::UpdateOnRemove_lagcompensation_finishoffset"));
	
	PrintToServer("GameConfGetAddress UpdateOnRemove LoadFromAddress for Finish: %x", g_hCLagCompensationManagerInstance_FinishCall);
	
	
	
	/*Address WorldSpawn = GetEntityAddress(0);
	
	if(WorldSpawn == Address_Null)
		SetFailState("Failed to get WorldSpawn Address");
	
	PrintToServer("WorldSpawn: %x", WorldSpawn);
	
	int WorldSpawnDeref = LoadFromAddress(WorldSpawn, NumberType_Int32);
	
	PrintToServer("WorldSpawnDeref: %x", WorldSpawnDeref);
																//Win: 109
	Address UpdateOnRemove = view_as<Address>(WorldSpawnDeref + (GameConfGetOffset(hConfig, "CBaseEntity::UpdateOnRemove") * 4));
	
	PrintToServer("UpdateOnRemove: %x", UpdateOnRemove);
	
	int UpdateOnRemoveDeref = LoadFromAddress(UpdateOnRemove, NumberType_Int32);
	
	PrintToServer("UpdateOnRemoveDeref: %x", UpdateOnRemoveDeref);
																				//Win: 29
	g_hCLagCompensationManagerInstance = view_as<Address>(UpdateOnRemoveDeref + (GameConfGetOffset(hConfig, "CBaseEntity::UpdateOnRemove_lagcompensation") * 4));
	
	PrintToServer("g_hCLagCompensationManagerInstance: %x", g_hCLagCompensationManagerInstance);*/
	
	
	StartPrepSDKCall(SDKCall_Raw);
	//PrepSDKCall_SetVirtual(0);
	
	PrepSDKCall_SetFromConf(hConfig, SDKConf_Signature, "StartLagCompensation");
	PrepSDKCall_AddParameter(SDKType_CBasePlayer, SDKPass_Pointer);
	PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Plain);
	PrepSDKCall_AddParameter(SDKType_Vector, SDKPass_ByRef);
	PrepSDKCall_AddParameter(SDKType_QAngle, SDKPass_ByRef);
	PrepSDKCall_AddParameter(SDKType_Float, SDKPass_Plain);
	g_hStartLagCompensationCall = EndPrepSDKCall();
	
	StartPrepSDKCall(SDKCall_Raw);
	//PrepSDKCall_SetVirtual(1);
	
	PrepSDKCall_SetFromConf(hConfig, SDKConf_Signature, "FinishLagCompensation");
	PrepSDKCall_AddParameter(SDKType_CBasePlayer, SDKPass_Pointer);
	g_hFinishLagCompensationCall = EndPrepSDKCall();
	
	if(g_hStartLagCompensationCall == null || g_hFinishLagCompensationCall == null)
		USE_ARBITRARY_LAGCOMPENSATION = true;
	
	if(USE_ARBITRARY_LAGCOMPENSATION)
		LogError("USING ARBITRARY LAGCOMPENSATION BECAUSE SOMETHING FAILED TO LOOK UP (Start OK: %i Finish OK: %i)", g_hStartLagCompensationCall != null, g_hFinishLagCompensationCall != null);
	else
		LogMessage("USING NATIVE LAGCOMPENSATION");
}

int g_iBulletImpactDecal = -1;

public void CacheNecessaryStuff() {
	//Since the bullet impact decal is precached, everything else *should* be as well
	if(!IsDecalPrecached("decals/brick/brick3") || g_iBulletImpactDecal == -1) {
		g_iBulletImpactDecal = PrecacheDecal("decals/brick/brick3", true);
		
		g_PlayerModelIndex = PrecacheModel(PLAYER_T_MODEL, true);
		
		g_iPunchModel = PrecacheModel("models/weapons/v_punchs.mdl", true);
		
		PrecacheModel("models/parachute/parachute_carbon.mdl", true);
		
		g_iv_BinocularModel = PrecacheModel("models/Go4TK/pickup/v_binoculars.mdl", true);
		//g_iw_BinocularModel = PrecacheModel("models/weapons/w_binoculars.mdl");
		no_z_BeamModelIndex = PrecacheModel("materials/sprites/radar.vmt", true);
		
		//BeamModelIndex = PrecacheModel("materials/sprites/laserbeam.vmt", true);
		
		BeamModelIndex = PrecacheModel("materials/sprites/cbbl_smoke.vmt", true);
		
		PrecacheModel("models/props/coop_cementplant/grenade_box/grenade_box_closed.mdl");
		
		PrecacheSound("hostage/huse/hostage_pickup.wav", true);
		PrecacheSound("player/bhit_helmet-1.wav", true);
		PrecacheSound("physics/plastic/plastic_box_break2.wav", true);
		
		PrecacheSound("SolidMetal.BulletImpact", true);
		PrecacheSound("Flesh.BulletImpact", true);
		
		PrecacheSounds("physics/metal/metal_solid_impact_bullet%i.wav", 4);
		PrecacheSounds("physics/flesh/flesh_impact_bullet%i.wav", 5);
		
		PrecacheSound("ambient/de_train_radio.wav", true);
		PrecacheSound("ambient/canals/ambience_canals_bell_bg.wav", true);
		PrecacheSound("ui/valve_logo_music.wav", true);
		
		//FakePrecacheSound("Go4TK/ambient.wav");
		//PrecacheSound("Go4TK/ambient.wav");
		//FakePrecacheSound("Go4TK/parachute_land.mp3");
		//PrecacheSound("Go4TK/parachute_land.mp3");
		//FakePrecacheSound("Go4TK/parachute_fly.mp3");
		//PrecacheSound("Go4TK/parachute_fly.mp3");
		//FakePrecacheSound("Go4TK/safezone_shrink.mp3");
		//PrecacheSound("Go4TK/safezone_shrink.mp3");
		//FakePrecacheSound("Go4TK/info.mp3");
		//PrecacheSound("Go4TK/info.mp3");
		
		for(int i = 0; i < sizeof(map_pakked_sounds); i++)
			PrecacheSound(map_pakked_sounds[i]);
		
		PrecacheSound("items/itempickup.wav", true);
		PrecacheSound("items/ammo_pickup.wav", true);
		PrecacheSound("weapons/ssg08/zoom.wav", true);
		PrecacheSound("weapons/sg556/sg556_zoom_in.wav", true);
		PrecacheSound("weapons/sg556/sg556_zoom_out.wav", true);
		
		//PrecacheSound("Concrete.BulletImpact", true);
		PrecacheSounds("physics/concrete/concrete_impact_bullet%i.wav", 4);
		
		PrecacheSounds("physics/metal/metal_chainlink_impact_soft%i.wav", 3);
		
		PrecacheSounds(">weapons/fx/nearmiss/bulletltor0%i.wav", 9, 6);
	
		//PrecacheSounds(")physics\body\body_medium_impact_hard%i.wav", 3, 6);
		
		PrecacheWeaponSounds();
	}
	
	if(CBaseEntity_vecOrigin == 0)
		return;
		
	CacheTpTriggers();
	
	GetEntDataVector(FindEntityByClassname(-1, "sky_camera"), CBaseEntity_vecOrigin, SkyCameraOrigin);
}

int tpTriggerCount = 0;
float[8][2][3] tpTriggerBounds;
float[8][3] tpTriggerLandmarkVecs;
float[8][3] tpTriggerDests;

//float[][3] tpTriggerTpOffsets;

void CacheTpTriggers() {
	float vecMins[3]; float vecMaxs[3]; float vecOrigin[3];
	
	int index = -1;
	tpTriggerCount = 0;
	
	while ((index = FindEntityByClassname(index, "trigger_teleport")) != -1) {
		//Get the Landmark ent of this TP trigger
		char entName[32]; int LM;
		
		GetEntPropString(index, Prop_Data, "m_iLandmark", entName, sizeof(entName));
		
		//If there is no landmark, this tp trigger is irrelevant to us.
		if(strlen(entName) <= 1)
			continue;
		
		LM = FindEntityByTargetname("info_landmark", entName);
		
		GetEntDataVector(index, CBaseEntity_vecOrigin, vecOrigin);
			
		GetEntPropVector(index, Prop_Send, "m_vecMins", vecMins);
		GetEntPropVector(index, Prop_Send, "m_vecMaxs", vecMaxs);

		tpTriggerBounds[tpTriggerCount][0][0] = vecMins[0] + vecOrigin[0];
		tpTriggerBounds[tpTriggerCount][0][1] = vecMins[1] + vecOrigin[1];
		tpTriggerBounds[tpTriggerCount][0][2] = vecMins[2] + vecOrigin[2];
		
		tpTriggerBounds[tpTriggerCount][1][0] = vecMaxs[0] + vecOrigin[0];
		tpTriggerBounds[tpTriggerCount][1][1] = vecMaxs[1] + vecOrigin[1];
		tpTriggerBounds[tpTriggerCount][1][2] = vecMaxs[2] + vecOrigin[2];
		
		GetEntDataVector(LM, CBaseEntity_vecOrigin, tpTriggerLandmarkVecs[tpTriggerCount]);
		
		//PrintToServer(">> %s %.2f %.2f %.2f", entName, tpTriggerLandmarkVecs[tpTriggerCount][0], tpTriggerLandmarkVecs[tpTriggerCount][1], tpTriggerLandmarkVecs[tpTriggerCount][2]);
		
		
		GetEntPropString(index, Prop_Data, "m_target", entName, sizeof(entName));
		
		//PrintToServer(">> %s %.2f %.2f %.2f", entName, tpTriggerDests[tpTriggerCount][0], tpTriggerDests[tpTriggerCount][0], tpTriggerDests[tpTriggerCount][0]);
		
		LM = FindEntityByTargetname("info_teleport_destination", entName);
		
		GetEntDataVector(LM, CBaseEntity_vecOrigin, tpTriggerDests[tpTriggerCount]);
		
		tpTriggerCount++;
	}
	
	if(tpTriggerCount != 8)
		LogError("INVALID AMOUNT OF TELEPORT TRIGGERS FOUND. Got %i expected 8", tpTriggerCount);
}

static char downloadables[][] = {
	"materials/models/Go4TK/pickup/FirstAidKit.vmt",
	"materials/models/Go4TK/pickup/FirstAidKit.vtf",
	"materials/models/Go4TK/pickup/gear_alice_backpack.vmt",
	"materials/models/Go4TK/pickup/gear_alice_backpack.vtf",
	"materials/models/Go4TK/pickup/gear_alice_backpack_n.vtf",
	"materials/models/Go4TK/pickup/gear_casual_backpack.vmt",
	"materials/models/Go4TK/pickup/gear_casual_backpack.vtf",
	"materials/models/Go4TK/pickup/gear_casual_backpack_n.vtf",
	
	"materials/models/parachute/pack_carbon.vmt",
	"materials/models/parachute/pack_carbon.vtf",
	"materials/models/parachute/parachute_carbon.vmt",
	"materials/models/parachute/parachute_carbon.vtf",
	
	"materials/models/Go4TK/pickup/helmet_pickup.vmt",
	"materials/models/Go4TK/pickup/helmet_pickup.vtf",
	"materials/models/Go4TK/pickup/helmet_pickup_n.vtf",
	"materials/models/Go4TK/pickup/makeshiftarmor_metal.vmt",
	"materials/models/Go4TK/pickup/makeshiftarmor_metal.vtf",
	"materials/models/Go4TK/pickup/makeshiftarmor_metal_n.vtf",
	"materials/models/Go4TK/pickup/Vest_torso_pickup.vmt",
	"materials/models/Go4TK/pickup/Vest_torso_pickup.vtf",
	"materials/models/Go4TK/pickup/Vest_torso_pickup_n.vtf",
	
	"materials/models/Go4TK/pickup/bino.vmt",
	"materials/models/Go4TK/pickup/bino.vtf",
	"materials/models/Go4TK/pickup/bino_n.vtf",
	"materials/models/Go4TK/pickup/lense.vmt",
	"materials/models/Go4TK/pickup/lense.vtf",
	"materials/models/Go4TK/pickup/lense_n.vtf",
	
	"materials/Go4TK/bleeding.vmt",
	"materials/Go4TK/bleeding.vtf",
	"materials/Go4TK/binocular.vmt",
	"materials/Go4TK/binocular.vtf",
	
	"models/parachute/parachute_carbon.dx90.vtx",
	"models/parachute/parachute_carbon.mdl",
	"models/parachute/parachute_carbon.sw.vtx",
	"models/parachute/parachute_carbon.vvd",
	
	"models/weapons/v_punchs.dx90.vtx",
	"models/weapons/v_punchs.mdl",
	"models/weapons/v_punchs.vvd",
	
	"models/Go4TK/pickup/FirstAidKit.dx90.vtx",
	"models/Go4TK/pickup/firstaidkit.mdl",
	"models/Go4TK/pickup/FirstAidKit.phy",
	"models/Go4TK/pickup/firstaidkit.vvd",
	"models/Go4TK/pickup/gear_alice_backpack.dx90.vtx",
	"models/Go4TK/pickup/gear_alice_backpack.mdl",
	"models/Go4TK/pickup/gear_alice_backpack.phy",
	"models/Go4TK/pickup/gear_alice_backpack.vvd",
	"models/Go4TK/pickup/gear_casual_backpack.dx90.vtx",
	"models/Go4TK/pickup/gear_casual_backpack.mdl",
	"models/Go4TK/pickup/gear_casual_backpack.phy",
	"models/Go4TK/pickup/gear_casual_backpack.vvd",
	"models/Go4TK/pickup/helmet_pickup.dx90.vtx",
	"models/Go4TK/pickup/helmet_pickup.mdl",
	"models/Go4TK/pickup/helmet_pickup.phy",
	"models/Go4TK/pickup/helmet_pickup.vvd",
	
	"models/Go4TK/pickup/376_ammo_pickup.dx90.vtx",
	"models/Go4TK/pickup/376_ammo_pickup.mdl",
	"models/Go4TK/pickup/376_ammo_pickup.phy",
	"models/Go4TK/pickup/376_ammo_pickup.vvd",
	"models/Go4TK/pickup/556_ammo_pickup.dx90.vtx",
	"models/Go4TK/pickup/556_ammo_pickup.mdl",
	"models/Go4TK/pickup/556_ammo_pickup.phy",
	"models/Go4TK/pickup/556_ammo_pickup.vvd",
	"models/Go4TK/pickup/shells_ammo_pickup.dx90.vtx",
	"models/Go4TK/pickup/shells_ammo_pickup.mdl",
	"models/Go4TK/pickup/shells_ammo_pickup.phy",
	"models/Go4TK/pickup/shells_ammo_pickup.vvd",
	"materials/models/Go4TK/pickup/shells_556_9mmstash.vmt",
	"materials/models/Go4TK/pickup/shells_556_9mmstash.vtf",
	"materials/models/Go4TK/pickup/shells_556_9mmstash_mask.vtf",
	
	"models/Go4TK/pickup/makeshift_armor_pickup.dx90.vtx",
	"models/Go4TK/pickup/makeshift_armor_pickup.mdl",
	"models/Go4TK/pickup/makeshift_armor_pickup.phy",
	"models/Go4TK/pickup/makeshift_armor_pickup.vvd",
	"models/Go4TK/pickup/Vest_torso_pickup.dx90.vtx",
	"models/Go4TK/pickup/vest_torso_pickup.mdl",
	"models/Go4TK/pickup/Vest_torso_pickup.phy",
	"models/Go4TK/pickup/vest_torso_pickup.vvd",
	"models/Go4TK/pickup/Vest_torso_folded.dx90.vtx",
	"models/Go4TK/pickup/vest_torso_folded.mdl",
	"models/Go4TK/pickup/Vest_torso_folded.phy",
	"models/Go4TK/pickup/vest_torso_folded.vvd",
	"models/Go4TK/pickup/v_binoculars.dx90.vtx",
	
	"models/Go4TK/pickup/v_binoculars.mdl",
	"models/Go4TK/pickup/v_binoculars.vvd",
	
	
	"models/Go4TK/pickup/v_binoculars.mdl",
	"models/Go4TK/pickup/v_binoculars.vvd"/*,
	
	"materials/models/Go4TK/vehicle/777_engine.vmt",
	"materials/models/Go4TK/vehicle/777_fuselage-1_normal.vtf",
	"materials/models/Go4TK/vehicle/777_fuselage-3.vtf",
	"materials/models/Go4TK/vehicle/777_gear_normal.vmt",
	"materials/models/Go4TK/vehicle/777_engine.vtf",
	"materials/models/Go4TK/vehicle/777_fuselage-2.vmt",
	"materials/models/Go4TK/vehicle/777_fuselage-3_normal.vmt",
	"materials/models/Go4TK/vehicle/777_gear_normal.vtf",
	"materials/models/Go4TK/vehicle/777_engine_normal.vtf",
	"materials/models/Go4TK/vehicle/777_fuselage-2.vtf",
	"materials/models/Go4TK/vehicle/777_fuselage-3_normal.vtf",
	"materials/models/Go4TK/vehicle/777_wings-1.vmt",
	"materials/models/Go4TK/vehicle/777_fuselage-1.vmt",
	"materials/models/Go4TK/vehicle/777_fuselage-2_normal.vtf",
	"materials/models/Go4TK/vehicle/777_gear.vmt",
	"materials/models/Go4TK/vehicle/777_wings-1.vtf",
	"materials/models/Go4TK/vehicle/777_fuselage-1.vtf",
	"materials/models/Go4TK/vehicle/777_fuselage-3.vmt",
	"materials/models/Go4TK/vehicle/777_gear.vtf",
	"materials/models/Go4TK/vehicle/777_wings-1_normal.vtf"*/
	//Is packed into the map file
	/*"sound/Go4TK/ambient.wav",
	"sound/Go4TK/info.mp3",
	"sound/Go4TK/parachute_fly.mp3",
	"sound/Go4TK/parachute_land.mp3",
	"sound/Go4TK/safezone_shrink.mp3",*/
	
	//Jpegs in the dl table aint work
	//"maps/lul5.jpg"
};

public void AddCustomStuffToDownloadtable() {
	//Add stuff to download table
	for(int i = 0; i < sizeof(downloadables); i++)
		AddFileToDownloadsTable(downloadables[i]);
}