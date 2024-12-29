#define LAGCOMP_SNAPSHOT_BACKLOG 4.0

enum LagRecord {
	LfFlags,
	//m_vecOrigin
	Float:LOrigin_X,
	Float:LOrigin_Y,
	Float:LOrigin_Z,
	//m_angEyeAngles[0] / m_angEyeAngles[1]
	Float:LAngles_X,
	Float:LAngles_Y,
	//TODO (Un)necessary?
	/*//m_vecMins
	Float:LvecMins_X,
	Float:LvecMins_Y,
	Float:LvecMins_Z,
	//m_vecMaxs
	Float:LvecMaxs_X,
	Float:LvecMaxs_Y,
	Float:LvecMaxs_Z,
	//m_flSimulationTime
	Float:LflSimulationTime*/
}

//https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/server/player_lagcompensation.cpp#L307

//TODO
/*float tmporigin1[3];
float tmporigin2[3];
float tmporigin3[3];

GetEntDataVector(client, CBaseEntity_vecOrigin, tmporigin3);

Debug("Pos Pre %.2f %.2f %.2f", tmporigin3[0], tmporigin3[1], tmporigin3[2]);

SDKCall(g_hStartLagCompensationCall, client, 1, tmporigin1, tmporigin2, 2.0);

GetEntDataVector(client, CBaseEntity_vecOrigin, tmporigin3);

Debug("Pos Post %.2f %.2f %.2f", tmporigin3[0], tmporigin3[1], tmporigin3[2]);


SDKCall(g_hFinishLagCompensationCall, client);

GetEntDataVector(client, CBaseEntity_vecOrigin, tmporigin3);

Debug("Pos Post Reset %.2f %.2f %.2f", tmporigin3[0], tmporigin3[1], tmporigin3[2]);*/

//one array per player which holds the last positions
static ArrayList LagCompBacklog[MAXPLAYERS+1] = {null, ...};

static float LagRecordItem[LagRecord];

static bool IsCurrentlyBackdated[MAXPLAYERS+1] = {false, ...};

static float currentBackdatedOriginalPosition[MAXPLAYERS+1][3];
//static float currentBackdatedOriginalAngles[MAXPLAYERS+1][3];

stock void InitLagcompForPlayer(int client) {
	if(LagCompBacklog[client] != null) {
		LagCompBacklog[client].Clear();
	} else {
		LagCompBacklog[client] = new ItemList();
	}
}

stock void AddSnaphotForPlayer(int client, const float angles[3]) {
	if(LagCompBacklog[client] == null)
		return;

	float tmpVec[3];
	
	GetEntDataVector(client, CBaseEntity_vecOrigin, tmpVec);
	LagRecordItem[LOrigin_X] = tmpVec[0];
	LagRecordItem[LOrigin_Y] = tmpVec[1];
	LagRecordItem[LOrigin_Z] = tmpVec[2];
	
	LagRecordItem[LAngles_X] = angles[0];
	LagRecordItem[LAngles_Y] = angles[1];
	
	LagCompBacklog[client].PushArray(LagRecordItem[0]);
	
	if(LagCompBacklog[client].Length > LAGCOMP_SNAPSHOT_BACKLOG)
		LagCompBacklog[client].Erase(0);
	
	LagCompBacklog[client].GetArray(0, LagRecordItem[0]);
	
	/*static float backTickedPos[3];
	backTickedPos[0] = LagRecordItem[LOrigin_X];
	backTickedPos[1] = LagRecordItem[LOrigin_Y];
	backTickedPos[2] = LagRecordItem[LOrigin_Z];
	
	static float backTickedPos2[3];
	backTickedPos2 = backTickedPos;
	backTickedPos[2] += 72.0;
	
	TE_SetupBeamPoints(backTickedPos, backTickedPos2, BeamModelIndex, 0, 0, 0, 0.1, 1.0, 1.0, 0, 0.0, {255, 0, 255, 255}, 0);
	TE_SendToAll();*/
}

stock void StartLagcompForPlayer(int client, int ticksToBackDate) {
	if(ticksToBackDate <= 0)
		return;
		
	if(LagCompBacklog[client] == null) { 
		Debug("PLAYER DOES NOT HAVE LAG COMPENSATION DATA!!!!");
		return;
	}
	
	int len = LagCompBacklog[client].Length;
	
	if(ticksToBackDate > len)
		ticksToBackDate = len;
		
	Debug("Backdating %N by %i ticks", client, ticksToBackDate);
	
	LagCompBacklog[client].GetArray(len - ticksToBackDate, LagRecordItem[0]);
	
	float backTickedPos[3];
	backTickedPos[0] = LagRecordItem[LOrigin_X];
	backTickedPos[1] = LagRecordItem[LOrigin_Y];
	backTickedPos[2] = LagRecordItem[LOrigin_Z];
	
	float backTickedAngles[3];
	backTickedAngles[0] = LagRecordItem[LAngles_X];
	backTickedAngles[1] = LagRecordItem[LAngles_Y];
	
	GetEntDataVector(client, CBaseEntity_vecOrigin, currentBackdatedOriginalPosition[client]);
	IsCurrentlyBackdated[client] = true;
	
	//TeleportEntity(client, backTickedPos, backTickedAngles, NULL_VECTOR);
	
	SetEntDataVector(client, CBaseEntity_vecOrigin, backTickedPos, false);
	SetEntDataVector(client, CBaseEntity_angRotation, backTickedAngles, false);
}

stock void ResetLagcompForPlayer(int client) {
	if(IsCurrentlyBackdated[client]) {
		SetEntDataVector(client, CBaseEntity_vecOrigin, currentBackdatedOriginalPosition[client], false);
		SetEntDataVector(client, CBaseEntity_angRotation, plRealEyeAngles[client], false);
		
		IsCurrentlyBackdated[client] = false;
	}
}

public Action LagcompOriginFaker(int client, const char[] PropName, float vecValues[3], int element) {
	if(IsCurrentlyBackdated[client]) {
		vecValues = currentBackdatedOriginalPosition[client];
		
		return Plugin_Changed;
	}
	return Plugin_Continue;
}

//ArrayList