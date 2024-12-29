#define MAX_WEAP_SOUND_RANGE 24000.0
#define MAX_NORMAL_SOUND_RANGE 14000.0
#define MUTE_DISTANT_SOUND_BEFORE 2000.0

public void PrecacheWeaponSounds() {
	PrecacheSound(")weapons/ak47/ak47_01.wav", true);
	PrecacheSound(")weapons/hkp2000/hkp2000_01.wav", true);
	PrecacheSound(")weapons/nova/nova-1.wav", true);
	PrecacheSound(")weapons/ssg08/ssg08_01.wav", true);
	PrecacheSound(")weapons/m4a1/m4a1_02.wav", true);
	
	PrecacheSound(")weapons/m4a1/m4a1_distant_01.wav", true);
	PrecacheSound(")weapons/ssg08/ssg08_distant.wav", true);
	PrecacheSound(")weapons/nova/nova-1-distant.wav", true);
	PrecacheSound(")weapons/hkp2000/hkp2000-1-distant.wav", true);
	PrecacheSound(")weapons/ak47/ak47_distant.wav", true);
}

stock void PlaySimulatedWeaponShot(int shooter, int weapon) {
	if(!IsValidClient(shooter))
		return;
	
	float clientAbs[3]; float tmpAbs[3];
	GetEntDataVector(shooter, CBaseEntity_vecOrigin, clientAbs);
	
	//clientAbs = view_as<float>({3416.1, -2585.7, -12241.9});
	
	//Default fallback (m4a4 sound)
	char distSount[64] = ")weapons/m4a1/m4a1_distant_01.wav";
	char closeSound[64] = ")weapons/m4a1/m4a1_02.wav";
	
	if(weapon == WEAPON_AK47) {
		distSount = ")weapons/ak47/ak47_distant.wav";
		closeSound = ")weapons/ak47/ak47_01.wav";
	} else if(weapon == WEAPON_HKP2000) {
		distSount = ")weapons/hkp2000/hkp2000-1-distant.wav";
		closeSound = ")weapons/hkp2000/hkp2000_01.wav";
	} else if(weapon == WEAPON_NOVA) {
		distSount = ")weapons/nova/nova-1-distant.wav";
		closeSound = ")weapons/nova/nova-1.wav";
	} else if(weapon == WEAPON_SSG08) {
		distSount = ")weapons/ssg08/ssg08_distant.wav";
		closeSound = ")weapons/ssg08/ssg08_01.wav";
	}
	
	float origins[MAP_LAYERS][3];
	
	GetPhysicalVectorOnAllLaysersFromPhysicalPosition(clientAbs, origins);
	
	LoopIngameClients(i) {
		if(i != shooter && (alivePlayers <= 28 || currentPlayerLayer[i] == currentPlayerLayer[shooter])) {
			GetEntDataVector(i, CBaseEntity_vecOrigin, tmpAbs);
			
			float uDist = GetVectorDistance(tmpAbs, origins[currentPlayerLayer[i]]);
			
			if(uDist < 2048.0 || i == g_iTvClient) 
				EmitSoundToClient(i, closeSound, SOUND_FROM_WORLD, SNDCHAN_STATIC, SNDLEVEL_SCREAMING, _, _, _, _, i == g_iTvClient ? clientAbs : origins[currentPlayerLayer[i]]);
			
			float sndVol = (uDist - 1000) / 2000;
			if(sndVol > 0.8) 
				sndVol = 0.8;
			
			if(uDist > 1000.0) {
				//Move the sound *somewhere* on a line between the shooter and the recipient to not xmit the exact sound position and adjust the volume somewhat accordingly
				if(i != g_iTvClient) {
					float lineMod[3]; float lineMod2[3];
					MakeVectorFromPoints(tmpAbs, origins[currentPlayerLayer[i]], lineMod2);
					NormalizeVector(lineMod2, lineMod);
					//PrintToChatAll("lineMod2 %.1f %.1f %.1f", lineMod2[0], lineMod2[1], lineMod2[2]);
					//PrintToChatAll("lineMod %.1f %.1f %.1f", lineMod[0], lineMod[1], lineMod[2]);
					
					float factor = 0.65 - (GetURandomFloat() * 1.3);
					
					float hackery = factor * uDist;
					
					//PrintToChatAll("Moving sound by %.2f (%.2f => )", factor, hackery);
					
					ScaleVector(lineMod, hackery);
					
					//PrintToChatAll("Hackery %.1f %.1f %.1f", lineMod[0], lineMod[1], lineMod[2]);
					
					AddVectors(origins[currentPlayerLayer[i]], lineMod, lineMod);
					
					clientAbs = lineMod;
					
					sndVol *= ((uDist + hackery) / uDist) * 0.75;
					if(sndVol > 1.0)
						sndVol = 1.0;
					
					//PrintToChatAll("Playing at %.1f %.1f %.1f", lineMod[0], lineMod[1], lineMod[2]);
				}
			
				EmitSoundToClient(i, distSount, SOUND_FROM_WORLD, SNDCHAN_STATIC, SNDLEVEL_GUNFIRE, _, sndVol, _, _, clientAbs);
			}
		}
	}
}