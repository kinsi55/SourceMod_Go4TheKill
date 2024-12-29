#define vAs(%1,%2) view_as<%1>(%2)
#define AsInt(%1) vAs(int, %1)

#if defined DEBUG
	int COMPILED_WITH_DEBUG;
#endif
#if defined NOWIN
	int COMPILED_WITH_NOWIN;
#endif
#if defined NOSAFEZONE
	int COMPILED_WITH_NOSAFEZONE;
#endif
#if defined INSECURE_WS
	int COMPILED_WITH_INSECURE_WS;
#endif
#if defined NOFOG
	int COMPILED_WITH_NOFOG;
#endif
#if defined ALLRANKED
	int COMPILED_WITH_ALLRANKED;
#endif
#if defined NO_QUEUE_CHECK
	int COMPILED_WITH_NO_QUEUE_CHECK;
#endif
#if defined NOWSWARN
	int COMPILED_WITH_NOWSWARN;
#endif
#if defined NOAUTOSTART
	int COMPILED_WITH_NOAUTOSTART;
#endif
#if defined EVERYONE_CAN_SPEC
	int COMPILED_WITH_EVERYONE_CAN_SPEC;
#endif
#if defined NOWS
	int COMPILED_WITH_NOWS;
#endif
#if defined NO_DBMOD
	int COMPILED_WITH_NO_DBMOD;
#endif

char RadioCMDS[][] = {"coverme", "takepoint", "holdpos", "regroup", "followme", "takingfire", "go", "fallback", "sticktog",
						"getinpos", "stormfront", "report", "roger", "enemyspot", "needbackup", "sectorclear", "inposition", "reportingin",
						"getout", "negative","enemydown", "compliment", "thanks", "cheer"};

int g_CurrentEntityCount = 0;

public void OnEntityCreated(int entity, const char[] classname) {
	g_CurrentEntityCount++;
}
public void OnEntityDestroyed(int entity) {
	g_CurrentEntityCount--;
}

void Debug(const char[] myString, any ...) {
	#if defined DEBUG
		int len = strlen(myString) + 255;
		char[] myFormattedString = new char[len];
		VFormat(myFormattedString, len, myString, 2);

		PrintToServer(myFormattedString);
	#endif
}

Handle PLAYER_BINOCULAR_TIMER[MAXPLAYERS+1];

public Action DisplayBinocularForClient(Handle timer, int client) {
	PLAYER_BINOCULAR_TIMER[client] = null;
	if(IsValidClient(client) && IsPlayerAlive(client)) {
		SetClientFov(client, BINOCULAR_FOV, 0.01);

		HideHud(client, HIDEHUD_CROSSHAIR);

		Debug("%N into binocular", client);

		SetEntData(client, CCSPlayer_bIsScoped, 1, 1, true);
		ChangeEdictState(client, CCSPlayer_bIsScoped);
		PLAYER_IS_IN_BINOCULAR[client] = true;

		SetClientOverlay(client);

		if(IsValidEntity(g_PVMid[client]))
			//SetEdictFlags(g_PVMid[client], FL_EDICT_DONTSEND);
			SetEntProp(g_PVMid[client], Prop_Send, "m_nModelIndex", -1);
	}
}

int PLAYER_FOG_CONTROLLERS[MAXPLAYERS+1] = {-1, ...};

public Action FogTimer(Handle timer) {
	LoopIngameClients(i)
		if(!IsFakeClient(i))
			UpdateFogForClient(i, true);
}

float g_lastPlayerFogRange[MAXPLAYERS+1] = 0.0;

void UpdateFogForClient(int client, bool fadeOver = true) {
	if(PLAYER_FOG_CONTROLLERS[client] != -1 && IsValidEdict(PLAYER_FOG_CONTROLLERS[client])) {
		float playerOrigin[3];
		GetEntDataVector(client, CBaseEntity_vecOrigin, playerOrigin);

		//float k16 = 16384.0;
		float k2 = 0.0;

		#define minFog 0.35

		GetLogicalCoordFromPhysical(playerOrigin, playerOrigin);

		float x = FloatAbs(30720.0 - playerOrigin[1]);
		float y = FloatAbs(30720.0 - playerOrigin[0]);

		//gets the BIGGER of the two
		float maxDist = Math_Max(x, y);



		//PrintToChatAll("Maxdist: %.2f (%.2f %.2f)", maxDist, x, y);
		//float x = k16 - FloatAbs(playerOrigin[1]);
		//float y = k16 - FloatAbs(playerOrigin[0]);

		//gets the BIGGER of the two
		//float maxDist = Math_Max(x, y);


		if(userSettings[client][CFG_highFpsMode] && maxDist > HIGHFPS_VIEWDISTANCE)
			maxDist = HIGHFPS_VIEWDISTANCE - 1024.0;

		float n = ((k16 - (maxDist - k2)) / k16) * 0.93;
		if(n < minFog) n = minFog;// if(n > 0.92 || userSettings[client][CFG_highFpsMode]) n = 0.92;

		if(userSettings[client][CFG_highFpsMode])
			n = 0.97;

		//Make fog a liiittle bit less dense when parachuting to give a bit more vision
		if(plIsCurrentlyParachuting[client] && n > 0.85)
			n = 0.85;

		//wenn der Absolute wert > MIN liegt darf der fog bis zu 2048.0 close sein, ansonsten 4096
		float fogEnd = Math_Min(maxDist - k2, 4096.0 * ((1.001 - n) / 0.2));

		//Fog-Start sollte 2048U vom Ende entfernt sein
		//float fogStart = fogEnd - k2;

		fogEnd = Math_Max(fogEnd * 2.0, k16);

		g_lastPlayerFogRange[client] = fogEnd;

		//fogStart = 0.0; //Math_Max(fogStart / 2, 14336.0);

		//Debug("%.2f %.2f", fogEnd, fogStart);

		/*Member: m_fog.colorPrimaryLerpTo (offset 912) (type integer) (bits 32) (Unsigned)
		Member: m_fog.colorSecondaryLerpTo (offset 916) (type integer) (bits 32) (Unsigned)
		Member: m_fog.startLerpTo (offset 936) (type float) (bits 0) (NoScale)
		Member: m_fog.endLerpTo (offset 940) (type float) (bits 0) (NoScale)
		Member: m_fog.maxdensityLerpTo (offset 944) (type float) (bits 0) (NoScale)
		Member: m_fog.lerptime (offset 948) (type float) (bits 0) (NoScale)
		Member: m_fog.duration (offset 952) (type float) (bits 0) (NoScale)*/

		//SetEntDataFloat(PLAYER_FOG_CONTROLLERS[i], CFogController_fog_end, fogEnd, true);
		//SetEntDataFloat(PLAYER_FOG_CONTROLLERS[i], CFogController_fog_start, fogStart, true);
		//SetEntDataFloat(PLAYER_FOG_CONTROLLERS[i], CFogController_fog_maxdensity, n, true);
		//SetEntDataFloat(PLAYER_FOG_CONTROLLERS[i], CFogController_fog_lerptime, 0.1, true);

		//SetEntDataFloat(PLAYER_FOG_CONTROLLERS[i], CFogController_fog_lerptime, 1.0, true);
		//SetEntPropFloat(PLAYER_FOG_CONTROLLERS[i], Prop_Send, "m_fog.startLerpTo", fogStart);

		//SetEntDataFloat(PLAYER_FOG_CONTROLLERS[i], CFogController_fog_start, GetEntPropFloat(PLAYER_FOG_CONTROLLERS[i], Prop_Send, "m_fog.startLerpTo"), true);
		//SetEntDataFloat(PLAYER_FOG_CONTROLLERS[i], CFogController_fog_end, GetEntPropFloat(PLAYER_FOG_CONTROLLERS[i], Prop_Send, "m_fog.endLerpTo"), true);
		//SetEntDataFloat(PLAYER_FOG_CONTROLLERS[i], CFogController_fog_maxdensity, GetEntPropFloat(PLAYER_FOG_CONTROLLERS[i], Prop_Send, "m_fog.maxdensityLerpTo"), true);
		//SetEntPropFloat(PLAYER_FOG_CONTROLLERS[i], Prop_Send, "m_fog.endLerpTo", fogEnd);
		//SetEntPropFloat(PLAYER_FOG_CONTROLLERS[i], Prop_Send, "m_fog.maxdensityLerpTo", n);

		if(fadeOver && IsPlayerAlive(client)) {
			SetVariantFloat(fogEnd);
			AcceptEntityInput(PLAYER_FOG_CONTROLLERS[client], "SetEndDistLerpTo");
			SetVariantFloat(n);
			AcceptEntityInput(PLAYER_FOG_CONTROLLERS[client], "SetMaxDensityLerpTo");
			SetVariantFloat(-200.0);
			AcceptEntityInput(PLAYER_FOG_CONTROLLERS[client], "SetStartDistLerpTo");

			AcceptEntityInput(PLAYER_FOG_CONTROLLERS[client], "StartFogTransition");
		} else {
			SetEntDataFloat(PLAYER_FOG_CONTROLLERS[client], CFogController_fog_end, fogEnd, true);
			SetEntDataFloat(PLAYER_FOG_CONTROLLERS[client], CFogController_fog_maxdensity, n, true);

			SetEntDataFloat(PLAYER_FOG_CONTROLLERS[client], CFogController_fog_start, -100.0, true);

			//SetEntPropFloat(PLAYER_FOG_CONTROLLERS[client], Prop_Send, "m_fog.endLerpTo", fogEnd);
			//SetEntPropFloat(PLAYER_FOG_CONTROLLERS[client], Prop_Send, "m_fog.maxdensityLerpTo", n);
		}

		//Debug("%.2f %.2f", GetEntDataFloat(PLAYER_FOG_CONTROLLERS[i], CFogController_fog_lerptime), GetGameTime());

		//SetEntDataFloat(PLAYER_FOG_CONTROLLERS[i], CFogController_fog_lerptime, GetGameTime() + 0.1, true);

		DoForEverySpectatingClientAndSelf(client, toDoOn)
			SetEntDataFloat(toDoOn, CBasePlayer_3d_fog_end, fogEnd, true);

		if(n < minFog)
			n = minFog + 0.01;

		DoForEverySpectatingClientAndSelf(client, toDoOn)
			SetEntDataFloat(toDoOn, CBasePlayer_3d_fog_maxdensity, n, true);
	}
}

public Action UserMessageHook2(UserMsg msg_hd, Handle pb, const int[] players, int playersNum, bool reliable, bool init) {
	PbReadString(pb, "params", sumTmpStr, sizeof(sumTmpStr), 0);

	if(StrEqual(sumTmpStr, "#SFUI_Notice_YouDroppedWeapon") ||
	   StrEqual(sumTmpStr, "#SFUI_Notice_Got_Bomb") ||
	   StrEqual(sumTmpStr, "#Game_connected") ||
	   StrEqual(sumTmpStr, "#SFUI_Notice_Game_will_restart_in") ||
	   StrContains(sumTmpStr, "#Player_Cash_Award_Killed_Enemy") >= 0)
		return Plugin_Stop;

	Debug("%s", sumTmpStr);
	return Plugin_Continue;
}

//TODO Checken ob der Spieler 2 oder Mehr von der Selben Waffe hat, und wenn das Duplikat schon in der Hand ist versuchen eine andere Waffe darzustellen
stock void CalculateAddonBits(int client, int curWeap, int weapArr[3]) {
	weapArr[0] = 0; weapArr[1] = 0;
	//int newPrim = -1;
	//int newSec = -1;

	for (int i = 1; i <= 3; i++) {
		int iEnt = GetPlayerFakeWeaponSlotWeapon(client, i);
		if(IsValidEntity(iEnt)) {
			if(iEnt != curWeap) {
				int defIndex = GetEntData(iEnt, CWeaponCSBaseGun_iItemDefinitionIndex);

				weapArr[defIndex == WEAPON_HKP2000 ? 1 : 0] = defIndex;
			}
		}
    }
	//Calc addonbits
	weapArr[2] = (weapArr[0] != 0 ? ADDONBIT_PRIMARY : 0);
	if (weapArr[1] != 0) weapArr[2] |= ADDONBIT_PISTOL;
}

#define HI_FL 99999999.0

stock void BlockSecondary(int weapon) {
	if(IsValidEntity(weapon))
		SetEntDataFloat(weapon, CWeaponCSBaseGun_flNextSecondaryAttack, HI_FL, true);
}

stock void HudMsgToAll(int iChannel,
			const float fPosition[2],
			const int iColor1[4],
			const int iColor2[4],
			int iEffect = 0,
			float fFadeInTime = 0.0,
			float fFadeOutTime = 0.0,
			float fHoldTime = 1.0,
			float fEffectTime = 0.0,
			const char[] szText, any:...) {

	char szBuffer[512];
	VFormat(szBuffer, sizeof(szBuffer), szText, 11);

	Handle hMessage = StartMessageAll("HudMsg");
	PbSetInt(hMessage, "channel", iChannel);
	PbSetVector2D(hMessage, "pos", fPosition);
	PbSetColor(hMessage, "clr1", iColor1);
	PbSetColor(hMessage, "clr2", iColor2);
	PbSetInt(hMessage, "effect", iEffect);
	PbSetFloat(hMessage, "fade_in_time", fFadeInTime);
	PbSetFloat(hMessage, "fade_out_time", fFadeOutTime);
	PbSetFloat(hMessage, "hold_time", fHoldTime);
	PbSetFloat(hMessage, "fx_time", fEffectTime);
	PbSetString(hMessage, "text", szBuffer);
	EndMessage();

	if(iChannel == HUDMSG_CHANNEL_GAMEINFOS)
		EmitSoundToAll(map_pakked_sounds[Go4TK_InfoSound]);
}

static int iClients[1];

stock void HudMsg(int iClient,
			int iChannel,
			const float fPosition[2],
			const int iColor1[4],
			const int iColor2[4],
			int iEffect = 0,
			float fFadeInTime = 0.0,
			float fFadeOutTime = 0.0,
			float fHoldTime = 1.0,
			float fEffectTime = 0.0,
			const char[] szText, any:...) {

	char szBuffer[512];
	VFormat(szBuffer, sizeof(szBuffer), szText, 12);

	iClients[0] = iClient;

	Handle hMessage = StartMessageEx(g_msgHudMsg, iClients, 1);
	PbSetInt(hMessage, "channel", iChannel);
	PbSetVector2D(hMessage, "pos", fPosition);
	PbSetColor(hMessage, "clr1", iColor1);
	PbSetColor(hMessage, "clr2", iColor2);
	PbSetInt(hMessage, "effect", iEffect);
	PbSetFloat(hMessage, "fade_in_time", fFadeInTime);
	PbSetFloat(hMessage, "fade_out_time", fFadeOutTime);
	PbSetFloat(hMessage, "hold_time", fHoldTime);
	PbSetFloat(hMessage, "fx_time", fEffectTime);
	PbSetString(hMessage, "text", szBuffer);
	EndMessage();
}

stock void SetClientOverlay(int client, int destClient = -1) {
	sumTmpStr = "r_screenoverlay \"\"";

	if(IsPlayerAlive(client)) {
		if(PLAYER_IS_IN_BINOCULAR[client])
			sumTmpStr = "r_screenoverlay \"Go4TK/binocular\"";
		else if(HM_PlayerIsBleading(client))
		sumTmpStr = "r_screenoverlay \"Go4TK/bleeding\"";
	}

	if(destClient != -1)
		ClientCommand(destClient, sumTmpStr);
	else
		DoForEverySpectatingClientAndSelf(client, toDoOn)
			ClientCommand(toDoOn, sumTmpStr);
}

stock bool TraceDown(const float startPos[3], float endPos[3] = NULL_VECTOR, int RAY_MASK = MASK_VISIBLE, int notToHit = INVALID_ENT_REFERENCE) {
	Handle trace;
	if(notToHit == INVALID_ENT_REFERENCE)
		trace = TR_TraceRayEx(startPos, view_as<float>({90.0, 0.0, 0.0}), RAY_MASK, RayType_Infinite);
	else
		trace = TR_TraceRayFilterEx(startPos, view_as<float>({90.0, 0.0, 0.0}), RAY_MASK, RayType_Infinite, TRDontHitSelf, notToHit);

	if(trace != null && TR_DidHit(trace)) {
		TR_GetEndPosition(endPos, trace);
		delete trace;
		return true;
	}
	return false;
}

stock bool SendYouDroppedMessage(int client, const char[] message) {
	Handle buf = StartMessageOne("VotePass", client);
	if(buf != null) {
		PbSetInt(buf, "team", GetClientTeam(client));
		//PbSetInt(buf, "vote_type", 0);
		PbSetString(buf, "disp_str", "SFUI_Notice_YouDroppedWeapon");
		PbSetString(buf, "details_str", message);
		EndMessage();
		return true;
	}
	return false;
}

stock float FloatMod(float a, float b)
{
	while(a > b)
		a -= b;
	return a;
}

#define TP_TRIGGER_WIDTH 2048.0 //+21 to calculate in half the used overlap-width

enum MAP_LAYERS {
	ML_BIG_MILITARY,
	ML_AIRPORT,
	ML_SMALL_MILITARY,
	ML_CITY
}

stock MAP_LAYERS GetOriginLayer(const float theOrigin[3]) {
	if(theOrigin[2] <= -8236.0) { //City
		return ML_CITY;
	} else if(theOrigin[2] <= -132.0) { //Kleine military
		return ML_SMALL_MILITARY;
	} else if(theOrigin[2] <= 7971.0) { //Flughafen
		return ML_AIRPORT;
	} else if(theOrigin[2] <= 16075.0) { //Military base
		return ML_BIG_MILITARY;
	} else {
		return ML_SMALL_MILITARY;
	}
}

stock MAP_LAYERS GetOriginLayerFromLogical(const float theOrigin[3]) {
	if(theOrigin[0] <= 29830.8 && theOrigin[1] <= 29830.8) { //City
		return ML_AIRPORT;
	} else if(theOrigin[0] >= 29830.8 && theOrigin[1] <= 29830.8) { //Kleine military
		return ML_BIG_MILITARY;
	} else if(theOrigin[0] >= 29830.8 && theOrigin[1] >= 29830.8) { //Flughafen
		return ML_CITY;
	} else if(theOrigin[0] <= 29830.8 && theOrigin[1] >= 29830.8) { //Military base
		return ML_SMALL_MILITARY;
	} else {
		return ML_SMALL_MILITARY;
	}
}

stock MAP_LAYERS GetCurrentPlayerLayer(int client) {
	float tmpOrigin[3];

	GetEntDataVector(client, CBaseEntity_vecOrigin, tmpOrigin);

	return GetOriginLayer(tmpOrigin);
}

stock int GetLogicalCoordFromPhysical(const float absolutePos[3], float relativePos[3]) {
	//16075 -> 8236
	//7971 -> 512
	//-132 -> -7971
	//-8236 -> -16075

	int PlLayer = 0;

	float yHeight = absolutePos[2];

	relativePos = absolutePos;

	relativePos[0] += 17220.0;
	relativePos[1] -= 17220.0;

	relativePos[2] += 16384.0 - 304.0;

	relativePos[2] = FloatMod(relativePos[2], 8104.0);

	if(yHeight <= -8236.0) { //City
		relativePos[0] += 32768.0 - TP_TRIGGER_WIDTH;
		relativePos[1] += TP_TRIGGER_WIDTH;
	} else if(yHeight <= -132.0) { //Kleine military
		relativePos[0] += TP_TRIGGER_WIDTH;
		relativePos[1] += TP_TRIGGER_WIDTH;
		PlLayer = 1;
	} else if(yHeight <= 7971.0) { //Flughafen
		relativePos[0] += TP_TRIGGER_WIDTH;
		relativePos[1] += 32768.0 - TP_TRIGGER_WIDTH;
		PlLayer = 2;
	} else if(yHeight <= 16075.0) { //Military base
		relativePos[0] += 32768.0 - TP_TRIGGER_WIDTH;
		relativePos[1] += 32768.0 - TP_TRIGGER_WIDTH;
		PlLayer = 3;
	}

	relativePos[0] -= 32768.0;
	relativePos[1] *= -1;
	//Act as if our world is bigger, and calculate it down to the actual size.
	relativePos[0] *= 34816.0 / 30720.0;
	relativePos[1] *= 34816.0 / 30720.0;

	relativePos[0] += 30720.0;
	relativePos[1] += 30720.0;

	//It was at this moment that i knew, i just dont want to debug this shit any more. This bare number fixes the problem.
	relativePos[0] *= 0.942;
	relativePos[1] *= 0.942;

	return PlLayer;
}

stock void GetPhysicalCoordFromLogical(const float absolutePos[3], float relativePos[3]) {
	//16075 -> 8236
	//7971 -> 512
	//-132 -> -7971
	//-8236 -> -16075

	relativePos = absolutePos;

	//It was at this moment that i knew, i just dont want to debug this shit any more. This bare number fixes the problem.
	relativePos[0] /= 0.942;
	relativePos[1] /= 0.942;

	relativePos[0] -= 30720.0;
	relativePos[1] -= 30720.0;

	relativePos[0] *= 30720.0 / 34816.0;
	relativePos[1] *= 30720.0 / 34816.0;

	relativePos[0] += 32768.0;
	relativePos[1] += 32768.0;

	if(relativePos[0] > 32768.0 && relativePos[1] > 32768.0) { //City
		relativePos[0] -= 32768.0 - TP_TRIGGER_WIDTH;
		relativePos[1] -= 32768.0 - TP_TRIGGER_WIDTH;
	} else if(relativePos[0] < 32768.0 && relativePos[1] > 32768.0) { //Kleine military
		relativePos[0] -= TP_TRIGGER_WIDTH;
		relativePos[1] -= 32768.0 - TP_TRIGGER_WIDTH;

		relativePos[2] += 8104.0;
	} else if(relativePos[0] < 32768.0 && relativePos[1] < 32768.0) { //Flughafen
		relativePos[0] -= TP_TRIGGER_WIDTH;
		relativePos[1] -= TP_TRIGGER_WIDTH;

		relativePos[2] += 8104.0 * 2;
	} else if(relativePos[0] > 32768.0 && relativePos[1] < 32768.0) { //Military base
		relativePos[0] -= 32768.0 - TP_TRIGGER_WIDTH;
		relativePos[1] -= TP_TRIGGER_WIDTH;

		relativePos[2] += 8104.0 * 3;
	}

	relativePos[0] -= 16720.0;
	relativePos[1] = (relativePos[1] - 16720.0) * -1;

	relativePos[2] -= 16384.0 - 304.0;
}

stock void GetRandomSpawnableSkyVectorLogical(float worldOrigin[3], int boundary = 4096) {
	worldOrigin[0] = float(GetRandomInt(boundary, 60416 - (boundary*2)));
	worldOrigin[1] = float(GetRandomInt(boundary, 60416 - (boundary*2)));
	worldOrigin[2] = 7650.0;
}

stock void GetRandomSpawnableSkyVector(float worldOrigin[3], int boundary = 4096) {
	GetRandomSpawnableSkyVectorLogical(worldOrigin, boundary);

	GetPhysicalCoordFromLogical(worldOrigin, worldOrigin);
}

stock void GetWorldPointIn3dSkybox(const float worldOrigin[3], float skyOrigin[3]) {
	skyOrigin = worldOrigin;

	ScaleVector(skyOrigin, 1.0/16.0);
	AddVectors(skyOrigin, SkyCameraOrigin, skyOrigin);
}

stock void GetPhysicalVectorOnAllLaysersFromLogicalPosition(float LogicalOrigin[3], float PhysicalOrigins[MAP_LAYERS][3]) {
	GetPhysicalCoordFromLogical(LogicalOrigin, PhysicalOrigins[0]);

	GetPhysicalVectorOnAllLaysersFromPhysicalPosition(PhysicalOrigins[0], PhysicalOrigins);
}

stock void GetPhysicalVectorOnAllLaysersFromPhysicalPosition(const float PhysicalOrigin[3], float PhysicalOrigins[MAP_LAYERS][3]) {
	//First get the Physical Origin from the Logical one
	float yHeight = PhysicalOrigin[2];

	for(int i = 0; i < 4; i++) {
		PhysicalOrigins[i] = PhysicalOrigin;
	}

	//In relation to the current physical origin, calculate the logical (physically resulting) one for all other layers
	static float twoTrig;
	twoTrig = 2.0*TP_TRIGGER_WIDTH;

	if(yHeight <= -8236.0) { //City
		PhysicalOrigins[ML_BIG_MILITARY][1] -= 32768.0 - twoTrig;

		PhysicalOrigins[ML_AIRPORT][0] += 32768.0 - twoTrig;
		PhysicalOrigins[ML_AIRPORT][1] -= 32768.0 - twoTrig;

		PhysicalOrigins[ML_SMALL_MILITARY][0] += 32768.0 - twoTrig;

		PhysicalOrigins[ML_BIG_MILITARY][2] += 8104.0 * 3.0;
		PhysicalOrigins[ML_AIRPORT][2] += 8104.0 * 2.0;
		PhysicalOrigins[ML_SMALL_MILITARY][2] += 8104.0;
	} else if(yHeight <= -132.0) { //Kleine military
		PhysicalOrigins[ML_AIRPORT][1] -= 32768.0 - twoTrig;

		PhysicalOrigins[ML_BIG_MILITARY][0] -= 32768.0 - twoTrig;
		PhysicalOrigins[ML_BIG_MILITARY][1] -= 32768.0 - twoTrig;

		PhysicalOrigins[ML_CITY][0] -= 32768.0 - twoTrig;

		PhysicalOrigins[ML_BIG_MILITARY][2] += (8104.0 * 2.0);
		PhysicalOrigins[ML_AIRPORT][2] += 8104.0;
		PhysicalOrigins[ML_CITY][2] -= 8104.0;
	} else if(yHeight <= 7971.0) { //Flughafen
		PhysicalOrigins[ML_BIG_MILITARY][0] -= 32768.0 - twoTrig;

		PhysicalOrigins[ML_CITY][0] -= 32768.0 - twoTrig;
		PhysicalOrigins[ML_CITY][1] += 32768.0 - twoTrig;

		PhysicalOrigins[ML_SMALL_MILITARY][1] += 32768.0 - twoTrig;

		PhysicalOrigins[ML_BIG_MILITARY][2] += 8104.0;
		PhysicalOrigins[ML_SMALL_MILITARY][2] -= 8104.0;
		PhysicalOrigins[ML_CITY][2] -= (8104.0 * 2);

	} else if(yHeight <= 16075.0) { //Military base
		PhysicalOrigins[ML_AIRPORT][0] += 32768.0 - twoTrig;

		PhysicalOrigins[ML_SMALL_MILITARY][0] += 32768.0 - twoTrig;
		PhysicalOrigins[ML_SMALL_MILITARY][1] += 32768.0 - twoTrig;

		PhysicalOrigins[ML_CITY][1] += 32768.0 - twoTrig;


		PhysicalOrigins[ML_AIRPORT][2] -= 8104.0;
		PhysicalOrigins[ML_SMALL_MILITARY][2] -= (8104.0 * 2.0);
		PhysicalOrigins[ML_CITY][2] -= (8104.0 * 3.0);
	}
}

int playerParachuteModels[MAXPLAYERS + 1] = {INVALID_ENT_REFERENCE, ...};

stock void GivePlayerParachute(int client, bool remove = false) {
	if(IsPlayerAlive(client)) {
		plIsCurrentlyParachuting[client] = !remove;

		if(!remove) {
			SetEntProp(client, Prop_Data, "m_CollisionGroup", COLLISION_GROUP_DEBRIS_TRIGGER);

			//Spawn parachute
			int para = CreateEntityByName("prop_dynamic_override");

			if(IsValidEdict(para)) {
				DispatchKeyValue(para, "model", "models/parachute/parachute_carbon.mdl");
				DispatchKeyValue(para, "solid", "0");

				float tmpRotation[3];
				GetEntDataVector(client, CBaseEntity_vecOrigin, tmpOrigin);
				GetEntDataVector(client, CBaseEntity_angRotation, tmpRotation);

				tmpRotation[0] = tmpRotation[2] = 0.0;

				TeleportEntity(para, tmpOrigin, tmpRotation, NULL_VECTOR);

				DispatchSpawn(para);

				SetVariantString("!activator");

				AcceptEntityInput(para, "SetParent", client);

				playerParachuteModels[client] = EntIndexToEntRef(para);
			}
		} else {
			if(IsValidEntity(playerParachuteModels[client])) {
				RemoveEdict(playerParachuteModels[client]);

				playerParachuteModels[client] = INVALID_ENT_REFERENCE;
			}
			EmitSoundToClient(client, map_pakked_sounds[Go4TK_ParachuteLandSound]);

			EmitSoundToClient(client, map_pakked_sounds[Go4TK_ParachuteFlySound], client, SNDCHAN_STATIC, _, SND_STOP | SND_CHANGEVOL, 0.0);
			//EmitSoundToClient(i, mapMusic, mapMusicEntity, SNDCHAN_STATIC, _, SND_CHANGEVOL, 0.08)
		}
	}
}

public Action Command_Block(int client, const char[] command, int argc) {
	#if !defined DEBUG
		return Plugin_Stop;
	#endif
}

public Action Event_BlockBroadcast(Event event, const char[] name, bool dontBroadcast){
	#if !defined DEBUG
		event.BroadcastDisabled = true;
		return Plugin_Changed;
	#endif
}

public Action ConCommand_Block(int client, int args) {
	#if !defined DEBUG
		return Plugin_Stop;
	#endif
}

public Action Event_ServerCvar(Handle event, const char[] name, bool dontBroadcast) {
	#if !defined DEBUG
		return Plugin_Stop;
	#endif
}

int Calculate_Points_For_Kill(int client, bool wasHeadshot = false) {
	if(!IsValidClient(client))
		return 0;

	int ap = 0;
	LoopIngameClients(i)
		if(IsPlayerAlive(i))
			ap++;

	int alive = ap;

	//Somehow rarely is possible that more players than 49 get on the server ... somehow
	int xAp = 50;
	if(ap >= 50)
		xAp = ap+1;

	ap = (xAp - ap) * ((1 + GetEntData(client, CCSPlayer_iNumRoundKills, 1)) / 2);

	if(wasHeadshot)
		ap = RoundFloat(float(ap) * 1.02);

	Debug("Currently alive: %i. %N has %i kills (Headshot: %b), thus will gain %i points", alive, client, GetEntData(client, CCSPlayer_iNumRoundKills, 1), wasHeadshot, ap);

	return ap;
}

void FadeScreen(const int toColor[4], int time = 6000, int hold = -1, int action = FFADE_OUT, int client = -1, bool isRelieable = false) {
	Handle h_msg;

	if(client != -1)
		h_msg = StartMessageOne("Fade", client, isRelieable ? USERMSG_RELIABLE : 0);
	else
		h_msg = StartMessageAll("Fade", isRelieable ? USERMSG_RELIABLE : 0);

	if(h_msg != null) {
		PbSetInt(h_msg, "duration", time);
		PbSetInt(h_msg, "hold_time", hold);
		PbSetInt(h_msg, "flags", action | FFADE_PURGE);
		PbSetColor(h_msg, "clr", toColor);

		EndMessage();
	}
}

int CheckVectorIsInTpTrigger(float thePos[3]) {
	//Optimization. The TP triggers are on the edge on every layer, so when you are not close to
	//the edge of the bounds theres no way you are even close to a TP trigger.
	if(FloatAbs(thePos[0]) <= 14000.0 && FloatAbs(thePos[1]) <= 14000.0)
		return -1;

	for(int i = 0; i < tpTriggerCount; i++) {
		if (!(tpTriggerBounds[i][0][2] <= thePos[2] <= tpTriggerBounds[i][1][2]))
			continue;

		if (!(tpTriggerBounds[i][0][0] <= thePos[0] <= tpTriggerBounds[i][1][0]))
			continue;

		if (!(tpTriggerBounds[i][0][1] <= thePos[1] <= tpTriggerBounds[i][1][1]))
			continue;

		return i;
	}

	return -1;
}

void MoveVectorByTpTrigger(float thePos[3], int TpTriggerIndex) {
	SubtractVectors(thePos, tpTriggerLandmarkVecs[TpTriggerIndex], thePos);

	AddVectors(tpTriggerDests[TpTriggerIndex], thePos, thePos);
}

//Shamelessly stolen from boomix https://forums.alliedmods.net/showthread.php?p=2470206
int xCount[MAXPLAYERS+1];
public void OnPostThinkPostAnimationFix(int client) {
	if(IsClientInGame(client) && IsPlayerAlive(client) && !IsFakeClient(client) && GetPlayerFakeWeaponSlot(client) == SLOT_FISTS) {
		if(g_PVMid[client] > 0) {
			int dataz = GetEntProp(g_PVMid[client], Prop_Data, "m_nSequence");

			if(dataz == 1 || dataz == 0)
				xCount[client] = 0;

			xCount[client]++;

			if(xCount[client] == 40)
				SetEntProp(g_PVMid[client], Prop_Send, "m_nSequence", 1);

			if(xCount[client] == 60)
				SetEntProp(g_PVMid[client], Prop_Send, "m_nSequence", 6);

			if(dataz == 2)
				SetEntProp(g_PVMid[client], Prop_Send, "m_nSequence", 1);
		}
	}
}

public void InitWorldEnts() {
	int i = -1;

	//Prepare all breakables in the map for custom handling
	while((i = FindEntityByClassname(i, "func_breakable")) != -1) {
		//Randomly (10% chance) pre-break this breakable
		if(GetURandomFloat() < 0.1)
			AcceptEntityInput(i, "Kill");
		else {
			SDKHook(i, SDKHook_OnTakeDamage, OnTakeDamage_Breakable);
		}
	}

	i = -1;
	//Remove some doors to squeeze out some more entities
	while((i = FindEntityByClassname(i, "func_door_rotating")) != -1) {
		//Randomly (5% chance) pre-break this breakable
		if(GetURandomFloat() < 0.05)
			AcceptEntityInput(i, "Kill");
	}

	i = -1;
	static char destiname[16];
	//Prepare all breakables in the map for custom handling
	while((i = FindEntityByClassname(i, "prop_dynamic")) != -1) {
		//For some reason, this returns garbage. Need to use the method below.
		GetEntPropString(i, Prop_Data, "m_iName", destiname, sizeof(destiname));

		if(!StrEqual(destiname, "breakable_fence")) {
			continue;
		}

		//Randomly pre-break this fence
		if(GetURandomFloat() < 0.08)
			AcceptEntityInput(i, "Kill");
		else { //ignore damage caused by ... damage
			//9 = pellets of shotgun
			SetEntProp(i, Prop_Data, "m_iMaxHealth", 9);
			SetEntProp(i, Prop_Data, "m_iHealth", 9);

			float das[3];

			GetEntDataVector(i, CBaseEntity_vecOrigin, das);
		}
	}
}

stock Action OnTakeDamage_Breakable(int victim, int &attacker, int &inflictor, float &damage, int &damagetype) {
	if(!IsValidClient(attacker) || GetPlayerFakeWeaponSlot(attacker) == SLOT_FISTS) {
		return Plugin_Continue;
	} else {
    	damage = 0.0;
    	return Plugin_Changed;
	}
}

stock int SpawnParticleSystem(const char[] particleSystemName, const float origin[3], float lifetime, int parent = -1) {
	int particle = CreateEntityByName("info_particle_system");

	if(IsValidEdict(particle)) {
		DispatchKeyValue(particle, "effect_name", particleSystemName);
		DispatchSpawn(particle);

		SetEntDataVector(particle, CBaseEntity_vecOrigin, origin, true);

		if(parent != -1) {
			SetVariantString("!activator");
			AcceptEntityInput(particle, "SetParent", parent);
		}

		ActivateEntity(particle);
		AcceptEntityInput(particle, "start");


		FormatEx(sumTmpStr, sizeof(sumTmpStr), "OnUser1 !self:kill::%.2f:1", lifetime);

		SetVariantString(sumTmpStr);
		AcceptEntityInput(particle, "AddOutput");
		AcceptEntityInput(particle, "FireUser1");

		return particle;
    }

	return -1;
}

public int Death_Menu_Handler(Menu menu, MenuAction action, int client, int param2) {
	/* If an option was selected, tell the client about the item. */
	if (action == MenuAction_Select) {
		char info[32];
		menu.GetItem(param2, info, sizeof(info));
		if(StrEqual(info, "spec") && (CanClientSpectate[client] || Lobby_GetAlivePlayersInLobby(CurrentClientLobby[client]) > 0)) {
			//Spectator HAHAHAHAHAHahdsabdnaskj.
			SetClientSpecMode(null, GetClientUserId(client));
			CreateTimer(0.5, SetClientSpecMode, GetClientUserId(client));
			CreateTimer(2.0, SetClientSpecMode, GetClientUserId(client));


			FadeScreen({0, 0, 0, 255}, 500, 0, FFADE_IN, client);

			EmitSoundToClient(client, "ambient/de_train_radio.wav", SOUND_FROM_PLAYER, client, _, SND_STOP | SND_CHANGEVOL | SND_STOPLOOPING, 0.0);

			UpdateTrashtalkStuff();

			return;
		}
	} else if(param2 != MenuCancel_Timeout) {
		return;
	} else if(action == MenuAction_End) {
        delete menu;
    }

    #if !defined DEBUG
		KickClient(client, "Thank you for playing!");
	#endif
}

static Action SetClientSpecMode(Handle timer, int userId) {
	int client = GetClientOfUserId(userId);

	if(IsValidClient(client))
		SpecNext(client);

	//Cmd_spec_mode(client, "", 0);
}

void ShowDisconnectMenu(int client, const char[] msg = "You have died", int time = 9, bool forceAllowSpec = false) {
	Menu newMenu = CreateMenu(Death_Menu_Handler);
	newMenu.ExitButton = false;
	newMenu.SetTitle(msg, alivePlayers);

	newMenu.AddItem("", "", ITEMDRAW_SPACER);
	newMenu.AddItem("", "", ITEMDRAW_SPACER);
	newMenu.AddItem("", "", ITEMDRAW_SPACER);
	newMenu.AddItem("", "", ITEMDRAW_SPACER);

	newMenu.AddItem("disc", "Disconnect");
	if((forceAllowSpec || (alivePlayers > 1 && CanClientSpectate[client])) && !IsPlayerAlive(client))
		newMenu.AddItem("spec", "Spectate");
	newMenu.Display(client, alivePlayers <= 1 ? 9 : time);
}

Handle endTimer = null;

void EndMatchDelayed(float time = 10.0) {
	if(endTimer != null && endTimer != null)
		KillTimer(endTimer);

	endTimer = CreateTimer(time, EndRoundTimer);
	PrintToServer(">>Delayed match end initiated...");
}

static Action EndRoundTimer(Handle timer) {
	endTimer = null;
	PrintToServer(">>Delayed match end excuting... (1s)");

	//IF there is a right place in the whole process to reload the plugin on changes, then this is it.
	ServerCommand("sm plugins refresh");

	#if !defined DEBUG
		ResetLobbyManager();
	#endif

	//When the Server ran for more than 20 days we reload the map to prevent GetGameTime from overflowing at 21.5 days, and thus causing problems.
	if(GetGameTime() >= 1728000000.0) {
		char mapname[64];
		GetCurrentMap(mapname, sizeof(mapname));

		ServerCommand("changelevel %s", mapname);

		CreateTimer(20.0, CheckRequested);
	} else {
		//if somebody connects to the server right as this fucking second happens its possible that the round restart will never happen.
		CS_TerminateRound(0.1, CSRoundEnd_Draw, true);
		CreateTimer(10.0, CheckRequested);
	}
}

//For some reason, sometimes its possible that the server wont call Event_RoundStart(?) which
//then would result in the srv not requesting players Lets work-around it.
static Action CheckRequested(Handle timer) {
	if(GetClientCount(false) == 0 /*&& Queue_ReservedCount() == 0*/ && Queue_GetRequestedCount() == 0)
		Event_RoundStart(null, "", true);
}

stock bool ShowVGUIPanelEx(int iClient, const char[] szTitle, const char[] szUrl, int iType = MOTDPANEL_TYPE_URL, int iFlags = 0, bool bShow = true, Handle hMsg = null) {
	KeyValues hKv = CreateKeyValues("data");

	hKv.SetString("title", szTitle);
	hKv.SetNum("type", iType);
	hKv.SetString("msg", szUrl);
	hKv.GotoFirstSubKey(false);

	bool bOverride = false;

	if (hMsg == null) {
		hKv.SetNum("cmd", 5);
		hMsg = StartMessageOne("VGUIMenu", iClient, iFlags);
	} else {
		bOverride = true;
	}

	char szKey[256]; char szValue[256];

	if (!bOverride) {
		PbSetString(hMsg, "name", "info");
		PbSetBool(hMsg, "show", bShow);
	}

	Handle hSubKey;

	do {
		hKv.GetSectionName(szKey, sizeof(szKey));
		hKv.GetString(NULL_STRING, szValue, sizeof(szValue), "");

		hSubKey = PbAddMessage(hMsg, "subkeys");

		PbSetString(hSubKey, "name", szKey);
		PbSetString(hSubKey, "str", szValue);

	} while (hKv.GotoNextKey(false));

	if (!bOverride) {
		EndMessage();
	}

	delete hKv;
	return true;
}

void InitFogControllerForClient(int client) {
	#if !defined NOFOG
		/* Create own fog controller per player */
		if(!IsValidEdict(PLAYER_FOG_CONTROLLERS[client])) {
			PLAYER_FOG_CONTROLLERS[client] = CreateEntityByName("env_fog_controller");

			if(IsValidEdict(PLAYER_FOG_CONTROLLERS[client])) {
				char pFog[16]; FormatEx(pFog, sizeof(pFog), "FogCtrl-%i", client);
				DispatchKeyValue(PLAYER_FOG_CONTROLLERS[client], "targetname", pFog);
				DispatchKeyValue(PLAYER_FOG_CONTROLLERS[client], "fogenable", "1");
				DispatchKeyValue(PLAYER_FOG_CONTROLLERS[client], "fogcolor", FOG_COLOR_STR);
				DispatchKeyValueFloat(PLAYER_FOG_CONTROLLERS[client], "foglerptime", 0.35);
				DispatchKeyValueFloat(PLAYER_FOG_CONTROLLERS[client], "fogstart", -200.0);
				DispatchKeyValueFloat(PLAYER_FOG_CONTROLLERS[client], "fogend", 11000.0);
				DispatchSpawn(PLAYER_FOG_CONTROLLERS[client]);

				PLAYER_FOG_CONTROLLERS[client] = EntIndexToEntRef(PLAYER_FOG_CONTROLLERS[client]);

				SetEntData(client, CBasePlayer_3d_fog_enable, 1, true);
				SetEntProp(client, Prop_Send, "m_skybox3d.fog.colorPrimary", RgbaToInt(FOG_COLOR));
				SetEntProp(client, Prop_Send, "m_skybox3d.fog.colorSecondary", RgbaToInt(FOG_COLOR));

				CreateTimer(0.0, SetFogControllerForClient, client, TIMER_FLAG_NO_MAPCHANGE);
			} else {
				LogError("COULD NOT CREATE FOG CONTROLLER FOR PLAYER %N", client);
			}
		}
	#endif
}

void UpdateTrashtalkStuff() {
	LoopIngameClients(client) {
		//Pre-game, every client that has disabled trashtalk should be muted, and listening to all other clients that arent in his team should be blocked
		//When the game starts, we do the same, but for everyone, even players that have trashtalk enabled.
		if(userSettings[client][CFG_preGameTrashtalk] && !Go4TK_Game_InProgress)
			continue;
		else {
			SetClientListeningFlags(client, Go4TK_Game_InProgress ? VOICE_NORMAL : VOICE_MUTED);
		}

		LoopIngameClients(clientOther) {
			//..I dont think we should block the player from hearing..himself?
			if(clientOther == client)
				continue;

			//Allow players in the same lobby to talk to each other
			if(CurrentClientLobby[client] == CurrentClientLobby[clientOther])
				continue;

			SetListenOverride(client, clientOther, Listen_No);
		}
	}
}

void AllowPlayersToTalkToEachOther(int client1, int client2) {
	if(IsValidClient(client1) || IsValidClient(client2)) {
		//SetClientListeningFlags(client1, VOICE_NORMAL);
		//SetClientListeningFlags(client2, VOICE_NORMAL);

		SetListenOverride(client1, client2, Listen_Yes);
		SetListenOverride(client2, client1, Listen_Yes);
	}
}

public Action SMAC_OnCheatDetected(int client, const char[] module, DetectionType type, Handle info) {
	static char WhateverText[128];

	//REDACTED//

	if(IsValidClient(g_iTvClient))
		CPrintToChat(g_iTvClient, "{default}[Go4TK-SMAC] Detected cheating by {red}%N (%s){default} from {red}%s{default}.", client, module, WhateverText);

	return Plugin_Stop;
}

/*
public Action FistHitSoundHook(int clients[64], int &numClients, char sound[PLATFORM_MAX_PATH], int &entity, int &channel, float &volume, int &level, int &pitch, int &flags) {
	if(IsValidEdict(entity) && HasEntProp(entity, Prop_Send, "m_hOwner")) {
		int attacker = GetEntPropEnt(entity, Prop_Send, "m_hOwner");

		//SlapPlayer(owner, 0, false);

		if(GetPlayerFakeWeaponSlot(attacker) == SLOT_FISTS) {
			PrintToServer("%s played by ent %i, m_hOwner: %i", sound, entity, attacker);

			if(StrContains(sound, "weapons/knife/knife_hit_") != -1) {
				//SetEntProp(player,Prop_Send,"m_lifeState", 1);

				//EmitSoundToAll(")physics/metal/chain_impact_soft1.wav", entity, channel, level, flags, volume, pitch);

		   		//sound = "physics/metal/chain_impact_soft1.wav";
		   		//volume = 0.3;
		   		//return Plugin_Changed;
		  	} else if(StrContains(sound, "weapons/knife/knife_hit") != -1) {
		  		PrecacheSound(")physics\body\body_medium_impact_hard4.wav");
		   		EmitSoundToAll(")physics\body\body_medium_impact_hard4.wav", entity, channel, level, flags, volume, pitch);

				return Plugin_Stop;
		  	}
		}
	}

	return Plugin_Continue;

	/ *int player = GetPlayerHoldingKnife(entity);
	if(player > 0 && IsClientInGame(player) && IsPlayerAlive(player)) {


		if(StrContains(sound, "weapons/knife/knife_hit_") != -1) {
			//SetEntProp(player,Prop_Send,"m_lifeState", 1);
			EmitSoundToAllAny("jailbreak/boxing_hitwall1.mp3", entity, channel, level, flags, volume, pitch);
			return Plugin_Stop;
	  	} else if(StrContains(sound, "weapons/knife/knife_slash") != -1) {
			PrecacheSoundAny(sound);
			EmitSoundToAllAny(sound, entity, channel, level, flags, volume, pitch);
			return Plugin_Stop;
	    } else if(StrContains(sound, "weapons/knife/knife_hit") != -1) {
	   		int random = GetRandomInt(1, 4);
	   		char soundPath[PLATFORM_MAX_PATH];
	   		Format(soundPath, sizeof(soundPath), "jailbreak/boxing_hit%i.mp3", random);
	   		EmitSoundToAllAny(soundPath, entity, channel, level, flags, volume, pitch);
			return Plugin_Stop;
	  	} else if(StrContains(sound, "weapons/knife/knife_stab") != -1) {
	  		EmitSoundToAllAny("jailbreak/boxing_stab.mp3", entity, channel, level, flags, volume, pitch);
			return Plugin_Stop;
	 	}
  	}

	return Plugin_Changed;* /
}*/

/*public Action GiveNamedItemPre(int iClient, char sClassname[64], CEconItemView &xItem) {
	//if(IsFakeClient(iClient))
	//	return Plugin_Continue;

	Debug("GIVEN ITEM %s %i", sClassname, xItem.GetItemDefinition());
	Debug("GIVEN ITEM %s %i", sClassname, xItem.GetItemDefinition());
	Debug("GIVEN ITEM %s %i", sClassname, xItem.GetItemDefinition());
	Debug("GIVEN ITEM %s %i", sClassname, xItem.GetItemDefinition());

	int iDefIndex = CSGOItems_GetWeaponDefIndexByClassName(sClassname);

	if(iDefIndex <= -1)
		return Plugin_Handled;

	if(iDefIndex > 40 && iDefIndex != 60 && iDefIndex != 61)
		return Plugin_Continue;

	int iWeaponSlot = CSGOItems_GetWeaponSlotByDefIndex(iDefIndex);

	if(iWeaponSlot <= -1)
		return Plugin_Handled;

	int iTempWeaponSlot = -1;

	for (int i = 0; i < g_iGiveCount[iClient]; i++) {
		iTempWeaponSlot = CSGOItems_GetWeaponSlotByDefIndex(g_iGiveItems[iClient][i]);

		if(iTempWeaponSlot <= -1 || iTempWeaponSlot != iWeaponSlot)
			continue;

		if(!CSGOItems_GetWeaponClassNameByDefIndex(g_iGiveItems[iClient][i], sClassname, 64))
			return Plugin_Continue;

		return Plugin_Changed;
	}

	return Plugin_Continue;
}*/

//SetEntProp(client, Prop_Send, "m_bNightVisionOn", 1);
//SetEntProp(client, Prop_Send, "m_bNightVisionOn", 1);

/*SendConVarValue(client, FindConVar("weapon_accuracy_nospread"), "1");
SendConVarValue(client, FindConVar("weapon_recoil_cooldown"), "0");
SendConVarValue(client, FindConVar("weapon_recoil_scale"), "0");
SendConVarValue(client, FindConVar("weapon_recoil_view_punch_extra"), "0");
SendConVarValue(client, FindConVar("weapon_recoil_decay_coefficient"), "0");*/

//SetEntPropFloat(client, Prop_Send, "m_flProgressBarStartTime", GetGameTime());
//SetEntProp(client, Prop_Send, "m_iProgressBarDuration", 5);

/*Handle buf = StartMessageOne("VotePass", client, USERMSG_RELIABLE);
if(buf != null) {
	PbSetInt(buf, "team", CS_TEAM_CT);
	PbSetInt(buf, "vote_type", 0);
	PbSetString(buf, "disp_str", "SFUI_Scoreboard_NormalPlayer");
	PbSetString(buf, "details_str", "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.");
	EndMessage();
}*/

/*Handle buf = StartMessageOne("GameTitle", client, USERMSG_RELIABLE);
if(buf != null) {
	//PbSetBool(buf, "reset", true);
	EndMessage();
}*/

/*int skin = CreateEntityByName("prop_dynamic_override");
DispatchKeyValue(skin, "model", "models/Go4TK/pickup/vest_torso_pickup.mdl");
DispatchKeyValue(skin, "disableshadows", "1");
DispatchKeyValue(skin, "solid", "0");
DispatchSpawn(skin);
SetEntProp(skin, Prop_Send, "m_fEffects", EF_BONEMERGE|EF_NOSHADOW|EF_PARENT_ANIMATES|EF_BONEMERGE_FASTCULL);
SetVariantString("!activator");
AcceptEntityInput(skin, "SetParent", client);
SetVariantString("primary");
AcceptEntityInput(skin, "SetParentAttachment");

skin = CreateEntityByName("prop_dynamic_override");
DispatchKeyValue(skin, "model", "models/Go4TK/pickup/helmet_pickup.mdl");
DispatchKeyValue(skin, "disableshadows", "1");
DispatchKeyValue(skin, "solid", "0");
DispatchSpawn(skin);
SetEntProp(skin, Prop_Send, "m_fEffects", EF_BONEMERGE|EF_NOSHADOW|EF_PARENT_ANIMATES|EF_BONEMERGE_FASTCULL);
SetVariantString("!activator");
AcceptEntityInput(skin, "SetParent", client);
SetVariantString("primary");
AcceptEntityInput(skin, "SetParentAttachment");*/


/*for (int i = 0; i <= 5; i++) {
	Item().Create(Yeezys, 1);
}

float start = GetEngineTime();

for (int i = 1; i < 5; i++) {
	Item(i).TotalWeight;
	Item(i - 1).TotalWeight;
	Item(i + 1).Amount += 1;
	Item(i + 1).Amount += 1;
	Item(i - 1).Amount += 1;
	Item(i + 1).Amount += 1;
	Item(i - 1).Amount += 1;
	Item(i + 1).Amount += 1;
	Item(i - 1).Amount += 1;
	Item(i - 1).Amount += 1;
	Item(i - 1).Amount += 1;
	Item(i + 1).Amount += 1;

	//PrintToServer("%i %i %i", i, Item(i).Weight, Item(i).Amount);
}

PrintToServer("Done %i Loads and %i Saves in %f Seconds", loads, saves, GetEngineTime() - start);*/

/*PrintToServer("");
PrintToServer("");
PrintToServer("");
PrintToServer("=======================================");

InitGo4TKInventoryForClient(0);

PLAYER_INV[0][Head] = Item().CreateOwnedItem(Helmet, 1, 0);
PLAYER_INV[0][Carrying].AddItem(Bandage, 5, 0);
PLAYER_INV[0][Carrying].AddItem(Duct_Tape, 1, 0);
PLAYER_INV[0][Carrying].AddItem(Composite_Fabric, 4, 0);

PrintToServer("Head isValid: %i", PLAYER_INV[0][Head].IsValid());
PrintToServer("Armor isValid: %i", PLAYER_INV[0][Armor].IsValid());

PrintToServer("carries %i weight", PLAYER_INV[0][Carrying].Size);

PrintToServer("============= SHREDDING HELMET =============");
PrintToServer("Result: %i", Go4TK_Playerinv(0).ShredItem(PLAYER_INV[0][Head]));
PrintToServer("============= POST SHRED =============");

PrintToServer("Head isValid: %i", PLAYER_INV[0][Head].IsValid());
Item aScrap = PLAYER_INV[0][Carrying].GetItemByType(Armor_Scrap);
PrintToServer("ArmorScrap is Valid: %i, owns %i (Owner ID: %i)", aScrap.IsValid(), aScrap.Amount, aScrap.CurrentOwner);

PrintToServer("============= CRAFTING VEST =============");
PrintToServer("Result: %i", Go4TK_Playerinv(0).CraftItem(Makeshift_Armor));
PrintToServer("============= POST CRAFT =============");

PrintToServer("ArmorScrap is Valid: %i, owns %i (Owner ID: %i)", aScrap.IsValid(), aScrap.Amount, aScrap.CurrentOwner);
PrintToServer("Armor isValid: %i, ID: %i", PLAYER_INV[0][Armor].IsValid(), PLAYER_INV[0][Armor]);

PrintToServer("Craft attempt w/o enough items: %i", Go4TK_Playerinv(0).CraftItem(Makeshift_Armor));*/

/*float tmporigin3[3];

GetEntDataVector(1, CBaseEntity_vecOrigin, tmporigin3);

//Address cPointer = SDKCall(g_hCLagCompensationManagerCall, "CLagCompensationManager");

Debug("%x", g_hCLagCompensationManagerInstance);

Debug("Pos Pre %.2f %.2f %.2f", tmporigin3[0], tmporigin3[1], tmporigin3[2]);

SDKCall(g_hStartLagCompensationCall, g_hCLagCompensationManagerInstance, 2, 2, CBaseEntity_vecOrigin, angles, 1024.0);

GetEntDataVector(1, CBaseEntity_vecOrigin, tmporigin3);

Debug("Pos Post %.2f %.2f %.2f", tmporigin3[0], tmporigin3[1], tmporigin3[2]);


SDKCall(g_hFinishLagCompensationCall, g_hCLagCompensationManagerInstance, 2);

GetEntDataVector(1, CBaseEntity_vecOrigin, tmporigin3);

Debug("Pos Post Reset %.2f %.2f %.2f", tmporigin3[0], tmporigin3[1], tmporigin3[2]);*/


/*float origins[MAP_LAYERS][3];

GetPhysicalVectorOnAllLaysersFromPhysicalPosition(vecMins, origins);

int i = -1;
while((i = FindEntityByClassname(i, "point_worldtext")) != -1)
	AcceptEntityInput(i, "Kill");

for(i = 0; i < 4; i++) {
	//TE_SendBeamRingEx(_, origins[i], 64.0, BeamModelIndex, _, _, _, 15.0, 5.0, _, {0, 0, 255, 255});

	//float pos2[3];
	//pos2 = PhysicalOrigins[0];
	//pos2[2] -= 500.0;

	//TE_SetupBeamPoints(PhysicalOrigins[0], pos2, BeamModelIndex, 0, 0, 0, 15.0, 10.0, 10.0, 0, 0.0, { 0, 255, 255, 255 }, 0);
	//TE_SendToAll();

	Debug("%i >> %.2f %.2f %.2f", i, origins[i][0], origins[i][1], origins[i][2]);

	//SpawnFormattedWorldText("LOL", 100, origins[i]);

	//GetWorldPointIn3dSkybox(origins[i], origins[i]);

	//TE_SendBeamRingEx(_, origins[i], 4096.0, BeamModelIndex, _, _, _, 15.0, 5.0, _, {255, 0, 0, 255}, 8);

	SpawnFormattedWorldText("LOL", 128, origins[i]);

	GetWorldPointIn3dSkybox(origins[i], origins[i]);

	SpawnFormattedWorldText("LOL", 8, origins[i]);

	TE_SendBeamRingEx(client, origins[i], 4096.0 / 16.0, BeamModelIndex, _, _, _, 15.0, 5.0, _, {255, 0, 255, 255}, 8);

	TE_SetupBeamRingPoint(origins[i], 4096.0 / 16.0, (4096.0 / 16.0) + 1.0, BeamModelIndex, 0, 0, 1, 0.3, 1.0, 0.0, {0, 255, 255, 255}, 0, 0);
	TE_SendToAll();
}*/

//GetClientEyePosition(client, vecMins);
//float vecOrigin[3];

//vecOrigin = vecMins;

//TE_SendBeamRingEx(client, vecMins, 5000.0, BeamModelIndex, _, _, _, 15.0, 5.0, _, {255, 0, 0, 255});



//GetClientEyePosition(client, vecMins);
//GetLogicalCoordFromPhysical(vecMins, vecMins);

//Debug("Zepos >> %.2f %.2f %.2f", vecMins[0], vecMins[1], vecMins[2]);

/*for (int i = AsInt(Duct_Tape); i <= AsInt(Yeezys); i++) {
	vecOrigin[0] = vecMins[0] + (RoundToFloor(float(i) / float(5)) * 60);
	vecOrigin[1] = vecMins[1] + ((i % 5) * 50);

	Handle trace = TR_TraceRayEx(vecOrigin, view_as<float>({90.0, 0.0, 0.0}), MASK_ALL, RayType_Infinite);

	if(trace != null && TR_DidHit(trace)) {
		float endPos[3];
		TR_GetEndPosition(endPos, trace);

		if(128.0 > GetVectorDistance(vecOrigin, endPos) > 5.0) {
			Item newItem = Item().Create(vAs(Go4TK_Item_Type, i), 1, endPos);

			if(newItem.SuperType == ST_Ammo)
				newItem.Amount = 500;
		}
		CloseHandle(trace);
	}
}*/


/*
float tmporigin3[3];

GetEntDataVector(1, CBaseEntity_vecOrigin, tmporigin3);

Debug("Physical >> %.2f %.2f %.2f", tmporigin3[0], tmporigin3[1], tmporigin3[2]);

GetLogicalCoordFromPhysical(tmporigin3, tmporigin3);

Debug("Logical >> %.2f %.2f %.2f", tmporigin3[0], tmporigin3[1], tmporigin3[2]);

currentSafezoneCenter = tmporigin3;
currentSafezoneRadius = 2048.0;

SendCurrentSafezoneToClient(client);
*/