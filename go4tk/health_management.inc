#define HEAL_TARGET_MEDIKIT 60
#define HEAL_TARGET_BANDAGE 10

#define MAX_BLEED_PER_SECOND 2
#define MAX_HEAL_PER_SECOND 4

static int player_heal_target[MAXPLAYERS + 1] = {100, ...};
static int player_heal_level[MAXPLAYERS + 1] = {0, ...};

static int player_bleed_level[MAXPLAYERS + 1] = {0, ...};

//used in transmit hook
int player_last_attacker[MAXPLAYERS + 1] = {-1, ...};

Handle HealProcessingTimer = null;

void HM_TakeDamge2(int client, int inflictor, int attacker, float damage, int damageType = DMG_GENERIC, bool fakeZoneDmg = false) {
	int currHealth = GetEntData(client, CBasePlayer_iHealth, 1);

	if(fakeZoneDmg && (currHealth % 3) == 0)
		fakeZoneDmg = false;

	if(!fakeZoneDmg)
		SDKHooks_TakeDamage(client, inflictor, attacker, damage, damageType);

	if(fakeZoneDmg || currHealth == GetEntData(client, CBasePlayer_iHealth, 1)) {
		//PrintToServer("We've tried to SDKHooks_TakeDamage the client %L but it resulted in no damage being dealt. %.2f", client, damage);
		//PrintToServer("=> %i %i %i", player_heal_target[client], player_heal_level[client], player_bleed_level[client]);

		if(float(currHealth) - damage >= 0)
			SetEntData(client, CBasePlayer_iHealth, RoundFloat(float(currHealth) - damage), 1, true);
		else if(IsPlayerAlive(client))
			ForcePlayerSuicide(client);

	}
}

public void HM_ResetPlayerHealthManagement(int client) {
	player_heal_target[client] = 100;
	player_bleed_level[client] = 0;
	player_heal_level[client] = 0;
	player_last_attacker[client] = -1;
}

public void InitHealthManagement() {
	LoopClients(i)
		HM_ResetPlayerHealthManagement(i);

	if(HealProcessingTimer == null)
		HealProcessingTimer = CreateTimer(1.0, MainHealthProcessTimer, _, TIMER_REPEAT);
}

static Action MainHealthProcessTimer(Handle timer) {
	if(!Go4TK_Game_InProgress)
		return;

	LoopIngameClients(i) if(g_bPlayerAliveCache[i] && !IsClientSourceTV(i)) {
		int currHealth = GetEntData(i, CBasePlayer_iHealth, 1);

		//If the player is bleeding, but has a healing target higher than his current health the bleeding-dmg is deducted from his
		//Healing target

		if(player_bleed_level[i] > 0) {
			Debug("%N is bleeding, Bleedlevel: %i, currHP: %i", i, player_bleed_level[i], currHealth);

			if(player_heal_target[i] > currHealth){
				if(player_heal_target[i] - player_bleed_level[i] > currHealth)
					player_heal_target[i] -= player_bleed_level[i];
				else
					player_heal_target[i] = currHealth;
			} else {
				player_heal_level[i] = 0;
				currHealth -= player_bleed_level[i];

				if(currHealth > 0)
					SetEntData(i, CBasePlayer_iHealth, currHealth, 1, true);
				else {
					if(player_last_attacker[i] != 0 && !IsValidClient(player_last_attacker[i]))
						player_last_attacker[i] = 0;

					//SetEntData(i, CCSPlayer_bGunGameImmunity, 0, 1, true);

					HM_TakeDamge2(i,
										player_last_attacker[i],
										player_last_attacker[i],
										1.0,
										DMG_GENERIC | DMG_NEVERGIB);

					//SetEntData(i, CCSPlayer_bGunGameImmunity, 1, 1, true);
				}
			}

			//Spawn bleed particlesystem
			GetEntDataVector(i, CBaseEntity_vecOrigin, tmpOrigin);
			tmpOrigin[2] += 48.0;

			//SpawnParticleSystem("blood_impact_mist_heavy", tmpOrigin, 0.6, i);

			Debug("new HP postbleed: %i", currHealth);
		}

		if(currHealth < player_heal_target[i] && player_heal_level[i] > 0) {
			Debug("%N has healingtarget: %i. Diff: %i (Pertick: %i)", i, player_heal_target[i], player_heal_target[i] - currHealth, player_heal_level[i]);

			int hp_to_add = player_heal_target[i] - currHealth;

			if(currHealth + hp_to_add > player_heal_level[i])
				hp_to_add = player_heal_level[i];

			if(hp_to_add > MAX_HEAL_PER_SECOND)
				hp_to_add = MAX_HEAL_PER_SECOND;

			if(currHealth + hp_to_add > 100)
				hp_to_add = 100 - currHealth;

			SetEntData(i, CBasePlayer_iHealth, currHealth + hp_to_add, 1, true);

			Debug("new HP postheal: %i", currHealth + hp_to_add);
		} else if(player_heal_level[i] > 0 && currHealth >= player_heal_target[i]) {
			player_heal_level[i] = 0;

			Debug("Aborting Heal of %N because his healtarget (%i) is below his current health (%i)", i, player_heal_target[i], currHealth);
		}
	}
}

stock void HM_OnTakeDamage(int victim, int damage, int attacker = 0) {
	//int currHealth = GetEntData(victim, CBasePlayer_iHealth, 1);

	//if(player_heal_target[victim] > currHealth)
	//	damage -= player_heal_target[victim] - currHealth;

	player_heal_target[victim] -= damage;

	player_last_attacker[victim] = attacker;

	if(attacker > 0 && player_bleed_level[victim] < MAX_BLEED_PER_SECOND && IsValidClient(attacker)){
		player_bleed_level[victim]++;

		SetClientOverlay(victim);
	}
}

stock void HM_AddHealTarget(int client, int healTarget, int bleedRemoved = 1) {
	int curPlayerHealth = GetEntData(client, CBasePlayer_iHealth, 1);

	if(player_heal_target[client] < curPlayerHealth)
		player_heal_target[client] = curPlayerHealth;

	player_heal_target[client] += healTarget;

	if(player_heal_target[client] > 100)
		player_heal_target[client] = 100;

	Debug("Adding Heal target for %N: %i. HP: %i, Target: %i", client, healTarget, curPlayerHealth, player_heal_target[client]);

	if(player_bleed_level[client] > 0)
		player_bleed_level[client] -= bleedRemoved;

	if(player_bleed_level[client] <= 0) {
		player_bleed_level[client] = 0;
		SetClientOverlay(client);
	}

	player_heal_level[client] += bleedRemoved;

	if(player_heal_level[client] > MAX_HEAL_PER_SECOND)
		player_heal_level[client] = MAX_HEAL_PER_SECOND;

	//if(healTarget == 0
}

stock bool HM_PlayerIsBleading(int client) {
	return player_bleed_level[client] > 0;
}