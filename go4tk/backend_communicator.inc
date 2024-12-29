#pragma newdecls optional
#include <SteamWorks>
#pragma newdecls required

#include <ripext>

static int Go4TK_Game_RoundId = -1;

char toSetClantag[MAXPLAYERS+1][8];
char toSetName[MAXPLAYERS+1][32];

static Database DB_CONN = null;

#define CLIENTFLAG_VIP 1
#define CLIENTFLAG_CANSPEC 2
#define CLIENTFLAG_VERIFIED 4
#define CLIENTFLAG_QUEUEPRIO 8

static int VoidHttpCallback(Handle hRequest, bool bFailure, bool bRequestSuccessful, EHTTPStatusCode eStatusCode, DataPack infoDpack) {
	if(bFailure || !bRequestSuccessful || eStatusCode != k_EHTTPStatusCode200OK) {
		infoDpack.Reset();

		infoDpack.ReadString(sumTmpStr, sizeof(sumTmpStr));

		LogError("[API] REQ failed! %i, %s", eStatusCode, sumTmpStr);
	}

	delete infoDpack;

	//Lets find out if this prevents 499's
	//SteamWorks_GetHTTPResponseBodyCallback(hRequest, VoidHttpDataCallbac);

	delete hRequest;
}

static int SetIngameHttpCallback(Handle hRequest, bool bFailure, bool bRequestSuccessful, EHTTPStatusCode eStatusCode, DataPack infoDpack) {
	infoDpack.Reset();

	/*		DataPack data = new DataPack();

		data.WriteCell(client);

		if(IsClientConnected(client))
			data.WriteCell(GetClientUserId(client));
		else
			data.WriteCell(-1);

		data.WriteCell(ingame ? 1 : 0);*/

	int setClient = infoDpack.ReadCell();
	int setUser = infoDpack.ReadCell();
	bool setState = infoDpack.ReadCell() == 1;


	if(!setState && (!IsClientConnected(setClient) || GetClientOfUserId(setUser) == setClient)) {
		if(bFailure || !bRequestSuccessful || eStatusCode != k_EHTTPStatusCode200OK) {
			LogError("[API] Failed to set InGame state of %i to %i", setClient, setState);

			//Lets restore the cldbid for now, we might be able to catch the setting of it on round end
			ClientUserDbId[setClient] = setState ? 1 : 0;
		} else {
			ClientUserDbId[setClient] = -1;
		}
	}

	delete infoDpack;

	delete hRequest;
}

//static int VoidHttpDataCallbac(const char[] csData) {}

static bool FormattedHttpQuery(SteamWorksHTTPRequestCompleted httpCallback, any data = 0, const char[] format, any ...) {
	int len = strlen(format) + 256;
	char[] myFormattedString = new char[len];
	VFormat(myFormattedString, len, format, 4);

	Debug("[API] REQ %s", myFormattedString);

	if(httpCallback == VoidHttpCallback) {
		DataPack dPack = CreateDataPack();
		dPack.WriteString(myFormattedString);

		data = dPack;
	}

	Format(myFormattedString, len, "%s/%s", g_sBackendUrl, myFormattedString);

	Handle hRequest = SteamWorks_CreateHTTPRequest(k_EHTTPMethodGET, myFormattedString);
	if (!hRequest ||
		!SteamWorks_SetHTTPCallbacks(hRequest, httpCallback) ||
		//REDACTED//
		!SteamWorks_SetHTTPRequestContextValue(hRequest, data) ||
		!SteamWorks_SetHTTPRequestNetworkActivityTimeout(hRequest, 30) //When node restarts, or has crashed, its possible requests will be delayed (a lot), so we might as well.

	|| !SteamWorks_SendHTTPRequest(hRequest)) {
		delete hRequest;
		return false;
	}

	return true;
}

public void InitBackendCommunicator() {
	//REDACTED//
}

static void DB_ConnCb(Database db, const char[] error, any data) {
	DB_CONN = db;

	if(DB_CONN == null)
		LogError("FAILED TO ESTABLISH SQL CONNECTION: %s", error);

	//invalidate any possible leftover connections from clients that were on the server when it crashed / the plugin was reloaded
	//REDACTED//
}

static void VoidDbCallback(Database db, DBResultSet results, const char[] error, any data) {
	if(strlen(error) > 0)
		LogError("Some error happened while executing myQuery: %s", error);
}

static bool FormattedDbQuery(SQLQueryCallback dbCallback, DBPriority prio = DBPrio_Normal, any data = 0, const char[] format, any ...) {
	int len = strlen(format) + 128;
	char[] myFormattedString = new char[len];
	VFormat(myFormattedString, len, format, 5);

	if(DB_CONN == null) {
		LogError("Couldnt execute query '%s' because of missing DB connection", myFormattedString);
		return false;
	}

 	Debug("Executing query: %s", myFormattedString);

	DB_CONN.Query(dbCallback, myFormattedString, data, prio);

	return true;
}

static void CheckStalledClients(Database db, DBResultSet results, const char[] error, any data) {
	#if defined NO_DBMOD
		if(data == -2312312312312 || data != -2312312312312)
			return;
	#endif
	//REDACTED//
}

int Backend_GetCurrentRoundId() {
	return Go4TK_Game_RoundId;
}

bool Backend_AddUserResultRAW(int clientId, int attackerId, int killCount, int score, int place, int sublobby_num) {
	#if defined NO_DBMOD
		if(clientId == -2312312312312 || clientId != -2312312312312)
			return false;
	#endif

	//REDACTED//
}

/*bool Backend_AddUserResult(int client, int attacker = -1, int killCount = -1, int score = -1, int place = -1) {
	if(/ *!IsValidClient(client) || IsFakeClient(client) ||* / g_bReadOnly || ClientUserDbId[client] == -1)
		return false;

	if(killCount == -1)
		killCount = GetEntData(client, CCSPlayer_iNumRoundKills, 1);

	if(score == -1)
		score = ClientScore[client];

	if(place == -1)
		place = alivePlayers;

	return Backend_AddUserResultRAW(ClientUserDbId[client], attacker != -1 ? ClientUserDbId[attacker] : 0, killCount, score, place);
}*/

bool Backend_SetUserIngame(int client, bool ingame = true) {
	if(/*(!IsClientConnected(client) || IsFakeClient(client)) && */ClientUserDbId[client] == -1)
		return false;

	#if defined NO_DBMOD
		if(client == -2312312312312 || client != -2312312312312)
			return false;
	#endif

	//REDACTED//
}

bool Backend_TimeoutReservedSlot(const char[] steamId64) {
	//REDACTED//
}

bool Backend_BanUser(int client, int admin = -1, int banSeconds = 10800, const char[] reason, const char[] extraInfo = "") {
	if(/*!IsValidClient(client) || IsFakeClient(client) ||*/ ClientUserDbId[client] == -1)
		return false;

	#if defined NO_DBMOD
		if(client == -2312312312312 || client != -2312312312312)
			return false;
	#endif

	if(admin != -1)
		admin = ClientUserDbId[admin];

	char[] encodedReason = new char[strlen(reason)+64];
	char[] encodedInfo = new char[strlen(extraInfo)+64];

	strcopy(encodedReason, strlen(reason)+64, reason);
	strcopy(encodedInfo, strlen(extraInfo)+64, extraInfo);

	//REDACTED//
}

//REDACTED//

bool Backend_FinishRound() {
	//REDACTED//
}


bool Backend_CreateRound(bool ranked) {
	//REDACTED//
}

static int InsertNewRoundHttpCallback(Handle hRequest, bool bFailure, bool bRequestSuccessful, EHTTPStatusCode eStatusCode) {
	if (bFailure || !bRequestSuccessful || eStatusCode != k_EHTTPStatusCode200OK) {
		PrintCenterTextAll("<font color='#ff0000'>Due to an error, the result wont be saved / count towards your leaderboard score!</font>");
	} else {
		SteamWorks_GetHTTPResponseBodyCallback(hRequest, InsertNewRoundHttpDataCallback);
	}

	delete hRequest;
}

static int InsertNewRoundHttpDataCallback(const char[] sData) {
	// Indicate that the response is a JSON object
	JSONObject theRound = JSONObject.FromString(sData);

	Go4TK_Game_RoundId = theRound.GetInt("id");

	if(Go4TK_Game_RoundId <= 0) {
		Go4TK_Game_RoundId = -1;

		PrintCenterTextAll("<font color='#ff0000'>Due to an error, the result wont be saved / count towards your leaderboard score!</font>");
		return;
	}
}

void InsertNewRoundCallback(Database db, DBResultSet results, const char[] error, int data) {
	if (results != null && results.InsertId > 0) {
		Go4TK_Game_RoundId = results.InsertId;
	} else {
		LogError("Query failed! %s", error);
		PrintCenterTextAll("<font color='#ff0000'>Due to an error, the result wont be saved / count towards your leaderboard score!</font>");
	}
}

bool Backend_GetUser(int client, const char[] steamId64) {
	if(!IsClientConnected(client) || IsFakeClient(client))
		return false;

	//REDACTED//
}

static int GetPlayerHttpCallback(Handle hRequest, bool bFailure, bool bRequestSuccessful, EHTTPStatusCode eStatusCode, int user) {
	int client = GetClientOfUserId(user);

	if(client <= 0)
		return;

	if (bFailure || !bRequestSuccessful || eStatusCode != k_EHTTPStatusCode200OK) {
		#if !defined DEBUG && !defined NO_QUEUE_CHECK
			KickClient(client, "Your account couldn't be verified. Please try again later");
		#endif
		return;
	} else {
		SteamWorks_GetHTTPResponseBodyCallback(hRequest, GetPlayerHttpDataCallback, user);
	}

	delete hRequest;
}

static int GetPlayerHttpDataCallback(const char[] sData, int user) {
	int client = GetClientOfUserId(user);

	if(client <= 0)
		return;

	// Indicate that the response is a JSON object
	JSONObject theUser = JSONObject.FromString(sData);

	theUser.GetString("name", toSetName[client], 32);

	ProcessPlayerInfo(
		client,
		theUser.GetInt("status"),
		theUser.GetInt("id"),
		theUser.GetInt("flags"),
		toSetName[client],
		theUser.GetInt("lowRenderDistance") == 1,
		theUser.GetInt("showBulletImpacts") == 0,
		theUser.GetInt("pregameTrashtalk") == 1,
		theUser.GetInt("videoAds") == 1);
}

static void QueryPlayerCallback(Database db, DBResultSet results, const char[] error, any user) {
	int client = GetClientOfUserId(user);

	if(client <= 0)
		return;

	if(IsClientConnected(client)) {
		if (results != null && results.RowCount > 0 && results.FetchRow()) {
			//Banned
			results.FetchString(3, toSetName[client], 32);

			ProcessPlayerInfo(
				client,
				results.FetchInt(1),
				results.FetchInt(0),
				results.FetchInt(2),
				toSetName[client],
				results.FetchInt(5) == 1,
				results.FetchInt(4) == 0,
				results.FetchInt(6) == 1,
				results.FetchInt(7) == 1
				);
		} else {
			#if !defined DEBUG && !defined NO_QUEUE_CHECK
				KickClient(client, "Your account couldn't be verified. Please try again later");
			#endif
		}
	}
}

static void ProcessPlayerInfo(int client, int status, int dbId, int clientFlags, const char[] name, bool highFpsMode, bool hasDisabledBulletMarkers, bool preGameTrashtalk, bool videoAds) {
	if(status == -1) {
		KickClient(client, "You are banned");
		return;
	}

	if(dbId <= 0) {
		#if !defined DEBUG && !defined NO_QUEUE_CHECK
			KickClient(client, "Your account couldnt be verified. Please try again later");
		#endif
		return;
	}

	#if !defined NO_DBMOD
		if(!g_bReadOnly)
			ClientUserDbId[client] = dbId;
	#endif

	userSettings[client][CFG_highFpsMode] = highFpsMode;
	userSettings[client][CFG_hasDisabledBulletMarkers] = hasDisabledBulletMarkers;
	userSettings[client][CFG_preGameTrashtalk] = preGameTrashtalk;
	userSettings[client][CFG_videoAds] = videoAds;

	client_Flags[client] = clientFlags;

	#if !defined EVERYONE_CAN_SPEC
		if(clientFlags & CLIENTFLAG_CANSPEC)
			CanClientSpectate[client] = true;
		else
			CanClientSpectate[client] =	false;
	#else
		CanClientSpectate[client] = true;
	#endif

	if(clientFlags & CLIENTFLAG_VIP)
		toSetClantag[client] = "[★]";
	else if(clientFlags & CLIENTFLAG_VERIFIED)
		toSetClantag[client] = "[✔]";

	//It can be possible that the authorization of the client happens late, waaay after he joined already
	//so we add this handling here because Event_OnFullConnect might've fired already, thus he would never join.
	if(IsClientInGame(client)) {
		CS_SetClientClanTag(client, toSetClantag[client]);

		CS_SetClientName(client, name);

		ClientFullConnect(client);
	}
}

//REDACTED//