static ArrayList IngameLobbies = null;

int PlayerLobbyResultCaches[256][MAXPLAYERS+1][5];
int ActiveLobbiesCached = 0;

void ResetLobbyManager() {
	if(IngameLobbies == null)
		IngameLobbies = new ArrayList(64);
	else
		IngameLobbies.Clear();
		
	for(int x = 0; x < 256; x++)
		for(int y = 0; y < MaxClients; y++)
			PlayerLobbyResultCaches[x][y] = {0, 0, 0, 0, 0};
}

void Lobby_SetPlayerLobby(int client, const char[] lobbyId) {
	int currentLobbyIndexInArray = IngameLobbies.FindString(lobbyId);
	
	if(currentLobbyIndexInArray == -1) {
		currentLobbyIndexInArray = IngameLobbies.Length;
		IngameLobbies.PushString(lobbyId);
	}
	
	Debug("Setting Lobby of client %i (%N) to (%i) %s", client, client, currentLobbyIndexInArray, lobbyId);
	
	CurrentClientLobby[client] = currentLobbyIndexInArray;
}

int Lobby_GetActiveLobbyCount(int toFilterSingle = 0, int &filterOut = 0) {
	bool[] ignoreSameLobby = new bool[IngameLobbies.Length+1];
	int aliveLobbiesCount = 0;
	
	Debug("Len: %i", IngameLobbies.Length);
	
	LoopIngameClients(i) if(g_bPlayerAliveCache[i]) {
		if(CurrentClientLobby[i] < 0 || CurrentClientLobby[i] > IngameLobbies.Length)
			continue;
		
		if(toFilterSingle == CurrentClientLobby[i])
			filterOut++;
	
		Debug(">> %i", CurrentClientLobby[i]);
		
		if(!ignoreSameLobby[CurrentClientLobby[i]]) {
			ignoreSameLobby[CurrentClientLobby[i]] = true;
			aliveLobbiesCount++;
		}
	}
	
	ActiveLobbiesCached = aliveLobbiesCount;
	
	return aliveLobbiesCount;
}

int Lobby_GetAlivePlayersInLobby(int lobbyNum) {
	int aliveLobbyCount = 0;
	
	if(lobbyNum < 0)
		return aliveLobbyCount;
	
	LoopIngameClients(i) if(g_bPlayerAliveCache[i]) {
		if(CurrentClientLobby[i] < 0 || CurrentClientLobby[i] > IngameLobbies.Length)
			continue;
			
		if(CurrentClientLobby[i] == lobbyNum)
			aliveLobbyCount++;
	}
	
	return aliveLobbyCount;
}

static int PlayerGlowToClient[MAXENTITIES+1] = {-1, ...};

void Lobby_HandlePlayerSpawn(int client) {
	int skin = CPS_SetSkin(client, PLAYER_T_MODEL, CPS_RENDER);

	if(!IsValidEntity(skin))
		return;
	else
		PlayerGlowToClient[skin] = client;

	// Validate skin entity by SDKHookEx native return
	static int offset;

	// Get sendprop offset for prop_dynamic_override
	if (!offset && (offset = GetEntSendPropOffs(skin, "m_clrGlow")) == -1) {
		LogError("Unable to find property offset: \"m_clrGlow\"!");
		return;
	}

	// Enable glow for custom skin
	SetEntProp(skin, Prop_Send, "m_bShouldGlow", true, true);
	SetEntProp(skin, Prop_Send, "m_nGlowStyle", GLOWSTYLE_WALLHACK);
	SetEntPropFloat(skin, Prop_Send, "m_flGlowMaxDist", 40000.0);

	// So now setup given glow colors for the skin
	SetEntData(skin, offset, 134, _, true);    // Red
	SetEntData(skin, offset + 1, 183, _, true); // Green
	SetEntData(skin, offset + 2, 40, _, true); // Blue
	SetEntData(skin, offset + 3, 200, _, true); // Alpha
	
	SDKHook(skin, SDKHook_SetTransmit, Glow_SetTransmit);
}

static Action Glow_SetTransmit(int glowSkin, int toClient) {
	//Dont send the body-addon to somebody spectating this player
	int otherClient = PlayerGlowToClient[glowSkin];
	
	if(toClient == g_iTvClient)
		return Plugin_Continue;
	
	if(spectatedPlayer[toClient] == otherClient)
		return Plugin_Handled;
	
	if(otherClient == toClient)
		return Plugin_Handled;
	
	if(CurrentClientLobby[toClient] != CurrentClientLobby[otherClient])
		return Plugin_Handled;
	
	if(Go4TK_Game_InProgress && currentPlayerLayer[toClient] != currentPlayerLayer[otherClient])
		return Plugin_Handled;

	return Plugin_Continue;
}

void Lobby_HandlePlayerElimination(int client, int attacker, bool forceEnd = false) {
	int currClLobby = CurrentClientLobby[client];
	int aliveInLobby = 0;
	int place = Lobby_GetActiveLobbyCount(currClLobby, aliveInLobby);
	
	
	
	
	int selfId = ClientUserDbId[client];
	int killerId = attacker != -1 ? ClientUserDbId[attacker] : 0;
	
	
	int killCount = GetEntData(client, CCSPlayer_iNumRoundKills, 1);
	int score = ClientScore[client];
	int clUidToClId = client;

	if(attacker != -1)
		AllowPlayersToTalkToEachOther(client, attacker);
	
	//CS_SwitchTeam(client, CS_TEAM_CT);
	//SetEntProp(client, Prop_Send, "m_iTeamNum", CS_TEAM_CT);
	//SetEntProp(client, Prop_Send, "m_iPendingTeamNum", CS_TEAM_T);
	
	
	//Change end message from X won to X got the final kill
	char tmpStr[128];
	
	if(aliveInLobby - 1 > 0 && !forceEnd) {
		EmitSoundToClient(client, "ambient/de_train_radio.wav", SOUND_FROM_PLAYER, client, 1);
	
		//Cache result, we will write it once other players from this lobby are ded as well
		int currentlyCachedScores = ++PlayerLobbyResultCaches[currClLobby][0][0];
		
		PlayerLobbyResultCaches[currClLobby][currentlyCachedScores][0] = selfId;
		PlayerLobbyResultCaches[currClLobby][currentlyCachedScores][1] = killerId;
		PlayerLobbyResultCaches[currClLobby][currentlyCachedScores][2] = killCount;
		PlayerLobbyResultCaches[currClLobby][currentlyCachedScores][3] = score;
		PlayerLobbyResultCaches[currClLobby][currentlyCachedScores][4] = GetClientUserId(client);
		
		SetEntProp(client, Prop_Send, "m_iObserverMode", SPECMODE_FIRSTPERSON);
		SpecNext(client);
		
		ShowDisconnectMenu(client, "You have died, but your teammate can still win!", _, true);
		
		//#if !defined DEBUG
		FadeScreen({0, 0, 0, 255}, 6000, _, FFADE_OUT, client);
		//#endif
		
		//Apparently, when suiciding a player, he is not dead here, thus no stuff would be dropped ¯\_(ツ)_/¯
		
		if(attacker != -1)
			Format(tmpStr, sizeof(tmpStr), "%sKilled by %N (%i HP)", tmpStr, attacker, GetClientHealth(attacker));
		else if(!forceEnd)
			StrCat(tmpStr, sizeof(tmpStr), "Killed by unknown");
			
		Format(tmpStr, sizeof(tmpStr), "%s\nYou scored %i points\n\n\nYou will be disconnected in 9 seconds", tmpStr, ClientScore[client]);
		
		HudMsg(client, HUDMSG_CHANNEL_PICKEDUP, vAs(float, { -1.0, 0.2 }), { 200, 150, 255, 255 }, {0, 0, 0, 0}, _, 1.0, 1.0, 15.0, _, tmpStr);
		
		return;
	} else if(forceEnd) {
		EmitSoundToClient(client, "ui/valve_logo_music.wav");
	} else { //if aliveInLobby - 1 <= 0 && !forceEnd
		//Its safe to overwrite the value here since we just have refreshed it, and this lobby dies at this very moment.
		ActiveLobbiesCached--;
		EmitSoundToClient(client, "ambient/de_train_radio.wav", SOUND_FROM_PLAYER, client, 1);
	}
	
	LoopIngameClients(i) {
		if(CurrentClientLobby[i] == currClLobby) {
			SetEntProp(i, Prop_Send, "m_iObserverMode", SPECMODE_NONE);
			SetEntPropEnt(i, Prop_Send, "m_hObserverTarget", -1);
			//SetEntityMoveType(i, MOVETYPE_NONE); //TODO possibly needed, no idea
		
			FadeScreen({0, 0, 0, 255}, 6000, _, FFADE_OUT, i);
		
			ShowDisconnectMenu(i, forceEnd ? "Game ending" : "You have died!");
		}
	}
	
	int totalCachedLobbyResults = PlayerLobbyResultCaches[currClLobby][0][0];
	
	for(;;) {
		//Write result
		
		/////////////////// Backend_AddUserResultRAW(int clientId, int attackerId, int killCount, int score, int place)
		Backend_AddUserResultRAW(selfId, killerId, killCount, score, place, currClLobby);
		///////////////////
		
		if(IsValidClient(clUidToClId)) {
			FormatEx(tmpStr, sizeof(tmpStr), "You have placed %i. with %i points\n\n", place, ClientScore[clUidToClId]);
			
			//Apparently, when suiciding a player, he is not dead here, thus no stuff would be dropped ¯\_(ツ)_/¯
			
			if(attacker != -1)
				Format(tmpStr, sizeof(tmpStr), "%sKilled by %N (%i HP)", tmpStr, attacker, GetClientHealth(attacker));
			else if(!forceEnd)
				StrCat(tmpStr, sizeof(tmpStr), "Killed by unknown");
				
			StrCat(tmpStr, sizeof(tmpStr), "\n\n\nYou will be disconnected in 9 seconds");
			
			HudMsg(clUidToClId, HUDMSG_CHANNEL_PICKEDUP, vAs(float, { -1.0, 0.2 }), { 200, 150, 255, 255 }, {0, 0, 0, 0}, _, 1.0, 1.0, 15.0, _, tmpStr);
		}
		
		if(totalCachedLobbyResults <= 0)
			break;
		
		selfId = PlayerLobbyResultCaches[currClLobby][totalCachedLobbyResults][0];
		killerId = PlayerLobbyResultCaches[currClLobby][totalCachedLobbyResults][1];
		killCount = PlayerLobbyResultCaches[currClLobby][totalCachedLobbyResults][2];
		score = PlayerLobbyResultCaches[currClLobby][totalCachedLobbyResults][3];
		clUidToClId = GetClientOfUserId(PlayerLobbyResultCaches[currClLobby][totalCachedLobbyResults][4]);
		
		totalCachedLobbyResults--;
	}
	
	PlayerLobbyResultCaches[currClLobby][0][0] = 0;
	
	//Lets leave the Clearing of IngameLobbies to the delayed match end.
	//IngameLobbies.Erase(currClLobby);
}