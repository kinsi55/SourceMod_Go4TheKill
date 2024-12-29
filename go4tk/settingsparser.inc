#define GAMEMODE_SOLO '1'
#define GAMEMODE_DUO '2'

#define SERVER_GAMEMODE g_sType[0]

public char g_sBackendAuth[128];
public char g_sRegion[8];
public char g_sType[8];
public bool g_bForceUnranked = false;
public bool g_bDisableWeaponADS = false;
public bool g_bDisableSemiAuto = false;
public bool g_bDisableFreelook = false;

public bool g_bReadOnly = false;
public bool g_bBackendApi = false;
public char g_sBackendUrl[128];

public bool g_bUseQueue = false;
public int g_iQueuePort = -1;
public char g_sQueueIp[128];

public char g_sExtraStartMsg[128];

public bool g_bRawWS = false;

public bool g_bRecordDemos = false;

public void LoadSettings() {
	char tmpBuf[128];
	BuildPath(Path_SM, tmpBuf, sizeof(tmpBuf), "configs/Go4TK_Cfg.txt");

	if(!FileExists(tmpBuf))
		SetFailState("Missing config file");

	KeyValues Go4TK_SETTINGS_KV = new KeyValues("Go4TK_Settings");
	Go4TK_SETTINGS_KV.ImportFromFile(tmpBuf);

	Go4TK_SETTINGS_KV.GetString("auth", g_sBackendAuth, sizeof(g_sBackendAuth));

	Go4TK_SETTINGS_KV.GetString("extra_start_msg", g_sExtraStartMsg, sizeof(g_sExtraStartMsg));

	Go4TK_SETTINGS_KV.GetString("region", g_sRegion, sizeof(g_sRegion), "1");
	Go4TK_SETTINGS_KV.GetString("type", g_sType, sizeof(g_sType), "1");

	g_bRecordDemos = Go4TK_SETTINGS_KV.GetNum("record_demos", 0) == 1;


	g_bForceUnranked = Go4TK_SETTINGS_KV.GetNum("forceUnranked", 0) == 1;

	g_bDisableWeaponADS = Go4TK_SETTINGS_KV.GetNum("disableWeaponAds", 0) == 1;

	g_bDisableSemiAuto = Go4TK_SETTINGS_KV.GetNum("disableSemiAuto", 0) == 1;

	g_bDisableFreelook = Go4TK_SETTINGS_KV.GetNum("disableFreelook", 0) == 1;

	//g_bDisableFreeLook

	Go4TK_SETTINGS_KV.JumpToKey("Backend");

	g_bReadOnly = Go4TK_SETTINGS_KV.GetNum("readOnly", 1) == 1;
	g_bBackendApi = Go4TK_SETTINGS_KV.GetNum("useApi", 0) == 1;

	PrintToServer("[Backend] Using %s to communicate w/ backend", g_bBackendApi ? "API" : "database");

	Go4TK_SETTINGS_KV.GetString("apiurl", g_sBackendUrl, sizeof(g_sBackendUrl));

	Go4TK_SETTINGS_KV.Rewind();
	Go4TK_SETTINGS_KV.JumpToKey("Queue");

	g_bUseQueue = Go4TK_SETTINGS_KV.GetNum("enable", 0) == 1;
	g_iQueuePort = Go4TK_SETTINGS_KV.GetNum("port", 0);

	Go4TK_SETTINGS_KV.GetString("ip", g_sQueueIp, sizeof(g_sQueueIp));

	//Lets just prevent happy little mistakes before they happen.
	#if defined DEBUG || defined NOWIN || defined NOSAFEZONE || defined NO_QUEUE_CHECK || defined ALLRANKED
		if(StrEqual(g_sQueueIp, "X.X.X.X") || StrEqual(g_sQueueIp, "X.X.X.X") || g_iQueuePort == 53380)
			g_bUseQueue = false;
	#endif

	Go4TK_SETTINGS_KV.Rewind();
	Go4TK_SETTINGS_KV.JumpToKey("WS");

	g_bRawWS = Go4TK_SETTINGS_KV.GetNum("raw_ip", 0) == 1;

	delete Go4TK_SETTINGS_KV;
}