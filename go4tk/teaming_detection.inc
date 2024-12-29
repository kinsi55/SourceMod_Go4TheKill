#define EnableTd

//Shamelessly stolen from SMAC
static bool IsInFieldOfView(const float start[3], const float angles[3], const float end[3]) {
	float Normal[3]; float plane[3];
	
	GetAngleVectors(angles, Normal, NULL_VECTOR, NULL_VECTOR);
	SubtractVectors(end, start, plane);
	NormalizeVector(plane, plane);
	
	//PrintToChatAll("DOT: %.2f", GetVectorDotProduct(plane, Normal));
	
	return GetVectorDotProduct(plane, Normal) > 0.1; // Cosine(Deg2Rad(179.9 / 2.0))
}

bool IsPointVisible(float start[3], float end[3]) {
	AddVectors(start, vAs(float, {0.0, 0.0, 64.0}), start);
	AddVectors(end, vAs(float, {0.0, 0.0, 64.0}), end);

	TR_TraceRayFilter(start, end, MASK_VISIBLE, RayType_EndPoint, Filter_NoPlayersAndNoAddons);

	return TR_GetFraction() == 1.0;
}

//We do only want to stop at entities which are neither players, nor player addons. 
public bool Filter_NoPlayersAndNoAddons(int entity, int mask) {
	return entity > MaxClients && GetAddonOwner(entity) == 0;
}

#define DEFO_IS_TEAMING_ABOVE 1200

float playerTeamingScore[MAXPLAYERS+1][MAXPLAYERS+1];

int totalDamageDealt[MAXPLAYERS+1][MAXPLAYERS+1];

float lastDamageDealtToTime[MAXPLAYERS+1][MAXPLAYERS+1];

float lastAttackTime[MAXPLAYERS+1];
int lastAttackAtClient[MAXPLAYERS+1];

bool wasInProximityToLastTick[MAXPLAYERS+1][MAXPLAYERS+1];
float isInProximitySinceTime[MAXPLAYERS+1][MAXPLAYERS+1];

float lastVisibleTime[MAXPLAYERS+1][MAXPLAYERS+1];
	
//After updating the teaming check values, lets find out if somebody is teaming with .. somebody
bool alreadyBanned[MAXPLAYERS+1];

void ResetTeamingDetectionForClient(int client) {
	lastAttackTime[client] = 0.0;
	
	lastAttackAtClient[client] = 0;
	
	alreadyBanned[client] = false;
	
	LoopClients(i) {
		playerTeamingScore[client][i] = 0.0;
		
		totalDamageDealt[client][i] = 0;
		lastDamageDealtToTime[client][i] = 0.0;
		lastVisibleTime[client][i] = 0.0;
		
		wasInProximityToLastTick[client][i] = false;
		isInProximitySinceTime[client][i] = 0.0;
	}
}

void DoTeamingCheck() {
	if(!Go4TK_Game_InProgress)
		return;
	
	//Completely disable teaming detection when theres more than 28 players because its pretty much useless
	if(alivePlayers > 28)
		return;
		
	//Completely disable teaming detection when theres less than 3 players because its pretty much useless
	if(alivePlayers < 3)
		return;
		
	float currentTime = GetGameTime();
		
	//Completely disable teaming detection before 40 seconds into the game as nobody will be able to team in that timespan anyways, at least it cant be detected properly that is.
	if(currentTime - Go4TK_Game_StartTime < 40)
		return;
	
	//Completely disable teaming detection when the safezone is very small already
	if(currentSafezoneRadius < 2500.0)
		return;

	LoopIngameClients(testClient) {
		if(!IsPlayerAlive(testClient) || IsClientSourceTV(testClient))
			continue;
			
		//Time since the client last attacked
		float lastFireDiff = currentTime - lastAttackTime[testClient];
		
		bool testClientHasWeaponThatCanFire = false;
		
		//Check if the client has any weapon with ammo in it.
		for (int wpSl = 1; wpSl <= 3; wpSl++) {
			int fakeWp = GetPlayerFakeWeaponSlotWeapon(testClient, wpSl);
			if(!testClientHasWeaponThatCanFire && IsValidEntity(fakeWp)) {
				if(GetEntData(fakeWp, CBaseCombatWeapon_iPrimaryReserveAmmoCount, 2) > 0) {
					testClientHasWeaponThatCanFire = true;
					break;
				}
			}
		}
		
		if(!testClientHasWeaponThatCanFire)
			continue;
		
		LoopIngameClients(otherClient) {
			if(otherClient == testClient || !IsPlayerAlive(otherClient) || IsClientSourceTV(otherClient))
				continue;
			
			//Dont do teaming detection between clients that are in the same lobby... because they obviously DO team, and are allowed to do so.
			if(CurrentClientLobby[testClient] == CurrentClientLobby[otherClient])
				continue;
			
			//If this other client doesnt have a weapon we dont need to care about it because the testClient does, and he is the one getting accounted the teamingScore rn
			/*bool otherClientHasWeaponThatCanFire = false;
			
			for (int wpSl = 1; wpSl <= 3; wpSl++) {
				int fakeWp = GetPlayerFakeWeaponSlotWeapon(otherClient, wpSl);
				if(!otherClientHasWeaponThatCanFire && IsValidEntity(fakeWp)) {
					if(GetEntData(fakeWp, CBaseCombatWeapon_iPrimaryReserveAmmoCount, 2) > 0) {
						otherClientHasWeaponThatCanFire = true;
						break;
					}
				}
			}
			
			if(!otherClientHasWeaponThatCanFire)
				continue;*/
			
			float timeSinceLastDamageTo = currentTime - lastDamageDealtToTime[testClient][otherClient];
			float timeSinceLastVisibilityTo = currentTime - lastVisibleTime[testClient][otherClient];
			
			//If the checked client has attacked the other client within 5 seconds, assume they are not teaming
			if(timeSinceLastDamageTo < 6.0)
				continue;
			
			float distance = GetVectorDistance(plLastPosition[testClient], plLastPosition[otherClient]);
			
			//If they are further than X units apart dont do further checks rn
			if(distance >= 1500.0 || distance <= 0.01) {
				wasInProximityToLastTick[testClient][otherClient] = false;
				continue;
			} else if (!wasInProximityToLastTick[testClient][otherClient]) {
				wasInProximityToLastTick[testClient][otherClient] = true;
				isInProximitySinceTime[testClient][otherClient] = currentTime - 1;
			}
			
			float inProximitySince = currentTime - isInProximitySinceTime[testClient][otherClient];
			if(inProximitySince <= 0.0)
				inProximitySince = 1.0;
			
			//If this player has been within proximity for 20 seconds we dont care if he is in the FOV 
			//as long as he was ever visible in the last 20 secs, we just assume that they know about each other
			if((timeSinceLastVisibilityTo > 12.0 || inProximitySince < 25.0) && !IsInFieldOfView(plLastPosition[testClient], plRealEyeAngles[testClient], plLastPosition[otherClient])) 
				continue;
			
			//PrintToChatAll("=> %N is in FOV of %N", otherClient, testClient);
			//If the player is within proximity for 35 seconds we dont care if we have a LOS either
			
			bool isVisible = timeSinceLastVisibilityTo < 15;
			
			//Lets re-do the visibility check after 8 seconds
			if(!isVisible || timeSinceLastVisibilityTo > 8) {
				if(IsPointVisible(plLastPosition[testClient], plLastPosition[otherClient])) {
					lastVisibleTime[testClient][otherClient] = currentTime;
					isVisible = true;
				}
			}
			
			//Only do teaming stuff if client ever was visible and is in proximity, or is currently visible.
			if(isVisible || (inProximitySince > 35.0 && lastVisibleTime[testClient][otherClient] != 0)) {
				//PrintToChatAll("=> %N is visible to %N", otherClient, testClient);
				//210 to shift the decimal place
				float teamingFactor = 260.0 / distance;
				
				//If the player hasnt attacked at all this game, lets assume he did, right now, and just re-check next time.
				if(lastAttackTime[testClient] == 0.0) {
					lastAttackTime[testClient] = GetGameTime();
				} else {
					float teamingFactorMulti = lastFireDiff;
					
					//If the otherclient was attacked by the testedclient lets factor in that instead
					if(totalDamageDealt[testClient][otherClient] > 0.0)
						teamingFactorMulti = timeSinceLastDamageTo;
					
					//Lower the multiplicator when the client being tested has fists, having a weapon next to somebody visible is way more likely to be teaming
					if((GetPlayerFakeWeaponSlot(testClient) == SLOT_FISTS || GetPlayerFakeWeaponSlot(testClient) == SLOT_BINOCULAR) && teamingFactorMulti > 8.0)
						teamingFactorMulti = 8.0;
					else if(teamingFactorMulti > 15.0)
						teamingFactorMulti = 15.0;
						
					//If the client has attacked somebody else last, and has not attacked the test client for a long while (But has done so before), up the multiplicator, a lot
					if(lastAttackAtClient[testClient] != otherClient && timeSinceLastDamageTo / lastFireDiff > 20 && totalDamageDealt[testClient][otherClient] > 0.0)
						teamingFactorMulti *= 5.0;
					
					//Finally, lets make the multi dependant off the time the client is within proximity already.
					
					//Setting some sane cap
					if(inProximitySince >= 30.0)
						inProximitySince = 30.0;
					
					inProximitySince /= 25.0;
					//Make it a curve func
					inProximitySince = inProximitySince * inProximitySince;
					
					playerTeamingScore[testClient][otherClient] += (teamingFactor * teamingFactorMulti) * inProximitySince;
				}
			} else if(playerTeamingScore[testClient][otherClient] > 1.0) {
				playerTeamingScore[testClient][otherClient] *= 0.99; //Remove 1% from the teaming score every check-tick conditions arent met.
			}
		}
	}
	
	LoopIngameClients(i) {
		if(alreadyBanned[i])
			continue;
		
		/*bool aHasWeapon = false;
		for (int wpSl = 1; wpSl <= 3; wpSl++) {
			if(!aHasWeapon && IsValidEntity(GetPlayerFakeWeaponSlotWeapon(i, wpSl)))
				aHasWeapon = true;
		}
		
		if(!aHasWeapon)
			continue;*/
		
		LoopIngameClients(otherClient) {
			if(playerTeamingScore[i][otherClient] + playerTeamingScore[otherClient][i] > 420.0 && playerTeamingScore[i][otherClient] > 250.0 && IsValidClient(g_iTvClient)) {
				TE_SetupBeamPoints(plLastPosition[i], plLastPosition[otherClient], BeamModelIndex, 0, 0, 0, 1.5, 8.0, 0.1, 0, 0.0, {255, 125, 0, 150}, 0);
				TE_SendToClient(g_iTvClient);
				
				if(IsValidClient(g_iTvClient))
					CPrintToChat(g_iTvClient, "{default}[Go4TK] {red}%N (%s){default} has high teaming score to {red}%N!{default} Score:{green} %.2f", i, client_AuthIds64[i], otherClient, playerTeamingScore[i][otherClient]);
			}
		
			//Client doesnt have high enough teaming score to the other player
			if(playerTeamingScore[i][otherClient] < DEFO_IS_TEAMING_ABOVE)
				continue;
				
			//Other player doesnt have high enough teaming score to client
			if(playerTeamingScore[otherClient][i] < DEFO_IS_TEAMING_ABOVE)
				continue;
				
			//Make sure both of them actually have a weapon to prevent two people running away from each other forever from getting banned
			/*bool bHasWeapon = false;
			for (int wpSl = 1; wpSl <= 3; wpSl++) {
				if(!bHasWeapon && IsValidEntity(GetPlayerFakeWeaponSlotWeapon(otherClient, wpSl)))
					bHasWeapon = true;
			}
		    
			if(!bHasWeapon)
				continue;*/
			
			if(!alreadyBanned[i]) {
				#if !defined DEBUG
					FormatEx(sumTmpStr, sizeof(sumTmpStr), "Game: %i, Score: %.1f, Other: %i;%s", Backend_GetCurrentRoundId(), playerTeamingScore[i][otherClient], ClientUserDbId[otherClient], toSetName[otherClient]);
					//Ban for 7 days
					alreadyBanned[i] = Backend_BanUser(i, _, 604800, SERVER_GAMEMODE == GAMEMODE_SOLO ? "Teaming in Solos" : "Teaming in Duos with players not in the lobby", sumTmpStr);
					
					if(IsValidClient(g_iTvClient))
						CPrintToChat(g_iTvClient, "{default}[Go4TK] Banned {red}%N (%s){default} for teaming with {red}%N.{default} Score:{green} %.2f", i, client_AuthIds64[i], otherClient, playerTeamingScore[i][otherClient]);
				#else
					CPrintToChatAll("{default}[Go4TK] Would've banned {red}%N{default} for teaming with {red}%N.{default} Score:{green} %i", i, otherClient, playerTeamingScore[i][otherClient]);
					
					alreadyBanned[i] = true;
				#endif
			}
			
			if(!alreadyBanned[otherClient]) {
				#if !defined DEBUG
					FormatEx(sumTmpStr, sizeof(sumTmpStr), "Game: %i, Score: %.1f, Other: %i;%s", Backend_GetCurrentRoundId(), playerTeamingScore[otherClient][i], ClientUserDbId[i], toSetName[i]);
					//Ban for 7 days
					alreadyBanned[otherClient] = Backend_BanUser(otherClient, _, 604800, SERVER_GAMEMODE == GAMEMODE_SOLO ? "Teaming in Solos" : "Teaming in Duos with players not in the lobby", sumTmpStr);
					
					if(IsValidClient(g_iTvClient))
						CPrintToChat(g_iTvClient, "{default}[Go4TK] Banned {red}%N (%s){default} for teaming with {red}%N.{default} Score:{green} %.2f", i, client_AuthIds64[i], otherClient, playerTeamingScore[i][otherClient]);
				#else
					CPrintToChatAll("{default}[Go4TK] Would've banned {red}%N{default} for teaming with {red}%N.{default} Score:{green} %i", otherClient, i, playerTeamingScore[otherClient][i]);
					
					alreadyBanned[otherClient] = true;
				#endif
			}
		}
	}
}

Action TeamingCheckTimer(Handle timer) {
	DoTeamingCheck();
}

void Teaming_DamageDealt(int attacker, int attacked, int theDamage) {
	if(IsValidClient(attacker) && IsValidClient(attacked) && IsPlayerAlive(attacked)) {
		totalDamageDealt[attacker][attacked] += theDamage;
		
		lastDamageDealtToTime[attacker][attacked] = GetGameTime();
		
		lastAttackAtClient[attacker] = attacked;
	}
}

int SortDesc(int[] x, int[] y, int[][] array, any data) {
    if (x[1] > y[1])
        return -1;
    return x[1] < y[1];
}

void Teaming_CheckForClientsTeamedWith(int client, int showTo) {
	int testArray[MAXPLAYERS+1][2];
	
	LoopClients(i) {
		testArray[i][0] = i;
		testArray[i][1] = RoundToNearest(playerTeamingScore[client][i]);
	}
	
	SortCustom2D(testArray, MAXPLAYERS+1, SortDesc);
	
	CPrintToChat(showTo, "[Go4TK] Teaming check for {red}%N", client);
	
	for(int i = 0; i < 7; i++) {
		if(testArray[i][1] == 0)
			break;
	
		CPrintToChat(showTo, "{green}=>{default} Score to {lightgreen}%s{default}(%i): {blue}%i", toSetName[testArray[i][0]], ClientUserDbId[testArray[i][0]], testArray[i][1]);
		if(testArray[i][1] > 100) {
			CPrintToChat(showTo, "{green}  =>{default} Score to self: {blue}%.0f", playerTeamingScore[testArray[i][0]][client]);
			PrintToChat(showTo, "=============");
		}
	}

	//playerTeamingScoreSortable[testClient][otherClient]
}