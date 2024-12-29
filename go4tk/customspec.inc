stock bool IsClientInTeam(int client, int team)
{
	return client > 0
			&& IsClientInGame(client)
			&& IsPlayerAlive(client)
			&& (team == -1 || GetClientTeam(client) == team);
}

stock int GetNextClient(int client, bool fw=true, int team=-1, int forClient) {
	//If the client can global-spec, allow any valid client to be spectated
	//If the client cannot global spec, only allow players that are in the same lobby as him to be spectated.
	
	/*if(client < 1) //0 is always world, so theres no way to spec 0
		client = 1;
	else if(client > MaxClients)
		client = MaxClients;
	
	int d = (fw ? 1 : 0);
	int i = client;
	int begin = (fw ? 1 : MaxClients);
	int limit = (fw ? MaxClients + 1 : 0);
	
	for(;;) {
		i += d;
		
		if(i == limit)
			i = begin;
		
		//CBA to find this shit bug
		if(i > MaxClients + 1 || i < -1)
			return -1;
		
		if(i == client) {
			client = -1;
			break;
		} else if(IsClientInTeam(i, team)) {
			if(CanClientSpectate[forClient] || CurrentClientLobby[i] == CurrentClientLobby[forClient]) {
				client = i;
				break;
			}
		}
	}*/
	
	int direction = fw ? 1 : -1;
	
	int current = client;
	
	if(current < 1) //0 is always world, so theres no way to spec 0
		current = 1;
	else if(current > MaxClients)
		current = MaxClients;
		
	int start = current;
	bool backAtStart = false;
	
	for(;;) {
		current += direction;
		
		if(current == start) {
			backAtStart = true;
			
			if(!IsValidClient(start))
				return -1;
		}
		
		if(current >= MaxClients) {
			current = 1;
			//Start = 1, current set to 1, next loop incremented to 2, inifnitely checking if 2-MaxClients is 1 => infinite loop
			if(current == start)
				backAtStart = true;
		} else if(current < 1)
			current = MaxClients;
		
		if(IsClientInTeam(current, team) && (CanClientSpectate[forClient] || CurrentClientLobby[current] == CurrentClientLobby[forClient]))
			break;
		else if(backAtStart) {
			return -1;
		}
	}
	
	return current;
}

// Credits: tilastokeskus

public Action Cmd_spec_next(int client, const char[] command, int argc) {
	return SpecNext(client);
}

void SetSpectated(int client, int spectated) {
	if(!IsValidClient(client))
		return;

	if(!IsValidClient(spectated)) {
		//spectated = currentTarget;
		spectated = -1;
		SetEntProp(client, Prop_Send, "m_iObserverMode", SPECMODE_FREELOOK);
		SetEntityFlags(client, GetEntityFlags(client) | FL_FROZEN);
		TeleportEntity(client, vAs(float, {0.0, 0.0, 0.0}), NULL_VECTOR, NULL_VECTOR);
		
		PrintHintText(client, "Spectating Nobody");
		return;
	}
	
	SetEntProp(client, Prop_Send, "m_iObserverMode", SPECMODE_FIRSTPERSON);
	if(GetEntityFlags(client) & FL_FROZEN)
		SetEntityFlags(client, GetEntityFlags(client) &~ FL_FROZEN);

	SetEntPropEnt(client, Prop_Send, "m_hObserverTarget", spectated);
	UpdateOverlay(client, spectated);
	
	PrintHintText(client, "Spectating %N", spectated);
}

public Action SpecNext(int client) {
	if (client == 0 || !IsClientInGame(client) || IsPlayerAlive(client))
		return Plugin_Handled;
	
	int target = GetEntPropEnt(client, Prop_Send, "m_hObserverTarget");
	int nextTarget = GetNextClient(target, true, _, client);
	
	SetSpectated(client, nextTarget);
	
	return Plugin_Handled;
}

public Action Cmd_spec_prev(int client, const char[] command, int argc) {
	return SpecPrev(client);
}

public Action SpecPrev(int client) {
	if (client == 0 || !IsClientInGame(client) || IsPlayerAlive(client))
		return Plugin_Handled;
	
	int target = GetEntPropEnt(client, Prop_Send, "m_hObserverTarget");
	int nextTarget = GetNextClient(target, false, _, client);
	
	SetSpectated(client, nextTarget);
	
	return Plugin_Handled;
}

public Action Cmd_spec_player(int client, const char[] command, int argc) {
	return Plugin_Handled;
}

public Action Cmd_spec_mode(int client, const char[] command, int argc) {
	//PrintToServer("Cmd_spec_mode");
	
	if(GetEntProp(client, Prop_Send, "m_iObserverMode") != SPECMODE_FIRSTPERSON) {
		/*if(!IsValidClient(GetEntPropEnt(client, Prop_Send, "m_hObserverTarget"))) {
			LoopIngameClients(i) {
				if(IsPlayerAlive(i) && i != client) {
					SetEntPropEnt(client, Prop_Send, "m_hObserverTarget", i);
					break;
				}
			}
		}*/
		
		SetEntProp(client, Prop_Send, "m_iObserverMode", SPECMODE_FIRSTPERSON);
		//already handled in the main settransmit hook
		//SDKHook(client, SDKHook_SetTransmit, Spec_SetTransmit);
	} else if (CanClientSpectate[client]) {
		SetEntProp(client, Prop_Send, "m_iObserverMode", SPECMODE_FREELOOK);
		SetEntPropEnt(client, Prop_Send, "m_hObserverTarget", -1);
	}
	return Plugin_Handled;
}

//Prevents specs from being transmitted, thus hiding him from cheater's spec-radar things
/*static Action Spec_SetTransmit(int spectatingPlayer, int client) {
	if(spectatingPlayer != client)
		return Plugin_Handled;
		
	return Plugin_Continue;
}*/

static void UpdateOverlay(int spectatorClient, int spectatedClient) {
	if(IsValidClient(spectatedClient) && IsPlayerAlive(spectatedClient)) {
		spectatedPlayer[spectatorClient] = spectatedClient;
		
		SetClientOverlay(spectatedClient, spectatorClient);
	}
}

// make any players observing a dead CT observe another CT 
/*public Action Timer_CheckObservers(Handle timer, int client) {
	if (IsClientInGame(client) && !IsPlayerAlive(client)) {
		LoopIngameClients(i) {
			if (!IsPlayerAlive(i) && i != client)
			{
				// who this player is observing now
				int target = GetEntPropEnt(i, Prop_Send, "m_hObserverTarget");
				if (target == client)
				{
					// if it's the dead player, pick a int target
					int nextTarget = GetNextClient(client, true);
					if (nextTarget > 0)
						SetEntPropEnt(i, Prop_Send, "m_hObserverTarget", nextTarget);
				}
			}
		}
	}
}*/