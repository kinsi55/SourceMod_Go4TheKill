//int g_iw_BinocularModel;    // Custom ViewModel index

#define FAKE_WEAPON_OFFSET 0

static int WP_SLOTS[10] = {-1, ...};
int PLAYER_EQUIP[MAXPLAYERS+1][10];
static int PLAYER_SLOT[MAXPLAYERS+1] = {-1, ...};

stock int IsFakeWP(int wp) {
	wp = EntIndexToEntRef(wp);
	for (int i = 0; i <= 9; i++)
		if (WP_SLOTS[i] == wp) return i;
	return -1;
}

static int DispatchEntityByName(const char[] classname) {
	//Valve FUCKED spawning of weapons with this because they are incapable and cant mitigate SHIT
	int x = CreateEntityByName(classname);
	/*int originalPackentitiesSetting = GetConVarInt(g_iSvParallelPackentitiesCvar);

	if(originalPackentitiesSetting != 0) {
		SetConVarInt(g_iSvParallelPackentitiesCvar, 0);

		//Needs to be in the frame after because having it in the same causes crashes.
		//RequestFrame(SetParalellPackentities, originalPackentitiesSetting);
		CreateTimer(0.5, SetParalellPackentities, originalPackentitiesSetting);
	}

	CEconItemDefinition ItemDefinition = PTaH_GetItemDefinitionByName(classname);

	if(!ItemDefinition)
		return INVALID_ENT_REFERENCE;

	int x = PTaH_SpawnItemFromDefIndex(ItemDefinition.GetDefinitionIndex(), vAs(float, {0.0, 0.0, 0.0}), vAs(float, {1.0, 1.0, 0.0}));*/

	if(!IsValidEdict(x))
		return INVALID_ENT_REFERENCE;

	//SetEdictFlags(x, FL_EDICT_DONTSEND);


	//DispatchKeyValue(x, "spawnflags", "9");
	DispatchSpawn(x);

	SetEntityMoveType(x, MOVETYPE_NONE);
	return EntIndexToEntRef(x);
}

/*static Action SetParalellPackentities(Handle timer, int originalPackentitiesSetting) {
	SetConVarInt(g_iSvParallelPackentitiesCvar, originalPackentitiesSetting);
}*/

/*public Action Hook_SetTransmit(int entity, int client) {
	if(entity != client)
		return Plugin_Handled;

	return Plugin_Continue;
}*/

void InitFakeWeaponStruct() {
	if(EntRefToEntIndex(WP_SLOTS[1]) == INVALID_ENT_REFERENCE)
		WP_SLOTS[1] = DispatchEntityByName("weapon_mac10");
	if(EntRefToEntIndex(WP_SLOTS[2]) == INVALID_ENT_REFERENCE)
		WP_SLOTS[2] = DispatchEntityByName("weapon_p250");
	if(EntRefToEntIndex(WP_SLOTS[3]) == INVALID_ENT_REFERENCE)
		WP_SLOTS[3] = DispatchEntityByName("weapon_knife");
	if(EntRefToEntIndex(WP_SLOTS[4]) == INVALID_ENT_REFERENCE)
		WP_SLOTS[4] = DispatchEntityByName("weapon_hegrenade"); //Also, slot6. Not distinguishable
	if(EntRefToEntIndex(WP_SLOTS[5]) == INVALID_ENT_REFERENCE)
		WP_SLOTS[5] = DispatchEntityByName("weapon_healthshot");

	if(EntRefToEntIndex(WP_SLOTS[7]) == INVALID_ENT_REFERENCE)
		WP_SLOTS[7] = DispatchEntityByName("weapon_flashbang");
	if(EntRefToEntIndex(WP_SLOTS[8]) == INVALID_ENT_REFERENCE)
		WP_SLOTS[8] = DispatchEntityByName("weapon_smokegrenade");
	if(EntRefToEntIndex(WP_SLOTS[9]) == INVALID_ENT_REFERENCE)
		WP_SLOTS[9] = DispatchEntityByName("weapon_decoy");
	//WP_SLOTS[10] = DispatchEntityByName("weapon_molotov"); //Detecting Lastinv AKA Q
	if(EntRefToEntIndex(WP_SLOTS[0]) == INVALID_ENT_REFERENCE)
		WP_SLOTS[0] = DispatchEntityByName("weapon_incgrenade");
	/*for (int i = 1; i <= 9; i++) {
		if(IsValidEntity(WP_SLOTS[i]))
			SDKHook(WP_SLOTS[i], SDKHook_SetTransmit, FakeWp_SetTransmit);
			//SetEdictFlags(WP_SLOTS[i], GetEdictFlags(WP_SLOTS[i]) | FL_EDICT_DONTSEND);
	}*/

	Debug("[WP] INITED WEAPON STRUCT!");
}

/*static Action FakeWp_SetTransmit(int entity, int client) {
	return Plugin_Stop;
}*/

stock void ResetFakeWeaponClient(int client) {
	PLAYER_SLOT[client] = -1;
	//Reset fakeinv for player so that others dont get influenced from this reset.

	if(IsClientInGame(client)) {
		SetEntDataEnt2(client, CBasePlayer_hActiveWeapon, PLAYER_SLOT[client], true);

		for(int offset = 0; offset < 128; offset += 4)
	        SetEntDataEnt2(client, CBasePlayer_hMyWeapons + offset, -1, true);
    }

	for (int i = 1; i <= 9; i++) {
		if(IsValidEntity(PLAYER_EQUIP[client][i]))
			RemoveEdict(PLAYER_EQUIP[client][i]);
		PLAYER_EQUIP[client][i] = -1;
	}
}

stock void RestoreFakeWeaponsForClient(int client) {
	//Restore fake weapons again.
	for (int i = 0; i <= 9; i++) {
		//Shouldnt be possible to happen anymore, but lets leave it here for security.
		if(!IsValidEntity(WP_SLOTS[i]) && i != 6)
			InitFakeWeaponStruct();
		SetEntDataEnt2(client, CBasePlayer_hMyWeapons + FAKE_WEAPON_OFFSET + 4*i, WP_SLOTS[i], true);
	}

	//Write the INC grenade (Slot 0) into the Weapons table so that Q (+lastinv) can refer to it
	//SetEntDataEnt2(client, CBasePlayer_hMyWeapons + FAKE_WEAPON_OFFSET + 120, WP_SLOTS[0], true);
	SetEntDataEnt2(client, CBasePlayer_hLastWeapon, WP_SLOTS[0], true);

	ChangeEdictState(client, CBasePlayer_hMyWeapons);
}

stock void InitFakeWeaponClient(int client) {
	//Per spec, the server should have no default-weapons set to be given on spawn, so this is useless
	//DropAllWeapons(client);
	//for(int i = 0; i <= 2; i++) {
	//
	//	CS_DropWeapon(client, Primary, false, true);
	//}
	ResetFakeWeaponClient(client);

	if(IsClientInGame(client)) {
		//Clean item table just to be sure.
		for(int offset = 0; offset < 128; offset += 4)
	        SetEntDataEnt2(client, CBasePlayer_hMyWeapons + offset, -1, true);

		for (int i = 1; i <= 9; i++) {
			if(IsValidEntity(WP_SLOTS[i])) {
				//Debug("[WP] Setting %i to %i for %N", FAKE_WEAPON_OFFSET + 4*i, WP_SLOTS[i], client);
				SetEntDataEnt2(client, CBasePlayer_hMyWeapons + FAKE_WEAPON_OFFSET + 4*i, WP_SLOTS[i], true);
				ChangeEdictState(client, CBasePlayer_hMyWeapons + FAKE_WEAPON_OFFSET + 4*i);
			}// else Debug("[WP] WP_SLOTS %i is invalid entity! (%i)", i, WP_SLOTS[i]);
	    }
    }

	Debug("[WP] inited client weapon struct for %N", client);
}

stock int EquipPlayerWeapon_FakeSafely(int client, int weapon) {
	//Temporarily remove all "fake" weapons so that they are not "un-eqipped" (dropped below the player) when eq' new weap of same type.
	for (int i = FAKE_WEAPON_OFFSET; i <= 96; i+=4)
    	SetEntDataEnt2(client, CBasePlayer_hMyWeapons + i, -1, true);

	EquipPlayerWeapon(client, weapon);

	RestoreFakeWeaponsForClient(client);
}

stock bool SetPlayerWeaponSlot(int client, int slot, int weapon = -1) {
	if(IsValidClient(client) && 9 >= slot >= 0) {
		Debug("[WP] Setting Fake WP slot %i of %i to %i", slot, client, weapon);

		if(weapon == -1) {
			PLAYER_EQUIP[client][slot] = -1;
			return true;
		}

		if(weapon != PLAYER_EQUIP[client][slot]) {
			bool toActiveSlot = GetEntDataEnt2(client, CBasePlayer_hActiveWeapon) == PLAYER_EQUIP[client][slot] && PLAYER_EQUIP[client][slot] != -1;

			if(toActiveSlot) CS_DropWeapon(client, PLAYER_EQUIP[client][slot], false);

			PLAYER_EQUIP[client][slot] = weapon;
			EquipPlayerWeapon_FakeSafely(client, weapon);

			if(toActiveSlot) SelectPlayerWeaponSlot(client, slot);

			return true;
		}
	}
	return false;
}

stock int SelectPlayerWeaponSlot(int client, int slot) {
	if(IsValidClient(client) && IsPlayerAlive(client) && 9 >= slot >= 0) {
		if(PLAYER_EQUIP[client][slot] == -1) return -1;

		int curActiveWeapon = GetEntDataEnt2(client, CBasePlayer_hActiveWeapon);
		int desiredWeapon = PLAYER_EQUIP[client][slot];

		if(!IsValidEntity(desiredWeapon))
			return -1;

		if(curActiveWeapon != desiredWeapon || PLAYER_SLOT[client] != slot) {
			Debug("Switching %N's weapon to slot %i (%i)", client, slot, desiredWeapon);

			EquipPlayerWeapon_FakeSafely(client, desiredWeapon);

			SDKCall(g_hSwitchWeaponCall, client, desiredWeapon, 0);
			SetEntDataEnt2(client, CBasePlayer_hActiveWeapon, desiredWeapon, true);

			ChangeEdictState(client, CBasePlayer_hActiveWeapon);
			SetEntDataEnt2(client, CBasePlayer_hLastWeapon, WP_SLOTS[0], true);

			PLAYER_SLOT[client] = slot;
			return desiredWeapon;
		}
	}
	return -1;
}

stock int GivePlayerWeaponSlot(int client, int slot, const char[] weapon) {
	if(IsValidClient(client) && 9 >= slot >= 0) {
		int weap = -1;

		if(StrEqual(weapon, "weapon_knife")) {
			weap = DispatchEntityByName(weapon);
		} else {
			//TODO(?) find a fix if player has sileneced equipped he will get silenced instead of A4, same w/ USP / P2K
			//If the to-give weapon is M4 or P2k change the team of the player to CT temporarily so that its skinned
			//if(StrEqual(weapon, "weapon_m4a1") || StrEqual(weapon, "weapon_hkp2000"))
			//	SetEntData(client, CDynamicProp_iTeamNum, CS_TEAM_CT, 1);

			//weap = DispatchEntityByName(weapon);

			//EquipPlayerWeapon_FakeSafely(client, weap);

			/*CEconItemDefinition ItemDefinition = PTaH_GetItemDefinitionByName(weapon);
			Debug("!!!!!!!! %i", ItemDefinition.GetLoadoutSlot());

			CEconItemView Item2 = PTaH_GetItemInLoadout(client, CS_TEAM_CT, ItemDefinition.GetLoadoutSlot());

			Debug("!!!!!!!!!!! %i", Item2.GetItemDefinition().GetDefinitionIndex());*/

			weap = GivePlayerItem(client, weapon);

			//SetEntData(client, CDynamicProp_iTeamNum, CS_TEAM_T, 1);
		}

		//if(weap != -1)
		//	SDKHook(weap, SDKHook_SetTransmit, SDKClientWeaponTransmit);

		SetPlayerWeaponSlot(client, slot, weap);
		return weap;
	}
	return -1;
}

/*public Action SDKClientWeaponTransmit(int weapon, int toTransmitTo) {
	if(!IsPlayerAlive(toTransmitTo) || !Go4TK_Game_InProgress || weapon == -1)
		return Plugin_Continue;

	int weapOwner = GetEntData(weapon, CBaseCombatWeapon_hOwner, 2);

	if(0 <= weapOwner <= 64)
		if(toTransmitTo == weapOwner)
			return Plugin_Continue;

	return Plugin_Handled;
}*/

stock int GetPlayerFakeWeaponSlot(int client) {
	return PLAYER_SLOT[client];
}

stock int GetPlayerWeapon(int client) {
	if(PLAYER_SLOT[client] == -1) return -1;
	return PLAYER_EQUIP[client][PLAYER_SLOT[client]];
}

stock int GetPlayerFakeWeaponSlotWeapon(int client, int slot) {
	return PLAYER_EQUIP[client][slot];
}