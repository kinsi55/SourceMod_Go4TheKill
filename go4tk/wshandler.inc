WebsocketHandle ws;

enum WS_CONNECTIONS {
	WSCONN_main,
	WSCONN_inv,
	WSCONN_map
}

bool isWindowOpen[MAXPLAYERS + 1][WS_CONNECTIONS];

WebsocketHandle ws_clients[MAXPLAYERS + 1];

#define CLIENT_AUTH_PEPPER 1292472576

char CLIENT_AUTH_TOKENS[MAXPLAYERS + 1][16];
bool allowWsConnect[MAXPLAYERS + 1] =  { false, ... };

/*public void GetClientAuthToken(int client, char[] output, int maxlength) {
	if(strlen(CLIENT_AUTH_TOKEN_CACHE[client])
	char inpStr[64];

	Format(inpStr, sizeof(inpStr), "%s%s%i", CLIENT_AUTH_PEPPER, CLIENT_AUTH_SALT, client);

	SHA1String(inpStr, output);
}*/

static char splitParts[4][16];

public Action:OnWebsocketIncoming(WebsocketHandle:websocket, WebsocketHandle:newWebsocket, const String:remoteIP[], remotePort, String:protocols[256], String:getPath[2000]) {
	PrintToServer("WS CONNECT IP: %s", remoteIP);

	Websocket_HookChild(newWebsocket, OnWebsocketReceive, OnWebsocketDisconnect, OnChildWebsocketError);
	Debug("readyState: %d", _:Websocket_GetReadyState(newWebsocket));
	Debug("getPath: %s", getPath);

	int strLen = strlen(getPath);

	Debug("Checking path length.. %i", strLen);

	// /II/IIIIIIIII

	if(strLen < 1+1+1+8 || strLen > 1+2+1+10)
		return Plugin_Stop; //GET path doesnt make sense at all.

	getPath[0] = ' ';
	if(ExplodeString(getPath, "/", splitParts, 4, 16) != 2)
		return Plugin_Stop; //Get path doesnt contain 2 parts (Clientide, Sec-ID)

	Debug("ReqParts: %s %s", splitParts[0], splitParts[1]);
	int clId = StringToInt(splitParts[0]);

	Debug("Checking ingame.. %i", clId);
	if(!IsClientInGame(clId) || ws_clients[clId] != INVALID_WEBSOCKET_HANDLE)
		return Plugin_Stop; //Client aint in game, or already has active socket conn

	allowWsConnect[clId] = false;

	Debug("Checking secmatch.. %s %s", splitParts[1], CLIENT_AUTH_TOKENS[clId]);

	#if !defined INSECURE_WS
		if(!StrEqual(splitParts[1], CLIENT_AUTH_TOKENS[clId]))
			return Plugin_Stop; //Security token aint matchin'
	#endif

	//char sha1SecCompHash[41];
	//SubString(getPath, daIndex, -1, sha1SecCompHash, sizeof(sha1SecCompHash));

	//Debug("Checking shalen.. %i %s", strlen(sha1SecCompHash), sha1SecCompHash);
	//if(strlen(sha1SecCompHash) != 40) return Plugin_Stop; //SHA1 SEC-Hash passed isnt 40 chars long

	//GetClientAuthToken(clId, protocols, sizeof(protocols));

	//if(!StrEqual(protocols, sha1SecCompHash)) return Plugin_Stop; //Security hash aint matchin'

	ws_clients[clId] = newWebsocket;

	Debug(">> Assigned socket to client %N", clId);

	client_connectionwarn_timer[clId] = WS_WARN_TIME;

	strcopy(protocols, sizeof(protocols), "");
	return Plugin_Continue;
}

public OnWebsocketMasterError(WebsocketHandle:websocket, const errorType, const errorNum) {
	ws = INVALID_WEBSOCKET_HANDLE;
	Debug("MASTER SOCKET ERROR: handle: %d type: %d, errno: %d", _:websocket, errorType, errorNum);
}

public OnWebsocketMasterClose(WebsocketHandle:websocket) {
	ws = INVALID_WEBSOCKET_HANDLE;
	Debug("MASTER SOCKET CLOSE: handle: %d", _:websocket);
}

public OnChildWebsocketError(WebsocketHandle:websocket, const errorType, const errorNum) {
	Debug("CHILD SOCKET ERROR: handle: %d, type: %d, errno: %d", _:websocket, errorType, errorNum);

	OnWebsocketDisconnect(websocket);
}

public OnWebsocketDisconnect(WebsocketHandle:websocket) {
	Debug("CHILD SOCKET DISCONNET: handle: %d", _:websocket);
	LoopClients(i) {
		if(ws_clients[i] != INVALID_WEBSOCKET_HANDLE && ws_clients[i] == websocket) {
			ws_clients[i] = INVALID_WEBSOCKET_HANDLE;

			//Main WS connection lost, but client is still ingame. Re-connect
			//Only insta-reconnect if the game is in progress, otherwise we use the game-progress loop
			//to not "cancel" out video ads accidently
			if(Go4TK_Game_InProgress)
				CreateTimer(0.5, WS_ConnectClientDelayed, i);

			isWindowOpen[i][WSCONN_map] = false;
			isWindowOpen[i][WSCONN_inv] = false;
			break;
		}
	}
}

public void WS_ConnectClient(int client) {
	WS_ConnectClientDelayed(null, client);
}

public Action WS_ConnectClientDelayed(Handle timer, int client) {
	if(IsValidClient(client) && !IsFakeClient(client) && !IsClientSourceTV(client) && ws_clients[client] == INVALID_WEBSOCKET_HANDLE) {
		char theUrl[512];

		//REDACTED//

		#if !defined NOWS
			Debug("Sending url %s to client %N", theUrl, client);

			//Broken since some csgo Update. Will only work once after joining.
			//HiddenBrowserPanel(client, theUrl);

			//ShowMOTDScreen(client, theUrl, true);

			ShowVGUIPanelEx(client, "HELLOO", theUrl, _, _, false);
		#endif
	}
}

stock char MsgTmpStr[48];

public bool OpenPanelForClient(int client, WS_CONNECTIONS type) {
	if(!IsValidClient(client))
		return false;

	#if !defined DEBUG
		if(!Go4TK_Game_InProgress) {
			HudMsg(client, HUDMSG_CHANNEL_PROGRESS, vAs(float, { -1.0, 0.65 }), { 255, 100, 255, 255 }, { 0, 255, 0, 255 }, 2, 0.0, 0.0, 2.0, 2.0, "You cannot do this right now.");
			return false;
		}
	#endif

	if(type == WSCONN_inv) {
		isWindowOpen[client][WSCONN_inv] = true;
		//allowWsConnect[client] = true;

		char toSendItemStr[2048];

		FormatInvForClient(client, toSendItemStr, sizeof(toSendItemStr));

		Format(toSendItemStr, sizeof(toSendItemStr), "openinv|%s", toSendItemStr);

		return SendToClient(client, toSendItemStr, WSCONN_main);
	}

	if(type == WSCONN_map) {
		isWindowOpen[client][WSCONN_map] = true;
		//allowWsConnect[client] = true;

		FormatEx(MsgTmpStr, sizeof(MsgTmpStr), "openmap|1,%s", lastClientCordon[client]);

		SendToClient(client, MsgTmpStr, WSCONN_main);

		if(Go4TK_Next_Safezone_Shrink != 9999999999.0) {
			SafezoneToTmpStr();

			return SendToClient(client, sumTmpStr, WSCONN_map);
		}

		return true;
	}

	/*if(type == WSCONN_inv || type == WSCONN_map) {
		allowWsConnect[client] = true;
		return SendToClient(client, type == WSCONN_inv ? "openinv" : "openmap", WSCONN_main);
	}*/
	return false;
}

public bool SendToClient(int client, char[] message, WS_CONNECTIONS destination) {
	if(!IsValidClient(client) || ws_clients[client] == INVALID_WEBSOCKET_HANDLE)
		return false;

	if(!isWindowOpen[client][destination])
		return false;

	WebsocketReadyState x = Websocket_GetReadyState(ws_clients[client]);

	if(x != State_Open)
		return false;

	char tmpStr2[2048];
	if(destination != WSCONN_main)
		FormatEx(tmpStr2, sizeof(tmpStr2), "%s|%s", destination == WSCONN_inv ? "invmsg" : "mapmsg", message);
	else
		strcopy(tmpStr2, sizeof(tmpStr2), message);

	return Websocket_Send(ws_clients[client], SendType_Text, tmpStr2);
}

enum OutMessageTypes {
	MoveItem,
	AmountChange,
	AddItems,
	SetItems,
	Interact
}

enum InMessageTypes {
	Use,
	Shred,
	Drop,
	Equip,
	Loot,
	Move,
	Craft
}

void FormatItem(Item item, char[] str, int maxlength, bool append = false) {
	Format(str, maxlength, "%s%i,%i,%i,%i,%i,%i,%i,%i|%i|%i\n",
							append ? str : "",
							item.id,
							item.CurrentOwnerSlot,
							item.SuperType,
							item.Type,
							item.Amount,
							item.TypeWeight,
							item.Stackable,
							item.IsShreddable,
							item.IsUseable,
							item.IsEquippable);
}

public bool SendChangedItemAmount(int client, Item item, int oldAmount, int newAmount) {
	FormatEx(MsgTmpStr, sizeof(MsgTmpStr), "%i,%i,%i", AmountChange, item.id, newAmount);
	return SendToClient(client, MsgTmpStr, WSCONN_inv);
}

public bool SendChangedItemSlot(int client, Item item, Go4TK_Inventory_Slots newSlot, Go4TK_Inventory_Slots oldSlot) {
	FormatEx(MsgTmpStr, sizeof(MsgTmpStr), "%i,%i,%i,%i", MoveItem, item.id, newSlot, oldSlot);
	return SendToClient(client, MsgTmpStr, WSCONN_inv);
}

public bool SendItemAdded(int client, Item item) {
	FormatItem(item, MsgTmpStr, sizeof(MsgTmpStr));
	Format(MsgTmpStr, sizeof(MsgTmpStr), "%i\n%s", AddItems, MsgTmpStr); //Prepend Messagetype
	return SendToClient(client, MsgTmpStr, WSCONN_inv);
}

public bool SendItemActionInProcess(int client, int interactTime, const char[] interactString) {
	FormatEx(MsgTmpStr, sizeof(MsgTmpStr), "%i,%i,%s", Interact, interactTime, interactString); //Prepend Messagetype
	return SendToClient(client, MsgTmpStr, WSCONN_inv);
}

public void FormatInvForClient(int client, char[] formattedInv, int maxSize) {
	Item tmpItm;
	FormatEx(formattedInv, maxSize, "%i\n", AddItems); //Set Messagetype

	for(int i = 1; i <= 6; i++) {
		tmpItm = vAs(Item, PlayerInv(client, vAs(Go4TK_Inventory_Slots, i)));

		if(tmpItm.IsValid())
			FormatItem(tmpItm, formattedInv, maxSize, true);
	}

	for(int i = 0; i < PlayerInv(client, Carrying).Length; i++) {
		tmpItm = PlayerInv(client, Carrying).GetItem(i);

		//Item either has more than 0, or is stackable
		if(tmpItm.IsValid(true) && (tmpItm.Amount > 0 || tmpItm.Stackable))
			FormatItem(tmpItm, formattedInv, maxSize, true);
	}

	float clPos[3]; GetClientEyePosition(client, clPos);

	for (int i = 0; i < sizeof(DROPPED_ITEM_ITEMS); i++) {
		if(DROPPED_ITEM_ITEMS[i] > -1) {
			if(GetVectorDistance(DROPPED_ITEM_POSITIONS[i], clPos) <= MAX_PICKUP_DISTANCE) {
				tmpItm = Item(DROPPED_ITEM_ITEMS[i]);

				//Item either has more than 0, or is stackable
				if(tmpItm.IsValid())
					FormatItem(tmpItm, formattedInv, maxSize, true);
			}
		}
	}

	//TODO seems like arbitrary drops are (partially) bugged. Drop more than 30 items, arbitrarily dropped ones after dont get displayed in proximity.
	for (int i = 0; i < Go4TK_Arbitrary_Dropped.Length; i++) {
		Item zeItem = Item(Go4TK_Arbitrary_Dropped.Get(i));
		//Item either has more than 0, or is stackable
		if(zeItem.IsValid() && zeItem.IsInReach(clPos))
			FormatItem(zeItem, formattedInv, maxSize, true);
	}
}

public void SafezoneToTmpStr() {
	#define IngameToMapCoord(%1) RoundFloat(%1 / INGAME_TO_MAP_RATIO_DIVISOR)

	FormatEx(sumTmpStr, sizeof(sumTmpStr), "2,%i,%i,%i,%i,%i,%i",
											IngameToMapCoord(targetSafezoneCenter[0]),
											IngameToMapCoord(targetSafezoneCenter[1]),
											RoundFloat((targetSafezoneRadius / INGAME_TO_MAP_RATIO_DIVISOR) / 0.942),
											IngameToMapCoord(currentSafezoneCenter[0]),
											IngameToMapCoord(currentSafezoneCenter[1]),
											RoundFloat((currentSafezoneRadius / INGAME_TO_MAP_RATIO_DIVISOR) / 0.942));
}

public bool SendCurrentSafezoneToClient(int client) {
	//if(ws_clients[client][2] == INVALID_WEBSOCKET_HANDLE)
	//	return false;

	SafezoneToTmpStr();

	return SendToClient(client, sumTmpStr, WSCONN_map);
}

public bool MapOpened(int client) {
	FormatEx(MsgTmpStr, sizeof(MsgTmpStr), "1,%s", lastClientCordon[client]);

	SendToClient(client, MsgTmpStr, WSCONN_map);

	if(Go4TK_Next_Safezone_Shrink != 9999999999.0)
		SendCurrentSafezoneToClient(client);
}

public OnWebsocketReceive(WebsocketHandle:websocket, WebsocketSendType:iType, const String:receiveData[], const dataSize) {
	if(iType == SendType_Text) {

		LoopIngameClients(client) {
			if(IsFakeClient(client))
				continue;

			if(websocket == ws_clients[client]) {
				Debug("RECEIVED: %s", receiveData);
				InMessageTypes inMsg = vAs(InMessageTypes, -1);

				//Apart from close messages, the only thing that can actually *send* messages is the inventory
				//So we assume the messages come from that if the first character is numeric and try and parse
				//It as an InMessageType of the inventory.
				if(IsCharNumeric(receiveData[0])) {
					inMsg = vAs(InMessageTypes, StringToInt(receiveData[0]));
				} else {
					//C == Close(/ing)
					if(receiveData[0] == 'C')
						isWindowOpen[client][receiveData[1] == 'I' ? WSCONN_inv : WSCONN_map] = false;

					return;
				}

				//Split the inventory in message
				int splitPartC = ExplodeString(receiveData, ",", splitParts, 4, 4);

				//Min 2 max 4 arguments, otherwise its malicious
				if(splitPartC < 2 || splitPartC > 4) {
					Websocket_UnhookChild(websocket);
					ws_clients[client] = INVALID_WEBSOCKET_HANDLE;
					return;
				}

				//To be honest, i have no fucking idea anymore what I did here
				if(splitPartC < 4) {
					splitParts[3] = "";
					if(splitPartC == 2)
						splitParts[2] = "";
				}

				if(inMsg == Craft) {
					ClientActionCraft(client, vAs(Go4TK_Item_Type, StringToInt(splitParts[1])));
				} else {
					int itemId = StringToInt(splitParts[1]);
					Item item = Item(itemId);
					if(itemId < 0 || !item.IsValid()) {
						Websocket_UnhookChild(websocket);
						ws_clients[client] = INVALID_WEBSOCKET_HANDLE;
						return;
					}

					if(inMsg == Use || inMsg == Shred) {
						ClientActionItem(client, item, inMsg);
						//Go4TK_Playerinv(client).UseItem(item);
						//Go4TK_Playerinv(client).ShredItem(item);
					} else if(inMsg == Drop) {
						if(GetEntityFlags(client) & FL_ONGROUND)
							Go4TK_Playerinv(client).DropItem(item);
					} else if(inMsg == Equip) {
						if(item.IsEquippable && (item.CurrentOwnerSlot == Carrying || item.CurrentOwnerSlot == SL_None)) {
							Go4TK_Inventory_Slots newSlot = Carrying;
							if(item.SuperType == ST_Head)
								newSlot = Head;
							else if(item.SuperType == ST_Armor)
								newSlot = Armor;
							else if(item.SuperType == ST_Weapon)
								newSlot = Weapon1;
							else if(item.SuperType == ST_Storage)
								newSlot = Storage;
							else if(item.Type == Yeezys)
								newSlot = Boots;

							Go4TK_Playerinv(client).PickupItem(item, newSlot);
						}
					} else if(inMsg == Loot) {
						Go4TK_Playerinv(client).PickupItem(item);
					} else if(inMsg == Move) {
						int destSlot = StringToInt(splitParts[2]);

						int toMove = StringToInt(splitParts[3]);

						if(destSlot > 8 || destSlot <= 0) {
							Go4TK_Playerinv(client).DropItem(item, toMove);
						} else {
							//if(item.CurrentOwner != client)
							Go4TK_Playerinv(client).PickupItem(item, vAs(Go4TK_Inventory_Slots, destSlot), toMove);
							//else {
							//	Go4TK_Playerinv(client).SetSlotItem(vAs(Go4TK_Inventory_Slots, destSlot), item, item.CurrentOwnerSlot);
							//}
						}
					}
				}
				break;
			}
		}
	}
}

//moveitem|id|newslot
//amount|id|newAmount

//setitems/additems
//slot|id|type|amount
//slot2|id2|type2|amount2
//...

//Shred: amount-1 / additems
//Drop: moveitem|id|proximity
//Pickup: moveitem|id|respective_slot
//Use: amount-x



//INCOMING
//[use/shred/drop/equip/loot]|id
//move|id|newslot
//craft|type