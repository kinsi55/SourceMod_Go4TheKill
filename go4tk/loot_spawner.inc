#define spawn_trigger_name "spawn_area"

#define LOWER_ITEM_CLAMP 4
#define UPPER_ITEM_CLAMP 10

#define LOWER_SIZE_CLAMP 50064
#define UPPER_SIZE_CLAMP 400000

#define CITY_DIVISOR 2.0 //Generate this times less loot in the city.
#define SMALL_THRESHHOLD 100000

static float avgPerSize = 0.0;

stock void SpawnLoot(int startFrom = -1) {
	avgPerSize = FloatDiv(float(UPPER_SIZE_CLAMP - LOWER_SIZE_CLAMP), float(UPPER_ITEM_CLAMP - LOWER_ITEM_CLAMP));
	
	char iName[32];
	int index = startFrom;
	//int free_entities = g_CurrentEntityCount;
	
	float vecMins[3]; float vecMaxs[3]; float vecOrigin[3];
	
	int count = 0;
	static int totalGenerated = 0;
	
	if(startFrom == -1)
		totalGenerated = 0;
		
	/*int SpawnableWeaponCount = 0;
	
	for(int x = 0; x < sizeof(ITEM_DEFINITIONS); x++)
		if(ITEM_DEFINITIONS[x][SpawnProbability]) SpawnableWeaponCount++;*/
	
	while ((index = FindEntityByClassname(index, "trigger_multiple")) != -1) {
		GetEntPropString(index, Prop_Data, "m_iName", iName, sizeof(iName));
		
		if(StrEqual(iName, spawn_trigger_name, false)) {
			GetEntDataVector(index, CBaseEntity_vecOrigin, vecOrigin);
			
			#if defined DEBUG
				vecMins = vecOrigin;
				vecMins[2] += 512.0;
				
				TE_SetupBeamPoints(vecOrigin, vecMins, BeamModelIndex, 0, 0, 0, 10.0, 20.0, 20.0, 0, 0.0, {0, 50, 255, 255}, 0);
				TE_SendToAll();
			#endif
			
			GetEntPropVector(index, Prop_Send, "m_vecMins", vecMins);
			GetEntPropVector(index, Prop_Send, "m_vecMaxs", vecMaxs);

			vecMins[0] += vecOrigin[0];
			vecMins[1] += vecOrigin[1];
		
			vecMaxs[0] += vecOrigin[0];
			vecMaxs[1] += vecOrigin[1];
			vecMins[2] = vecMaxs[2] = vecOrigin[2];
			
			float size = (vecMaxs[0] - vecMins[0]) * (vecMaxs[1] - vecMins[1]);
			
			if(size > float(UPPER_SIZE_CLAMP)) 
				size = float(UPPER_SIZE_CLAMP);
			else if (size < float(LOWER_SIZE_CLAMP)) 
				size = float(LOWER_SIZE_CLAMP);
			
			int toGenerate = RoundFloat((size / avgPerSize) * (1.0 - (GetURandomFloat() * GetURandomFloat()))) + 1;
			
			if(size < SMALL_THRESHHOLD)
				toGenerate = RoundToCeil(float(toGenerate) / 2.0);
				
			//if(vecOrigin[2] < -8263.0)
			//	toGenerate = RoundToCeil(float(toGenerate) / CITY_DIVISOR);
			
			totalGenerated += toGenerate;
			
			if(toGenerate > UPPER_ITEM_CLAMP) toGenerate = UPPER_ITEM_CLAMP;
			
			#if defined DEBUG
				vecOrigin[2] += 32.0;
				
				//SpawnFormattedWorldText("Size: %.2f (%.1f %.1f)\nTo Generate: %i", _, vecOrigin, size, (vecMaxs[0] - vecMins[0]), (vecMaxs[1] - vecMins[1]), toGenerate);
				
				vecOrigin[2] -= 32.0;
				
				TE_SetupBeamPoints(vecMins, vecMaxs, no_z_BeamModelIndex, 0, 0, 0, 10.0, 2.0, 2.0, 0, 0.0, {0, 255, 0, 255}, 0);
				TE_SendToAll();
				
				float tmp[3];
				tmp[0] = vecMaxs[0];
				tmp[1] = vecMins[1];
				tmp[2] = vecOrigin[2];
				
				TE_SetupBeamPoints(vecMins, tmp, no_z_BeamModelIndex, 0, 0, 0, 10.0, 2.0, 2.0, 0, 0.0, {0, 255, 0, 255}, 0);
				TE_SendToAll();
				
				tmp[0] = vecMaxs[0];
				tmp[1] = vecMins[1];
				
				TE_SetupBeamPoints(tmp, vecMaxs, no_z_BeamModelIndex, 0, 0, 0, 10.0, 2.0, 2.0, 0, 0.0, {0, 255, 0, 255}, 0);
				TE_SendToAll();
			#endif
			
			while(toGenerate-- > 0) {
				vecOrigin[0] = GetRandomFloat(vecMins[0], vecMaxs[0]);
				vecOrigin[1] = GetRandomFloat(vecMins[1], vecMaxs[1]);
				
				//TraceRay Downwards
				Handle trace = TR_TraceRayEx(vecOrigin, view_as<float>({90.0, 0.0, 0.0}), MASK_ALL, RayType_Infinite);
				
				if(trace != null && TR_DidHit(trace)) {
					float endPos[3];
					TR_GetEndPosition(endPos, trace);
					
					if(192.0 > GetVectorDistance(vecOrigin, endPos) > 5.0) {
						#if defined DEBUG
							TE_SetupBeamPoints(vecOrigin, endPos, no_z_BeamModelIndex, 0, 0, 0, 10.0, 2.0, 2.0, 0, 0.0, {0, 255, 0, 255}, 0);
							TE_SendToAll();
						#endif
						
						float spawnPropability = 1.0 - (GetURandomFloat() * GetURandomFloat());
						
						//int toSpawn = GetRandomInt(0, SpawnableWeaponCount-1);
						int eligableNum = 0;
						for(int x = 0; x < sizeof(ITEM_DEFINITIONS); x++) {
							if(spawnPropability <= ITEM_DEFINITIONS[x][SpawnProbability])
								eligableNum++;
						}
						//Normally, this should NEVER be the case, but well, security.
						if(eligableNum == 0)
							continue;
						
						int toSpawn = GetRandomInt(0, eligableNum-1);
						
						for(int x = 0; x < sizeof(ITEM_DEFINITIONS); x++) {
							if(spawnPropability <= ITEM_DEFINITIONS[x][SpawnProbability]) {
								if(toSpawn > 0)
									toSpawn--;
								else {
									toSpawn = x;
									break;
								}
							}
						}
						
						Item newItem = Item().Create(vAs(Go4TK_Item_Type, toSpawn), 1, endPos);
						
						if(newItem.SuperType == ST_Weapon) {
							endPos[2] += 1.0;
							Go4TK_Item_Type weapAmmo = GetWeapTypeAmmoType(newItem.Type);
							
							if(weapAmmo != None) {
								int amountToSpawn = 60;
								if(weapAmmo == Ammo_Shotgun)
									amountToSpawn = 12;
								else if(weapAmmo == Ammo_P2000)
									amountToSpawn = 26;
									
								Item().Create(vAs(Go4TK_Item_Type, weapAmmo), amountToSpawn, endPos);
							}
						}
					}
					delete trace;
				}
			}
			
			//Kill the trigger when we are done with it to free up some entities, it will respawn in the next round.
			RemoveEdict(index);
			index = startFrom;
		}
		//Spawn stuff in batches
		count++;
		if(count >= 18) {
			CreateTimer(0.3, TimerNextTick, index);
			
			Debug("%i", totalGenerated);
			return;
		}
	}
}

Action TimerNextTick(Handle timer, int toUseStart) {
	SpawnLoot(toUseStart);
}