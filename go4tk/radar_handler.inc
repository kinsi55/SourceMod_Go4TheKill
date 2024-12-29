static int lastHudTransmit[MAXPLAYERS+1] = {-1, ...};
static char lastHudText[MAXPLAYERS+1][48];

void UpdateHudInfoForClient(int client, float currentOrigin[3]) {
	GetLogicalCoordFromPhysical(currentOrigin, currentOrigin);
	
	currentOrigin[0] /= 14915.4;
	currentOrigin[1] /= 14915.4;
	
	char PosAndDir[48];
	
	if(currentOrigin[0] < 1.0)
		PosAndDir = "A";
	else if(currentOrigin[0] < 2.0)
		PosAndDir = "B";
	else if(currentOrigin[0] < 3.0)
		PosAndDir = "C";
	else if(currentOrigin[0] >= 3.0)
		PosAndDir = "D";
	
	int x = RoundToFloor(currentOrigin[1]) + 1;
	
	if(x > 4)
		x = 4;
	else if(x < 1)
		x = 1;
	
	Format(PosAndDir, sizeof(PosAndDir), "%s%i", PosAndDir, x);
	
	if(!StrEqual(lastClientCordon[client], PosAndDir)) {
		FormatEx(lastClientCordon[client], 48, "1,%s", PosAndDir);
		
		SendToClient(client, lastClientCordon[client], WSCONN_map);
		
		lastClientCordon[client] = PosAndDir;
	}
	
	float pos[2] = {0.01, 0.1};
	
	if(spectatedPlayer[client] > 0) {
		if(ActiveLobbiesCached >= alivePlayers)
			Format(PosAndDir, sizeof(PosAndDir), "%s %s\nAlive: %i\nScore: %i\n\n%s", PosAndDir, directions[RoundFloat((180.0 + plRealEyeAngles[client][1]) / 22.5)], alivePlayers, ClientScore[spectatedPlayer[client]], STEAMID_WATERMARK ? client_AuthIds[client] : "");
		else
			Format(PosAndDir, sizeof(PosAndDir), "%s %s\nAlive: %i\nTeams: %i\nScore: %i\n\n%s", PosAndDir, directions[RoundFloat((180.0 + plRealEyeAngles[client][1]) / 22.5)], alivePlayers, ActiveLobbiesCached, ClientScore[spectatedPlayer[client]], STEAMID_WATERMARK ? client_AuthIds[client] : "");
		pos[1] = 0.3; //lower it a bit because spectator ui is bugged AF and sometimes doesnt hide the radar HUD, which then could overlay this
	} else
		if(ActiveLobbiesCached >= alivePlayers)
			Format(PosAndDir, sizeof(PosAndDir), "%s %s\nAlive: %i\nScore: %i\n\n%s", PosAndDir, directions[RoundFloat((180.0 + plRealEyeAngles[client][1]) / 22.5)], alivePlayers, ClientScore[client], STEAMID_WATERMARK ? client_AuthIds[client] : "");
		else
			Format(PosAndDir, sizeof(PosAndDir), "%s %s\nAlive: %i\nTeams: %i\nScore: %i\n\n%s", PosAndDir, directions[RoundFloat((180.0 + plRealEyeAngles[client][1]) / 22.5)], alivePlayers, ActiveLobbiesCached, ClientScore[client], STEAMID_WATERMARK ? client_AuthIds[client] : "");
	
	if(GetTime() - lastHudTransmit[client] >= 5 || !StrEqual(lastHudText[client], PosAndDir)) {
		HudMsg(client, HUDMSG_CHANNEL_POSANDDIR, pos, { 222, 222, 222, 255 }, {0, 0, 0, 0}, _, _, _, 6.0, _, PosAndDir);
		lastHudTransmit[client] = GetTime();
		lastHudText[client] = PosAndDir;
	}
}