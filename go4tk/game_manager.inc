//Works, DONT fuck with it.
//Bots somehow manage to FUCK UP their viewangles
char directions[25][4] = {"W", "WSW", "SW", "SSW", "S", "SSE", "SE", "ESE", "E", "ENE", "NE", "NNE", "N", "NNW", "NW", "WNW", "W", "BOT", "BOT", "BOT", "BOT", "BOT", "BOT", "BOT", "BOT"};

bool hasGasSpread = false;
float prevTick = 0.0;
int prevFullSecond = 0;

static int gasToAnnounce = 0;

static float physicalPositions[MAP_LAYERS][3];

public Action Go4TK_Tick(Handle timer) {
	alivePlayers = 0;
	LoopIngameClients(client)
		if(IsPlayerAlive(client)) alivePlayers++;

	int curThing = gasToAnnounce;
	int toAnnounceGasClients[MAXPLAYERS+1];
	int toAnnounceGasClientCount = 0;

	bool layerHasPlayers[4];

	while(alivePlayers > 0) {
		gasToAnnounce++;

		if(gasToAnnounce >= 4)
			gasToAnnounce = 0;

		bool doBreak = false;

		LoopIngameClients(c) {
			if(currentPlayerLayer[c] == gasToAnnounce && c != g_iTvClient) {
				doBreak = true;
				toAnnounceGasClients[toAnnounceGasClientCount++] = c;
			}

			layerHasPlayers[currentPlayerLayer[c]] = true;
		}

		if(doBreak || curThing == gasToAnnounce)
			break;
	}

	bool announceToTv = false;

	if(IsValidClient(g_iTvClient) && g_bRecordDemos && layerHasPlayers[gasToAnnounce]) {
		announceToTv = true;
		toAnnounceGasClients[toAnnounceGasClientCount] = g_iTvClient;
	}

	float beamTimeMultiplier = 0.0;

	for(int y = 0; y < 4; y++) {
		if(layerHasPlayers[y])
			beamTimeMultiplier += 1.0;
	}

	if(beamTimeMultiplier == 0.0)
		beamTimeMultiplier = 1.0;

	float curTime = GetGameTime();

	//Since this timer runs every .25 seconds, its impossible to be within the to-check time - .25 for two runs
	#define ifTimeIs(%1) if(prevTick < %1 && curTime >= %1)

	if(Go4TK_Game_InProgress) {
		bool announceGasAndSafezone = false;

		#if !defined NOWIN
			if(ActiveLobbiesCached <= 1 && Go4TK_Game_StartTime + 15.0 < curTime) {
				Go4TK_Game_InProgress = false;

				//Well, the winners gotta get their score somehow right?
				if(alivePlayers > 0) LoopIngameClients(client) {
					if(IsPlayerAlive(client))
						Lobby_HandlePlayerElimination(client, -1, true);
					else
						ShowDisconnectMenu(client, "Game ending");
				}

				//Might as well give it some headroom to work with
				EndMatchDelayed(15.0);

				Go4TK_Game_Ended();

				return;
			}
		#endif
		static float skyboxPositions[MAP_LAYERS][3];
		static float skyboxRad;
		static float SafezoneWallOffset = 0.0;
		static float CurrentSafezoneDamage = 0.0;

		#if !defined NOSAFEZONE
			//Define and announce the first safezone X seconds after the roundstart
			ifTimeIs(Go4TK_Game_StartTime + TIME_BEFORE_FIRST_WAVE) {
				Go4TK_Next_Safezone_Shrink = curTime + TIME_BETWEEN_WAVES;
				Go4TK_Safezone_Shrink_Targettime = Go4TK_Next_Safezone_Shrink + GAS_TRANSITION_TIME;

				targetSafezoneRadius = FIRST_SAFEZONE_SIZE;

				GetRandomSpawnableSkyVectorLogical(targetSafezoneCenter, 12000);
				targetSafezoneCenter[2] = 4200.0; //lower the vertical coord a bit.
				announceGasAndSafezone = true;

				HudMsgToAll(HUDMSG_CHANNEL_GAMEINFOS, vAs(float, { -1.0, 0.02 }), { 255, 255, 255, 255 }, {255, 0, 255, 255}, 2, _, 2.0, 10.0, 5.0, "The new safezone has been marked on your map.\nRestricting play-area in %.0f seconds", TIME_BETWEEN_WAVES);
			}

			if(SafezoneWallOffset >= 512.0)
				SafezoneWallOffset = 16.0;
			else
				SafezoneWallOffset += 16.0;

			ifTimeIs(Go4TK_Next_Safezone_Shrink) {
				Debug("curr: %.2f\tTarget: %.2f\tCheck1: %b\tCheck2: %b", curTime, Go4TK_Next_Safezone_Shrink, curTime >= Go4TK_Next_Safezone_Shrink - 0.26, curTime <= Go4TK_Next_Safezone_Shrink + 0.01);
				HudMsgToAll(HUDMSG_CHANNEL_GAMEINFOS, vAs(float, { -1.0, 0.02 }), { 255, 100, 100, 255 }, {0, 0, 0, 0}, _, _, 2.0, 10.0, _, "Restricting play area...");
			}

			float trAmount;
			float trProgress;
			float trMultiplier;

			if(curTime >= Go4TK_Next_Safezone_Shrink && curTime <= Go4TK_Safezone_Shrink_Targettime) {
				trAmount = Go4TK_Safezone_Shrink_Targettime - Go4TK_Next_Safezone_Shrink; //Seconds it will take to transition

				trProgress = trAmount - (Go4TK_Safezone_Shrink_Targettime - curTime); //Current second in transition

				if(trProgress < 0.1)
					trProgress = 0.1;

				trMultiplier = trProgress / trAmount;

				currentSafezoneRadius = transitionSafezoneRadius - (trMultiplier * (transitionSafezoneRadius - targetSafezoneRadius));

				for(int i = 0; i <= 2; i++)
					currentSafezoneCenter[i] = transitionSafezoneCenter[i] - (trMultiplier * (transitionSafezoneCenter[i] - targetSafezoneCenter[i]));

				hasGasSpread = true;
				announceGasAndSafezone = true;
			} else if (curTime >= Go4TK_Safezone_Shrink_Targettime && Go4TK_Safezone_Shrink_Targettime > Go4TK_Next_Safezone_Shrink && currentSafezoneRadius > MIN_SAFEZONE_SIZE) {
				currentSafezoneCenter = targetSafezoneCenter;
				currentSafezoneRadius = targetSafezoneRadius;

				transitionSafezoneCenter = targetSafezoneCenter;
				transitionSafezoneRadius = targetSafezoneRadius;

				//Define and announce new safezone
				announceGasAndSafezone = true;

				Go4TK_Next_Safezone_Shrink = curTime + TIME_BETWEEN_WAVES;
				Go4TK_Safezone_Shrink_Targettime = Go4TK_Next_Safezone_Shrink + GAS_TRANSITION_TIME;

				targetSafezoneRadius = targetSafezoneRadius * SAFEZONE_SHRINK_PER_WAVE;

				if(alivePlayers < SHRINK_FASTER_BELOW) {
					float extraShrink = float(alivePlayers) / float(SHRINK_FASTER_BELOW);

					if(extraShrink < 0.8)
						extraShrink = 0.8;

					targetSafezoneRadius *= extraShrink;
				}

				Debug("Shrink this wave: %f", transitionSafezoneRadius - targetSafezoneRadius);

				if(transitionSafezoneRadius - targetSafezoneRadius < MIN_SHRINK_PER_WAVE)
					targetSafezoneRadius = targetSafezoneRadius - MIN_SHRINK_PER_WAVE;

				if(targetSafezoneRadius < MIN_SAFEZONE_SIZE)
					targetSafezoneRadius = MIN_SAFEZONE_SIZE;

				float randomAng; float shrinkby;

				if(SHRINK_TOWARDS_QUADRANT_UNTIL <= targetSafezoneRadius && targetSafezoneRadius <= SHRINK_TOWARDS_QUADRANT_BELOW) {
					MAP_LAYERS curSafezoneML = GetOriginLayerFromLogical(currentSafezoneCenter);
					if(curSafezoneML == ML_BIG_MILITARY)
						randomAng = GetRandomFloat(300.0, 330.0);
					else if(curSafezoneML == ML_AIRPORT)
						randomAng = GetRandomFloat(210.0, 240.0);
					else if(curSafezoneML == ML_SMALL_MILITARY)
						randomAng = GetRandomFloat(120.0, 150.0);
					else
						randomAng = GetRandomFloat(30.0, 60.0);

					shrinkby = GetRandomFloat((currentSafezoneRadius - targetSafezoneRadius) / 1.5, currentSafezoneRadius - targetSafezoneRadius);

					//shrinkby = currentSafezoneRadius - targetSafezoneRadius;
				} else {
					randomAng = 359.0 * GetURandomFloat();

					shrinkby = (currentSafezoneRadius - targetSafezoneRadius) * GetURandomFloat();
				}
				//Angle to radians
				randomAng *= PI / 180.0;

				targetSafezoneCenter[0] = currentSafezoneCenter[0] + (shrinkby * Cosine(randomAng));
				targetSafezoneCenter[1] = currentSafezoneCenter[1] + (shrinkby * Sine(randomAng));

				HudMsgToAll(HUDMSG_CHANNEL_GAMEINFOS, vAs(float, { -1.0, 0.02 }), { 255, 255, 255, 255 }, {255, 0, 255, 255}, 2, _, 2.0, 10.0, 5.0, "The new safezone has been marked on your map.\nRestricting play-area in %.0f seconds", TIME_BETWEEN_WAVES);
			} else if(curTime <= Go4TK_Next_Safezone_Shrink) {
				ifTimeIs(Go4TK_Next_Safezone_Shrink - 10.0) {
					HudMsgToAll(HUDMSG_CHANNEL_GAMEINFOS, vAs(float, { -1.0, 0.02 }), { 255, 100, 100, 255 }, {0, 0, 0, 0}, _, _, 2.0, 10.0, _, "Restricting play-area in 10 seconds");

					EmitAmbientSound(map_pakked_sounds[Go4TK_SafezoneShrinkSound], NULL_VECTOR, SOUND_FROM_WORLD, 1);
				}
			}

			if(announceGasAndSafezone) {
				GetPhysicalVectorOnAllLaysersFromLogicalPosition(currentSafezoneCenter, physicalPositions);

				for(int i = 0; i < 4; i++)
					GetWorldPointIn3dSkybox(physicalPositions[i], skyboxPositions[i]);

				skyboxRad = currentSafezoneRadius / 8.0;

				CurrentSafezoneDamage = (FIRST_SAFEZONE_SIZE - currentSafezoneRadius) / 16000.0;
				if(CurrentSafezoneDamage > 4.0)
					CurrentSafezoneDamage = 4.0;
				else if(CurrentSafezoneDamage < 1.0)
					CurrentSafezoneDamage = 1.0;
			}
		#endif

		static bool dealSafezoneDamageNow;

		dealSafezoneDamageNow = !dealSafezoneDamageNow;

		LoopIngameClients(client) {
			if(IsClientSourceTV(client))
			 	continue;

			GetEntDataVector(client, CBaseEntity_vecOrigin, tmpOrigin);
			currentPlayerLayer[client] = AsInt(GetOriginLayer(tmpOrigin));

			tmpOrigin[2] = physicalPositions[currentPlayerLayer[client]][2];
			float PlayerDistanceFromSafezoneCenter = GetVectorDistance(tmpOrigin, physicalPositions[currentPlayerLayer[client]]);

			UpdateHudInfoForClient(client, tmpOrigin);

			if(announceGasAndSafezone)
				SendCurrentSafezoneToClient(client);

			//Since we only want the horizontal distance, not vertical..

			if(IsPlayerAlive(client) && hasGasSpread && (PlayerDistanceFromSafezoneCenter > currentSafezoneRadius || currentSafezoneRadius <= MIN_SAFEZONE_SIZE)) {
				if(dealSafezoneDamageNow) {
					HM_TakeDamge2(client, 0, 0, CurrentSafezoneDamage, DMG_NERVEGAS);

					HM_OnTakeDamage(client, RoundToCeil(CurrentSafezoneDamage));

					if(!plIsCurrentlyOutsideSafezone[client]) {
						FadeScreen({IN_GAS_COLOR, 100}, 250, _, FFADE_OUT, client);

						plIsCurrentlyOutsideSafezone[client] = true;
					}
				}
			} else if(plIsCurrentlyOutsideSafezone[client]) {
				FadeScreen({IN_GAS_COLOR, 100}, 250, 0, FFADE_IN, client);

				plIsCurrentlyOutsideSafezone[client] = false;
			}
		} //LoopIngameClients(client)

		if(hasGasSpread) {
			//Since the Sprites are always force-rotated to face the player i need to make the beams smaller as the player approaches the border to prevent confusion

			physicalPositions[gasToAnnounce][2] -= SafezoneWallOffset;

			skyboxPositions[gasToAnnounce][2] -= SafezoneWallOffset / 16.0;


			int zoneColor[4];

			if(curTime >= Go4TK_Next_Safezone_Shrink && curTime <= Go4TK_Safezone_Shrink_Targettime) {
				zoneColor = {150, 0, 0, 255};
			} else {
				zoneColor = {0, 150, 0, 255};
			}

			static bool ignoreTv[MAP_LAYERS];

			ignoreTv[gasToAnnounce] = !ignoreTv[gasToAnnounce];

			for(int z = 0; z < 7; z += 1) {
				physicalPositions[gasToAnnounce][2] -= z * 512.0;

				skyboxPositions[gasToAnnounce][2] -= z * 32.0;


				int numAnnounce = toAnnounceGasClientCount;

				//Only announce upper and lower ring of ringcircle to TV to minimize demo size
				if((z == 0 || z == 6) && announceToTv && !ignoreTv[gasToAnnounce])
					numAnnounce += 1;

				TE_SendBeamRingCliensEx(toAnnounceGasClients, numAnnounce, physicalPositions[gasToAnnounce], currentSafezoneRadius, BeamModelIndex, _, _, _, 0.30 * beamTimeMultiplier, 32.0, _, zoneColor, _, 30);

				if(skyboxRad < 4096.0) {
					TE_SetupBeamRingPoint(skyboxPositions[gasToAnnounce], skyboxRad, skyboxRad - 0.1, BeamModelIndex, 0, 0, 1, 0.30 * beamTimeMultiplier, 2.0, 0.0, zoneColor, 0, 0);
					TE_Send(toAnnounceGasClients, toAnnounceGasClientCount);

					//TE_SendBeamRingEx(client, skyboxPositions[currentPlayerLayer[client]], skyboxRad, BeamModelIndex, _, _, _, 0.0, 1.0 * spriteSizeModifier, _, {150, 0, 0, 255}, _, 100);
				}

				physicalPositions[gasToAnnounce][2] += z * 512.0;
				skyboxPositions[gasToAnnounce][2] += z * 32.0;
			}

			physicalPositions[gasToAnnounce][2] += SafezoneWallOffset;

			skyboxPositions[gasToAnnounce][2] += SafezoneWallOffset / 16.0;
		}



	} else {




		if(alivePlayers >= MIN_PLAYERS_TO_START || forceStart || (Go4TK_Game_StartDelay != -1.0 && alivePlayers >= 5)) {
			#if !defined NOAUTOSTART
				if(Go4TK_Game_StartDelay == -1.0)
					Go4TK_Game_StartDelay = curTime + GRACE_PERIOD_BEFORE_START_LOW;

				bool nobodyConnectingRightnow = GetClientCount(false) == GetClientCount(true);

				//When theres more than 35 players on the server,
				//and the game would wait more than GRACE_PERIOD_BEFORE_START_RANKED secs before starting, lower it to GRACE_PERIOD_BEFORE_START_RANKED secs.
				if(alivePlayers >= 30 && Go4TK_Game_StartDelay - curTime > GRACE_PERIOD_BEFORE_START_RANKED)
					Go4TK_Game_StartDelay = curTime + GRACE_PERIOD_BEFORE_START_RANKED;

				//When theres more than 40 players on the server, nobody is connecting rn, and the game would wait more than 15 secs before starting, lower it to 15 secs.
				//Or, if nobody is connecting, the queue gate is closed already, and everyone thats reserved has joined, lower it to 15 secs as well.
				if(	Go4TK_Game_StartDelay - curTime > 15 && (
					(alivePlayers >= 40 && nobodyConnectingRightnow) ||
					(nobodyConnectingRightnow && Go4TK_Game_StartDelay - curTime < CLOSE_QUEUE_GATE_AT && Queue_ReservedCount() <= alivePlayers))
				)
					Go4TK_Game_StartDelay = curTime + 15;

				GameRules_SetPropFloat("m_flRestartRoundTime", Go4TK_Game_StartDelay + 0.5);
			#endif


			float curt = Go4TK_Game_StartDelay - curTime;
			//if(curt > -1.0 && curt < 1.0)
			//	Debug("curr: %.2f\tCurr - .25: %.2f\ttoCheck: %.2f", curTime, curTime - 0.27, Go4TK_Game_StartDelay);

			ifTimeIs(Go4TK_Game_StartDelay) {
				//HudMsgToAll(HUDMSG_CHANNEL_GAMEINFOS, vAs(float, { -1.0, 0.01 }), { 255, 0, 0, 255 }, {0, 0, 0, 0}, _, _, 3.0, 4.0, _, "The game has started");

				FadeScreen({0, 0, 0, 255}, 500, 0, FFADE_IN, _, true);

				bool beenSpawned[MAXPLAYERS+1];

				LoopIngameClients(i) {
					if(IsClientSourceTV(i))
					 	continue;

					float spawnPos[3];

					GetRandomSpawnableSkyVector(spawnPos, 1024);

					float randomRotation[3]; randomRotation[1] = GetURandomFloat() * 360.0;

					if(IsPlayerAlive(i)) {
						if(beenSpawned[i])
							continue;

						int clLobby = CurrentClientLobby[i];

						LoopIngameClients(i2) {
							if(beenSpawned[i2])
								continue;

							if(CurrentClientLobby[i2] == clLobby) {
								beenSpawned[i2] = true;

								TeleportEntity(i2, spawnPos, randomRotation, view_as<float>({0.0, 0.0, 0.0}));

								//if(i2 != i)
								//	PrintToChatAll("Spawning %N together with %N because they are in the same team", i, i2);
							}
						}

						//Needs to be per-client otherwise we cant stop it later on
						EmitSoundToClient(i, map_pakked_sounds[Go4TK_ParachuteFlySound], i, SNDCHAN_STATIC);
					}
				}

				CreateTimer(0.0, MakeAllAlivePlayersParachute, _, TIMER_FLAG_NO_MAPCHANGE);

				EmitAmbientSound(map_pakked_sounds[Go4TK_AmbientSound], NULL_VECTOR, SOUND_FROM_WORLD, 1);

				Go4TK_Game_StartTime = GetGameTime();
				Go4TK_Game_StartDelay = -1.0;

				forceStart = false;

				Go4TK_Game_InProgress = true;

				GameRules_SetProp("m_bGameRestart", 0);
				GameRules_SetPropFloat("m_flRestartRoundTime", 0.0);

				//Soo source is fucking broken. If we dont start the record in freezetime, the demo will playback at 10 fps, for whatever fucking reason.
				//This starts a freezetime for .1 seconds when the game starts, which works around that, for whatever.. fucking.. reason.
				GameRules_SetPropFloat("m_fRoundStartTime", GetGameTime() + 0.1);
				GameRules_SetProp("m_bFreezePeriod", 1);

				Go4TK_Game_Started();
			} else if(curt >= 0.0 && prevFullSecond != RoundFloat(curt)) {
				if(prevFullSecond == -1) {
					EmitSoundToAll(map_pakked_sounds[Go4TK_InfoSound]);
					GameRules_SetProp("m_bGameRestart", 1);
				}

				prevFullSecond = RoundFloat(curt);

				if(prevFullSecond <= CLOSE_QUEUE_GATE_AT && prevFullSecond >= CLOSE_QUEUE_GATE_AT - 5.0)
					Queue_SendPlayerRequest(0);

				if(prevFullSecond == 6) {
					GameRules_SetProp("m_bGameRestart", 0);
					GameRules_SetPropFloat("m_flRestartRoundTime", 0.0);

					EmitSoundToAll("ambient/canals/ambience_canals_bell_bg.wav", _, _, _, _, 0.5);

					FadeScreen({0, 0, 0, 255}, 3000, 3000, FFADE_OUT);

					char tmpStr[256];
					FormatEx(tmpStr, sizeof(tmpStr), "You are playing Go for the Kill by Kinsi\n\nhttps://Go4TheKill.net\n\n%s", g_sExtraStartMsg);

					HudMsgToAll(HUDMSG_CHANNEL_GAMEINFOS, vAs(float, { -1.0, 0.2 }), { 200, 150, 255, 255 }, {0, 0, 0, 0}, _, 1.0, 1.0, 6.0, _, tmpStr);

					Go4TK_Game_Prestart();
				} else if((prevFullSecond % 10) == 0 && prevFullSecond != 0) {
					//char tmpStr[64];
					//Format(tmpStr, sizeof(tmpStr), "The game will start in %i seconds", prevFullSecond);

					//HudMsgToAll(HUDMSG_CHANNEL_GAMEINFOS, vAs(float, { -1.0, 0.05 }), { 200, 255, 0, 255 }, {0, 0, 0, 0}, _, _, 1.0, 6.0, _, tmpStr);

					//if(prevFullSecond <= 40 && g_bForceUnranked)
					//	PrintToChatAll("[Go4TK] This server is being tested right now, and could be unstable / crash. Because of this, this round will be unranked.");
				}
			}
		} else {
			Go4TK_Game_StartDelay = -1.0;
			prevFullSecond = -1;
			GameRules_SetProp("m_bGameRestart", 0);
			//Allow balls in lobby ¯\_(ツ)_/¯
			SetConVarInt(FindConVar("phys_pushscale"), 9000);
			SetConVarInt(FindConVar("sv_airaccelerate"), 100);
		}
	}

	prevTick = curTime;
}

public Action MakeAllAlivePlayersParachute(Handle timer) {
	LoopIngameClients(i)
		if(IsPlayerAlive(i) && !IsClientSourceTV(i))
			GivePlayerParachute(i);
}

public Action Go4TK_GameProgressor(Handle timer) {
	LoopIngameClients(client) {
		if(GetClientTeam(client) == CS_TEAM_NONE || IsClientSourceTV(client))
			continue;

		//Only do ad stuff if the game is at most 30 sec before the start
		float curTime = GetGameTime();
		#if !defined DEBUG
			if(userSettings[client][CFG_videoAds] && (Go4TK_Game_StartDelay == -1 || Go4TK_Game_StartDelay - curTime > 20.0) && (ws_clients[client] != INVALID_WEBSOCKET_HANDLE || clientReceivedAdAt[client] != -1.0)) {
				//If the client hasnt been served the ad yet retry until we finally did.
				if(clientReceivedAdAt[client] == -1.0 && ws_clients[client] != INVALID_WEBSOCKET_HANDLE) {
					ShowVGUIPanelEx(client, "HELLOO", VIDEO_ADS, _, _, false);
					clientReceivedAdAt[client] = GetGameTime();

					Debug("Trying to serve (wanted) video-ad to client %N because he successfully motd-connected", client);

					HudMsg(client, HUDMSG_CHANNEL_PICKEDUP, vAs(float, { -1.0, 0.6 }), { 0, 255, 0, 255 }, { 0, 255, 255, 255 }, 2, _, _, 10.1, 10.0, "An ad will be opened in the background now.\nFor more details check your settings.\nThanks for supporting Go4TK!");
					continue;
				}

				//If the user accepted video ads we want to delay the motd connection until he has been served the ad until he has seen it for at least 40 seconds
				//Unless the game starts before that, that is ofc.

				//The client hasnt seen the ad for long enough yet

				if(curTime - clientReceivedAdAt[client] >= 50.0)
					CreateTimer(2.0, WS_ConnectClientDelayed, client, TIMER_FLAG_NO_MAPCHANGE);

				if(curTime - clientReceivedAdAt[client] <= 55.0) {
					//PrintToServer("%.1f %.1f %.1f %.1f", Go4TK_Game_StartDelay, Go4TK_Game_StartDelay - curTime, clientReceivedAdAt[client], curTime - clientReceivedAdAt[client]);

					continue;
				}
			}
		#endif

		if(client_connectionwarn_timer[client] != -1 && IsClientInGame(client) && (ws_clients[client] == INVALID_WEBSOCKET_HANDLE || Websocket_GetReadyState(ws_clients[client]) != State_Open)) {
			if(ws_clients[client] != INVALID_WEBSOCKET_HANDLE) {
				Websocket_UnhookChild(ws_clients[client]);

				ws_clients[client] = INVALID_WEBSOCKET_HANDLE;
			}

			if(!Go4TK_Game_InProgress) {
				if(client_connectionwarn_timer[client] > 5) {
					#if !defined DEBUG && !defined NOWSWARN
						client_connectionwarn_timer[client] -= 5;
					#else
						client_connectionwarn_timer[client] = 50;
					#endif
				} else {
					KickClient(client, "Something is blocking your connection to the server. Check https://Go4TheKill.net/FAQ for more informations");
					client_connectionwarn_timer[client] = -1;
					continue;
				}
				#if !defined NOWSWARN
					if(client_connectionwarn_timer[client] < 55)
						QueryClientConVar(client, "cl_disablehtmlmotd", DisableMotdCallback);

					//char msg[512] = "Something is blocking your connection to the server!\nPlease make sure cl_disablehtmlmotd is set to 0!\nSwitchting to Windowed mode and back can possibly fix this\n\nThis notice will disappear once a\nconnection has been established.\n\nIf no connection is established within\n%i seconds you will be kicked!";
				#endif
			}
			CreateTimer(4.0, WS_ConnectClientDelayed, client, TIMER_FLAG_NO_MAPCHANGE);
		}
	}

	if(!Go4TK_Game_InProgress && Go4TK_Game_StartDelay == -1.0 && alivePlayers > 0 && alivePlayers < MIN_PLAYERS_TO_START) {
		char tmpStr[64];
		FormatEx(tmpStr, sizeof(tmpStr), "The game will start once theres at least %i players", MIN_PLAYERS_TO_START);

		HudMsgToAll(HUDMSG_CHANNEL_PROGRESS, vAs(float, { -1.0, 0.05 }), { 200, 255, 0, 255 }, {0, 0, 0, 0}, _, _, 1.0, 5.0, _, tmpStr);
	}

	if(Go4TK_Game_InProgress && g_bRecordDemos && IsValidClient(g_iTvClient)) {
		char specUiText[512]; static char specUiTextPrev[512]; static int lastShow;
		char specUiText2[512]; static char specUiText2Prev[512]; static int lastShow2;

		FormatEx(specUiText, sizeof(specUiText), "T %i, A %i, C %i\n", ActiveLobbiesCached, alivePlayers, GetClientCount());

		LoopIngameClients(client) if(IsPlayerAlive(client)) {
			SubString(toSetName[client], 0, 10, sumTmpStr, sizeof(sumTmpStr));

			if(strlen(specUiText) < 480)
				Format(specUiText, sizeof(specUiText), "%s%s %i %i\n", specUiText, sumTmpStr, ClientUserDbId[client], GetEntData(client, CCSPlayer_iNumRoundKills, 1));
			else
				Format(specUiText2, sizeof(specUiText2), "%s%s %i %i\n", specUiText2, sumTmpStr, ClientUserDbId[client], GetEntData(client, CCSPlayer_iNumRoundKills, 1));
		}

		int curT = GetTime();

		if(!StrEqual(specUiText, specUiTextPrev) || curT - lastShow > 20.0) {
			lastShow = curT;
			strcopy(specUiTextPrev, sizeof(specUiTextPrev), specUiText);
			HudMsg(g_iTvClient, 0, vAs(float, { 0.0, 0.0 }), { 0, 150, 0, 50 }, {0, 0, 0, 0}, _, _, _, 26.0, 0.0, specUiText);
		}

		if(!StrEqual(specUiText2, specUiText2Prev) || curT - lastShow2 > 20.0) {
			lastShow2 = curT;
			strcopy(specUiText2Prev, sizeof(specUiText2Prev), specUiText2);
			HudMsg(g_iTvClient, 1, vAs(float, { 0.18, 0.015 }), { 0, 150, 120, 50 }, {0, 0, 0, 0}, _, _, _, 26.0, 0.0, specUiText2);
		}
	}
}

static void DisableMotdCallback(QueryCookie cookie, int client, ConVarQueryResult result, const char[] cvarName, const char[] cvarValue) {
	char msg[512] = "Something is blocking your connection to the server!\n\n%s\n\nThis notice will disappear once a\nconnection has been established.";

	if(cvarValue[0] != '0') {
		Format(msg, sizeof(msg), msg, "Please make sure cl_disablehtmlmotd is set to 0!");
	} else if(strlen(cvarValue) > 0) {
		Format(msg, sizeof(msg), msg, "Switching to windowed mode and back can possibly fix this");
	} else {
		Format(msg, sizeof(msg), msg, "Please make sure cl_disablehtmlmotd is set to 0!\n\nSwitching to windowed mode and back can also possibly fix this");
	}

	Format(msg, sizeof(msg), "%s\n\nIf no connection is established within\n%i seconds you will be kicked!", msg, client_connectionwarn_timer[client]);

	HudMsg(client, HUDMSG_CHANNEL_PICKEDUP, vAs(float, { -1.0, -1.0 }), { 255, 0, 0, 255 }, { 255, 0, 255, 255 }, 2, _, _, 5.1, 5.0, msg);

	//Workaround for the stupid motd "stuck" url bug
	//trying to load a URL on the same domain as the current one will only result in the current one being reloaded
	//Need to load a url on a diff. domain to allow loading the other one first.
	ShowVGUIPanelEx(client, "HELLOO", "http://kinsi.me/sinkhole", _, _, false);
}

void Go4TK_Game_Prestart() {
	InitWorldEnts();

	SpawnLoot();

	bool isRanked = alivePlayers >= MIN_PLAYERS_FOR_RANKED;

	#if defined DEBUG || defined ALLRANKED
		isRanked = true;
	#endif

	if(g_bForceUnranked)
		isRanked = false;

	if(!Backend_CreateRound(isRanked))
		isRanked = false;

	if(!isRanked)
		PrintCenterTextAll("<font color='#ff0000'>This round will be unranked! The result wont count towards your leaderboard score!</font>");

	transitionSafezoneCenter = vAs(float, DEFAULT_GAS_CENTER);
	transitionSafezoneRadius = DEFAULT_GAS_RADIUS;

	currentSafezoneCenter = vAs(float, DEFAULT_GAS_CENTER);
	currentSafezoneRadius = DEFAULT_GAS_RADIUS;

	targetSafezoneCenter = vAs(float, DEFAULT_GAS_CENTER);
	targetSafezoneRadius = DEFAULT_GAS_RADIUS;

	Go4TK_Next_Safezone_Shrink = 9999999999.0;
	Go4TK_Safezone_Shrink_Targettime = 0.0;

	GetPhysicalVectorOnAllLaysersFromLogicalPosition(currentSafezoneCenter, physicalPositions);

	hasGasSpread = false;
}

void Go4TK_Game_Started() {
	//Dont allow balls anymore :(
	SetConVarInt(FindConVar("phys_pushscale"), 1);
	SetConVarInt(FindConVar("sv_airaccelerate"), 12);

	AcceptEntityInputEx("*", "killme", "Kill", _, _, true);
	AcceptEntityInputEx("*", "fail1", "Kill");
	AcceptEntityInputEx("*", "fail2", "Kill");
	AcceptEntityInputEx("*", "fail3", "Kill");
	AcceptEntityInputEx("*", "fail4", "Kill");
	AcceptEntityInputEx("*", "fail5", "Kill");
	AcceptEntityInputEx("*", "fail6", "Kill");
	AcceptEntityInputEx("*", "fail7", "Kill");
	AcceptEntityInputEx("*", "fail8", "Kill");
	AcceptEntityInputEx("*", "lj1", "Kill");
	AcceptEntityInputEx("*", "lj2", "Kill");
	AcceptEntityInputEx("*", "lj3", "Kill");
	AcceptEntityInputEx("*", "lj4", "Kill");
	AcceptEntityInputEx("*", "lj5", "Kill");
	//Seems like these two ents are created by default, but are unnecessary. Needs more testing.
	//AcceptEntityInputEx("*", "commentary_semaphore", "Kill");
	//AcceptEntityInputEx("*", "BigNet", "Kill");

	Lobby_GetActiveLobbyCount(); //Calling it here once just to populate the lobby manager's cached value

	UpdateTrashtalkStuff();

	bool[] ignoreSameLobby = new bool[ActiveLobbiesCached+1];
	bool alreadyHasLaser[MAXPLAYERS+1];

	LoopIngameClients(i) if(IsPlayerAlive(i)) {
		if(CurrentClientLobby[i] < 0 || CurrentClientLobby[i] > ActiveLobbiesCached)
			continue;

		if(!alreadyHasLaser[i] && g_bRecordDemos && IsValidClient(g_iTvClient)) {
			LoopIngameClients(i2) if(IsPlayerAlive(i2)) {
				if(CurrentClientLobby[i2] > 0 && CurrentClientLobby[i2] < ActiveLobbiesCached &&CurrentClientLobby[i2] == CurrentClientLobby[i]) {
					TE_SetupBeamEnts(i, i2, BeamModelIndex, 0, 0, 0, 0.0, 1.0, 1.0, 0, 0.0, {0, 255, 125, 50}, 5);

					TE_SendToClient(g_iTvClient);

					alreadyHasLaser[i] = true;
					alreadyHasLaser[i2] = true;
				}
			}
		}

		if(!ignoreSameLobby[CurrentClientLobby[i]]) {
			ignoreSameLobby[CurrentClientLobby[i]] = true;

			int aliveLobbyCount = 0;

			LoopIngameClients(i2) if(IsPlayerAlive(i2)) {
				if(CurrentClientLobby[i2] == CurrentClientLobby[i])
					aliveLobbyCount++;
			}

			//Only one person in this lobby to begin with? lets remove his skin because its unnecessary anyways.
			if(aliveLobbyCount == 1) {
				CPS_RemoveSkin(i, CPS_RENDER);
				Debug("Well remove the skin of %N because he is playing alone ¯\\_(ツ)_/", i);
			}
		}
	}

	if(g_bRecordDemos) {
		ServerCommand("tv_record go4tk_live_%i", ws_port+1);
	}
}

void Go4TK_Game_Ended() {
	if(g_bRecordDemos) {
		FormatEx(sumTmpStr, sizeof(sumTmpStr), "go4tk_live_%i.dem", ws_port + 1);

		if(FileExists(sumTmpStr)) {
			DataPack dPack;

			CreateDataTimer(12.0, RenameRecdDemo, dPack);

			dPack.WriteCell(Backend_GetCurrentRoundId());

			dPack.WriteString(sumTmpStr);

			CreateTimer(8.0, StopDemo);
		}
	}

	Backend_FinishRound();
}

Action StopDemo(Handle timer) {
	ServerCommand("tv_stoprecord");
}

Action RenameRecdDemo(Handle timer, DataPack dPack) {
	dPack.Reset();

	int rndId = dPack.ReadCell();

	dPack.ReadString(sumTmpStr, sizeof(sumTmpStr));

	if(FileExists(sumTmpStr)) {
		char newDemoName[64];

		if(rndId == -1)
			FormatEx(newDemoName, sizeof(newDemoName), "records/go4tk_fin__%i.dem", GetTime());
		else
			FormatEx(newDemoName, sizeof(newDemoName), "records/go4tk_fin_%i.dem", rndId);

		RenameFile(newDemoName, sumTmpStr);
	}
}