#define BULLET_DROP_DIVISOR 250.0 //Lower = more bulletdrop //TODO tweak (200)

#define BULLET_VEL 380.0
//#define BULLET_VEL 819.7 //Combat update
//#define BULLET_VEL 200.0 //Taken from H1 JS and ported to units
#define NEARMISS_DISTANCE 20.0
#define BULLET_MAX_TICKAHEAD 4.0

static float bullet_units_pertick;
static float bullet_drop_mult_pertick;

static ArrayList ToSimulateBullets = null;

//REDACTED//

enum SimulatedBullet {
	Float:sStartpos_X,
	Float:sStartpos_Y,
	Float:sStartpos_Z,
	Float:sLastpos_X,
	Float:sLastpos_Y,
	Float:sLastpos_Z,
	Float:sDir_X,
	Float:sDir_Y,
	Float:sDir_Z,
	Float:sCurDrop,
	sClient,
	sWeapon,
	sWeaponType,
	sBackTick,
	showTracer,
	Float:sTraveledDistance
}

enum LagCompensationType {
	LAG_COMPENSATE_BOUNDS,
	LAG_COMPENSATE_HITBOXES,
	LAG_COMPENSATE_HITBOXES_ALONG_RAY
};

static float BulStructItem[SimulatedBullet];

static char toUseNearMissSound[64];

public void HandleBulletImpact(const float pos[3], const float ang[3]) {
	//World decal for.. world brushes
	TE_Start("World Decal");
	TE_WriteVector("m_vecOrigin", pos);
	TE_WriteNum("m_nIndex", g_iBulletImpactDecal);

	TE_SendToAll();

	//For anything thats not worldbrushes, we need a projected decal
	TE_Start("Projected Decal");
	TE_WriteVector("m_vecOrigin", pos);
	TE_WriteVector("m_angRotation", ang);
	TE_WriteFloat("m_flDistance", 32.0);
	TE_WriteNum("m_nIndex", g_iBulletImpactDecal);

	TE_SendToAll();

	//Some other stuff like doors will not get an impact but oh well.

	int ent = CreateEntityByName("info_particle_system");
	if(IsValidEdict(ent)) {
		DispatchKeyValue(ent, "effect_name", "impact_carpet");
		DispatchSpawn(ent);

		SetEntDataVector(ent, CBaseEntity_vecOrigin, pos, true);
		SetEntDataVector(ent, CBaseEntity_angRotation, ang, true);

		ActivateEntity(ent);
		AcceptEntityInput(ent, "start");

		SetVariantString("OnUser1 !self:kill::0.6:1");
		AcceptEntityInput(ent, "AddOutput");
		AcceptEntityInput(ent, "FireUser1");
	}

	char toPlayHitSound[64];

	FormatEx(toPlayHitSound, sizeof(toPlayHitSound), "physics/concrete/concrete_impact_bullet%i.wav", GetRandomInt(1, 4));

	EmitAmbientSound(toPlayHitSound, pos);
}

static float startVec[3];
static float endVec[3];
static int hGroup, targetEnt, weaponType, attacker, weapon;
static float clientPositions[MAXPLAYERS+1][3];

stock void ProcessBullets(int bulletToProcess = -1, int timesToProcess = 1, int attackerToProcess = -1) {
	if(ToSimulateBullets == null)
		return;

	LoopIngameClients(i)
		GetClientEyePosition(i, clientPositions[i]);
		//GetEntDataVector(i, CBaseEntity_vecOrigin, clientPositions[i]);

	int index = bulletToProcess != -1 ?
					bulletToProcess :
					ToSimulateBullets.Length - 1;

	Handle tr;

	static bool finishLagcompensation = false;

	while(index >= 0) {
		//No idea how that could happen.
		if(index > ToSimulateBullets.Length - 1) {
			index = ToSimulateBullets.Length - 1;

			if(index < 0)
				break;
		}

		ToSimulateBullets.GetArray(index, BulStructItem[0]);

		//Is passed w/ native lag compensation method because when lag compensating, the function needs the user's UsrCmd to be set,
		//which is only the case in the pre-think (cleared post-think) so we need to process the bullets per-user instead of processing
		//all of them in the game tick
		if(attackerToProcess != -1 && BulStructItem[sClient] != attackerToProcess)
			timesToProcess = 0;

		for(; timesToProcess > 0; timesToProcess--) {
			if(attackerToProcess != -1 && !finishLagcompensation) {
				finishLagcompensation = true;

				/*#if defined VERBOSE
					Debug("CALLING STARTLAGCOMPENSATION!");
				#endif
				SDKCall(g_hStartLagCompensationCall, g_hCLagCompensationManagerInstance, attackerToProcess, LAG_COMPENSATE_HITBOXES, NULL_VECTOR, NULL_VECTOR, 32000.0);
				#if defined VERBOSE
					Debug("CALLED!");
				#endif*/
			}

			startVec[0] = BulStructItem[sLastpos_X];
			startVec[1] = BulStructItem[sLastpos_Y];
			startVec[2] = BulStructItem[sLastpos_Z];

			endVec[0] = BulStructItem[sDir_X];
			endVec[1] = BulStructItem[sDir_Y];
			endVec[2] = BulStructItem[sDir_Z];

			float Origin2[3];

			#if defined DEBUG && defined VERBOSE
				Origin2[0] = startVec[0];
				Origin2[1] = startVec[1];
				Origin2[2] = startVec[2] + 32.0;

				TE_SetupBeamPoints(startVec, Origin2, BeamModelIndex, 0, 0, 0, 15.0, 2.0, 1.0, 0, 0.0, {255, 0, 0, 255}, 0);
				TE_SendToAll();
			#endif

			//Scale the (normalized) direction vector by the velocity to get the current ticks VelocityScale
			ScaleVector(endVec, bullet_units_pertick);
			//Add the calculated velocity delta to the last point
			AddVectors(endVec, startVec, endVec);

			//Deduct the bullets gravity off the endpoints Z axis
			endVec[2] -= BulStructItem[sCurDrop];

			//Debug("Drop this tick: %.4f", BulStructItem[sCurDrop]);

			//before casting the ray lagcompensate (backdate) any players which are within BULLET_VEL*3 units of startVec by the amount of the attackers latency in additon
			//to the already in-place ahead-start of the bullet

			attacker = BulStructItem[sClient];

			if(USE_ARBITRARY_LAGCOMPENSATION) {
				LoopIngameClients(i) if(i != attacker && g_bPlayerAliveCache[i]) {
					if(GetVectorDistance(clientPositions[i], startVec) <= bullet_units_pertick + (BulStructItem[sBackTick] * 10.0)) { //Should be plenty room of error on any tickrate
						#if defined DEBUG && defined VERBOSE
							static float backTickedPos2[3];
							backTickedPos2 = clientPositions[i];
							backTickedPos2[2] -= 72.0;


							TE_SetupBeamPoints(clientPositions[i], backTickedPos2, BeamModelIndex, 0, 0, 0, 10.0, 1.0, 1.0, 0, 0.0, {255, 0, 0, 255}, 0);
							TE_SendToAll();
						#endif

						StartLagcompForPlayer(i, BulStructItem[sBackTick]);
						GetClientEyePosition(i, clientPositions[i]);

						#if defined DEBUG && defined VERBOSE
							backTickedPos2 = clientPositions[i];
							backTickedPos2[2] -= 72.0;

							TE_SetupBeamPoints(clientPositions[i], backTickedPos2, BeamModelIndex, 0, 0, 0, 10.0, 1.0, 1.0, 0, 0.0, {0, 0, 25, 255}, 0);
							TE_SendToAll();
						#endif
					}
				}
			} else {
				//if(attackerToProcess == -1) {
				static float Ang[3];
				Ang[0] = BulStructItem[sDir_X];
				Ang[1] = BulStructItem[sDir_Y];
				Ang[2] = BulStructItem[sDir_Z];
				#if defined VERBOSE
					Debug("CALLING STARTLAGCOMPENSATION!");
				#endif
				SDKCall(g_hStartLagCompensationCall, g_hCLagCompensationManagerInstance, attacker, LAG_COMPENSATE_HITBOXES_ALONG_RAY, startVec, Ang, bullet_units_pertick);
				#if defined VERBOSE
					Debug("CALLED!");
				#endif
				//}
			}

			tr = TR_TraceRayFilterEx(startVec, endVec, MASK_SHOT, RayType_EndPoint, TRDontHitSelfAndBreakBreakables, BulStructItem[sClient]);

			if(USE_ARBITRARY_LAGCOMPENSATION) {
				LoopIngameClients(i)
					if(i != attacker)
						ResetLagcompForPlayer(i);
			} else {
				//if(attackerToProcess == -1) {
				#if defined VERBOSE
					Debug("CALLING FINISHLAGCOMPENSATION!");
				#endif
				SDKCall(g_hFinishLagCompensationCall, g_hCLagCompensationManagerInstance_FinishCall, attacker);
				#if defined VERBOSE
					Debug("CALLED!");
				#endif
				//}
			}

			//Doesnt matter if it hit or nah, lets show the bullet tracer
			//This looks only semi-good on 64 tick because simulation code is not tickrate relieant, this however is.
			#if !defined DEBUG && defined VERBOSE
				if(BulStructItem[showTracer] == 1) {
					TE_SetupBeamPoints(startVec, endVec, BeamModelIndex, 0, 0, 0, 0.1, 0.05, 0.1, 0, 0.0, {255, 255, 100, 150}, 0);
					TE_SendToAll();
				}
			#endif

			#if defined DEBUG && defined VERBOSE
				static int clr[4];
				clr[0] = GetRandomInt(50, 190);
				clr[1] = GetRandomInt(20, 120);
				clr[2] = GetRandomInt(50, 100);
				clr[3] = 80;

				if(bulletToProcess != -1) {
					clr[0] = 0;
					clr[1] = 255;
					clr[2] = 255;
					clr[3] = 255;
				}
			#endif

			if (!TR_DidHit(tr)) {
				delete tr;

				#if defined DEBUG && defined VERBOSE
					TE_SetupBeamPoints(startVec, endVec, BeamModelIndex, 0, 0, 0, 15.0, 2.0, 0.5, 0, 0.0, clr, 0);
					TE_SendToAll();
				#endif

				LoopIngameClients(i) if(i != attacker && !IsFakeClient(i)) {
					GetVectorClosestPointToLine(clientPositions[i], startVec, endVec, startVec);

					if(GetVectorDistance(clientPositions[i], startVec) <= NEARMISS_DISTANCE)
						EmitSoundToClient(i, toUseNearMissSound, _, _, _, _, 0.3, _, _, startVec);

					#if defined DEBUG && defined VERBOSE
						if(GetVectorDistance(clientPositions[i], startVec) <= NEARMISS_DISTANCE) {
							TE_SetupBeamPoints(clientPositions[i], startVec, BeamModelIndex, 0, 0, 0, 15.0, 0.2, 0.2, 0, 0.0, {0, 255, 0, 255}, 0);
							TE_SendToAll();
						} else {
							//TE_SetupBeamPoints(clientPositions[i], startVec, BeamModelIndex, 0, 0, 0, 15.0, 0.2, 0.2, 0, 0.0, {255, 255, 0, 255}, 0);
						}
					#endif
				}

				BulStructItem[sTraveledDistance] += bullet_units_pertick;

				if(BulStructItem[sWeaponType] == WEAPON_NOVA && BulStructItem[sTraveledDistance] > 3672.0) { //3672 = 18 bulletticks
					//Bullettravel surpassed max distance
					ToSimulateBullets.Erase(index);
					timesToProcess = 0;
				} else {
					int inTpTrigger = CheckVectorIsInTpTrigger(endVec);

					//Check if endpoint of current tick intersects with a TP trigger
					if(inTpTrigger != -1) { //The bullet ended up inside an edge-teleport trigger
						//Move the STARTpoint of the current simtick in relation to the tpdest
						MoveVectorByTpTrigger(startVec, inTpTrigger);

						//Set the ENDpoint to be the startpoint of the current simtick, but teleported into the respective other layer
						BulStructItem[sLastpos_X] = startVec[0];
						BulStructItem[sLastpos_Y] = startVec[1];
						BulStructItem[sLastpos_Z] = startVec[2];

						//Subtract the traveled distance from this tick because all we do is re-simulate the bullet in the other layer, thus it didnt actually travel any further
						BulStructItem[sTraveledDistance] -= bullet_units_pertick;

						//PrintToChatAll("We teleported the bullet in respect to trigger #%i and will re-simulate it in that layer", inTpTrigger);
					} else {
						//Since we are going to re-simulate the same lifetime-tick,
						//but in the respective other layer we dont want to modify the drop
						BulStructItem[sLastpos_X] = endVec[0];
						BulStructItem[sLastpos_Y] = endVec[1];
						BulStructItem[sLastpos_Z] = endVec[2];

						BulStructItem[sCurDrop] *= bullet_drop_mult_pertick;

						if(BulStructItem[sWeaponType] == WEAPON_NOVA)
							BulStructItem[sCurDrop] *= bullet_drop_mult_pertick;
					}

					ToSimulateBullets.SetArray(index, BulStructItem[0]);

					//The bullet was teleported for re-simulation, so lets do it.
					if(inTpTrigger != -1)
						ProcessBullets(bulletToProcess, 1, attackerToProcess); //We dont need to pass the attacker, since we are already in the correct stack, but well ¯\_(ツ)_/¯
				}
			} else {
				hGroup = TR_GetHitGroup(tr);
				targetEnt = TR_GetEntityIndex(tr);

				weaponType = BulStructItem[sWeaponType];
				weapon = BulStructItem[sWeapon];

				TR_GetEndPosition(endVec, tr);

				///////
				if(IsValidClient(attacker)) {
					int Color[4] = {255, 255, 0, 150};

					float OriginX[3];
					OriginX[0] = endVec[0];
					OriginX[1] = endVec[1];
					OriginX[2] = endVec[2] + 10.0;

					int toSendToC[MAXPLAYERS+1]; int x = 0;
					DoForEverySpectatingClientAndSelf(attacker, toSendTo)
						if(!userSettings[toSendTo][CFG_hasDisabledBulletMarkers])
							toSendToC[x++] = toSendTo;

					TE_SetupBeamPoints(endVec, OriginX, BeamModelIndex, 0, 0, 0, 1.0, 0.1, 6.0, 1, 0.0, Color, 5);
					TE_Send(toSendToC, x);

					OriginX[2] = endVec[2] - 10.0;

					TE_SetupBeamPoints(endVec, OriginX, BeamModelIndex, 0, 0, 0, 1.0, 0.1, 4.0, 1, 0.0, Color, 5);
					TE_Send(toSendToC, x);
				}
				///////


				#if defined DEBUG && defined VERBOSE
					TE_SetupBeamPoints(startVec, endVec, BeamModelIndex, 0, 0, 0, 15.0, 2.0, 0.5, 0, 0.0, clr, 0);
					TE_SendToAll();
					//TE_SetupBeamPoints(endVec, Origin2, BeamModelIndex, 0, 0, 0, 15.0, 0.1, 20.0, 0, 0.0, {255, 0, 0, 255}, 0);
					//TE_SendToAll();

					Debug("Hitgroup: %i, Ent: %i, pos: %.2f %.2f %.2f", hGroup, targetEnt, endVec[0], endVec[1], endVec[2]);
				#endif

				if(targetEnt == -1 || targetEnt > 64 || !IsValidClient(targetEnt)) {
					//shot did not land in a player but in sth different, spawn bullet decal

					MakeVectorFromPoints(startVec, endVec, Origin2);
					//NegateVector(Origin2);
					GetVectorAngles(Origin2, Origin2);

					HandleBulletImpact(endVec, Origin2);

					//Doesnt work for props
					//TE_SetupWorldDecal(endVec, bullet_decal_hole);
				} else {
					//Shot landed in a client, simulate hit
					BulStructItem[sTraveledDistance] += GetVectorDistance(startVec, endVec);

					//WARNING we are not doing this because bullets that wrapped-around could show a wrong damage origin when damaging the player below,
					//instead we will use the bullets last position so it shows the correct..er origin.
					//startVec[0] = BulStructItem[sStartpos_X];
					//startVec[1] = BulStructItem[sStartpos_Y];
					//startVec[2] = BulStructItem[sStartpos_Z];

					//Default fallback (m4a4 damage)
					float weaponBaseDamage = 27.0;
					float rangeModifier = 0.98;

					if(weaponType == WEAPON_AK47) {
						weaponBaseDamage = 35.0;
						rangeModifier = 0.98;
					} else if(weaponType == WEAPON_HKP2000) {
						weaponBaseDamage = 25.0;
						rangeModifier = 0.75;
					} else if(weaponType == WEAPON_NOVA) {
						weaponBaseDamage = 23.0;
						rangeModifier = 0.45;
					} else if(weaponType == WEAPON_SSG08) {
						weaponBaseDamage = 80.0;
						rangeModifier = 0.99;
					}

					//Calc DMG Damage * RangeModifier^(range/500)
					float calculatedDamage = weaponBaseDamage * Pow(rangeModifier, BulStructItem[sTraveledDistance] / 500.0);

					//Stock damage modifiers
					if(hGroup == HITGROUP_HEAD) {
						calculatedDamage *= 4.0;
					} else if(hGroup == HITGROUP_STOMACH) {
						calculatedDamage *= 1.25;
					} else if(hGroup == HITGROUP_LEFTLEG || hGroup == HITGROUP_RIGHTLEG) {
						calculatedDamage *= 0.75;
					}

					//Do modifications and events such as helmet popping etc.
					char toPlaySound[64] = "\0";

					if(hGroup == HITGROUP_HEAD) {
						Item nItem = PlayerInv(targetEnt, Head);

						if(nItem.IsValid(true)) {
							nItem.Durability -= RoundFloat(calculatedDamage);

							calculatedDamage /= 8.0;

							toPlaySound = "player/bhit_helmet-1.wav";

						}
					} else if(hGroup == HITGROUP_CHEST || hGroup == HITGROUP_STOMACH) {
						Item nItem = PlayerInv(targetEnt, Armor);

						if(nItem.IsValid(true)) {
							nItem.Durability -= RoundFloat(calculatedDamage);

							if(nItem.Durability <= 0) {
								toPlaySound = "physics/plastic/plastic_box_break2.wav";
							} else {
								FormatEx(toPlaySound, sizeof(toPlaySound), "physics/metal/metal_solid_impact_bullet%i.wav", GetRandomInt(1, 4));
							}

							calculatedDamage /= nItem.Type == Laminated_Vest ? 2.5 : 2.0;
						}
					} else if(hGroup == HITGROUP_LEFTARM || hGroup == HITGROUP_RIGHTARM || hGroup == HITGROUP_LEFTLEG || hGroup == HITGROUP_RIGHTLEG) {
						calculatedDamage /= 2.0;
					}

					if(toPlaySound[0] == '\0')
						FormatEx(toPlaySound, sizeof(toPlaySound), "physics/flesh/flesh_impact_bullet%i.wav", GetRandomInt(1, 5));

					//EmitSoundToClient(targetEnt, toPlaySound, _, _, _, _, 0.3);
					EmitAmbientSound(toPlaySound, endVec, _, SNDLEVEL_HOME);

					if(IsValidClient(attacker))
						DoForEverySpectatingClientAndSelf(attacker, toSendTo)
							EmitSoundToClient(toSendTo, toPlaySound, _, _, _, _, 0.5);

					//REDACTED//


					int intDmg = RoundToFloor(calculatedDamage);

					//Calculate shot velocity for takedamage

					endVec[0] = BulStructItem[sDir_X];
					endVec[1] = BulStructItem[sDir_Y];
					endVec[2] = BulStructItem[sDir_Z];

					//Scale the (normalized) direction vector by the velocity to get the current ticks VelocityScale
					ScaleVector(endVec, calculatedDamage);

					Debug("Calculated Damage dealt: %.2f (Range: %.2f, hGroup: %i, head: %b)", calculatedDamage, BulStructItem[sTraveledDistance], hGroup, hGroup == HITGROUP_HEAD);

					//SetEntData(targetEnt, CCSPlayer_bGunGameImmunity, 0, 1, true);

					#if !defined DEBUGWEAPONS
						HM_OnTakeDamage(targetEnt, intDmg, attacker);

						Teaming_DamageDealt(attacker, targetEnt, intDmg);

						SDKHooks_TakeDamage(targetEnt,
											(weapon != -1) ? weapon : attacker,
											attacker,
											calculatedDamage,
											(DMG_BULLET|DMG_ALWAYSGIB) + (hGroup == 1 ? CS_DMG_HEADSHOT : 0),
											weapon,
											endVec,
											startVec);
					#else
						PrintToChat(attacker, "Calculated Damage: %.2f (Range: %.2f, hGroup: %i, head: %b)", calculatedDamage, BulStructItem[sTraveledDistance], hGroup, hGroup == HITGROUP_HEAD);
					#endif

					//SetEntData(targetEnt, CCSPlayer_bGunGameImmunity, 1, 1, true);
				}

				ToSimulateBullets.Erase(index);
				timesToProcess = 0;

				delete tr;
			}

		} // for(; timesToProcess > 0; timesToProcess--) {

		if(bulletToProcess != -1)
			break;
		else
			timesToProcess = 1;

		index--;
	} //while(index >= 0) {

	if(finishLagcompensation) {
		finishLagcompensation = false;

		/*#if defined VERBOSE
			Debug("CALLING FINISHLAGCOMPENSATION!");
		#endif
		SDKCall(g_hFinishLagCompensationCall, g_hCLagCompensationManagerInstance_FinishCall, attackerToProcess);
		#if defined VERBOSE
			Debug("CALLED!");
		#endif*/
	}
}

stock void InitBulletSimStruct() {
	float x[SimulatedBullet];

	if(ToSimulateBullets == null)
		ToSimulateBullets = new ArrayList(sizeof(x));
	else
		ToSimulateBullets.Clear();

	bullet_units_pertick = BULLET_VEL / tickCorrector;
	PrintToServer("Bulletspeed per Tick: %f", bullet_units_pertick);

	bullet_drop_mult_pertick = 1 + ((BULLET_VEL / BULLET_DROP_DIVISOR / 800) / tickCorrector);
}

stock void AddSimulatedBullet(int client, const float vecStartPos[3], const float vecDir[3]) {
	if(client > 64 || client < 0 || !IsValidClient(client))
		return;

	BulStructItem[sClient] = client;
	BulStructItem[sWeapon] = GetEntDataEnt2(client, CBasePlayer_hActiveWeapon);
	BulStructItem[sWeaponType] = GetEntData(BulStructItem[sWeapon], CWeaponCSBaseGun_iItemDefinitionIndex);

	//Traffic going OUT TO the client, so the delay he has on player positions
	if(USE_ARBITRARY_LAGCOMPENSATION) {
		BulStructItem[sBackTick] = RoundToFloor(GetClientAvgLatency(client, NetFlow_Outgoing) / tickMs);

		if(BulStructItem[sBackTick] > LAGCOMP_SNAPSHOT_BACKLOG)
			BulStructItem[sBackTick] = RoundFloat(LAGCOMP_SNAPSHOT_BACKLOG);
	}

	//else
	//	BulStructItem[sBackTick] = GetGameTickCount() - BulStructItem[sBackTick];

	BulStructItem[sCurDrop] = (BULLET_VEL / BULLET_DROP_DIVISOR) / tickCorrector;

	BulStructItem[sLastpos_X] = BulStructItem[sStartpos_X] = vecStartPos[0];
	BulStructItem[sLastpos_Y] = BulStructItem[sStartpos_Y] = vecStartPos[1];
	BulStructItem[sLastpos_Z] = BulStructItem[sStartpos_Z] = vecStartPos[2];

	BulStructItem[sDir_X] = vecDir[0];
	BulStructItem[sDir_Y] = vecDir[1];
	BulStructItem[sDir_Z] = vecDir[2];

	BulStructItem[sTraveledDistance] = 0.0;

	BulStructItem[showTracer] = (GetRandomInt(0, 2) == 0) ? 1 : 0;

	//int addedBullet =
	ToSimulateBullets.PushArray(BulStructItem[0]);

	//We need to give the bullet a headstart based off the players incoming delay TO US + 1 to make it move at least one tick right when added
	//because with native bullet sim, the bullet travels instantly where aimed, so we only need to lag-compensate the other players back by the
	//amount the shooter sees them delayed. But with a traveling bullet,

	//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ WRONG!!!!
	//We have 50ms delay to client, and 50ms from him. Client shoots, it takes 50ms to get to us (irrelevant) because we will then backtick any
	//players that he could possibly hit to the position where HE SAW them on his send when HE shot. Giving the bullet a headstart of this OUTGOING latency
	//would be double-compensating / be a viable workaround if we didnt have compensation for other players.

	//WORKAROUND headstart disabled for now as it would cause player twiching when used in combination w/ native lagcomp
	//#define headStart 1

	//int headStart = 1 + RoundToFloor(GetClientAvgLatency(client, NetFlow_Incoming) / tickMs);

	//Limit the headstart to a sane value to prevent abuse (7 * tickCorrector, so ~85 MS)
	//if(headStart > BULLET_MAX_TICKAHEAD * tickCorrector)
	//	headStart = RoundToFloor(BULLET_MAX_TICKAHEAD * tickCorrector);

	//ProcessBullets(addedBullet, headStart, client);

	//Debug("Bullet had a headstart of %i ticks", headStart);

	//Re-Randomize the cached nearmiss sound
	FormatEx(toUseNearMissSound, sizeof(toUseNearMissSound), ">weapons/fx/nearmiss/bulletltor0%i.wav", GetRandomInt(6, 9));
}

public bool TRDontHitSelfAndBreakBreakables(int entity, int mask, any data) {
	if(entity == data)
		return false;

	GetEntityClassname(entity, sumTmpStr, sizeof(sumTmpStr));

	//If the simulated bullet passes trough a breakable, break it.
	bool isBreakable = StrEqual(sumTmpStr, "func_breakable");

	if(isBreakable)
		AcceptEntityInput(entity, "Break");
	else if(entity > 64 && IsValidEdict(entity) && StrEqual(sumTmpStr, "prop_dynamic")) {
		//shot did not land in a player but in sth different, spawn bullet decal
		GetEntPropString(entity, Prop_Data, "m_iName", sumTmpStr, sizeof(sumTmpStr));

		if(StrEqual(sumTmpStr, "breakable_fence")) {
			SetVariantInt(1);
			AcceptEntityInput(entity, "RemoveHealth");

			GetEntPropString(entity, Prop_Data, "m_ModelName", sumTmpStr, sizeof(sumTmpStr));

			//"Mesh" fence in military base
			if(StrContains(sumTmpStr, "fence03a.mdl", false) != -1) {
				//We want bullets to go straight trough it.
				return false;
			}
		}
	}

	return !isBreakable;
}