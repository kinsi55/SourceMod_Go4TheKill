static Socket BACKEND_CONN = null;
static bool backendIsConnected = false;

static StringMap acceptedSteamids = null;

static int requestedPlayerCount = 0;

public int Queue_GetRequestedCount() {
	return requestedPlayerCount;
}

public void InitQueueConn() {
	if(BACKEND_CONN != null)
		return;
	
	if(acceptedSteamids == null)
		acceptedSteamids = new StringMap();
	
	if(g_bUseQueue) {
		#if !defined NO_QUEUE && !defined DEBUG
			char tmpBuf[128];
			BuildPath(Path_SM, tmpBuf, sizeof(tmpBuf), "data/reservedSlots%i.txt", ws_port);
			
			if(BACKEND_CONN == null && FileExists(tmpBuf)) {
				if(!HALT_AFTER_MATCH) {
					File queueFlushFile = OpenFile(tmpBuf, "r", false);
					
					if(queueFlushFile != null) {
						char readLine[4096];
						
						queueFlushFile.ReadString(readLine, sizeof(readLine), -1);
						
						/*			stSn.GetKey(i, sumTmpStr, sizeof(sumTmpStr));
		
						queueFlushFile.WriteString(sumTmpStr, false);
					
						queueFlushFile.WriteString(":", false);
						
						acceptedSteamids.GetString(sumTmpStr, sumTmpStr, sizeof(sumTmpStr));
					
						queueFlushFile.WriteString(sumTmpStr, false);
						
						if(i != acceptedSteamids.Length -1) {
							queueFlushFile.WriteString("|", false);*/
						
						char resvStrs[50][80];
						
						int reservations = ExplodeString(readLine, "|", resvStrs, 50, 80);

						for(int i = 0; i < reservations; i++) {
							char resvStrs2[2][64];
							
							ExplodeString(resvStrs[i], ":", resvStrs2, 2, 64);
							
							//Format(readLine, sizeof(readLine), "ReserveSlot;%s;%s", resvStrs2[0], resvStrs2[1]);
							
							Queue_ReserveSlotFor(resvStrs2[0], resvStrs2[1]);
							
							PrintToServer("Restoring slot reservation of client %s of lobby %s", resvStrs2[0], resvStrs2[1]);
						
							//Queue_ConnMessage(null, readLine, sizeof(readLine), 0);
						}
					}
					
					queueFlushFile.Close();
				}
				
				DeleteFile(tmpBuf, false);
			}
		#endif
		
		#if !defined NO_QUEUE
			backendIsConnected = false;
			
			BACKEND_CONN = new Socket(SOCKET_TCP, Queue_ConnError);
			
			BACKEND_CONN.SetOption(SocketReuseAddr, true);
			
			BACKEND_CONN.SetOption(SocketKeepAlive, true);
			
			BACKEND_CONN.SetOption(SocketReceiveTimeout, 45000);
			
			BACKEND_CONN.Connect(Queue_ConnConnected, Queue_ConnMessage, Queue_ConnDisconnected, g_sQueueIp, g_iQueuePort);
		#endif
	}
}

public int Queue_ReservedCount() {
	if(acceptedSteamids == null)
		return 0;
		
	return acceptedSteamids.Size;
}

public void Queue_RequestPlayers() {
	//Any player that currently is not alive should be a reserved slot, as far as the pre-game goes that is at least.
	int needed = PLAYER_LIMIT - (alivePlayers + Queue_ReservedCount());
	
	PrintToServer("REQUESTING %i PLAYERS NAOW", needed);
	
	if(needed > 0)
		Queue_SendPlayerRequest(needed);
}

public void Queue_SendPlayerRequest(int amount) {
	//Technically only possible on plugin reload.
	//if(requestedPlayerCount == amount)
	//	return;
	
	if(Go4TK_Game_InProgress && amount != 0) {
		amount = 0;
		PrintToServer("REQUESTING 0 BECAUSE IN PROGRESS!");
	}
	
	//30 seconds before the round starts its pretty much impossible to accept + join
	if(Go4TK_Game_StartDelay != -1 && 0.0 <= Go4TK_Game_StartDelay - GetGameTime() <= CLOSE_QUEUE_GATE_AT) {
		amount = 0;
		PrintToServer("REQUESTING 0 BECAUSE NOT -1");
	}
	
	if(HALT_AFTER_MATCH) {
		PrintToServer("QUEUE SYSTEM HALTED FOR MAINTENANCE");
		amount = 0;
	}

	requestedPlayerCount = amount;
	
	FormatEx(sumTmpStr, sizeof(sumTmpStr), "RequestPlayers;%i", requestedPlayerCount);
	
	if(BACKEND_CONN != null && backendIsConnected) {
		BACKEND_CONN.Send(sumTmpStr);
		LogMessage("Requested %i players", amount);
	} else
		LogError("Cannot request players right(%i) now because we have no backend connection, queued for later....", requestedPlayerCount);
}

static void Queue_ConnMessage(Socket socket, char[] receiveData, const int dataSize, any arg) {
	Debug("> Received message from backend! %s", receiveData);
	
	char receiveSplit[65][64];
	
	int splitVals = ExplodeString(receiveData, ";", receiveSplit, 65, 64);
	
	if(splitVals < 2) //ReserveSlots lobbyid steamid
		return;
	
	//Remove the cmd from the data
	strcopy(receiveData, dataSize, receiveData[strlen(receiveSplit[0])]);
	
	if(StrEqual(receiveSplit[0], "ReserveSlots")) {
		//Technically, this should NEVER fail, unless we get invalid data
		
		char reservedSteamids[2048];
		
		FormatEx(reservedSteamids, sizeof(reservedSteamids), "ReservedSlots;%s", receiveSplit[1]);
		
		for(int i = 2; i < splitVals; i++) {
			//Technically, this should NEVER fail, unless we get invalid data
		
			if(Queue_ReserveSlotFor(receiveSplit[i], receiveSplit[1]))
				Format(reservedSteamids, sizeof(reservedSteamids), "%s;%s", reservedSteamids, receiveSplit[i]);
		}
		
		if(BACKEND_CONN != null && backendIsConnected) {
			BACKEND_CONN.SetOption(SocketReceiveTimeout, 0);
		
			BACKEND_CONN.Send(reservedSteamids);
		}
	}
}

public bool Queue_ReserveSlotFor(const char[] steamid64, const char[] lobbyid) {
	if(strlen(steamid64) != 17) {
		Debug("Tried to reserve slot for Steamid thats not 17 characters long! '%s'", steamid64);
	} else if(!acceptedSteamids.GetString(steamid64, sumTmpStr, 0)) {
		acceptedSteamids.SetString(steamid64, lobbyid, true);
		requestedPlayerCount--;
		
		DataPack stringProxy;
		CreateDataTimer(180.0, Queue_RemoveReservedSlot, stringProxy);
		
		stringProxy.WriteString(steamid64);
		
		LogMessage("Reserving slot for '%s' (Lobby: %s) for 180 seconds", steamid64, lobbyid);
		return true;
	} else {
		//Lets just silently confirm the reservation when trying to reserve somebody who's reserved already. Whats the worst that can happen?
		return true;
	}
	return false;
}

bool Queue_CheckinSteamid(const char[] steamid64, bool checkOnly = false, char[] lobbyId = sumTmpStr, int maxlength = 0) {
	if(acceptedSteamids == null)
		return false;
	
	if(!acceptedSteamids.GetString(steamid64, lobbyId, maxlength))
		return false;
		
	if(!checkOnly)
		acceptedSteamids.Remove(steamid64);
	
	return true;
}

static Action Queue_RemoveReservedSlot(Handle timer, DataPack stringProxy) {
	char steamid[32];
	
	stringProxy.Reset();
	
	stringProxy.ReadString(steamid, sizeof(steamid));
	
	Debug("Time-outing slot reservation for %s", steamid);
	
	//Player is still (not) here..
	if(Queue_CheckinSteamid(steamid)) {
		Backend_TimeoutReservedSlot(steamid);
	
		Queue_RequestPlayers();
	}
}

static void Queue_ConnConnected(Socket socket, any arg) {
	LogMessage("> Connected to backend successfully!");
	
	backendIsConnected = true;
	
	RequestFrame(IdentifyServer);
}

static void IdentifyServer(any data) {
	//Format(sumTmpStr, sizeof(sumTmpStr), "Identify;%s;%i", sServerIPAndPort, requestedPlayerCount);
	char authMsg[128];
	
	FormatEx(authMsg, sizeof(authMsg), "Identify;%s;%i;%s;%s;%s", sServerIPAndPort, requestedPlayerCount, g_sBackendAuth, g_sRegion, g_sType);
	
	if(BACKEND_CONN != null && backendIsConnected)
		BACKEND_CONN.Send(authMsg);
}

static Action Queue_RetryConnection(Handle timer) {
	InitQueueConn();
}

static void Queue_ConnDisconnected(Socket socket, any arg) {
	#if !defined NO_QUEUE
		if(g_bUseQueue) {
			LogError("> Backend connection failed! Reconnecting in 8 seconds..");
			
			delete socket;
			
			backendIsConnected = false;
			BACKEND_CONN = null;
			
			CreateTimer(8.0, Queue_RetryConnection);
		}
	#endif
}

static void Queue_ConnError(Socket socket, const int errorType, const int errorNum, any arg) {
	Queue_ConnDisconnected(socket, arg);
}

void Queue_FlushReservedSlotsToDisk() {
	if(acceptedSteamids == null)
		return;
		
	char tmpBuf[128];
	BuildPath(Path_SM, tmpBuf, sizeof(tmpBuf), "data/reservedSlots%i.txt", ws_port);
	
	File queueFlushFile = OpenFile(tmpBuf, "w", false);
	
	if(queueFlushFile != null) {
		StringMapSnapshot stSn = acceptedSteamids.Snapshot();
	
		for(int i = 0; i < stSn.Length; i++) {
			stSn.GetKey(i, sumTmpStr, sizeof(sumTmpStr));
		
			queueFlushFile.WriteString(sumTmpStr, false);
		
			queueFlushFile.WriteString(":", false);
			
			acceptedSteamids.GetString(sumTmpStr, sumTmpStr, sizeof(sumTmpStr));
		
			queueFlushFile.WriteString(sumTmpStr, false);
			
			if(i != stSn.Length -1) {
				queueFlushFile.WriteString("|", false);
			} else {
				queueFlushFile.WriteString("", true);
			}
		}
		
		queueFlushFile.Close();
	}
}