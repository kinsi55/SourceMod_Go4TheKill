#pragma semicolon 1
#pragma dynamic 32768 //I dont really need that much space, but you never know what will come

#define DEBUG 1
#define NOWIN
//#define NOSAFEZONE
//#define NOFOG
#define NO_DBMOD
#define NO_QUEUE
#define NO_QUEUE_CHECK
//#define ALLRANKED
//#define VERBOSE
#define INSECURE_WS
//#define NOWS
//#define NOWSWARN
#define NOAUTOSTART
//#define EVERYONE_CAN_SPEC

#define NOAC

bool STEAMID_WATERMARK = false;

#define PLUGIN_AUTHOR "Kinsi"
#define PLUGIN_VERSION "0.1.1b"

public char sumTmpStr[64];
public float tmpOrigin[3];

#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
//#include <smlib>
#include <kinsi>
#include <darksession>
#include <motd_urls>
#include <env_instructor_hint>
#include <math>
//#include <sha1>
#include <websocket>
//#include <Redis>
//#include <sendproxy>
#include <socket_newdecals>

//#pragma newdecls optional
//#include <filenetmessages>
//#pragma newdecls required

//#include <SMObjects>
//#include <PTaH> //For weapon spawning
//#include <dhooks>

//REDACTED//

#pragma newdecls optional
#include <CustomPlayerSkins> // for colored glow
#include <colors>
#undef REQUIRE_PLUGIN
//REDACTED//
#define REQUIRE_PLUGIN
#pragma newdecls required

#define BANNER_ADS "http://redacted"
#define VIDEO_ADS "http://redacted"

//REDACTED//

//Argument 1 = Client, Argument 2 = player to execute stuff on(Either self, or spectator)
#define DoForEverySpectatingClientAndSelf(%1,%2)\
	LoopIngameClients(%2)\
		if(%1 == %2 || spectatedPlayer[%2] == %1)

public Plugin myinfo = {
	name = "Go4TK",
	author = PLUGIN_AUTHOR,
	description = "KoTK in CS:GO!",
	version = PLUGIN_VERSION,
	url = "https://Go4TheKill.net"
};


enum userSetting {
	CFG_highFpsMode,
	CFG_hasDisabledBulletMarkers,
	CFG_preGameTrashtalk,
	CFG_videoAds
}

bool userSettings[MAXPLAYERS+1][userSetting];

int ws_port;
char sServerIP[40];
char sServerIPAndPort[40];
int NullPlayerArray[MAXPLAYERS+1];

float tickMs = 0.0;
float tickCorrector = 0.0;

float Go4TK_Game_StartTime = -1.0;

int alivePlayers = 0;

bool CanClientSpectate[MAXPLAYERS+1] =  {false, ...};
bool CanClientBan[MAXPLAYERS+1] =  {false, ...};
int ClientUserDbId[MAXPLAYERS+1] = {-1, ...};

bool PLAYER_IS_IN_BINOCULAR[MAXPLAYERS+1] = {false, ...};
float plRealEyeAngles[MAXPLAYERS + 1][3];
float plLastPosition[MAXPLAYERS + 1][3];
bool plIsCurrentlyParachuting[MAXPLAYERS + 1] = {false, ...};
bool plIsCurrentlyOutsideSafezone[MAXPLAYERS + 1] = {false, ...};


int CurrentClientLobby[MAXPLAYERS+1] = {-1, ...};


bool HALT_AFTER_MATCH = false;

bool IsPlayerInited[MAXPLAYERS+1] = {false, ...};

int spectatedPlayer[MAXPLAYERS+1] = {0, ...};

//WARNING is only filled while the game is progressing.
char lastClientCordon[MAXPLAYERS+1][48];
MAP_LAYERS currentPlayerLayer[MAXPLAYERS+1];

char client_AuthIds[MAXPLAYERS+1][16];
char client_AuthIds64[MAXPLAYERS+1][32];
int client_Flags[MAXPLAYERS+1];

int g_PVMid[MAXPLAYERS+1]; // Predicted ViewModel ID's

int client_connectionwarn_timer[MAXPLAYERS+1] = {-1, ...};

bool g_bPlayerAliveCache[MAXPLAYERS+1] = {false, ...};

//bool hasClientClosedMotd[MAXPLAYERS+1] = {false, ...};
float clientReceivedAdAt[MAXPLAYERS+1] = {-1.0, ...};

int g_iTvClient = 0;

#define SLOT_FISTS 4
#define SLOT_BINOCULAR 5
#define DEFAULT_MAXSPEED 230.0
#define BINOCULAR_MAXSPEED 70.0
#define MAX_PICKUP_DISTANCE 128.0
#define BINOCULAR_FOV 12
#define ADS_FOV_SUBTRACT 5
#define SCOUT_FOV 15

#define HUDMSG_CHANNEL_PROGRESS 1
#define HUDMSG_CHANNEL_PICKEDUP 2
#define HUDMSG_CHANNEL_LOADOUT 3
#define HUDMSG_CHANNEL_POSANDDIR 4
#define HUDMSG_CHANNEL_GAMEINFOS 5

#define HIDE_BACKPACKS_WHEN_ALIVE 33
#define HIDE_ARMOR_WHEN_ALIVE 39

#define TIME_BETWEEN_WAVES 70.0 //s
#define TIME_BEFORE_FIRST_WAVE 100.0 //s
#define GAS_TRANSITION_TIME 30.0 //s
#define SAFEZONE_SHRINK_PER_WAVE 0.72 //%
#define INGAME_TO_MAP_RATIO_DIVISOR 113.691 //LOGICAL coordinates, not PHYSICAL 13984 / (492 / 4), size of airstrip thing in map VS minimap
#define GRACE_PERIOD_BEFORE_START_LOW 110.0 //s
#define GRACE_PERIOD_BEFORE_START_RANKED 60.0 //s
#define MATCH_LEAVE_COOLDOWN 10.0 //s

#define CLOSE_QUEUE_GATE_AT 50 //s

#define WS_WARN_TIME 60

#define MIN_PLAYERS_TO_START 10
#define MIN_PLAYERS_FOR_RANKED 14
#define PLAYER_LIMIT 49

#define FIRST_SAFEZONE_SIZE 35000.0 //u
//#define MIN_SAFEZONE_SIZE 2000.0 //u
#define MIN_SAFEZONE_SIZE 32.0 //u. We remove the min size to prevent two people from deadlocking the server permanently.
#define SHRINK_TOWARDS_QUADRANT_BELOW 20000.0 //u
#define SHRINK_TOWARDS_QUADRANT_UNTIL 11000.0 //u
#define MIN_SHRINK_PER_WAVE 2048.0
#define SHRINK_FASTER_BELOW 10

bool Go4TK_Game_InProgress = false;
float Go4TK_Game_StartDelay = -1.0;

#define DEFAULT_GAS_CENTER {29382.0, 29382.0, 4600.0}
#define DEFAULT_GAS_RADIUS 41000.0

float transitionSafezoneCenter[3] = DEFAULT_GAS_CENTER;
float transitionSafezoneRadius = DEFAULT_GAS_RADIUS;

float currentSafezoneCenter[3] = DEFAULT_GAS_CENTER;
float currentSafezoneRadius = DEFAULT_GAS_RADIUS;

float targetSafezoneCenter[3] = DEFAULT_GAS_CENTER;
float targetSafezoneRadius = DEFAULT_GAS_RADIUS;

float Go4TK_Next_Safezone_Shrink = 9999999999.0;
float Go4TK_Safezone_Shrink_Targettime = 0.0;

int ClientScore[MAXPLAYERS+1] = {0, ...};

bool forceStart = false;

//#define FOG_COLOR 37, 33, 53
//#define FOG_COLOR_STR "37 33 53"

//#define FOG_COLOR 175, 98, 86
//#define FOG_COLOR_STR "175 98 86"

//#define FOG_COLOR 155, 139, 128
//#define FOG_COLOR_STR "155 139 128"

#define FOG_COLOR 131, 117, 108
#define FOG_COLOR_STR "131 117 108"

//#define FOG_COLOR 152, 156, 132
//#define FOG_COLOR_STR "152 156 132"

#define IN_GAS_COLOR 150, 255, 0

#define HIGHFPS_VIEWDISTANCE 10000.0


#include "go4tk/settingsparser"

#include "go4tk/cached_stuff"
#include "go4tk/health_management"
#include "go4tk/helpers"

#include "go4tk/customspec"

#include "go4tk/backend_communicator"

#include "go4tk/inventory_system"

#include "go4tk/fake_weapons"
#include "go4tk/loot_spawner"

#include "go4tk/lagcompensation_manager"
#include "go4tk/bullet_simulator"
#include "go4tk/weaponsound_simulator"

#pragma newdecls optional
#include "go4tk/wshandler"
#pragma newdecls required

#include "go4tk/lobby_manager"

#include "go4tk/queue_communicator"
#include "go4tk/game_manager"
#include "go4tk/radar_handler"

#include "go4tk/teaming_detection"

  /*===================/
 / Game related Stuff /
/===================*/

static bool enforceSingleshots[MAXPLAYERS+1] = {false, ...};
static bool PLAYER_WEAPON_ISSCOUT[MAXPLAYERS+1] = {false, ...};
static int clientAddonBits[MAXPLAYERS+1][3];

static int prevClientAmmoCount[MAXPLAYERS+1] =  {0, ...};
static Handle CurrentActionTimer[MAXPLAYERS+1] = {null, ...};

public void OnPluginStart() {
	ws_port = GetConVarInt(FindConVar("hostport"))-1;
	//REDACTED//

	FormatEx(sServerIPAndPort, sizeof(sServerIPAndPort), "%s:%i", sServerIP, ws_port+1);

	//REDACTED//

	tickMs = GetTickInterval();

	tickCorrector = (1.0 / 64.0) / tickMs;

	PrintToServer("Tickcorrector: %f", tickCorrector);

	LoadNetPropOffsets();

	LoadSettings();

	HookUserMessage(GetUserMessageId("TextMsg"), UserMessageHook2, true);

	AddCommandListener(SayCallback, "say");
	AddCommandListener(SayCallback, "say_team");

	HookEvent("round_prestart", Event_RoundPreStart, EventHookMode_Pre);

	HookEvent("round_start", Event_RoundStart, EventHookMode_PostNoCopy);
	HookEvent("player_spawn", Event_PlayerSpawn, EventHookMode_Post);
	HookEvent("weapon_fire", Event_WeaponFire, EventHookMode_Pre);
	HookEvent("bullet_impact", Event_BulletImpact, EventHookMode_Pre);
	HookEvent("player_death", Event_PlayerDeath, EventHookMode_Pre);

	HookEvent("player_disconnect", Event_OnDisconnectPre, EventHookMode_Pre);

	HookEvent("hltv_status", Event_BlockBroadcast, EventHookMode_Pre);
	HookEvent("hltv_rank_entity", Event_BlockBroadcast, EventHookMode_Pre);
	HookEvent("player_connect", Event_BlockBroadcast, EventHookMode_Pre);

	HookEvent("player_connect_full", Event_OnFullConnect, EventHookMode_Pre);
	//HookEvent("player_activate", Event_OnActivate, EventHookMode_Pre);

	HookEvent("player_changename", Event_OnPlayerNamechange, EventHookMode_Pre);

	HookEvent("server_cvar", Event_ServerCvar, EventHookMode_Pre); // http://wiki.alliedmods.net/Generic_Source_Server_Events#server_cvar

	AddCommandListener(Command_LookAtWeapon, "+lookatweapon");
	AddCommandListener(Command_Autobuy_Openmap, "autobuy");
	//Legacy commands, do not exist in CS:GO but are bound by default to , and . respectively
	AddCommandListener(Command_Buyammo1, "buyammo1"); //Default bound to ,
	AddCommandListener(Command_ToggleHighFPS, "buyammo2");

	AddCommandListener(Command_Block, "jointeam");
	AddCommandListener(Command_Block, "kill");
	AddCommandListener(Command_Block, "explode");
	AddCommandListener(Command_Block, "explodevector");

	for(int i; i < sizeof(RadioCMDS); i++)
		AddCommandListener(Command_Block, RadioCMDS[i]);

	AddCommandListener(Cmd_spec_next, "spec_next");
	AddCommandListener(Cmd_spec_prev, "spec_prev");
	AddCommandListener(Cmd_spec_player, "spec_player");
	AddCommandListener(Cmd_spec_mode, "spec_mode");

	CreateTimer(0.5, FogTimer, _, TIMER_REPEAT);

	CreateTimer(0.25, Go4TK_Tick, _, TIMER_REPEAT);

	CreateTimer(5.0, Go4TK_GameProgressor, _, TIMER_REPEAT);

	CreateTimer(1.0, TeamingCheckTimer, _, TIMER_REPEAT);

	InitHealthManagement();

	ResetLobbyManager();
	InitQueueConn();

	//InitBackendCommunicator();
	CreateTimer(5.0, DelayedBackendCommunicatorInit);

	RegConsoleCmd("status", ConCommand_Block);
	RegConsoleCmd("ping", ConCommand_Block);

	AddTempEntHook("Shotgun Shot", TEHook_ShotgunShot);

	int i = -1;
	while((i = FindEntityByClassname(i, "env_fog_controller")) != -1)
		AcceptEntityInput(i, "Kill");

	//InitGo4TKInventory();

	LoopIngameClients(client)
		OnClientPutInServer(client);

	RegAdminCmd("go4tk_start", StartCb, ADMFLAG_GENERIC);

	RegAdminCmd("go4tk_halt", HaltCb, ADMFLAG_GENERIC);

	RegAdminCmd("go4tk_request", RequestTest, ADMFLAG_GENERIC);

	RegAdminCmd("go4tk_refresh_if_idle", RefreshIfIdle, ADMFLAG_GENERIC);

	HookUserMessage(GetUserMessageId("SayText2"), SayText2, true);

	Queue_RequestPlayers();

	//AddNormalSoundHook(FistHitSoundHook);

	g_CurrentEntityCount = GetCurrentEntityCount();
}

public Action RefreshIfIdle(int client, int args) {
	if(!Go4TK_Game_InProgress && GetClientCount(false) == (IsClientConnected(g_iTvClient) ? 1 : 0) && Queue_ReservedCount() == 0)
		ServerCommand("sm plugins refresh");
}

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max) {
	CreateNative("Go4TK_DoWhCheck", Native_DoWhCheck);

	//REDACTED//

	return APLRes_Success;
}

public int Native_DoWhCheck(Handle plugin, int numParams) {
	//REDACTED//
}

public Action DelayedBackendCommunicatorInit(Handle timer) {
	InitBackendCommunicator();
}

public Action SayText2(UserMsg msg_id, Handle bf, int[] players, int playersNum, bool reliable, bool init) {
	PbReadString(bf, "msg_name", sumTmpStr, sizeof(sumTmpStr));

	if(StrEqual(sumTmpStr, "#Cstrike_Name_Change", false))
		return Plugin_Handled;

	return Plugin_Continue;
}

public Action Event_OnPlayerNamechange(Event event, const char[] name, bool dontBroadcast) {
	//Block name change by changing the name back to the old name.
	event.GetString("oldname", sumTmpStr, sizeof(sumTmpStr));

	CS_SetClientName(GetClientOfUserId(event.GetInt("userid")), sumTmpStr, false);

	event.BroadcastDisabled = true;

	return Plugin_Stop;
}

public Action OnClientCommandKeyValues(int client, KeyValues kv) {
	if (kv.GetSectionName(sumTmpStr, sizeof(sumTmpStr)) && StrEqual(sumTmpStr, "ClanTagChanged", false))
		return Plugin_Handled;

	return Plugin_Continue;
}

//Fallback commands incase steamwebhelper crashes
public Action SayCallback(int client, const char[] command, int argc) {
	if(IsValidClient(client) && argc >= 1) {
		char text[16];
		GetCmdArg(1, text, sizeof(text));

		#if !defined DEBUG
		if(IsPlayerAlive(client) && ws_clients[client] == INVALID_WEBSOCKET_HANDLE) {
		#else
		if(IsPlayerAlive(client)) {
		#endif
			if(StrEqual(text, "!helmet", false)) {
				if(!PlayerInv(client, Head).IsValid()) {
					Item findHelmet = PlayerInv(client, Carrying).GetItemByType(Helmet);

					if(findHelmet.IsValid()) {
						Go4TK_Playerinv(client).PickupItem(findHelmet, Head, AsInt(Carrying));
						HudMsg(client, HUDMSG_CHANNEL_PROGRESS, vAs(float, { -1.0, 0.65 }), { 255, 255, 100, 255 }, { 0, 255, 0, 255 }, 2, 0.0, 0.0, 2.0, 2.0, "Equipped helmet");
					} else {
						//No helmet found
						HudMsg(client, HUDMSG_CHANNEL_PROGRESS, vAs(float, { -1.0, 0.65 }), { 255, 100, 100, 255 }, { 255, 0, 0, 255 }, 2, 0.0, 0.0, 2.0, 2.0, "You dont have any more helmets");
					}
				} else {
					//Helmet already equipped
					HudMsg(client, HUDMSG_CHANNEL_PROGRESS, vAs(float, { -1.0, 0.65 }), { 255, 255, 100, 255 }, { 0, 255, 0, 255 }, 2, 0.0, 0.0, 2.0, 2.0, "You already have a helmet equipped");
				}
				return Plugin_Handled;
			} else if(StrEqual(text, "!kevlar", false) || StrEqual(text, "!armor", false)) {
				if(!PlayerInv(client, Armor).IsValid()) {
					ItemList pCarrying = PlayerInv(client, Carrying);

					for (int i = 0; i < pCarrying.Length; i++) {
						Item tItem = pCarrying.GetItem(i);

						if(tItem.Type == Laminated_Vest || tItem.Type == Makeshift_Armor) {
							Go4TK_Playerinv(client).PickupItem(tItem, Armor, AsInt(Carrying));
							HudMsg(client, HUDMSG_CHANNEL_PROGRESS, vAs(float, { -1.0, 0.65 }), { 255, 255, 100, 255 }, { 0, 255, 0, 255 }, 2, 0.0, 0.0, 2.0, 2.0, "Equipped %s", ITEM_INFOS[tItem.Type][Name]);

							return Plugin_Handled;
						}
					}

					//check if player has enough items to craft makeshift
					if(!Go4TK_Playerinv(client).CraftItem(Makeshift_Armor, true)) {
						HudMsg(client, HUDMSG_CHANNEL_PROGRESS, vAs(float, { -1.0, 0.65 }), { 255, 255, 100, 255 }, { 0, 255, 0, 255 }, 2, 0.0, 0.0, 2.0, 2.0, "You dont have all required items\nto craft a makeshift armor");
					} else
						ClientActionCraft(client, Makeshift_Armor);
				} else {
					HudMsg(client, HUDMSG_CHANNEL_PROGRESS, vAs(float, { -1.0, 0.65 }), { 255, 255, 100, 255 }, { 0, 255, 0, 255 }, 2, 0.0, 0.0, 2.0, 2.0, "You already have equipped a %s", ITEM_INFOS[PlayerInv(client, Armor).Type][Name]);
				}
				return Plugin_Handled;
			}
		}

		if(StrEqual(text, "!leave", false)) {
			if(!IsPlayerAlive(client) || !Go4TK_Game_InProgress)
				KickClient(client, "Thank you for playing!");
			else {
				AbortInteract(client, true);

				DataPack dPack;
				CurrentActionTimer[client] = CreateDataTimer(MATCH_LEAVE_COOLDOWN, DoDelayedAction, dPack);

				dPack.WriteCell(client); dPack.WriteCell(-1);

				DoForEverySpectatingClientAndSelf(client, toSendTo)
					HudMsg(toSendTo, HUDMSG_CHANNEL_PROGRESS, vAs(float, { -1.0, 0.7 }), { 255, 100, 255, 255 }, { 0, 255, 0, 255 }, 2, 0.0, 0.0, MATCH_LEAVE_COOLDOWN, MATCH_LEAVE_COOLDOWN, "Leaving match...");

				SetEntPropFloat(client, Prop_Send, "m_flProgressBarStartTime", GetGameTime());
				SetEntProp(client, Prop_Send, "m_iProgressBarDuration", RoundFloat(MATCH_LEAVE_COOLDOWN));
			}
			return Plugin_Handled;
		} else if(StrEqual(text, "!banplayer", false) && CanClientBan[client]) {
			if(!IsPlayerAlive(client)) {

			}
		} else if(StrEqual(text, "!team", false) && CanClientSpectate[client] && GetClientTeam(client) == CS_TEAM_SPECTATOR) {
			Teaming_CheckForClientsTeamedWith(spectatedPlayer[client], client);
		}

		PrintToChat(client, "Chat is disabled.");
		return Plugin_Handled;
	}
	return Plugin_Continue;
}
//Seems unnecessary w/ ignore_round_win_conditions
public Action CS_OnTerminateRound(float &delay, CSRoundEndReason &reason) {
	if(reason == CSRoundEnd_TargetSaved || reason == CSRoundEnd_Draw || reason == CSRoundEnd_GameStart)
		return Plugin_Stop;

	return Plugin_Continue;
}

public Action StartCb(int client, int args) {
	Go4TK_Game_StartDelay = GetGameTime() + 20.0;
	forceStart = true;
}

public Action HaltCb(int client, int args) {
	HALT_AFTER_MATCH = true;
	PrintToServer("QUEUE SYSTEM HALTED FOR MAINTENANCE");
}

public Action RequestTest(int client, int args) {
	Queue_RequestPlayers();
	PrintToServer("Trying to request?");
}

static Action Command_ToggleHighFPS(int client, const char[] command, int argc) {
	if(PLAYER_FOG_CONTROLLERS[client] != -1) {
		userSettings[client][CFG_highFpsMode] = !userSettings[client][CFG_highFpsMode];

		SetEntDataFloat(PLAYER_FOG_CONTROLLERS[client], CFogController_fog_farZ, userSettings[client][CFG_highFpsMode] ? HIGHFPS_VIEWDISTANCE : 0.0, true);
		UpdateFogForClient(client, false);
	}

	return Plugin_Stop;
}

public Action TEHook_ShotgunShot(const char[] te_name, const int[] Players, int numClients, float delay) {
	//if(delay != 0.1) {

	PlaySimulatedWeaponShot(TE_ReadNum("m_iPlayer") + 1, TE_ReadNum("m_nItemDefIndex"));
	//Block Normal decal, tracer, sound etc handling
	return Plugin_Stop;

	//}
	//return Plugin_Continue;
}

public void OnPluginEnd() {
	LoopIngameClients(i)
		OnClientDisconnect(i);

	if(ws != INVALID_WEBSOCKET_HANDLE)
		Websocket_Close(ws);

	for(int i = 1; i < MaxClients; i++) {
		delete PLAYER_INV[i][Carrying];
	}

	OnMapEnd();

	if(Go4TK_Items != null) {
		ResetGo4TKInventory();
		delete Go4TK_Items;
	}

	Queue_FlushReservedSlotsToDisk();

	CS_TerminateRound(0.2, CSRoundEnd_Draw, true);
}

public void OnAllPluginsLoaded() {
	LoopClients(i)
		ws_clients[i] = INVALID_WEBSOCKET_HANDLE;

	// Open a new child socket
	if(ws == INVALID_WEBSOCKET_HANDLE)
		ws = Websocket_Open(sServerIP, ws_port, OnWebsocketIncoming, OnWebsocketMasterError, OnWebsocketMasterClose);
}

public void OnConfigsExecuted() {
	SetConVarInt(FindConVar("mp_teammates_are_enemies"), 1);
	SetConVarInt(FindConVar("sv_delta_entity_full_buffer_size"), 262144); //Apparently max
	SetConVarInt(FindConVar("sv_disable_show_team_select_menu"), 1);
	SetConVarInt(FindConVar("mp_spawnprotectiontime"), 0);
	SetConVarInt(FindConVar("sv_damage_print_enable"), 0);
	SetConVarInt(FindConVar("sv_specspeed"), 7); //Lets make spec faster by default.
	SetConVarInt(FindConVar("mp_buytime"), 0);
	SetConVarInt(FindConVar("mp_maxmoney"), 0);
	SetConVarInt(FindConVar("mp_forcecamera"), 0);
	SetConVarInt(FindConVar("mp_do_warmup_period"), 0);
	SetConVarInt(FindConVar("mp_freezetime"), 0);
	SetConVarInt(FindConVar("mp_respawn_immunitytime"), 0);

	SetConVarInt(FindConVar("mp_playerid"), 2); //Disable hover names

	//Apparently, the CS:GO server can crash when players spawn with no weapons?!
	SetConVarString(FindConVar("mp_t_default_melee"), "");
	SetConVarString(FindConVar("mp_t_default_secondary"), "");

	//SetConVarString(FindConVar("mp_t_default_melee"), "weapon_knife");
	//SetConVarString(FindConVar("mp_t_default_secondary"), "weapon_glock");

	SetConVarInt(FindConVar("mp_timelimit"), 65536);

	SetConVarInt(FindConVar("sv_spawn_afk_bomb_drop_time"), 65536);
	SetConVarInt(FindConVar("mp_force_pick_time"), 65536);

	SetConVarFloat(FindConVar("spec_freeze_deathanim_time"), 0.0);

	//Valveeeeeee
	ServerCommand("exec server");

	SetConVarInt(FindConVar("bot_quota"), 0);
	SetConVarInt(FindConVar("spec_replay_enable"), 0);


	SetConVarInt(FindConVar("sv_disable_motd"), 1);

	ServerCommand("exec gotv");

	#if defined DEBUG
		SetConVarString(FindConVar("sv_password"), "redacted");
	#endif
}

public void OnMapStart() {
	if(EntRefToEntIndex(CCSPlayerResource_Manager) == INVALID_ENT_REFERENCE) {
		CCSPlayerResource_Manager = GetPlayerResourceEntity();

		if(IsValidEdict(CCSPlayerResource_Manager)) {
			SDKHook(CCSPlayerResource_Manager, SDKHook_ThinkPost, OnPlayerResourceThinkPost);
			CCSPlayerResource_Manager = EntIndexToEntRef(CCSPlayerResource_Manager);
		}
	}

	//for(int client = 1; client <= MaxClients; client++)
	//	SendProxy_HookArrayProp(CCSPlayerResource_Manager, "m_iTeam", client, Prop_Int, MakeAllDisconnected);

	AddCustomStuffToDownloadtable();

	CacheNecessaryStuff();

	InitFakeWeaponStruct();

	InitGo4TKInventory();

	LoopClients(client) {
		PLAYER_EQUIP[client] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
		NullPlayerArray[client] = 0;

		//Prevents crashes when reloading the plugin
		InitGo4TKInventoryForClient(client);

		ws_clients[client] = INVALID_WEBSOCKET_HANDLE;
	}

	InitBulletSimStruct();

	LoopIngameClients(client)
		OnClientAuthorized(client, "");
}

public void OnMapEnd() {
	//for(int client = 1; client <= MaxClients; client++)
	//	SendProxy_UnhookArrayProp(CCSPlayerResource_Manager, "m_iTeam", client, Prop_Int, MakeAllDisconnected);
}

static void OnPlayerResourceThinkPost(int entity) {
	//#if !defined DEBUG
		//if(!Go4TK_Game_InProgress)
	    //SetEntDataArray(entity, CCSPlayerResource_bConnected, NullPlayerArray, sizeof(NullPlayerArray));
	    if(CCSPlayerResource_bAlive != 0) {
    		SetEntDataArray(entity, CCSPlayerResource_bAlive, NullPlayerArray, sizeof(NullPlayerArray));
    		SetEntDataArray(entity, CCSPlayerResource_iTeam, NullPlayerArray, sizeof(NullPlayerArray));
    	}
    //#endif
}

public bool OnClientConnect(int client, char[] rejectMsg, int maxlen) {
	//3 seconds left before the game init starts
	if(Go4TK_Game_InProgress || 0.0 <= Go4TK_Game_StartDelay - GetGameTime() <= 10.0) {
		strcopy(rejectMsg, maxlen, "You cannot join because there is already a game in progress. Visit https://Go4TheKill.net for more info / how to play!");

		return false;
	}

	return true;
}

public void OnClientPutInServer(int client) {
	if(IsClientSourceTV(client)) {
		g_iTvClient = client;

		CurrentClientLobby[client] = -1;//make sure tv NEVER gets in a team w/ somebody
		return;
	}

	SDKHook(client, SDKHook_WeaponCanUse, SDKOnWeaponCanUse);
	SDKHook(client, SDKHook_WeaponSwitchPost, SDKWeaponSwitchPost);
	SDKHook(client, SDKHook_PostThinkPost, OnPostThinkPostAnimationFix);
	SDKHook(client, SDKHook_PostThinkPost, SDKPostThinkPost);
	SDKHook(client, SDKHook_TraceAttack, SDKTraceAttack);
	SDKHook(client, SDKHook_OnTakeDamage, SDKOnTakeDamage);
	SDKHook(client, SDKHook_OnTakeDamageAlivePost, SDKOnTakeDamageAlivePost);
	SDKHook(client, SDKHook_PreThinkPost, SDKPreThinkPost_ParachuteLogic);
	SDKHook(client, SDKHook_SetTransmit, SDKClientSetTransmit);

	//SDKHook(client, SDKHook_ThinkPost, SDThinkPost);
	if(!USE_ARBITRARY_LAGCOMPENSATION)
		SDKHook(client, SDKHook_PreThink, SDKPreThink);

	IntToString(RoundFloat(CLIENT_AUTH_PEPPER * (GetURandomFloat() + 0.5)), CLIENT_AUTH_TOKENS[client], 16);

	if(ws_clients[client] != INVALID_WEBSOCKET_HANDLE) {
		WebsocketHandle x = ws_clients[client];

		ws_clients[client] = INVALID_WEBSOCKET_HANDLE;

		Websocket_UnhookChild(x);
	}
}

native bool SMAC_WH_IsClientVisibleToPlayer(int client, int otherClient);

public Action SDKClientSetTransmit(int client, int toTransmitTo) {
	//not even sure if this can happen
	//if(!IsValidClient(client) || !IsValidClient(toTransmitTo))
	//	return Plugin_Continue;

	//Always transmit a client to itself, and to gotv
	if(toTransmitTo == client || toTransmitTo == g_iTvClient)
		return Plugin_Continue;
	//No need to do any checking for spectators, or pre-game
	if(!Go4TK_Game_InProgress || CanClientSpectate[toTransmitTo])
		return Plugin_Continue;
	//never transmit dead / spectating players
	if(!g_bPlayerAliveCache[client])
		return Plugin_Stop;

	//Fully stop players xmitting that are in a different area
	if(currentPlayerLayer[client] != currentPlayerLayer[toTransmitTo]) {
		return Plugin_Stop;
	} else if(g_bPlayerAliveCache[toTransmitTo] && CurrentClientLobby[client] != CurrentClientLobby[toTransmitTo]) {
		//REDACTED//

		//Esp. in ontransmit, this could cause quite some load, so we do it only beginning of the mid-game
		#if !defined NOFOG
			if(alivePlayers <= 25 && g_lastPlayerFogRange[toTransmitTo] <= 4500.0) {
				if(player_last_attacker[toTransmitTo] == client)
					return Plugin_Continue;
				else {
					static float pl1[3]; static float pl2[3];

					GetEntDataVector(client, CBaseEntity_vecOrigin, pl1);
					GetEntDataVector(toTransmitTo, CBaseEntity_vecOrigin, pl2);

					//Not perfect, but will have to work. Some balance between useability and cheat-prevention
					float testRange = g_lastPlayerFogRange[toTransmitTo] * (PLAYER_IS_IN_BINOCULAR[toTransmitTo] ? 3.0 : 2.4);
					if(testRange > 13000.0)
						testRange = 13000.0;

					if(GetVectorDistance(pl1, pl2) > testRange)
						return Plugin_Handled;
				}
			}
		#endif
	} else if(CurrentClientLobby[client] == CurrentClientLobby[toTransmitTo]) {
		return Plugin_Continue;
	}

	if(!SMAC_WH_IsClientVisibleToPlayer(toTransmitTo, client))
		return Plugin_Stop;

	return Plugin_Continue;
}

public void SDKPostThinkPost(int client) {
	if(CBasePlayer_bSpotted == 0)
		return;

	SetEntData(client, CBasePlayer_bSpotted, 0, 1, true);
	//SetEntData(client, CCSPlayer_bGunGameImmunity, 1, 1, true);

	//SetEntProp(client, Prop_Send, "m_lifeState", 1);

	SetEntData(client, CCSPlayer_iPrimaryAddon, clientAddonBits[client][0], 1, true);
	SetEntData(client, CCSPlayer_iSecondaryAddon, clientAddonBits[client][1], 1, true);
	SetEntData(client, CCSPlayer_iAddonBits, clientAddonBits[client][2], 1, true);
}

public Action Event_OnFullConnect(Event event, const char[] name, bool dontBroadcast) {
	int client = GetClientOfUserId(event.GetInt("userid"));

	if(IsClientAuthorized(client) && !IsPlayerAlive(client) && !IsClientSourceTV(client)) {
		if(strlen(toSetClantag[client]) > 0) {
			CS_SetClientClanTag(client, toSetClantag[client]);
			toSetClantag[client] = "";
		}

		if(strlen(toSetName[client]) > 0) {
			CS_SetClientName(client, toSetName[client]);
		} else {
			GetClientName(client, toSetName[client], 32);
		}

		ClientFullConnect(client);
	}

	event.BroadcastDisabled = true;
	return Plugin_Changed;
}

public void OnClientAuthorized(int client, const char[] auth) {
	if(IsClientSourceTV(client))
		return;

	#if !defined DEBUG
		if(IsFakeClient(client)) {
			KickClient(client, "BOT");
			ServerCommand("bot_quota 0");
			LogError("Bot tried to join in production?!");

			return;
		}
	#endif

	GetClientAuthId(client, AuthId_Steam3, sumTmpStr, sizeof(sumTmpStr));

	strcopy(client_AuthIds[client], strlen(sumTmpStr) - 3, sumTmpStr[3]);

	//Fallback if db query fails
	GetClientName(client, toSetName[client], 32);

	if(!GetClientAuthId(client, AuthId_SteamID64, sumTmpStr, sizeof(sumTmpStr))) {
		client_AuthIds64[client] = "";
		client_Flags[client] = 0;
		#if !defined DEBUG && !defined NO_QUEUE_CHECK
			KickClient(client, "Your account couldn't be verified by Steam. Please try again later");
			return;
		#else
			if(!GetConVarBool(FindConVar("sv_lan"))) {
				ClientFullConnect(client);
				return;
			} else {
				sumTmpStr = "76561197993575363";
			}
		#endif
	}

	#if !defined NO_QUEUE_CHECK
		char lobbyId[64];

		if(!Queue_CheckinSteamid(sumTmpStr, true, lobbyId, sizeof(lobbyId))) {
			KickClient(client, "This Account is not permitted to join, make sure that you are on the correct account. Visit https://Go4TheKill.net for more info");
			return;
		} else {
			Lobby_SetPlayerLobby(client, lobbyId);
		}

		strcopy(client_AuthIds64[client], 32, sumTmpStr);
	#else
		//We dont have a lobby id for this client because we have the queue check disabled
		//However, if we dont set a different lobby for this client, all would be in the same one
		//Causing.. issues.. while testing at least.
		char ClIdString[8];
		IntToString(client, ClIdString, sizeof(ClIdString));

		//if(!IsFakeClient(client))
		//	ClIdString = "99";

		Lobby_SetPlayerLobby(client, ClIdString);
	#endif

	if(!Backend_GetUser(client, sumTmpStr) && IsClientConnected(client) && IsClientInGame(client)) {
		//Well, if the database isnt working there really isnt another option.
		//ClientFullConnect(client);

		KickClient(client, "Your account couldn't be verified due to an error. Please try rejoining.");
		return;
	}
}

void ClientFullConnect(int client) {
	//This function is called twice, once on authorize, and once when the player info is gotten
	//We must / should prevent it from actually executing twice.
	if(IsClientSourceTV(client))
		return;

	if(IsClientInGame(client) && GetClientTeam(client) != CS_TEAM_NONE)
		return;

	InitPlayer(client);

	#if defined EVERYONE_CAN_SPEC
		CanClientSpectate[client] = true;
	#endif

	if(!Go4TK_Game_InProgress) { //  && !IsFakeClient(client)
		//ChangeClientTeam(client, CS_TEAM_T);
		SetEntProp(client, Prop_Send, "m_iTeamNum", CS_TEAM_T);

		if(!IsPlayerAlive(client))
			CS_RespawnPlayer(client);
	} else if(!CanClientSpectate[client]) { //  && !IsFakeClient(client)
		KickClient(client, "You joined too late as the game is already in progress and do not have permission to spectate");
		return;
	} else {
		ChangeClientTeam(client, CS_TEAM_SPECTATOR);
		SetEntProp(client, Prop_Send, "m_iTeamNum", CS_TEAM_SPECTATOR);

		PrintCenterText(client, "<font color='#ff0000'>You cant play because the game already started, sorry!</font>");
	}

	//Queue_CheckinSteamid(client_AuthIds64[client]);

	Backend_SetUserIngame(client);

	UpdateTrashtalkStuff();
}

Action SetFogControllerForClient(Handle timer, int client) {
	if(IsValidClient(client)) {
		char pFog[16]; FormatEx(pFog, sizeof(pFog), "FogCtrl-%i", client);
		SetVariantString(pFog);
		AcceptEntityInput(client, "SetFogController");
	}
}

static void InitPlayer(int client) {
	if(LibraryExists("sendproxy")) {
		//SendProxy_Hook(client, "m_angEyeAngles[0]", Prop_Float, FreelookEyeangFaker1);
		//SendProxy_Hook(client, "m_angEyeAngles[1]", Prop_Float, FreelookEyeangFaker2);

		/*for (int i = 0; i <= 9; i++)
			SendProxy_HookArrayProp(client, "m_hMyWeapons", FAKE_WEAPON_OFFSET + i, Prop_Int, hMyWeaponsFaker);*/

		//TODO seems unnencessary, maybe needed
		//SendProxy_Hook(client, "m_vecOrigin", Prop_Vector, LagcompOriginFaker);
	}

	if(USE_ARBITRARY_LAGCOMPENSATION && !IsClientSourceTV(client))
		InitLagcompForPlayer(client);

	HM_ResetPlayerHealthManagement(client);

	ClientScore[client] = 0;

	ResetTeamingDetectionForClient(client);
}

//player_disconnect gets called before OnClientDisconnect is ¯\_(ツ)_/¯
public Action Event_OnDisconnectPre(Event event, const char[] name, bool dontBroadcast) {
	event.BroadcastDisabled = true;

	int client = GetClientOfUserId(event.GetInt("userid"));

	//Can happen when client is rejected in onclientconnect
	if(client <= 0 || IsClientSourceTV(client))
		return Plugin_Changed;

	if(IsValidEdict(PLAYER_FOG_CONTROLLERS[client]))
		AcceptEntityInput(PLAYER_FOG_CONTROLLERS[client], "Kill");

	PLAYER_FOG_CONTROLLERS[client] = -1;

	Debug("DISCONNECT EINZ");

	if(client >= 0 && client <= 64) {
		PrintToServer("%i disconnecting, steamid: %s, dbid: %i", client, client_AuthIds[client], ClientUserDbId[client]);

		if(IsClientConnected(client) && IsValidEdict(client)) {
			Debug("DISCONNECT ZWEI");

			#if !defined DEBUG
				if(Go4TK_Game_InProgress) {
					if(IsPlayerAlive(client) && !IsFakeClient(client)) {
						event.GetString("reason", sumTmpStr, sizeof(sumTmpStr));

						if(StrEqual(sumTmpStr, "disconnect", false)) {
							//Ban user for 1800 seconds (30min)
							Backend_BanUser(client, -1, 1800, "Leaving a match that is in progress without using !leave");
						}

						//////////////////

						for(int offset = 0; offset < 128; offset += 4)
							SetEntDataEnt2(client, CBasePlayer_hMyWeapons + offset, -1, true);

					    //If the player is alive, and the game is in progress we want to suicide the client when the disconnects
					    //To "better" do stuff like granting kills to attackers, ragdoll spawning, etc. (less redundancy)
					    //This might either work well, or it might not work at all. We'll find out.
						//ForcePlayerSuicide(client);
					}
				}
			#endif

			if(IsPlayerAlive(client)) {
				if(!Go4TK_Game_InProgress) //We dont want to drop shit for players pre-game.
					HandlePlayerDeath(client, true, true);
				else { //We gotta always call player_death because thats where we do magic

					//Well, we cant damage the client on disconnect as that wont do nuffin. Sending a faked death-event wont appear on the upper right.
					//Using suicide will leave a bugged player shadow behind, but thats the best we can do for now i guess.

					Event FakeDeathEvent = CreateEvent("player_death");

					FakeDeathEvent.SetInt("userid", event.GetInt("userid"));

					if(IsValidClient(player_last_attacker[client])) {
						SetEntData(player_last_attacker[client], CCSPlayer_iNumRoundKills, GetEntData(player_last_attacker[client], CCSPlayer_iNumRoundKills, 1) + 1, 1, true);

						FakeDeathEvent.SetInt("attacker", GetClientUserId(player_last_attacker[client]));
					}

					FakeDeathEvent.SetBool("headshot", false);

					FakeDeathEvent.Fire();

					//ForcePlayerSuicide(client);
				}
			}
		}

		Backend_SetUserIngame(client, false);

		isWindowOpen[client][WSCONN_map] = false;
		isWindowOpen[client][WSCONN_inv] = false;

		plIsCurrentlyParachuting[client] = false;

		if(IsValidEntity(playerParachuteModels[client])) {
			RemoveEdict(playerParachuteModels[client]);

			playerParachuteModels[client] = INVALID_ENT_REFERENCE;
		}

		delete CurrentActionTimer[client];

		ResetTeamingDetectionForClient(client);

		if(ws_clients[client] != INVALID_WEBSOCKET_HANDLE) {
			WebsocketHandle x = ws_clients[client];

			ws_clients[client] = INVALID_WEBSOCKET_HANDLE;

			Websocket_UnhookChild(x);
		}
	}

	return Plugin_Changed;
}

public void OnClientDisconnect(int client) {
	if(g_iTvClient == client)
		g_iTvClient = 0;

	if(LibraryExists("sendproxy")) {
		//SendProxy_Unhook(client, "m_angEyeAngles[0]", FreelookEyeangFaker1);
		//SendProxy_Unhook(client, "m_angEyeAngles[1]", FreelookEyeangFaker2);

		//TODO seems unnencessary, maybe needed
		//SendProxy_Unhook(client, "m_vecOrigin", LagcompOriginFaker);
	}

	if(IsClientInGame(client) && !IsFakeClient(client) && !IsClientSourceTV(client))
		Backend_SetUserIngame(client, false);

	IsPlayerInited[client] = false;
	xCount[client] = 0;

	CanClientSpectate[client] = false;
	CanClientBan[client] = false;
	g_bPlayerAliveCache[client] = false;

	client_AuthIds[client] = "";
	client_AuthIds64[client] = "";
	client_Flags[client] = 0;

	prevClientAmmoCount[client] = 0;

	ClientScore[client] = 0;

	g_PVMid[client] = -1;

	clientReceivedAdAt[client] = -1.0;

	for(int i = 0; i < 3; i++)
		isWindowOpen[client][i] = false;

	//We only want to request players on disconnect if they disconnect before the game has started
	if(!Go4TK_Game_InProgress && IsClientInGame(client) && Go4TK_Game_StartDelay > GetGameTime())
		Queue_RequestPlayers();
}

public Action FreelookEyeangFaker1(int entity, const char[] PropName, float &iValue, int element) {
	iValue = plRealEyeAngles[entity][0];
	return Plugin_Changed;
}

public Action FreelookEyeangFaker2(int entity, const char[] PropName, float &iValue, int element) {
	iValue = plRealEyeAngles[entity][1];
	return Plugin_Changed;
}

public Action SDKTraceAttack(int victim, int &attacker, int &inflictor, float &damage, int &damagetype, int &ammotype, int hitbox, int hitgroup) {
	if(Go4TK_Game_InProgress) {
		if(IsValidClient(victim) && IsValidClient(attacker) && (damagetype & DMG_SLASH)) {

			//SetEntData(victim, CCSPlayer_bGunGameImmunity, 0, 1, true);
			damage = 12.0;
			//SetEntData(victim, CCSPlayer_bGunGameImmunity, 1, 1, true);

			return Plugin_Changed;
		}
	}

	//Damage dealing is handled by Bulletsimulator
	return Plugin_Stop;
}

public Action SDKOnTakeDamage(int victim, int &attacker, int &inflictor, float &damage, int &damagetype) {
	if(!Go4TK_Game_InProgress)
		return Plugin_Stop;

	if(damagetype == DMG_FALL) {
		//No fall damage in the first 21 seconds of the game)
		if(GetGameTime() - Go4TK_Game_StartTime < 21.0)
			return Plugin_Stop;
		else {
			//Lets lower the fall damage shall we
			damage /= 2.0;
			if(damage < 4.0)
				return Plugin_Stop;
			else
				return Plugin_Changed;
		}
	} else if(damagetype == DMG_DROWN)
		return Plugin_Continue;


	return Plugin_Continue;
}

public void SDKOnTakeDamageAlivePost(int victim, int attacker, int inflictor, float damage, int damagetype) {
	//if the player will die from this damage we need to strip his m_hMyWeapons table before he does
	//because otherwise the weapons will be killed as technically only he can own them.
	if(IsValidClient(victim)) {
		Debug("%N Will be at %i health after taking this damage", victim, GetClientHealth(victim));

		if(GetClientHealth(victim) <= 0) {
			for(int offset = 0; offset < 128; offset += 4)
		        SetEntDataEnt2(victim, CBasePlayer_hMyWeapons + offset, -1, true);

			SetEntDataEnt2(victim, CBasePlayer_hActiveWeapon, -1, true);
        }
    }
}

static Action Event_PlayerDeath(Event event, const char[] name, bool dontBroadcast) {
	int client = GetClientOfUserId(event.GetInt("userid"));

	g_PVMid[client] = -1;

	if(!Go4TK_Game_InProgress)
		return Plugin_Continue;

	int attacker = GetClientOfUserId(event.GetInt("attacker"));
	bool changed = false;

	if(!IsValidClient(attacker)) {
		attacker = -1;

		if(IsValidClient(player_last_attacker[client])) {
			attacker = player_last_attacker[client];

			event.SetInt("attacker", GetClientUserId(attacker));
			changed = true;
		}
	} else {
		if(CurrentClientLobby[attacker] == CurrentClientLobby[client]) {
			PrintCenterText(attacker, "<font color='#ef6868'>No points for teamkilling</font> %N", client);
		} else if(IsValidClient(attacker) && IsPlayerAlive(attacker)) {
			//Give points to killer
			int points = Calculate_Points_For_Kill(attacker, event.GetBool("headshot"));

			PrintCenterText(attacker, "<font color='#69f0ae'>+%i points</font> for killing %N", points, client);

			ClientScore[attacker] += points;
		}
	}

	//Fixes crash on death when the team menu popped upp
	if(!IsFakeClient(client))
		SendConVarValue(client, FindConVar("sv_disable_show_team_select_menu"), "0");

	HM_ResetPlayerHealthManagement(client);
	SetClientOverlay(client);

	Lobby_HandlePlayerElimination(client, attacker);

	HandlePlayerDeath(client, IsClientInGame(client));

	g_bPlayerAliveCache[client] = false;

	//After processing the death with the CURRENT alive state, we need to update the cached, global alive state for
	//further actions, like ending the game.
	//Lobby_GetActiveLobbyCount();

	return (changed ? Plugin_Changed : Plugin_Continue);
}

static Action Event_BulletImpact(Event event, const char[] name, bool dontBroadcast) {
	float Origin[3];
	Origin[0] = event.GetFloat("x");
	Origin[1] = event.GetFloat("y");
	Origin[2] = event.GetFloat("z");

	#if defined DEBUG
		int Color[4] = {255, 255, 0, 150};

		float Origin2[3];
		Origin2[0] = Origin[0];
		Origin2[1] = Origin[1];
		Origin2[2] = Origin[2] + 32.0;

		TE_SetupBeamPoints(Origin, Origin2, BeamModelIndex, 0, 0, 0, 10.0, 0.1, 10.0, 1, 0.0, Color, 10);
		TE_SendToAll();

		Origin2[2] = Origin[2] - 64.0;

		TE_SetupBeamPoints(Origin, Origin2, BeamModelIndex, 0, 0, 0, 10.0, 0.1, 10.0, 1, 0.0, Color, 10);
		TE_SendToAll();
	#endif

	int client = GetClientOfUserId(event.GetInt("userid"));

	if(client > 0 && IsClientInGame(client)) {
		float eyePos[3];
		GetClientEyePosition(client, eyePos);

		MakeVectorFromPoints(eyePos, Origin, Origin);
		NormalizeVector(Origin, Origin);

		AddSimulatedBullet(client, eyePos, Origin);
	}

	event.BroadcastDisabled = true;
	return Plugin_Changed;
}

//If set to true, the client will be aimpunched in the next OnPlayerRunCmd
//static bool clientShouldReceiveCrosshairPunch[MAXPLAYERS + 1] = false;

static Action Event_WeaponFire(Event event, const char[] name, bool dontBroadcast) {
	int client = GetClientOfUserId(event.GetInt("userid"));

	if(!g_bDisableSemiAuto && enforceSingleshots[client])
		SetEntDataFloat(client, CCSPlayer_flNextAttack, HI_FL, true);

	#if defined VERBOSE
		int weapon = GetPlayerWeapon(client);
		Debug("m_weaponMode: %i", GetEntData(weapon, CWeaponCSBaseGun_weaponMode));
	#endif

	lastAttackTime[client] = GetGameTime();

	//if(GetPlayerFakeWeaponSlot(client) != SLOT_FISTS)
	//	clientShouldReceiveCrosshairPunch[client] = GetEntData(weapon, CWeaponCSBaseGun_weaponMode) != 1;

	//SetEntDataVector(client, CBasePlayer_aimPunchAngleVel, NULL_VECTOR, false);
	//SetEntDataVector(client, CBasePlayer_aimPunchAngle, BULLETDROP_AIMPUNCH_VEL, false);

	//Member: m_viewPunchAngle (offset 120) (type vector) (bits 0) (Coord|ChangesOften)
	//Member: m_aimPunchAngle (offset 132) (type vector) (bits 0) (Coord|ChangesOften)
	//Member: m_aimPunchAngleVel (offset 144) (type vector) (bits 0) (Coord|ChangesOften)

	//Abort Crafting / Shredding / Using on Weaponfire
	if(CurrentActionTimer[client] != null) {
		delete CurrentActionTimer[client];
		SetEntProp(client, Prop_Send, "m_iProgressBarDuration", 0);
	}
}

static Action Event_RoundPreStart(Event event, const char[] name, bool dontBroadcast) {
	if(Go4TK_Items != null)
		ResetGo4TKInventory();
}

Action Event_RoundStart(Event event, const char[] name, bool dontBroadcast) {
	PrintToServer("Round start...");

	LoadSettings();

	forceStart = false;

	InitFakeWeaponStruct();
	/*for(i = 1; i <= MaxClients; i++)
		PLAYER_FOG_CONTROLLERS[i] = -1;

	while((i = FindEntityByClassname(i, "env_fog_controller")) != -1)
		AcceptEntityInput(i, "Kill");*/

	//Allow for hot-reloading

	//if((i = FindEntityByClassname(-1, "env_cascade_light")) != -1)
	//	AcceptEntityInput(i, "Kill");

	//SetConVarFloat(FindConVar("weapon_recoil_view_punch_extra"), 0.02, true);
	//SetConVarFloat(FindConVar("weapon_recoil_scale"), 0.0, true);

	g_CurrentEntityCount = GetCurrentEntityCount();

	Go4TK_Game_StartDelay = -1.0;
	Go4TK_Game_InProgress = false;

	transitionSafezoneCenter = vAs(float, DEFAULT_GAS_CENTER);
	transitionSafezoneRadius = DEFAULT_GAS_RADIUS;

	currentSafezoneCenter = vAs(float, DEFAULT_GAS_CENTER);
	currentSafezoneRadius = DEFAULT_GAS_RADIUS;

	targetSafezoneCenter = vAs(float, DEFAULT_GAS_CENTER);
	targetSafezoneRadius = DEFAULT_GAS_RADIUS;

	Go4TK_Next_Safezone_Shrink = 9999999999.0;
	Go4TK_Safezone_Shrink_Targettime = 0.0;

	hasGasSpread = false;

	LoopIngameClients(client)
		InitPlayer(client);

	Queue_RequestPlayers();

	LoopClients(client) if(!IsValidClient(client) || !IsClientSourceTV(client))
		if(!IsClientConnected(client)) {
			CurrentClientLobby[client] = -1;

			if(ClientUserDbId[client] != -1) {
				Backend_SetUserIngame(client, false);
				ClientUserDbId[client] = -1;
			}
		} else if(IsClientInGame(client)) {
			//ChangeClientTeam(client, CS_TEAM_T);
			//CS_SwitchTeam(client, CS_TEAM_T);
			SetEntProp(client, Prop_Send, "m_iTeamNum", CS_TEAM_T);
		}
}

//REDACTED//

static Action Event_PlayerSpawn(Event event, const char[] name, bool dontBroadcast) {
	int client = GetClientOfUserId(event.GetInt("userid"));

	SetEntPropFloat(client, Prop_Data, "m_flMaxspeed", DEFAULT_MAXSPEED);

	spectatedPlayer[client] = -1;

	InitGo4TKInventoryForClient(client);

	client_connectionwarn_timer[client] = WS_WARN_TIME;

   	CreateTimer(0.0, OnPlayerSpawned, client, TIMER_FLAG_NO_MAPCHANGE);

	//Csgo is so fucking buggy. It somehow ignores the set roundtime cvar until a round restart, which never happens, so we just set it manually.
	GameRules_SetProp("m_iRoundTime", 3600, 4, 0, true);

	Lobby_HandlePlayerSpawn(client);

	//REDACTED//
}

static Action OnPlayerSpawned(Handle timer, int client) {
	if(IsValidClient(client) && IsPlayerAlive(client)) {
		PrintToServer("Player spawned %N", client);

		//Hides Radar, timer, players / scores, ...
		HideHud(client, HIDEHUD_RADARANDTIMER);

   		g_PVMid[client] = GetClientViewModelIndex(client, -1);

		//Strip weapons given by default
		/*int Secondary = GetPlayerWeaponSlot(client, CS_SLOT_SECONDARY);
		if(Secondary > MaxClients && IsValidEntity(Secondary) && HasEntProp(Secondary, Prop_Send, "m_hOwner") && GetEntPropEnt(Secondary, Prop_Send, "m_hOwner") == client) {
			CS_DropWeapon(client, Secondary, false, true);
			AcceptEntityInput(Secondary, "kill");
		}

		int Knife = GetPlayerWeaponSlot(client, CS_SLOT_KNIFE);
		if(Knife > MaxClients && IsValidEntity(Knife) && HasEntProp(Knife, Prop_Send, "m_hOwner") && GetEntPropEnt(Knife, Prop_Send, "m_hOwner") == client) {
			CS_DropWeapon(client, Knife, false, true);
			AcceptEntityInput(Knife, "kill");
		}*/

		InitFakeWeaponClient(client);

		GivePlayerWeaponSlot(client, SLOT_FISTS, "weapon_knife");
		/*int bino = */
		GivePlayerWeaponSlot(client, SLOT_BINOCULAR, "weapon_c4"); //Binocular Proxyitem

		//if(bino != -1 && IsValidEdict(bino))
		//	SDKHook(bino, SDKHook_SetTransmit, C4_LimitTransmit);

		//Give player 5 Bandages...
		PlayerInv(client, Carrying).AddItem(Bandage, 5, client);

		if(!IsFakeClient(client)) {
			#if defined NOAC
				SendConVarValue(client, FindConVar("weapon_recoil_view_punch_extra"), "0.02");
				SendConVarValue(client, FindConVar("weapon_recoil_scale"), "1.0");
				SendConVarValue(client, FindConVar("view_recoil_tracking"), "0.5");
			#else
				//REDACTED//
			#endif

			//Connecting is now handled trough the looping timer things in game_manager
			//WS_ConnectClient(client);
		}

		SetEntProp(client, Prop_Send, "m_bHasHelmet", 0);
		SetEntProp(client, Prop_Send, "m_ArmorValue", 0);
		#if defined DEBUG
			/*Go4TK_Playerinv(client).PickupItem(Item().Create(Helmet));
			Go4TK_Playerinv(client).PickupItem(Item().Create(Laminated_Vest));
			Go4TK_Playerinv(client).PickupItem(Item().Create(Military_Backpack));
			Go4TK_Playerinv(client).PickupItem(Item().Create(Medikit, 5));
			Go4TK_Playerinv(client).PickupItem(Item().Create(Ammo_M4, 90));
			Go4TK_Playerinv(client).PickupItem(Item().Create(M4));*/
		#endif

   		GetClientModel(client, sumTmpStr, sizeof(sumTmpStr));

   		//Should get set with the map KV file, but if i should forget to copy that file meh ¯\_(ツ)_/¯
   		if(!StrEqual(sumTmpStr, PLAYER_T_MODEL)) {
	   		SetEntProp(client, Prop_Send, "m_nModelIndex", g_PlayerModelIndex);
	   		ChangeEdictState(client, FindDataMapInfo(client, "m_nModelIndex"));
   		}

		if(IsValidEdict(PLAYER_FOG_CONTROLLERS[client]))
			AcceptEntityInput(PLAYER_FOG_CONTROLLERS[client], "Kill");

		PLAYER_FOG_CONTROLLERS[client] = -1;

   		InitFogControllerForClient(client);

		CreateTimer(0.0, OnPlayerSpawnedPost, client, TIMER_FLAG_NO_MAPCHANGE);
	}
}

/*static Action C4_LimitTransmit(int entity, int client) {
	if(IsValidClient(client) && GetPlayerFakeWeaponSlotWeapon(client, SLOT_BINOCULAR) != entity)
		return Plugin_Stop;

	return Plugin_Continue;
}*/

static Action OnPlayerSpawnedPost(Handle timer, int client) {
	if(IsValidClient(client)) {
		if(IsPlayerAlive(client))
			SelectPlayerWeaponSlot(client, SLOT_FISTS);
		IsPlayerInited[client] = true;


		//if(LibraryExists("filenetmessages"))
		//	FNM_SendFile(client, "maps/br_go4tk_b1.jpg");
	}
}

static void AbortInteract(int client, bool onlyTimer = false) {
	if(CurrentActionTimer[client] != null) {
		delete CurrentActionTimer[client];

		if(!onlyTimer) {
			DoForEverySpectatingClientAndSelf(client, toDoOn) {
				Handle pb = StartMessageOne("HudMsg", toDoOn);
				PbSetInt(pb, "channel", HUDMSG_CHANNEL_PROGRESS);
				EndMessage();
			}

			//Is already auto-replicated by spectating logic
			SetEntProp(client, Prop_Send, "m_iProgressBarDuration", 0);
		}
	}
}

static void SendInteract(int client, char[] interaction, Go4TK_Item_Type iType, int interactTime) {
	char itemInteractString[32];
	FormatEx(itemInteractString, sizeof(itemInteractString), "%s %s..", interaction, ITEM_INFOS[iType][Name]);

	DoForEverySpectatingClientAndSelf(client, toSendTo)
		HudMsg(toSendTo, HUDMSG_CHANNEL_PROGRESS, vAs(float, { -1.0, 0.7 }), { 255, 100, 255, 255 }, { 0, 255, 0, 255 }, 2, 0.0, 0.0, float(interactTime), float(interactTime), itemInteractString);

	SendItemActionInProcess(client, interactTime, itemInteractString);
}

void ClientActionItem(int client, Item item, InMessageTypes action) {
	if(IsValidClient(client) && IsPlayerAlive(client)) {
		//Attempted action doesnt make sense.
		if((action == Shred && !item.IsShreddable) ||
			(action == Use && !item.IsUseable) ||
			(action != Use && action != Shred)) {

			return;
		}

		#if !defined DEBUG
			if(!Go4TK_Game_InProgress) {
				HudMsg(client, HUDMSG_CHANNEL_PROGRESS, vAs(float, { -1.0, 0.65 }), { 255, 100, 255, 255 }, { 0, 255, 0, 255 }, 2, 0.0, 0.0, 2.0, 2.0, "You cannot do this right now.");
				return;
			}
		#endif

		if(!item.IsValid()) return;

		AbortInteract(client, true);

		if(action == Shred && !item.CanMoveToSlot(SL_None)) return;

		int itemInteractTime = ITEM_DEFINITIONS[item.Type][action == Use ? Useable : Shreddable];

		DataPack dPack;
		CurrentActionTimer[client] = CreateDataTimer(float(itemInteractTime), DoDelayedAction, dPack);

		dPack.WriteCell(client); dPack.WriteCell(action); dPack.WriteCell(item);

		SendInteract(client, action == Use ? "Using" : "Shredding", item.Type, itemInteractTime);

		SetEntPropFloat(client, Prop_Send, "m_flProgressBarStartTime", GetGameTime());
		SetEntProp(client, Prop_Send, "m_iProgressBarDuration", itemInteractTime);
	}
}

void ClientActionCraft(int client, Go4TK_Item_Type iType = None) {
	if(IsValidClient(client) && IsPlayerAlive(client)) {
		AbortInteract(client);

		if(ITEM_DEFINITIONS[iType][Craftable] == -1) return;

		DataPack dPack;
		CurrentActionTimer[client] = CreateDataTimer(float(ITEM_DEFINITIONS[iType][Craftable]), DoDelayedAction, dPack);

		dPack.WriteCell(client); dPack.WriteCell(Craft); dPack.WriteCell(iType);

		SendInteract(client, "Crafting", iType, ITEM_DEFINITIONS[iType][Craftable]);

		SetEntPropFloat(client, Prop_Send, "m_flProgressBarStartTime", GetGameTime());
		SetEntProp(client, Prop_Send, "m_iProgressBarDuration", ITEM_DEFINITIONS[iType][Craftable]);
	}
}

static Action DoDelayedAction(Handle timer, DataPack dPack) {
	dPack.Reset();

	int client = dPack.ReadCell();

	CurrentActionTimer[client] = null;

	if(IsValidClient(client) && IsPlayerAlive(client)) {
		SetEntProp(client, Prop_Send, "m_iProgressBarDuration", 0);

		int action_int = dPack.ReadCell();

		//Leave action
		if(action_int == -1) {
			KickClient(client, "Thank you for playing!");
			return;
		}

		InMessageTypes action = vAs(InMessageTypes, action_int);

		if(action == Craft) {
			Go4TK_Playerinv(client).CraftItem(vAs(Go4TK_Item_Type, dPack.ReadCell()));
		} else if(action == Use || action == Shred) {
			Item item = Item(dPack.ReadCell());

			if(action == Use) {
				if(Go4TK_Playerinv(client).UseItem(item)) {
					if(item.Type == Bandage)
						HM_AddHealTarget(client, HEAL_TARGET_BANDAGE);
					else if(item.Type == Medikit)
						HM_AddHealTarget(client, HEAL_TARGET_MEDIKIT, 4);
					else if(item.Type == Procoagulant)
						HM_AddHealTarget(client, 0, 4);
				}
			} else if(action == Shred) {
				Go4TK_Playerinv(client).ShredItem(item);
			}
		}
	}
}

float plEyeAngles[MAXPLAYERS + 1][3];
bool plIsInAutorun[MAXPLAYERS + 1] = {false, ...};

static Action Command_Buyammo1(int client, const char[] command, int argc) {
	plIsInAutorun[client] = !plIsInAutorun[client];
	return Plugin_Stop;
}

//REDACTED//

#if defined DEBUG || defined NOAC
	public Action OnPlayerRunCmd(int client, int &buttons, int &impulse, float vel[3], float angles[3], int &weapon, int &subtype, int &cmdnum, int &tickcount, int &seed, int mouse[2]) {
#else
	//REDACTED//
#endif
	if(IsClientSourceTV(client)) {
		g_bPlayerAliveCache[client] = false;
	 	return Plugin_Continue;
 	}

	plRealEyeAngles[client] = angles;

	static int PLAYER_LASTBUTTONS[MAXPLAYERS+1] = {0, ...};

	g_bPlayerAliveCache[client] = IsPlayerAlive(client);

	if(CBaseEntity_vecOrigin == 0)
		return Plugin_Continue;

	GetEntDataVector(client, CBaseEntity_vecOrigin, plLastPosition[client]);

	float xxxxxx[3];

	GetLogicalCoordFromPhysical(plLastPosition[client], xxxxxx);

	//currentSafezoneCenter = xxxxxx;
	//transitionSafezoneCenter = xxxxxx;
	//targetSafezoneCenter = xxxxxx;

	if(!g_bPlayerAliveCache[client]) {
		if((buttons&~PLAYER_LASTBUTTONS[client])&IN_SCORE) {
			OpenPanelForClient(client, WSCONN_map);

			HideHud(client, HIDEHUD_RADARANDTIMER);
		}

		if(PLAYER_LASTBUTTONS[client] != buttons)
			PLAYER_LASTBUTTONS[client] = buttons;

		int playerInTpTrigger = CheckVectorIsInTpTrigger(plLastPosition[client]);

		if(playerInTpTrigger != -1) {
			MoveVectorByTpTrigger(plLastPosition[client], playerInTpTrigger);

			TeleportEntity(client, plLastPosition[client], NULL_VECTOR, NULL_VECTOR);
		}
	}

	if(g_bPlayerAliveCache[client] && IsPlayerInited[client]) {
		bool weaponSwitched = false;
		int previousSlot = -1;
		int btns = buttons;
		bool changed = false;

		if(USE_ARBITRARY_LAGCOMPENSATION)
			AddSnaphotForPlayer(client, angles);

		if(weapon > 0 && IsValidEdict(weapon)) {
			changed = true;
			Debug("Wants to switch to %i", weapon/*, GetEntPropEnt(client, Prop_Send, "m_hLastWeapon")*/); // (%i)

			int desiredSlot = IsFakeWP(weapon);

			Debug("%N pressed slot%i / weap %i", client, desiredSlot, weapon);

			//0 == Incgrenade = Lastinv (Q)
			if(desiredSlot == 0) {
				ClientActionItem(client, PlayerInv(client, Carrying).GetItemByType(Bandage), Use);
			#if defined DEBUG
			} else if(desiredSlot == 7) {

				float vecMins[3];
				GetClientEyePosition(client, vecMins);
				float vecOrigin[3];

				vecOrigin = vecMins;


				for (int i = AsInt(Duct_Tape); i <= AsInt(Yeezys); i++) {
					vecOrigin[0] = vecMins[0] + (RoundToFloor(float(i) / float(5)) * 60);
					vecOrigin[1] = vecMins[1] + ((i % 5) * 50);

					Handle trace = TR_TraceRayEx(vecOrigin, view_as<float>({90.0, 0.0, 0.0}), MASK_ALL, RayType_Infinite);

					if(trace != null && TR_DidHit(trace)) {
						float endPos[3];
						TR_GetEndPosition(endPos, trace);

						if(128.0 > GetVectorDistance(vecOrigin, endPos) > 5.0) {
							Item newItem = Item().Create(vAs(Go4TK_Item_Type, i), 1, endPos);

							if(newItem.SuperType == ST_Ammo)
								newItem.Amount = 31;
						}
						delete trace;
					}
				}
			#endif
			} else if(desiredSlot == 8) {
				OpenPanelForClient(client, WSCONN_map);
			#if defined DEBUG
			} else if(desiredSlot == 9) {
				Queue_FlushReservedSlotsToDisk();

				/*AddSimulatedBullet(client, {-2257.593018, -2753.569824, -5484.262207}, {10.490873, 33.977917, 0.000000});

				TE_Start("Shotgun Shot");
				TE_WriteVector("m_vecOrigin", {-2257.593018, -2753.569824, -5484.262207});
				TE_WriteFloat("m_vecAngles[0]", 10.490873);
				TE_WriteFloat("m_vecAngles[1]", 33.977917);

				TE_WriteNum("m_weapon", -1);
				TE_WriteFloat("m_fInaccuracy", 0.0);
				TE_WriteFloat("m_flRecoilIndex", 0.0);
				TE_WriteFloat("m_fSpread", 0.0);
				TE_WriteNum("m_nItemDefIndex", WEAPON_SSG08);
				TE_WriteNum("m_iSoundType", 0);

				TE_SendToAll(0.1);*/


			#endif
			} else {
				previousSlot = GetPlayerFakeWeaponSlot(client);
				weaponSwitched = SelectPlayerWeaponSlot(client, desiredSlot) != -1;
			}
		}
		int currentSlot = GetPlayerFakeWeaponSlot(client);

		//WORKAROUND so that the player will re-ads incase he was ads'ing while switching the weapon
		if(weaponSwitched && !g_bDisableWeaponADS)
			PLAYER_LASTBUTTONS[client] = PLAYER_LASTBUTTONS[client] & IN_SPEED ? IN_SPEED : 0;

		if(plIsInAutorun[client] && !plIsCurrentlyParachuting[client]) {
			if(btns & IN_FORWARD || btns & IN_BACK) {
				plIsInAutorun[client] = false;
			} else {
				vel[0] = 450.0; //GetEntDataFloat(client, CBasePlayer_flMaxspeed);

				buttons |= IN_FORWARD;
				changed = true;
			}
		}

		//PICKUP Logic
		if((btns&~PLAYER_LASTBUTTONS[client])&IN_USE) {
			#define PickupVars client, HUDMSG_CHANNEL_PICKEDUP, vAs(float, {-1.0, 0.52}), {220,220,220,255}, { 255, 255, 255, 255 }, 2, _, 0.5, 1.5, 1.0

			if(GetEntData(GetPlayerWeapon(client), CWeaponCSBaseGun_bReloadVisuallyComplete) != 1) {
				HudMsg(PickupVars, "You cannot pick up items while reloading");
			} else {
				float m_vecOrigin[3];
				GetClientEyePosition(client, m_vecOrigin);

				Handle tr = TR_TraceRayFilterEx(m_vecOrigin, angles, MASK_VISIBLE, RayType_Infinite, TRDontHitSelf, client);

				int x;

				if(TR_DidHit(tr) && (x = TR_GetEntityIndex(tr)) > 0 && IsValidEdict(x) && DROPPED_ITEM_ITEMS[x] != -1) {
					//-2 is set for dropped "death crates"
					if(DROPPED_ITEM_ITEMS[x] == BODYBAG_FAKEID)
						OpenPanelForClient(client, WSCONN_inv);
					else {
						Item toUseItem = Item(DROPPED_ITEM_ITEMS[x]);

						int prevAmount = toUseItem.Amount;

						if(toUseItem.IsValid() && toUseItem.BoundEnt == x && toUseItem.IsInReach(m_vecOrigin)) {
							Item pickedUp = Go4TK_Playerinv(client).PickupItem(toUseItem);

							if(pickedUp.IsValid()) {
								if(pickedUp.Stackable)
									if(toUseItem.Amount != 0 && toUseItem.id != pickedUp.id)
										HudMsg(PickupVars, "Picked up %i/%i %s(s)", prevAmount - toUseItem.Amount, prevAmount, ITEM_INFOS[pickedUp.Type][Name]);
									else
										HudMsg(PickupVars, "Picked up %i %s(s)", prevAmount, ITEM_INFOS[pickedUp.Type][Name]);
								else
									HudMsg(PickupVars, "Picked up %s", ITEM_INFOS[pickedUp.Type][Name]);
							} else {
								HudMsg(PickupVars, "You dont have enough space for that");
							}
						}
					}
				}

				delete tr;
			}
		}

		//TODO Needs tweaking.
		if(btns & IN_ATTACK) {
			if(currentSlot == SLOT_FISTS)
				if(GetGameTime() > GetEntDataFloat(client, CCSPlayer_flNextAttack) + 0.1)
					RequestFrame(DebounceFistSpeed, client);


		}

		//Binocular Handler
		if(currentSlot == SLOT_BINOCULAR && (btns&~PLAYER_LASTBUTTONS[client])&IN_ATTACK2) { //btns NOT lastbuttons & in_attack => rmb just pressed
			if((PLAYER_BINOCULAR_TIMER[client] = CreateTimer(0.1, DisplayBinocularForClient, client, TIMER_FLAG_NO_MAPCHANGE)) == null)
				DisplayBinocularForClient(null, client);
			EmitSoundToClient(client, "hostage/huse/hostage_pickup.wav", _, _, _, _, 0.2);
			SetEntPropFloat(client, Prop_Data, "m_flMaxspeed", BINOCULAR_MAXSPEED);

			Debug("%N into binocular (pre)", client);
		} else if((weaponSwitched && previousSlot == SLOT_BINOCULAR) || (currentSlot == SLOT_BINOCULAR && (btns^PLAYER_LASTBUTTONS[client])&IN_ATTACK2)) { //btns XOR lastbuttons => rmb just unpressed
			if(PLAYER_BINOCULAR_TIMER[client] != null) {
				KillTimer(PLAYER_BINOCULAR_TIMER[client]);
				PLAYER_BINOCULAR_TIMER[client] = null;
			} else {
				if(!weaponSwitched) {
					SetClientFov(client);

					if(IsValidEntity(g_PVMid[client]))
						SetEntProp(g_PVMid[client], Prop_Send, "m_nModelIndex", g_iv_BinocularModel);
				}
				UnhideHud(client, HIDEHUD_CROSSHAIR);
			}
			SetEntPropFloat(client, Prop_Data, "m_flMaxspeed", DEFAULT_MAXSPEED);


			SetEntData(client, CCSPlayer_bIsScoped, 0, 1, true);
			ChangeEdictState(client, CCSPlayer_bIsScoped);
			Debug("%N out of binocular", client);
			PLAYER_IS_IN_BINOCULAR[client] = false;
			SetClientOverlay(client);
		}

		//Block in_attack when player has bino in hand (bomb)
		if(currentSlot == SLOT_BINOCULAR) {
			buttons &= ~IN_ATTACK; changed = true;
		} else {
			//PLAYER NOT USING BINOCULAR
			float gameTime = GetGameTime();
			int curWeap = GetPlayerWeapon(client);
			if(curWeap == -1) curWeap = GetEntDataEnt2(client, CBasePlayer_hActiveWeapon); //FALLBACK

			if(currentSlot != SLOT_FISTS && IsValidEdict(curWeap) /* || (previousSlot != SLOT_FISTS && weaponSwitched)*/) {
				float nextPrimAttack;

				if(!g_bDisableSemiAuto || !g_bDisableWeaponADS)
					nextPrimAttack = GetEntDataFloat(curWeap, CWeaponCSBaseGun_flNextPrimaryAttack);

				//set m_flNextAttack to m_flNextPrimaryAttack on release when weapon is semi auto
				if(!g_bDisableSemiAuto && enforceSingleshots[client] && (btns^PLAYER_LASTBUTTONS[client])&IN_ATTACK && !(btns & IN_ATTACK))
					SetEntDataFloat(client, CCSPlayer_flNextAttack, nextPrimAttack, true);

				if(!g_bDisableWeaponADS) {
					//If weapon cant attack, block ADS - retry next Runcmd
					if(nextPrimAttack > gameTime && //Next Primaryattack is in Future
							btns & IN_ATTACK2 && //IN_ATTACK2 (RMB) is Held
							(PLAYER_WEAPON_ISSCOUT[client] || (!(PLAYER_LASTBUTTONS[client] & IN_ATTACK2) || weaponSwitched))) //Weapon either is scout, or when non-scout RMB was down last frame, and weapon wasnt switched
						btns &= ~IN_ATTACK2; //Block rmb from propagating to PLAYER_LASTBUTTONS so its re-tried next tick

					//Allow rightclick (ADS)
					if((btns^PLAYER_LASTBUTTONS[client]) & IN_ATTACK2) {
						int newFov = GetEntProp(client, Prop_Send, "m_iDefaultFOV");

						int isInADS = !!(btns & IN_ATTACK2);
						#if defined VERBOSE
							Debug("InADS: %i", isInADS);
						#endif

						if(isInADS) newFov -= ADS_FOV_SUBTRACT;

						//Setting it here as well to allow the client proper prediction
						SetEntPropFloat(client, Prop_Data, "m_flMaxspeed", isInADS ? 100.0 : DEFAULT_MAXSPEED);

						SetEntData(curWeap, CWeaponCSBaseGun_weaponMode, isInADS, 1, true);

						SetEntDataFloat(client, CCSPlayer_flNextAttack, isInADS ? gameTime + 0.25 : nextPrimAttack, true);

						//Need to set isScoped to 1 even for rifles because otherwise fog will behave .. weirdly
						SetEntData(client, CCSPlayer_bIsScoped, isInADS, 1, true);

						if(PLAYER_WEAPON_ISSCOUT[client] || weaponSwitched) {
							bool kp = isInADS && (PLAYER_WEAPON_ISSCOUT[client] || !weaponSwitched);
							//SetEntProp(curWeap, Prop_Send, "m_zoomLevel", kp);

							if(kp) newFov = SCOUT_FOV;
							EmitSoundToClient(client, "weapons/ssg08/zoom.wav", _, _, _, _, 0.2);
						} else {

							if(isInADS) EmitSoundToClient(client, "weapons/sg556/sg556_zoom_in.wav", _, _, _, _, 0.2);
							else EmitSoundToClient(client, "weapons/sg556/sg556_zoom_out.wav", _, _, _, _, 0.2);
						}

						SetClientFov(client, newFov, 0.3);
						ChangeEdictState(client, CCSPlayer_bIsScoped);
					}
				}

				//Check if reserveammo changed so we can update it in the inventory too
				int curReserveAmmoCount = GetEntData(curWeap, CBaseCombatWeapon_iPrimaryReserveAmmoCount, 2);

				if(!weaponSwitched && prevClientAmmoCount[client] != curReserveAmmoCount && curReserveAmmoCount < prevClientAmmoCount[client]) {
					Go4TK_Item_Type type = GetWeapAmmoType(curWeap);

					if(type != None) {
						Item ownedAmmo = PlayerInv(client, Carrying).GetItemByType(type);

						if(ownedAmmo.IsValid(true)) {
							int usedAmmoInReload = prevClientAmmoCount[client] - curReserveAmmoCount;

							Debug("UsedInReload = %i, available: %i, inReserve: %i", usedAmmoInReload, ownedAmmo.Amount, curReserveAmmoCount);

							//The reload used MORE ammo than the player even has. First off remove the too-much used ammo in the primary clip, then empty the secondary slot as there obviously is no more.
							if(ownedAmmo.Amount < usedAmmoInReload) {
								int newAmmo = GetEntData(curWeap, CBaseCombatWeapon_iClip1, 2) + (ownedAmmo.Amount - usedAmmoInReload);
								if(newAmmo < 0) newAmmo = 0;

								SetEntData(curWeap, CBaseCombatWeapon_iClip1, newAmmo, 2, true);

								SetEntData(curWeap, CBaseCombatWeapon_iPrimaryReserveAmmoCount, 0, 2, true);
							} else {
								Debug("Setting Ammotype %i, to amount: %i", ownedAmmo.Type, curReserveAmmoCount);

								ownedAmmo.Amount = curReserveAmmoCount;
							}
						}
					}
				}
				prevClientAmmoCount[client] = curReserveAmmoCount;
			}

			BlockSecondary(curWeap);
		}

		if((btns&~PLAYER_LASTBUTTONS[client])&IN_SCORE)
			OpenPanelForClient(client, WSCONN_inv);

		//Abort Crafting / Shredding / Using on move
		GetEntDataVector(client, CCSPlayer_vecVelocity, tmpOrigin);

		if(FloatAbs(tmpOrigin[0]) + FloatAbs(tmpOrigin[1]) > 90.0 || btns & IN_JUMP)
			AbortInteract(client);

		//always block in_attack2 because of custom logic
		if(buttons & IN_ATTACK2)
			buttons &= ~IN_ATTACK2; //changed = true;

		//Freelock. While sneak is held the serverside eyeangles get frozen
		if(!g_bDisableFreelook) {
			if(btns & IN_SPEED) {
				buttons &= ~IN_SPEED; //changed = true;

				if(!(PLAYER_LASTBUTTONS[client] & IN_SPEED))
					plEyeAngles[client] = angles;

				SetEntDataFloat(client, CCSPlayer_flNextAttack, HI_FL, true);

				//angles = New
				angles = plEyeAngles[client];
			//When sneak is released force the client to their "correct" eyeangles
			} else if((btns^PLAYER_LASTBUTTONS[client])&IN_SPEED) {
				angles = plEyeAngles[client];
				//TODO possibly find a workaround to prevent model twitching when going out of freelook mode. Possibly with sendproxy
				/*SetEntPropFloat(client, Prop_Send, "m_angEyeAngles[0]", plEyeAngles[client][0]);
				SetEntPropFloat(client, Prop_Send, "m_angEyeAngles[1]", plEyeAngles[client][1]);
				ChangeEdictState(client, FindDataMapInfo(client, "m_angEyeAngles[0]"));
				ChangeEdictState(client, FindDataMapInfo(client, "m_angEyeAngles[1]"));*/

				TeleportEntity(client, NULL_VECTOR, plEyeAngles[client], NULL_VECTOR);

				int activeWeapon = GetEntDataEnt2(client, CBasePlayer_hActiveWeapon);

				if(IsValidEntity(activeWeapon)) {
					float nextWeapPrimAttack = GetEntDataFloat(activeWeapon, CWeaponCSBaseGun_flNextPrimaryAttack);

					SetEntDataFloat(client, CCSPlayer_flNextAttack, nextWeapPrimAttack, true);
				}
				//changed = true;
			}
		}

		if(PLAYER_LASTBUTTONS[client] != btns)
			PLAYER_LASTBUTTONS[client] = btns;

		if(weapon != 0 || changed) {
			weapon = 0;
			return Plugin_Changed;
		}
	} else if(CanClientSpectate[client] && IsClientInGame(client)) {
		spectatedPlayer[client] = GetEntData(client, CCSPlayer_hObserverTarget, 1);

		if(spectatedPlayer[client] > 63)
			spectatedPlayer[client] = -1;
	}

	return Plugin_Continue;
}

public void DebounceFistSpeed(int client) {
	if(IsValidClient(client) && GetPlayerFakeWeaponSlot(client) == SLOT_FISTS) {
		//SetEntDataFloat(client, CCSPlayer_flNextAttack, GetGameTime() + 0.8, true);

		int activeWeapon = GetPlayerWeapon(client);

		if(IsValidEntity(activeWeapon)) {
			float nextAttack = GetGameTime() + 0.60;

			SetEntDataFloat(activeWeapon, CWeaponCSBaseGun_flNextPrimaryAttack, nextAttack + 0.05, true);

			BlockSecondary(activeWeapon);

			SetEntDataFloat(client, CCSPlayer_flNextAttack, nextAttack, true);
		}
	}
}

public void OnGameFrame() {
	if(USE_ARBITRARY_LAGCOMPENSATION)
		ProcessBullets();
}

public void SDKPreThink(int client) {
	if(!USE_ARBITRARY_LAGCOMPENSATION)
		ProcessBullets(_, _, client);
}

//Player PreThink hook
//static float clEyeAng[3], newPunch[3];
public void SDKPreThinkPost_ParachuteLogic(int client) {
	if(g_bPlayerAliveCache[client]) {

		//Parachuting physics
		if(plIsCurrentlyParachuting[client]) {
			//The player cannot be on the ground yet technically.
			bool tooEarly = GetGameTime() - Go4TK_Game_StartTime < 7.0;

			if(!tooEarly && GetEntityFlags(client) & FL_ONGROUND) {
				GivePlayerParachute(client, true);
			} else {
				float velTest[3]; float downVel;

				GetEntDataVector(client, CCSPlayer_vecVelocity, velTest);
				//If player is moving upwards abort.
				//Normally, you cannot get to this state 5.5 seconds into the round, but it can be caused by tabbing into the game while parachuting, so we do thizz
				if(!tooEarly && velTest[2] > 0.0) {
					GivePlayerParachute(client, true);
					//We dont need this hook anymore, lets get rid of it.
					SDKUnhook(client, SDKHook_PreThinkPost, SDKPreThinkPost_ParachuteLogic);
				} else {
					//Downward
					if((downVel = velTest[2]) < -450.0)
						downVel = -450.0;

					//Forward
					velTest[0] = (plRealEyeAngles[client][0] / 2.0) + 20.0;
					if(velTest[0] < 0.0)
						velTest[0] = 0.0;
					//Sidewards
					velTest[1] = plRealEyeAngles[client][1];
					velTest[2] = 0.0;

					GetAngleVectors(velTest, velTest, NULL_VECTOR, NULL_VECTOR);
					ScaleVector(velTest, 1300.0);

					velTest[2] = downVel;

					TeleportEntity(client, NULL_VECTOR, NULL_VECTOR, velTest);
				}
			}
		}
	}
}

static Action Command_LookAtWeapon(int client, const char[] command, int argc) {
	if(IsValidClient(client) && IsClientInGame(client))
		ClientActionItem(client, PlayerInv(client, Carrying).GetItemByType(Medikit), Use);
	return Plugin_Stop;
}

static Action Command_Autobuy_Openmap(int client, const char[] command, int argc) {
	OpenPanelForClient(client, WSCONN_map);
	return Plugin_Stop;
}

public Action CS_OnCSWeaponDrop(int client, int weapon) {
	ClientActionItem(client, PlayerInv(client, Carrying).GetItemByType(Procoagulant), Use);
	return Plugin_Stop;
}

static Action SDKOnWeaponCanUse(int client, int weapon) {
	if (IsFakeWP(weapon) != -1) return Plugin_Stop;
	return Plugin_Continue;
}

static Action SDKWeaponSwitchPost(int client, int weapon) {
	if(weapon == -1)
		return;

	//WORKAROUND Valve decided bReloadVisuallyComplete should be 0 when switching the weapon, even if the weapon is "ready to shoot", would prevent item pickup
	SetEntData(weapon, CWeaponCSBaseGun_bReloadVisuallyComplete, 1, 1);

	if(weapon == EntRefToEntIndex(PLAYER_EQUIP[client][SLOT_FISTS])) { //Slot 4 = Nothing / fists.
		if(IsValidEdict(g_PVMid[client])) {
			SetEntProp(weapon, Prop_Send, "m_nModelIndex", 0); //Hide knife Firstperson
			SetEntProp(weapon, Prop_Send, "m_hWeaponWorldModel", 0); //Hide knife Thirdperson

			//Custom Fists model removed for now as im unsure wether its bannable or nah.
			SetEntProp(g_PVMid[client], Prop_Send, "m_nModelIndex", g_iPunchModel);

			//SetEntPropFloat(client, Prop_Send, "m_flNextAttack", HI_FL);
		}
	} else if(weapon == EntRefToEntIndex(PLAYER_EQUIP[client][SLOT_BINOCULAR])) {
		if(IsValidEdict(g_PVMid[client])) {
			SetEntProp(weapon, Prop_Send, "m_hWeaponWorldModel", 0); //Hide Bomb Thirdperson

			SetEntProp(g_PVMid[client], Prop_Send, "m_nModelIndex", g_iv_BinocularModel);
		}
	} else {
		char wpClassName[16];
		GetEntityClassname(weapon, wpClassName, sizeof(wpClassName));

		enforceSingleshots[client] =
			(PLAYER_WEAPON_ISSCOUT[client] = StrEqual("weapon_ssg08", wpClassName, false)) ||
			StrEqual("weapon_m4a1", wpClassName, false);

		//if(PLAYER_WEAPON_ISSCOUT[client])
		//	SetEntDataFloat(weapon, CWeaponCSBaseGun_flNextPrimaryAttack, HI_FL, true);

		//WORKAROUND m_flNextPrimaryAttack is "0" on switch, would allow exploiting of singleshot Mechanism
		SetEntDataFloat(weapon, CWeaponCSBaseGun_flNextPrimaryAttack, GetEntDataFloat(client, CCSPlayer_flNextAttack), true);

		Go4TK_Item_Type type = GetWeapAmmoType(weapon);

		Debug("AMMO TYPE: %i", type);

		prevClientAmmoCount[client] = 0;

		if(type != None) {
			Item ownedAmmo = PlayerInv(client, Carrying).GetItemByType(type);
			int ownedAmmoAmount = ownedAmmo.IsValid() ? ownedAmmo.Amount : 0;

			Debug("Player has %i ammo!", ownedAmmoAmount);

			//Set the weapons Secondary Ammo to the Ammocount in Inventory
			SetEntData(weapon, CBaseCombatWeapon_iPrimaryReserveAmmoCount, ownedAmmoAmount, 2, true);
		}
	}

	CalculateAddonBits(client, weapon, clientAddonBits[client]);
	//WORKAROUND Re-Set m_weaponMode to 0 on switch as it can get "stuck" when switchting while in ADS

	SetEntData(weapon, CWeaponCSBaseGun_weaponMode, 0, 1, true);

	BlockSecondary(weapon);

	char formattedLoutoutString[64];

	for (int i = 0; i < 3; i++) {
		Go4TK_Inventory_Slots checkSlot = Weapon1;
		if(i == 1) checkSlot = Weapon2;
		else if(i == 2) checkSlot = Weapon3;

		if(!Item(PLAYER_INV[client][checkSlot]).IsValid()) {
			Format(formattedLoutoutString, sizeof(formattedLoutoutString), "%s[%i -] ", formattedLoutoutString, i+1);
		} else {
			if(GetPlayerFakeWeaponSlotWeapon(client, i+1) == weapon)
				Format(formattedLoutoutString, sizeof(formattedLoutoutString), "%s>%i %s< ", formattedLoutoutString, i+1, ITEM_INFOS[Item(PLAYER_INV[client][checkSlot]).Type][Name]);
			else
				Format(formattedLoutoutString, sizeof(formattedLoutoutString), "%s[%i %s] ", formattedLoutoutString, i+1, ITEM_INFOS[Item(PLAYER_INV[client][checkSlot]).Type][Name]);
		}
	}

	if(weapon == EntRefToEntIndex(PLAYER_EQUIP[client][SLOT_FISTS])) {
		StrCat(formattedLoutoutString, sizeof(formattedLoutoutString), ">4 Fists< ");
	} else {
		StrCat(formattedLoutoutString, sizeof(formattedLoutoutString), "[4 Fists] ");
	}

	if(weapon == EntRefToEntIndex(PLAYER_EQUIP[client][SLOT_BINOCULAR])) {
		StrCat(formattedLoutoutString, sizeof(formattedLoutoutString), ">5 Bino<");
	} else {
		StrCat(formattedLoutoutString, sizeof(formattedLoutoutString), "[5 Bino]");
	}

	HudMsg(client, HUDMSG_CHANNEL_LOADOUT, vAs(float, { -1.0, 0.85 }), { 200, 0, 200, 255 }, { 100, 0, 100, 200 }, 2, 0.0, 0.5, 4.0, 1.2, formattedLoutoutString);
}