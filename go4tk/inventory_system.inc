ArrayList Go4TK_Items = null;
ArrayList Go4TK_Arbitrary_Dropped = null;

enum Go4TK_Item_Type {
	None, Binoculars, Map, 
	Duct_Tape, 
	AK,
	M4,
	Shotgun,
	Scout,
	P2000,
	Helmet,
	Laminated_Vest,
	Makeshift_Armor,
	Bandage,
	Medikit,
	Composite_Fabric,
	Fabric,
	Armor_Scrap,
	Procoagulant,
	Backpack,
	Military_Backpack,
	Ammo_AK,
	Ammo_M4,
	Ammo_Shotgun,
	Ammo_Scout,
	Ammo_P2000,
	Hegrenade,
	Flashbang,
	Smokegrenade,
	Molotov,
	Yeezys
}

enum Go4TK_Item_Supertypes {
	ST_Item,
	ST_Armor,
	ST_Head,
	ST_Storage,
	ST_Utility,
	ST_Weapon,
	ST_Ammo
}

enum Go4TK_Item_Definition {
	Weight,
	bool:Equippable,
	Shreddable,
	Useable,
	Craftable,
	Float:SpawnProbability,
	Go4TK_Item_Supertypes:SuperType,
	bool:Stackable,
	DefaultDurability
}

enum Go4TK_Item_Info {
	Name,
	Type,
	WorldModel,
	WearableModel,
	DropSequence
}

int ITEM_DEFINITIONS[Go4TK_Item_Type][Go4TK_Item_Definition];
int ITEM_SHREDDABLES[Go4TK_Item_Type][Go4TK_Item_Type];
int ITEM_CRAFTABLES[Go4TK_Item_Type][Go4TK_Item_Type];
int DROPPED_ITEM_ITEMS[2048] = {-1, ...};

char ITEM_INFOS[Go4TK_Item_Type][Go4TK_Item_Info][64];
//int Go4TK_NUM_ITEM_TYPES = 0;
float ITEM_DROP_ROTATION[Go4TK_Item_Type][3];
float ITEM_DROP_OFFSET[Go4TK_Item_Type][3];
float DROPPED_ITEM_POSITIONS[2048][3];

//Item INVALID_GO4TK_ITEM = view_as<Item>(-1);

#define LoopItemDefinitions(%1) for(int %1=1; %1 < sizeof(ITEM_SHREDDABLES); %1++)

#define BODYBAG_FAKEID -2

#define INVALID_GO4TK_ITEM Item(-1)
#define INV_BASE_SIZE 200
#define NORMAL_BACKPACK_SIZE 1000
#define MILITARY_BACKPACK_SIZE 2000
#define INVALID_BOUNDENT_INDEX 4096

enum Go4TK_Item {
	Float:DropPos_X,
	Float:DropPos_Y,
	Float:DropPos_Z,
	Go4TK_Item_Type:Type,
	Amount,
	Bound_ent,
	CurrentOwner,
	Go4TK_Inventory_Slots:CurrentOwnerSlot,
	Durability
}

enum Go4TK_Inventory_Slots {
	SL_None,
	Item:Head,
	Item:Armor,
	Item:Storage,
	Item:Weapon1,
	Item:Weapon2,
	Item:Weapon3,
	Item:Boots,
	ItemList:Carrying
}

int PLAYER_INV[MAXPLAYERS+1][Go4TK_Inventory_Slots];
int PLAYER_DROPPED_ITEMS[MAXPLAYERS+1] = {0, ...};

#define PlayerInv(%1,%2) PLAYER_INV[%1][%2]

int dmap_data_item[Go4TK_Item];
static int dmap_curr_index = -1;
static bool dmap_change_pending = false;

//int loads = 0;
//int saves = 0;

methodmap Item {
	public Item(int arrIndex = -1) {
		if(Go4TK_Items == null)
			return view_as<Item>(-1);
	
		if(arrIndex >= Go4TK_Items.Length || arrIndex < -1) arrIndex = -1;
	
		//As were about to cache a different, new item, re-save into  the array
		if(dmap_change_pending && dmap_curr_index != arrIndex && dmap_curr_index != -1) {
			Go4TK_Items.SetArray(dmap_curr_index, dmap_data_item[0]);
			dmap_change_pending = false;
			//saves++;
			//Debug("[INV] [i] Resaving %i as about to load %i", dmap_curr_index, arrIndex);
		}
		
		//TODO breaks caching mechanism, possibly accept overhead and ALWAYS invalidate cache on load of -1 (INVALID_GO4TK_ITEM) ?
		//dmap_data_item[Type] = None;
		//dmap_data_item[Amount] = 0;
		//dmap_data_item[Bound_ent] = -1;
		//dmap_data_item[CurrentOwner] = -1;
	
		if(arrIndex != -1 && dmap_curr_index != arrIndex) {
			//Debug("[INV] [i] Loading %i as it differs from cached %i", arrIndex, dmap_curr_index);
			
			dmap_curr_index = arrIndex;
			Go4TK_Items.GetArray(dmap_curr_index, dmap_data_item[0]);
			
			//loads++;
		}
		return view_as<Item>(arrIndex);
	}
	
	property int id {
		public get() {
			return AsInt(this);
		}
	}
	
	public void Refresh() {
		if(this.id != dmap_curr_index)
			Item(this.id);
	}
	
	/**
	 * If the item is currently dropped get the props EntityId its associated to
	 */	
	property int BoundEnt {
		public get() {
			this.Refresh();
			if(dmap_data_item[Bound_ent] == INVALID_ENT_REFERENCE) return INVALID_ENT_REFERENCE;
			return EntRefToEntIndex(dmap_data_item[Bound_ent]);
		}
	}
	
	/**
	 * If the item is currently dropped move it into "limbo" (Not dropped and not owned by anyone)
	 */	
	public void RemoveWorldItem() {
		this.Refresh();
		if(dmap_data_item[Bound_ent] == INVALID_ENT_REFERENCE || dmap_data_item[Bound_ent] == INVALID_BOUNDENT_INDEX) {
			int aPos = Go4TK_Arbitrary_Dropped.FindValue(this.id);
			
			if(aPos != -1) {
				Go4TK_Arbitrary_Dropped.Erase(aPos);
				
				dmap_change_pending = true;
				dmap_data_item[Bound_ent] = INVALID_ENT_REFERENCE;
			}
			return;
		}
		#if defined VERBOSE
			Debug("[INV] WorldEnt: %i, isvalid: %i", dmap_data_item[Bound_ent], IsValidEdict(dmap_data_item[Bound_ent]));
		#endif
		
		int wIndex = EntRefToEntIndex(dmap_data_item[Bound_ent]);
		
		if(IsValidEdict(wIndex)) {
			RemoveEdict(wIndex);
			dmap_change_pending = true;
			DROPPED_ITEM_POSITIONS[wIndex] = NULL_VECTOR;
			DROPPED_ITEM_ITEMS[wIndex] = -1;
			dmap_data_item[Bound_ent] = INVALID_ENT_REFERENCE;
		}
	}
	
	property Go4TK_Item_Type Type {
		public get() {
			this.Refresh(); return dmap_data_item[Type];
		}
	}
	
	property bool Stackable {
		public get() {
			this.Refresh(); return ITEM_DEFINITIONS[this.Type][Stackable];
		}
	}
	
	property Go4TK_Item_Supertypes SuperType {
		public get() {
			this.Refresh(); return ITEM_DEFINITIONS[this.Type][SuperType];
		}
	}
	
	property bool IsShreddable {
		public get() {
			this.Refresh(); return ITEM_DEFINITIONS[this.Type][Shreddable] != -1;
		}
	}
	
	property bool IsUseable {
		public get() {
			this.Refresh(); return ITEM_DEFINITIONS[this.Type][Useable] != -1;
		}
	}
	
	property bool IsEquippable {
		public get() {
			this.Refresh(); return ITEM_DEFINITIONS[this.Type][Equippable];
		}
	}
	
	property int TypeWeight {
		public get() {
			this.Refresh(); return ITEM_DEFINITIONS[this.Type][Weight];
		}
	}
	
	/**
	 * Get or Set who currently owns this item
	 */	
	property int CurrentOwner {
		public get() {
			this.Refresh(); return dmap_data_item[CurrentOwner];
		}
		//Use with caution!
		public set(int newOwner) {
			this.Refresh();
			int prevOwner = dmap_data_item[CurrentOwner];
			
			if(prevOwner != newOwner) {
				ItemOwnerChangedPre(this, prevOwner, newOwner);
				this.Refresh();
			
				dmap_change_pending = true;
				dmap_data_item[CurrentOwner] = newOwner;
				
				if(newOwner != -1) this.RemoveWorldItem();
				
				ItemOwnerChanged(this, prevOwner, newOwner);
			}
		}
	}
	
	/**
	 * Get or Set whatever slot its owner has the item in
	 */	
	property Go4TK_Inventory_Slots CurrentOwnerSlot {
		public get() {
			this.Refresh(); return dmap_data_item[CurrentOwnerSlot];
		}
		//Use with caution!
		public set(Go4TK_Inventory_Slots newOwnerSlot) {
			this.Refresh();
			
			Go4TK_Inventory_Slots prevOwnerSlot = dmap_data_item[CurrentOwnerSlot];
			
			if(prevOwnerSlot != newOwnerSlot) {
				dmap_change_pending = true;
				dmap_data_item[CurrentOwnerSlot] = newOwnerSlot;
			}
			//Always call event because..reasons
			if(this.Stackable || (dmap_data_item[Amount] > 0 || dmap_change_pending))
				ItemSlotChangedPost(this, newOwnerSlot, prevOwnerSlot);
		}
	}
	
	/**
	 * Get or Set the amount in this Item Stack
	 */
	property int Amount {
		public get() {
			this.Refresh(); return dmap_data_item[Amount];
		}
		public set(int newAmount) {
			this.Refresh();
			if(newAmount <= 0)
				newAmount = 0;
			
			int prevAmount = dmap_data_item[Amount];
			
			if(prevAmount != newAmount) {
				dmap_change_pending = true;
				dmap_data_item[Amount] = newAmount;
				
				ItemAmountChanged(this, prevAmount, newAmount);
			}
			
			if(newAmount == 0)
				this.RemoveWorldItem();
			
			//Item is unstackable. It is now a ded index.
			if(!this.Stackable && newAmount == 0) {
				dmap_change_pending = true;
				this.CurrentOwnerSlot = SL_None;
			}
		}
	}
	
	/**
	 * Get the weight of this item Stack (Weight of type * amount in Stack)
	 */
	property int TotalWeight {
		public get() {
			this.Refresh(); return this.TypeWeight * this.Amount;
		}
	}
	
	/**
	 * Check if this Item is valid and the Stack is not empty
	 *
	 * @param ignoreEmpty			Ignore if this item is empty, only check if it is there
	 * @return						Item is Valid
	 */
	public bool IsValid(bool ignoreEmpty = false) {
		//Checking the ID is possible without loading the Item's data from the array, because that is what is passed.
		if(this.id == -1) return false;
		if(ignoreEmpty) return true;
		
		this.Refresh();
		
		return this.Amount != 0;
	}
	
	/**
	 * Check if this item is within reach of a given origin
	 *
	 * @param client				Origin to check
	 * @return						Origin is within reach of the Item
	 */
	public bool IsInReach(const float pos[3]) {
		this.Refresh();
		if(this.CurrentOwner == -1 && this.BoundEnt != INVALID_ENT_REFERENCE) {
			float compVec[3];
			if(this.BoundEnt != INVALID_BOUNDENT_INDEX) {
				int eIdx = EntRefToEntIndex(this.BoundEnt);
				compVec = DROPPED_ITEM_POSITIONS[eIdx];
			} else {
				compVec[0] = dmap_data_item[DropPos_X];
				compVec[1] = dmap_data_item[DropPos_Y];
				compVec[2] = dmap_data_item[DropPos_Z];
			}
			
			return GetVectorDistance(pos, compVec) <= MAX_PICKUP_DISTANCE;
		}
		return false;
	}
	
	/**
	 * Check if this item is within reach to a given player and if he could technically pick it up
	 *
	 * @param client				Client ID of the user to check
	 * @return						User is within reach of the Item and can pick it up
	 */
	public bool IsInClientReach(int client) {
		this.Refresh();
		if(this.CurrentOwner == client)
			return true;
		
		if(this.CurrentOwner == -1) {
			#if defined VERBOSE
				Debug("NEW ITEM CURRENT OWNER: %i ENT: %i", this.CurrentOwner, this.BoundEnt);
			#endif
			
			//If its neither dropped, nor owned its likely just been created, and given to a player
			if(this.BoundEnt == INVALID_ENT_REFERENCE)
				return true;
		
			float m_vecOrigin[3];
			GetClientEyePosition(client, m_vecOrigin);
			
			return this.IsInReach(m_vecOrigin);
		}
		return false;
	}
	
	/**
	 * Check if the item can move to a given slot from wherever it currently is
	 *
	 * @param newSlot				Go4TK_Inventory_Slots where we want to check if it can move to
	 * @return						Item would be able to move to the given slot
	 */
	public bool CanMoveToSlot(Go4TK_Inventory_Slots newSlot) {
		this.Refresh();
		return ItemSlotChangedPre(this, newSlot, this.CurrentOwnerSlot);
	}
	
	/**
	 * Drop this item
	 *
	 * @param dropPos				Origin to drop this item in
	 * @param enforceArbitraryDrop	True if you do not want to drop a visibible item (Only visibible in the inventorys proximity field)
	 * @return						Item was dropped successfully
	 */
	public bool Drop(float dropPos[3], bool enforceArbitraryDrop = false) {
		this.Refresh();
		int wIndex = this.BoundEnt;
		
		if(this.IsValid() && wIndex == INVALID_ENT_REFERENCE) {
			#if defined VERBOSE
				Debug("[INV] Attempting to drop item...");
			#endif
			
			dmap_change_pending = true;
			
			//We CANNOT risk to physically drop this item if theres 1980 or more entities (2048 hardlimit). 
			//We will drop it and write it into the arbitrarily dropped list but not create an entity to prevent a server crash.
			
			if(!enforceArbitraryDrop && g_CurrentEntityCount <= 1980) {
				//Need to use a prop_physics_override because for some reason a non-solid dynamic prop is ignored by rays.
				wIndex = CreateEntityByName("prop_physics_override");
				//Make sure that we actually can physically drop this, otherwise fallback to arbitrary drop method.
				if(!IsValidEdict(wIndex)) {
					enforceArbitraryDrop = true;
				} else {
					AddVectors(dropPos, ITEM_DROP_OFFSET[this.Type], dropPos);
					
					ITEM_DROP_ROTATION[this.Type][1] = GetRandomFloat(_, 360.0);
					
					TeleportEntity(wIndex, dropPos, ITEM_DROP_ROTATION[this.Type], NULL_VECTOR);
					DispatchKeyValue(wIndex, "model", ITEM_INFOS[this.Type][WorldModel]);
					DispatchKeyValue(wIndex, "fademaxdist", "2048");
					DispatchKeyValue(wIndex, "drawinfastreflection", "1");
					//DispatchKeyValue(wIndex, "solid", "2");
					DispatchKeyValue(wIndex, "disableshadows", "1");
					
					DispatchKeyValue(wIndex, "spawnflags", "4367");
					
					DispatchSpawn(wIndex);
					
					if(strlen(ITEM_INFOS[this.Type][DropSequence]) > 0)
						SetEntData(wIndex, CDynamicProp_nSequence, StringToInt(ITEM_INFOS[this.Type][DropSequence]), 1, true);
					
					//TODO doesnt work with a CPhysicsProp, see above.
					//SetEntProp(wIndex, Prop_Send, "m_bShouldGlow", true);
					//SetEntProp(wIndex, Prop_Send, "m_nGlowStyle", 2);
					//SetEntPropFloat(wIndex, Prop_Send, "m_flGlowMaxDist", 512.0);
					
					DROPPED_ITEM_POSITIONS[wIndex] = dropPos;
					DROPPED_ITEM_ITEMS[wIndex] = this.id;
					dmap_data_item[Bound_ent] = EntIndexToEntRef(wIndex);
				}
			}
			
			if(enforceArbitraryDrop) {
				dmap_data_item[Bound_ent] = INVALID_BOUNDENT_INDEX;
				Go4TK_Arbitrary_Dropped.Push(this.id);
			}
			
			dmap_data_item[DropPos_X] = dropPos[0];
			dmap_data_item[DropPos_Y] = dropPos[1];
			dmap_data_item[DropPos_Z] = dropPos[2];
			
			this.CurrentOwnerSlot = SL_None;
			
			ItemDropped(this, this.CurrentOwner);
			
			this.CurrentOwner = -1;
			return true;
		}
		return false;
	}
	
	/**
	 * Create and Return an Item
	 *
	 * @param type					Go4TK_Item_Type to create
	 * @param amount				Amount of Items this Item stack should have
	 * @param spawnPos				Should this item be dropped in the map provide and origin here, otherwise leave default
	 * @param forceArbitraryDrop	True if you do not want to drop a visibible item (Only visibible in the inventorys proximity field)
	 * @return						Item that was created
	 */
	public Item Create(Go4TK_Item_Type type, int amount = 1, float spawnPos[3] = {-91337.1337, 0.0, 0.0}, bool forceArbitraryDrop = false) {
		dmap_data_item[Type] = type;
		dmap_data_item[CurrentOwner] = -1;
		dmap_data_item[CurrentOwnerSlot] = SL_None;
		dmap_data_item[Amount] = amount;
		dmap_data_item[Bound_ent] = INVALID_ENT_REFERENCE;
		dmap_data_item[Durability] = ITEM_DEFINITIONS[type][DefaultDurability];
		
		dmap_data_item[DropPos_X] = dmap_data_item[DropPos_Y] = dmap_data_item[DropPos_Z] = 0.0;
		
		dmap_curr_index = Go4TK_Items.PushArray(dmap_data_item[0]);
		
		#if defined VERBOSE
			Debug("[INV] Created Itemstack %i (%ix%i)", dmap_curr_index, amount, type);
		#endif
		
		Item newItem = Item(dmap_curr_index);
		
		if(spawnPos[0] != -91337.1337)
			newItem.Drop(spawnPos, forceArbitraryDrop);
		
		return newItem;
	}
	
	/**
	 * Remove this Item from any player that possibly currently owns it and destroy it.
	 *
	 * @return			Item was valid and is now destroyed
	 */
	public bool Destroy() {
		this.Refresh();
		if(this.IsValid()) {
			if(this.CurrentOwnerSlot != Carrying && this.CurrentOwnerSlot != SL_None && this.CurrentOwner != -1)
				PlayerInv(this.CurrentOwner, this.CurrentOwnerSlot) = -1;
			
			//Setting the Amount to 0 pretty much makes the item invalid and thus "deleted".
			//We cant fully remove it from the array because Otherwise baaaad stuff would happen. (Shifted indexes, wrong references)
			this.Amount = 0;
			return true;
		}
		return false;
	}
	
	/**
	 * Get or Set this items durability. If durability is set to 0 the item will be destroyed.
	 */
	property int Durability {
		public get() {
			this.Refresh(); return dmap_data_item[Durability];
		}
		public set(int newDurability) {
			this.Refresh();
			if(newDurability <= 0)
				newDurability = 0;
				
			dmap_change_pending = true;
			dmap_data_item[Durability] = newDurability;
			
			if(newDurability == 0 && this.IsValid())
				this.Destroy();
		}
	}
}

methodmap ItemList < ArrayList {
	public ItemList() {
		int x[Go4TK_Item];
		return view_as<ItemList>(new ArrayList(sizeof(x)));
	}
	
	/**
	 * Get an Item based off its index in this ItemList
	 *
	 * @param index		Index to read
	 * @return			Item
	 */
	public Item GetItem(int index) {
		return Item(this.Get(index));
	}
	
	/**
	 * Delete the underlying ArrayList. USE WITH CAUTION AND ONLY IF THIS ItemList IS NOT REFERENCED ANYMORE ANYWHERE!
	 */
	public void Dispose() {
		delete this;
	}
	
	/**
	 * Check if an Item of given Type exists in this ItemList and get it
	 *
	 * @param type		Go4TK_Item_Type to search
	 * @return			Item if available, else INVALID_GO4TK_ITEM
	 */
	public Item GetItemByType(Go4TK_Item_Type type) {
		if(this != null) {
			for (int i = 0; i < this.Length; i++) {
				Item tItem = this.GetItem(i);
				
				//Debug("Got %i of %i, type: %i, IsValid: %i, ID: %i", i, 0, tItem.Type, tItem.IsValid(), tItem.id);
				
				if(tItem.Type == type) return tItem;
			}
		}
		return view_as<Item>(INVALID_GO4TK_ITEM);
	}
	
	/**
	 * Store an Item that already exists in this ItemList
	 *
	 * @param item		Item to store
	 * @param owner		Set the (new) owner of this item, i.e. the user that this ItemList belongs to. -1 to set no owner.
	 * @return			Item was stored
	 */
	public bool StoreItem(Item item, int owner = -1) {
		//Make sure we do not have stored the said item yet.
		Debug(">> %i", this.FindValue(item.id));
		
		if(this.FindValue(item.id) == -1)
			this.Push(item.id);
		
		if(owner != -1) 
			item.CurrentOwner = owner;
		item.CurrentOwnerSlot = Carrying;
		
		return true;
	}
	
	/**
	 * Remove an Item from this ItemList
	 *
	 * @param item		Item to remove
	 * @return			Item existed in this ItemList (and was removed)
	 */
	public bool RemoveItem(Item item) {
		int indx = this.FindValue(item.id);
		if(indx != -1) {
			this.Erase(indx);
			return true;
		}
		return false;
	}
	
	/**
	 * Remove an Item from this ItemList based off an item ID
	 *
	 * @param itemId	ID of item to remove
	 * @return			Item was found (and removed)
	 */
	public bool RemoveItemId(int itemId) {
		int indx = this.FindValue(itemId);
		if(indx != -1) {
			this.Erase(indx);
			return true;
		}
		return false;
	}
	
	/**
	 * Create and store a new Item of the given criteria in this ItemList
	 *
	 * @param type		Go4TK_Item_Type type to create
	 * @param amount	amount of the item type to add in this Item stack if the item type is stackable
	 * @param owner		Pre-set the owner of this item, i.e. the user that this ItemList belongs to. -1 to set no owner.
	 * @return			Item that was just created
	 */
	public Item AddItem(Go4TK_Item_Type type, int amount, int owner = -1) {
		Item x;
		if(ITEM_DEFINITIONS[type][Stackable]) {
			x = this.GetItemByType(type);
			
			if(x.IsValid(true)) {
				x.Amount += amount;
			
				Debug("[INV] Incremented Itemstack %i of %i by %i", x.id, 0, amount);
				return x;
			}
		}
		
		x = Item().Create(type, amount);
		this.StoreItem(x, owner);
		Debug("[INV] Pushed new item into carried...");
		
		return x;
	}
	
	/**
	 * Total weight of all the items in this ItemList
	 *
	 * @return			integer
	 */
	property int Size {
		public get() {
			int size = 0;
			
			for (int i = 0; i < this.Length; i++) {
				Item itm = this.GetItem(i);
				if(itm.IsValid())
					size += itm.TotalWeight;
			}
			return size;
		}
	}
	
	/**
	 * Check if there are more items in this ItemList than the player would be able to carry without a backpack
	 *
	 * @return			boolean
	 */
	property bool StorageHoldsItems {
		public get() {
			return this.Size > INV_BASE_SIZE;
		}
	}
}

methodmap Go4TK_Playerinv {
	public Go4TK_Playerinv(int client) {
		return view_as<Go4TK_Playerinv>(client);
	}
	
	property int client {
		public get() {
			return AsInt(this);
		}
	}
	
	/**
	 * How much the player can carry
	 *
	 * @return			integer
	 */
	property int invLimit {
		public get() {
			int size = INV_BASE_SIZE;
			
			if(PlayerInv(this, Storage).IsValid())
				size += PlayerInv(this, Storage).Type == Backpack ? NORMAL_BACKPACK_SIZE : MILITARY_BACKPACK_SIZE;
			
			return size;
		}
	}
	
	/**
	 * How much the player is currently carrying
	 *
	 * @return			integer
	 */
	property int invSize {
		public get() {
			return PlayerInv(this, Carrying).Size;
		}
	}
	
	/**
	 * Set a slot of the player to an item
	 *
	 * @param slot		Go4TK_Inventory_Slots slot to set
	 * @param item		item to set the slot to
	 * @param previousSlot		Workaround, which slot is the item currently in?
	 * @return			Picking up the item succeeded
	 */
	public bool SetSlotItem(Go4TK_Inventory_Slots slot, Item item, Go4TK_Inventory_Slots previousSlot = vAs(Go4TK_Inventory_Slots, -1)) {
		if(!ItemSlotChangedPre(item, slot, previousSlot)) return false;
			
		Item currentItemInSlot = vAs(Item, PlayerInv(this.client, slot));
		bool curIsValid = currentItemInSlot.IsValid();
		//Check if there currently is an item in the slot requested to set
		if(curIsValid) {
			Debug("[INV] [!] Slot currently contains item, PRE-HOOK...");
			
			if(!ItemSlotSwapPre(currentItemInSlot, item)) return false;
		}
		
		Debug("[INV]>>>>>>>>> Setting slot %i of player %i to %i...", slot, this.client, item);
		
		//Set owner, override slot
		item.CurrentOwner = this.client;
		PLAYER_INV[this.client][slot] = AsInt(item);
		
		//Check if there currently is an item in the slot requested to set
		if(curIsValid) {
			Debug("[INV] [!] Slot currently contains item, processing...");
			
			//The item moved, and the item in the slot are both weapons. Check if its a slot-swap and process
			if(currentItemInSlot.SuperType == ST_Weapon && item.SuperType == ST_Weapon && 
				(item.CurrentOwnerSlot == Weapon1 || item.CurrentOwnerSlot == Weapon2 || item.CurrentOwnerSlot == Weapon3)) {
				PLAYER_INV[this.client][item.CurrentOwnerSlot] = AsInt(currentItemInSlot);
				
				currentItemInSlot.CurrentOwnerSlot = item.CurrentOwnerSlot;
				
				//TODO//WORKAROUND Doesnt really belong here.. tbh Maybe fix up later ¯\_(ツ)_/¯
				PerformWeaponSwap(this.client, slot, item.CurrentOwnerSlot);
			} else {
				int currFree = this.invLimit - this.invSize;
				
				//If the previous item does not fit in storage, drop it.
				if(currFree < currentItemInSlot.TypeWeight) {
					float clientAbs[3];
					if(IsValidClient(this.client)) GetEntDataVector(this.client, CBaseEntity_vecOrigin, clientAbs);
					
					currentItemInSlot.Drop(clientAbs);
				} else {
					PlayerInv(this.client, Carrying).StoreItem(currentItemInSlot);
				}
			}
		} else {
			//Eventhough we move the weapon into a slot which does not contain a weapon yet we need to perform a 
			//weaponswap on it to move it in the inventory, and in the fakeweapons system
			
			if ((item.CurrentOwnerSlot == Weapon1 || item.CurrentOwnerSlot == Weapon2 || item.CurrentOwnerSlot == Weapon3) && 
				(slot == Weapon1 || slot == Weapon2 || slot == Weapon3)) {
				
				PLAYER_INV[this.client][item.CurrentOwnerSlot] = AsInt(INVALID_GO4TK_ITEM);
				
				PerformWeaponSwap(this.client, item.CurrentOwnerSlot, slot);
			}
		}
		
		//if(AsInt(previousSlot) != -1 && previousSlot != Carrying && previousSlot != SL_None && vAs(Item, PLAYER_INV[this.client][previousSlot]) == item)
		//	PLAYER_INV[this.client][previousSlot] = AsInt(INVALID_GO4TK_ITEM);
		
		//Set new item's slot AFTER the previously contained was possibly moved out of its slot.
		item.CurrentOwnerSlot = slot;
		return true;
	}
	
	/**
	 * Attempt to pick up an item
	 *
	 * @param item		item to pick up
	 * @param newSlot		pre-defined slot to *attempt* to put this item into
	 * @param amountToPickup		amount to pick up of this item, or -1 to take all
	 * @return			Picking up the item succeeded
	 */
	public Item PickupItem(Item item, Go4TK_Inventory_Slots newSlot = SL_None, int amountToPickup = -1) {
		if(!item.IsValid() || (item.CurrentOwner != -1 && item.CurrentOwner != this.client)) {
			LogError("[INV] Client attempted to pick up an item that isnt valid, or belongs to somebody else! Client: %i Owner: %i ItemType: %i", this.client, item.CurrentOwner, item.Type);
			return INVALID_GO4TK_ITEM; //TODO normally(?) impossible to achieve. Possibly kick player.
		} else {
			//Can be caused by desync
			if(item.CurrentOwner == -1 && item.BoundEnt != INVALID_ENT_REFERENCE && !item.IsInClientReach(this.client))
				return INVALID_GO4TK_ITEM;
		
			//Item is valid, check the amountToPickup value
			if(amountToPickup <= 0 || amountToPickup > item.Amount)
				amountToPickup = item.Amount;
		}
	
		if(newSlot == SL_None) {
			//Check if item has a dedicated slot, and if it should go there.
			newSlot = Carrying;
			
			if(item.SuperType == ST_Head) {
				if(!PlayerInv(this.client, Head).IsValid()) 
					newSlot = Head;
					
			} else if(item.SuperType == ST_Armor) {
				if(!PlayerInv(this.client, Armor).IsValid()) 
					newSlot = Armor;
				else if(PlayerInv(this.client, Armor).Type == Makeshift_Armor && item.Type == Laminated_Vest)
					newSlot = Armor;
					
			} else if(item.SuperType == ST_Storage) {
				if(!PlayerInv(this.client, Storage).IsValid()) 
					newSlot = Storage;
				else if(PlayerInv(this.client, Storage).Type == Backpack && item.Type == Military_Backpack)
					newSlot = Storage;
			} else if(item.SuperType == ST_Weapon) {
				if(!PlayerInv(this.client, Weapon1).IsValid())
					newSlot = Weapon1;
				else if(!PlayerInv(this.client, Weapon2).IsValid()) 
					newSlot = Weapon2;
				else if(!PlayerInv(this.client, Weapon3).IsValid()) 
					newSlot = Weapon3;
			} else if(item.Type == Yeezys) {
				newSlot = Boots;
			}
		} else {
			//Check if it makes sense to place the item in the requested slot
			if(	(newSlot == Head && item.SuperType != ST_Head) ||
				(newSlot == Armor && item.SuperType != ST_Armor) ||
				(newSlot == Storage && item.SuperType != ST_Storage) ||
				((newSlot == Weapon1 || newSlot == Weapon2 || newSlot == Weapon3) && item.SuperType != ST_Weapon) ||
				(newSlot == Boots && item.Type != Yeezys))
				newSlot = Carrying;
		}
		
		//Item is already in the requested state. Lets return true.
		if(item.CurrentOwnerSlot == newSlot) 
			return item;
		
		int currFree = this.invLimit - this.invSize;
		
		float clientAbs[3];
		if(IsValidClient(this.client)) 
			GetEntDataVector(this.client, CBaseEntity_vecOrigin, clientAbs);
		
		Debug("[INV] Attempting pickup of Item %i(%i/%i), Free: %i, Weight: %i(%i). newSlot: %i", item, item.SuperType, item.Type, currFree, item.TotalWeight, item.TypeWeight, newSlot);
		
		if(newSlot == SL_None || newSlot == Carrying) {
			//Does the inv have enough free space for even one of this type?
			if(currFree >= item.TypeWeight) {
				//If the item is currently in a slot, and we're looking to move it into the Carried stack we need to empty the current slot first
				if(item.CurrentOwner != -1) {
					if(item.CurrentOwner != -1 && item.CurrentOwnerSlot != SL_None) {
						if(!ItemSlotChangedPre(item, newSlot, item.CurrentOwnerSlot))
							return INVALID_GO4TK_ITEM;
						//Clear the slot the item is currently in as we're about to store or drop it.
						PLAYER_INV[item.CurrentOwner][item.CurrentOwnerSlot] = -1;
						
						Debug("YUP!");
					}
				}
			
				int maxPickupable = Math_Max(RoundToFloor(float(currFree) / item.TypeWeight), item.Amount);
				
				//Item is either Unstackable (Always stored), or the client doesnt own the to-pickup type yet 
				//and tries to pick up the whole stack so AddItem would result in unnecessary overhead
				//since it would create a new ItemStack
				if(!item.Stackable || (maxPickupable == item.Amount && maxPickupable == amountToPickup && !PlayerInv(this, Carrying).GetItemByType(item.Type).IsValid(true))) {
					Debug("[INV] Storing item as its either non-stackable or client doesnt own type yet...");
					
					PlayerInv(this.client, Carrying).StoreItem(item, this.client);
					
					return item;
				} else {
					if(amountToPickup > maxPickupable)
						amountToPickup = maxPickupable;
				
					Debug("[INV] Will pickup %i(Max: %i) / %i", amountToPickup, maxPickupable, item.Amount);
					
					item.Amount -= amountToPickup;
					//If the item is Stackable its possible that it was only partially picked up because the inventory is full. Attempt to drop the other items in case they arent already.
					if(item.Amount != 0) item.Drop(clientAbs);
					
					return PlayerInv(this.client, Carrying).AddItem(item.Type, amountToPickup, this.client);
				}
			} else {
				//Try to drop the item that failed to pick up incase it doesnt have a worldent yet (Crafted, shredded, ...)
				//and isnt in posession of the client
				if(item.CurrentOwner != this.client)
					item.Drop(clientAbs);
			}
		} else {
			//We need to remove the Item from the Carried array because we are MOVING it, and not duplicating (Old Amount = 0, New item) it
			PlayerInv(this.client, Carrying).RemoveItem(item);
		
			this.SetSlotItem(newSlot, item, item.CurrentOwnerSlot);
			
			
			return item;
		}
		return INVALID_GO4TK_ITEM;
	}
	
	/**
	 * Drop an item onto the floor below its current owner
	 *
	 * @param item		item to drop
	 * @param amountToDrop		How many of this type should be dropped, or -1 to drop all
	 * @return			Dropping was successful
	 */
	public bool DropItem(Item item, int amountToDrop = -1) {
		//Does this item even belong to this client?
		if(item.CurrentOwner == this.client) {
			if(!ItemSlotChangedPre(item, SL_None, item.CurrentOwnerSlot)) return false;
		
			if(amountToDrop <= 0 || amountToDrop > item.Amount)
				amountToDrop = item.Amount;
		
			float clientAbs[3];
			if(IsValidClient(this.client)) 
				GetEntDataVector(this.client, CBaseEntity_vecOrigin, clientAbs);
			
			bool forceArbitraryDrop = (PLAYER_DROPPED_ITEMS[this.client] > 30);
			PLAYER_DROPPED_ITEMS[this.client]++;
			
			//We are attempting to drop all items of this stack, so we can just drop the whole stack.
			if(amountToDrop == item.Amount) {
				if(item.CurrentOwnerSlot != Carrying) {
					PlayerInv(this.client, item.CurrentOwnerSlot) = -1;
				} else {
					PlayerInv(this.client, Carrying).RemoveItem(item);
				}
				
				return item.Drop(clientAbs, forceArbitraryDrop);
			} else { //Partial drop, need to create a new stack and drop it.
				item.Amount -= amountToDrop;
				//Item().Create(this.Type, amountToDrop, dropPos); //Create => Drop
				return Item().Create(item.Type, amountToDrop, clientAbs, forceArbitraryDrop).IsValid();
			}
		} else {
			//Malicious request
		}
		return false;
	}
	
	//TODO add some handling for currently-equipped items(?). Amount is set to -1, which makes it invalid but yeah ¯\_(ツ)_/¯
	/**
	 * Shred an item into its sub-items
	 *
	 * @param item		item to shred
	 * @return			Shredding as successful
	 */
	public bool ShredItem(Item item) {
		if(item.IsShreddable && item.IsValid()) {
			if(!item.CanMoveToSlot(SL_None)) return false;
			
			//Could happen when somebody shreds something from proximity while somebody else picks it up.
			if(item.CurrentOwner != this.client && item.CurrentOwner != -1) return false;
			
			//Possible desync
			if(item.CurrentOwner != this.client && !item.IsInClientReach(this.client)) return false;
		
			item.Amount--;
			Go4TK_Item_Type shreddedType = item.Type;
			
			//bool someDropped = false;
			LoopItemDefinitions(i) {
				if(ITEM_SHREDDABLES[shreddedType][i] > 0)
					this.PickupItem(Item().Create(vAs(Go4TK_Item_Type, i), ITEM_SHREDDABLES[shreddedType][i]));
			}
			Debug("DONE LOL");
			return true;
		}
		return false;
	}
	
	/**
	 * Craft a requested item from items owned by the client
	 *
	 * @param Go4TK_Item_Type		item type to craft
	 * @param bool					if true, this will return if it would be possible to craft the requested item, and not actually craft it.
	 * @return						Crafting attempt was successful
	 */
	public bool CraftItem(Go4TK_Item_Type type, bool onlyCheckPossibility = false) {
		if(ITEM_DEFINITIONS[type][Craftable] != -1) {
			LoopItemDefinitions(i) {
				if(ITEM_CRAFTABLES[type][i] > 0) {
					Item currUserItem = PlayerInv(this, Carrying).GetItemByType(vAs(Go4TK_Item_Type, i));
					
					if(!currUserItem.IsValid() || currUserItem.Amount < ITEM_CRAFTABLES[type][i])
						return false;
				}
			}
			
			if(onlyCheckPossibility)
				return true;
			
			LoopItemDefinitions(i) {
				if(ITEM_CRAFTABLES[type][i] > 0) {
					Item currUserItem = PlayerInv(this, Carrying).GetItemByType(vAs(Go4TK_Item_Type, i));
					
					currUserItem.Amount -= ITEM_CRAFTABLES[type][i];
				}
			}
			
			//Crafting succeeded but inventory cant hold the item because of too little space
			//this.PickupItem(Item().Create(type));
			if(!this.PickupItem(Item().Create(type)).IsValid())
				SendYouDroppedMessage(this.client, ITEM_INFOS[type][Name]);
			return true;
		}
		return false;
	}
	
	/**
	 * Proxy function to "Use" the item. Will decrease its Amount by one
	 *
	 * @param item		item to use
	 * @return				Usage-attempt was successfull and valid
	 */
	public bool UseItem(Item item) {
		if(item.IsUseable && item.IsValid()) {
			//TODO Malicious attempt, kick
			if(item.CurrentOwner != this.client && item.CurrentOwner != -1) {
				LogError("[INV] Client attempted to use an item that does belong to somebody else! Client: %i Owner: %i ItemType: %i", this.client, item.CurrentOwner, item.Type);
				return false;
			}
			
			//Possible desync
			if(item.CurrentOwner != this.client && !item.IsInClientReach(this.client)) return false;
			
			item.Amount--;
			
			return true;
		}
		return false;
	}
}

stock void ResetGo4TKInventory() {
	if(Go4TK_Items != null) {
		dmap_change_pending = false;
		for (int i = 0; i < Go4TK_Items.Length; i++) {
			if(Item(i).IsValid(true))
				Item(i).RemoveWorldItem();
		}
		dmap_change_pending = false;
		Go4TK_Items.Clear();
	}
	Go4TK_Arbitrary_Dropped.Clear();
	for (int i = 0; i < 2048; i++)
		DROPPED_ITEM_ITEMS[i] = -1;
}

stock void InitGo4TKInventory() {
	dmap_change_pending = false;
	if(Go4TK_Items == null)
		Go4TK_Items = new ItemList();
	else
		Go4TK_Items.Clear();
	
	if(Go4TK_Arbitrary_Dropped == null)
		Go4TK_Arbitrary_Dropped = new ArrayList();
	else
		Go4TK_Arbitrary_Dropped.Clear();
	
	//int x[Go4TK_Item_Type];
	//Go4TK_NUM_ITEM_TYPES = sizeof(x);
	
	for (int i = 0; i < sizeof(ITEM_SHREDDABLES); i++) {
		ITEM_SHREDDABLES[i][0] = -1;
		ITEM_CRAFTABLES[i][0] = -1;
	}
	
	char tmpBuf[128];
	BuildPath(Path_SM, tmpBuf, sizeof(tmpBuf), "data/Go4TK_Items.txt");
	
	KeyValues Go4TK_ITEMS_KV = new KeyValues("Go4TK_Items");
	Go4TK_ITEMS_KV.ImportFromFile(tmpBuf);
	int typeIndex;
	
	Go4TK_ITEMS_KV.JumpToKey("Definitions");
	Go4TK_ITEMS_KV.GotoFirstSubKey();
	//Load Item Definitions
	do {
		Go4TK_ITEMS_KV.GetSectionName(tmpBuf, sizeof tmpBuf);
		typeIndex = StringToInt(tmpBuf);
		
		ITEM_DEFINITIONS[typeIndex][Weight] = Go4TK_ITEMS_KV.GetNum("Weight", 0);
		ITEM_DEFINITIONS[typeIndex][DefaultDurability] = Go4TK_ITEMS_KV.GetNum("Durability", 1);
		
		ITEM_DEFINITIONS[typeIndex][Stackable] = Go4TK_ITEMS_KV.GetNum("Stackable", 0) == 1;
		ITEM_DEFINITIONS[typeIndex][SpawnProbability] = Go4TK_ITEMS_KV.GetFloat("SpawnProbability", 0.0);
		
		Go4TK_ITEMS_KV.GetString("Name", ITEM_INFOS[typeIndex][Name], 32);
		Go4TK_ITEMS_KV.GetString("Dropmodel", ITEM_INFOS[typeIndex][WorldModel], 64, "models/extras/info_speech.mdl");
		Go4TK_ITEMS_KV.GetString("WearableModel", ITEM_INFOS[typeIndex][WearableModel], 64, ITEM_INFOS[typeIndex][WorldModel]);
		Go4TK_ITEMS_KV.GetString("DropSequence", ITEM_INFOS[typeIndex][DropSequence], 64, "");
		
		PrecacheModel(ITEM_INFOS[typeIndex][WearableModel], true);
		if(!StrEqual(ITEM_INFOS[typeIndex][WearableModel], ITEM_INFOS[typeIndex][WorldModel]) && !IsModelPrecached(ITEM_INFOS[typeIndex][WorldModel]))
			PrecacheModel(ITEM_INFOS[typeIndex][WorldModel], true);
		
		Go4TK_ITEMS_KV.GetString("Type", ITEM_INFOS[typeIndex][Type], 32);
		
		Go4TK_ITEMS_KV.GetVector("Droprotation", ITEM_DROP_ROTATION[typeIndex], NULL_VECTOR);
		Go4TK_ITEMS_KV.GetVector("Dropoffset", ITEM_DROP_OFFSET[typeIndex], NULL_VECTOR);
		
		Go4TK_Item_Supertypes newSType = ST_Item;
		if(StrEqual(ITEM_INFOS[typeIndex][Type], "armor")) newSType = ST_Armor;
		else if(StrEqual(ITEM_INFOS[typeIndex][Type], "weapon")) newSType = ST_Weapon;
		else if(StrEqual(ITEM_INFOS[typeIndex][Type], "head")) newSType = ST_Head;
		else if(StrEqual(ITEM_INFOS[typeIndex][Type], "storage")) newSType = ST_Storage;
		else if(StrEqual(ITEM_INFOS[typeIndex][Type], "utility")) newSType = ST_Utility;
		else if(StrEqual(ITEM_INFOS[typeIndex][Type], "ammo")) newSType = ST_Ammo;
		
		ITEM_DEFINITIONS[typeIndex][SuperType] = newSType;
		
		if(Go4TK_ITEMS_KV.JumpToKey("Actions")) {
			ITEM_DEFINITIONS[typeIndex][Equippable] = Go4TK_ITEMS_KV.GetNum("Equip", 0) == 1;
			ITEM_DEFINITIONS[typeIndex][Shreddable] = Go4TK_ITEMS_KV.GetNum("Shred", -1);
			ITEM_DEFINITIONS[typeIndex][Craftable] = Go4TK_ITEMS_KV.GetNum("Craft", -1);
			ITEM_DEFINITIONS[typeIndex][Useable] = Go4TK_ITEMS_KV.GetNum("Use", -1);
			
			Go4TK_ITEMS_KV.GoBack();
		} else {
			ITEM_DEFINITIONS[typeIndex][Equippable] = false;
			ITEM_DEFINITIONS[typeIndex][Shreddable] = -1;
			ITEM_DEFINITIONS[typeIndex][Craftable] = -1;
			ITEM_DEFINITIONS[typeIndex][Useable] = -1;
		}
	} while (Go4TK_ITEMS_KV.GotoNextKey());
	
	Go4TK_ITEMS_KV.Rewind();
	Go4TK_ITEMS_KV.JumpToKey("Shredables");
	Go4TK_ITEMS_KV.GotoFirstSubKey();
	do {
		Go4TK_ITEMS_KV.GetSectionName(tmpBuf, sizeof(tmpBuf));
		typeIndex = StringToInt(tmpBuf);
		
		for (int i = 1; i < sizeof(ITEM_SHREDDABLES); i++) {
			IntToString(i, tmpBuf, sizeof(tmpBuf));
			
			ITEM_SHREDDABLES[typeIndex][i] = Go4TK_ITEMS_KV.GetNum(tmpBuf, -1);
		}
	} while (Go4TK_ITEMS_KV.GotoNextKey());
	
	
	Go4TK_ITEMS_KV.Rewind();
	Go4TK_ITEMS_KV.JumpToKey("Craftables");
	Go4TK_ITEMS_KV.GotoFirstSubKey();
	do {
		Go4TK_ITEMS_KV.GetSectionName(tmpBuf, sizeof(tmpBuf));
		typeIndex = StringToInt(tmpBuf);
		ITEM_CRAFTABLES[typeIndex][0] = 0;
		
		for (int i = 1; i < sizeof(ITEM_CRAFTABLES); i++) {
			IntToString(i, tmpBuf, sizeof(tmpBuf));
			
			ITEM_CRAFTABLES[typeIndex][i] = Go4TK_ITEMS_KV.GetNum(tmpBuf, -1);
		}
	} while (Go4TK_ITEMS_KV.GotoNextKey());
	
	delete Go4TK_ITEMS_KV;
}

/**
 * Get the weapon Types related Ammo-Type
 *
 * @param weaponEntity		Weapon-edict
 * @return				Weapons needed Ammo-Type as Go4TK_Item_Type
 */
stock Go4TK_Item_Type GetWeapTypeAmmoType(Go4TK_Item_Type type) {
	if(type == AK)
		return Ammo_AK;
	else if(type == M4)
		return Ammo_M4;
	else if(type == P2000)
		return Ammo_P2000;
	else if(type == Shotgun)
		return Ammo_Shotgun;
	else if(type == Scout)
		return Ammo_Scout;
		
	return None;
}

static char tmpWpStr[32];

/**
 * Get the weapons related Ammo-Type
 *
 * @param weaponEntity		Weapon-edict
 * @return				Weapons needed Ammo-Type as Go4TK_Item_Type
 */
stock Go4TK_Item_Type GetWeapAmmoType(int weaponEntity) {
	if(!IsValidEdict(weaponEntity)) return None;
	
	GetEntityClassname(weaponEntity, tmpWpStr, sizeof(tmpWpStr));

	if(StrEqual(tmpWpStr, "weapon_m4a1"))
		return Ammo_M4;
	else if(StrEqual(tmpWpStr, "weapon_ak47"))
		return Ammo_AK;
	else if(StrEqual(tmpWpStr, "weapon_hkp2000"))
		return Ammo_P2000;
	else if(StrEqual(tmpWpStr, "weapon_nova"))
		return Ammo_Shotgun;
	else if(StrEqual(tmpWpStr, "weapon_ssg08"))
		return Ammo_Scout;
		
	return None;
}

/**
 * Get the String-Name for a given Go4TK_Item_Type
 *
 * @param type		Go4TK_Item_Type 
 * @param wpName		Buffer string
 * @param maxlength		Buffer string size
 */
stock bool GetWeapByItemType(Go4TK_Item_Type type, char[] wpName, int maxlength) {
	if(type == AK)
		strcopy(wpName, maxlength, "weapon_ak47");
	else if(type == M4)
		strcopy(wpName, maxlength, "weapon_m4a1");
	else if(type == P2000)
		strcopy(wpName, maxlength, "weapon_hkp2000");
	else if(type == Shotgun)
		strcopy(wpName, maxlength, "weapon_nova");
	else if(type == Scout)
		strcopy(wpName, maxlength, "weapon_ssg08");
	else
		return false;
		
	return true;
}

static void ItemAmountChanged(Item item, int oldAmount, int newAmount) {
	//Debug("CURRENT OWNER: %i", item.CurrentOwner);

	if(item.CurrentOwner != -1) {
		if(newAmount != -1) SendChangedItemAmount(item.CurrentOwner, item, oldAmount, newAmount);
		else newAmount = 0;
		
		if(item.SuperType == ST_Ammo) {
			int playerWeapon = GetPlayerWeapon(item.CurrentOwner);
			Go4TK_Item_Type type = GetWeapAmmoType(playerWeapon);
			
			if(type != None) {
				Item ownedAmmo = PlayerInv(item.CurrentOwner, Carrying).GetItemByType(type);
				int ownedAmmoAmount = ownedAmmo.IsValid() ? ownedAmmo.Amount : 0;
				
				Debug("[INV] Player has %i ammo!", ownedAmmoAmount);
				
				//Set the weapons Secondary Ammo to the Ammocount in Inventory
				SetEntData(playerWeapon, CBaseCombatWeapon_iPrimaryReserveAmmoCount, ownedAmmoAmount, 2, true);
			}
		}
	} else {
		LoopIngameClients(i) if(IsPlayerAlive(i)) {
			if(item.IsInClientReach(i))
				SendChangedItemAmount(i, item, oldAmount, newAmount);
		}
	}
}

static bool ItemSlotChangedPre(Item item, Go4TK_Inventory_Slots newSlot, Go4TK_Inventory_Slots oldSlot) {
	if(item.CurrentOwner != -1) {
		if(oldSlot == Storage) {
			//Theres currently items in the Backpack! We cannot allow it to move out of its slot.
			int newInvSize = PlayerInv(item.CurrentOwner, Carrying).Size;
			if(newSlot == Carrying) newInvSize += item.TypeWeight;
			
			Debug("PREHOOK SLOT CHANGE %i => %i", oldSlot, newSlot);
			
			if(newInvSize > INV_BASE_SIZE)
				return false;
		}
	}
	
	//SendChangedItemSlot(item.CurrentOwner, item, oldSlot, newSlot);
	return true;
}

static bool ItemSlotSwapPre(Item itemOld, Item itemNew) {
	if(itemOld.CurrentOwner != -1) {
		int client = itemOld.CurrentOwner;
		if(itemOld.SuperType == ST_Storage && itemOld.Type == Military_Backpack && itemNew.Type == Backpack) {
			int CurrInvSize = PlayerInv(client, Carrying).Size;
			
			CurrInvSize -= itemNew.TypeWeight; //Subtract weight of To-Move backpack
			
			//All the Items currently in the Inventory simply wouldnt fit in a normal backpack (Even if the Military one was dropped as a result of the swap).
			if(CurrInvSize > INV_BASE_SIZE + NORMAL_BACKPACK_SIZE)
				return false;
		}
	}
	return true;
}

 int PlayerAddons[MAXPLAYERS+1][3];

static int PlayerAddonToClient[MAXENTITIES+1];

stock int GetAddonOwner(int entity) {
	return PlayerAddonToClient[entity];
}

stock void SetPlayerAddon(int client, Go4TK_Inventory_Slots slot, Go4TK_Item_Type type = None) {
	int slotIndex = AsInt(slot) - 1; //Need to account for SL_NONE
	int tmpAddon = EntRefToEntIndex(PlayerAddons[client][slotIndex]);
	if(tmpAddon != INVALID_ENT_REFERENCE) {
		RemoveEdict(tmpAddon);
		PlayerAddons[client][slotIndex] = INVALID_ENT_REFERENCE;
		PlayerAddonToClient[tmpAddon] = 0;
	}
		
	if(type != None) {
		//When theres 1950 Ents on the server the smallest problem would be an invisible player addon.
		if(g_CurrentEntityCount > 1950)
			return;
	
		tmpAddon = CreateEntityByName("prop_dynamic_override");
		//Ill rather not create the visual addon as opposed to causing an error
		if(!IsValidEdict(tmpAddon))
			return;
		
		DispatchKeyValue(tmpAddon, "model", ITEM_INFOS[type][WearableModel]);
		//DispatchKeyValue(skin, "disableshadows", "1");
		DispatchKeyValue(tmpAddon, "solid", "0");
		DispatchSpawn(tmpAddon);
		SetEntProp(tmpAddon, Prop_Send, "m_fEffects", EF_BONEMERGE|EF_NOSHADOW|EF_PARENT_ANIMATES|EF_BONEMERGE_FASTCULL);
		SetVariantString("!activator");
		AcceptEntityInput(tmpAddon, "SetParent", client);
		SetVariantString("primary");
		AcceptEntityInput(tmpAddon, "SetParentAttachment");
		
		//Set "Owner" of addon to client to prevent xMission to owner
		SetEntData(tmpAddon, CDynamicProp_iTeamNum, client);
		
		PlayerAddonToClient[tmpAddon] = client;
		
		if(slot == Storage && alivePlayers >= HIDE_BACKPACKS_WHEN_ALIVE) {
			SetEdictFlags(tmpAddon, FL_EDICT_DONTSEND);
		} else if(slot == Armor && alivePlayers >= HIDE_ARMOR_WHEN_ALIVE) {
			SetEdictFlags(tmpAddon, FL_EDICT_DONTSEND);
		}
		
		SDKHook(tmpAddon, SDKHook_SetTransmit, Addon_SetTransmit);
		
		PlayerAddons[client][slotIndex] = EntIndexToEntRef(tmpAddon);
	}
}

stock void SetPlayerAddonHidden(int client, Go4TK_Inventory_Slots slot, bool hidden = false) {
	int slotIndex = AsInt(slot) - 1; //Need to account for SL_NONE
	int tmpAddon = EntRefToEntIndex(PlayerAddons[client][slotIndex]);
	if(tmpAddon != INVALID_ENT_REFERENCE) {
		if(hidden)
			SetEdictFlags(tmpAddon, FL_EDICT_DONTSEND);
		else
			SetEdictFlags(tmpAddon, FL_FULL_EDICT_CHANGED | FL_EDICT_FULL | FL_EDICT_PVSCHECK);
	}
}

static Action Addon_SetTransmit(int entity, int client) {
	#if defined DEBUG
		if(!Go4TK_Game_InProgress)
			return Plugin_Continue;
	#endif

	//Dont send the body-addon to somebody spectating this player, and dont send it to gotv at all
	if(spectatedPlayer[client] == PlayerAddonToClient[entity] || client == g_iTvClient)
		return Plugin_Handled;
	
	if(PlayerAddonToClient[entity] == client)
		return Plugin_Handled;
	
	if(CanClientSpectate[client] && PlayerAddonToClient[entity] == spectatedPlayer[client])
		return Plugin_Handled;
		
	/*entity = GetEntData(entity, CDynamicProp_iTeamNum);	
	//Dont send the body-addon to its wear-er
	if(client == entity)
		return Plugin_Handled;*/
		
	//if(currentPlayerLayer[client] != currentPlayerLayer[spectatedPlayer[client]])
	//	return Plugin_Handled;

	return Plugin_Continue;
}

stock void ItemOwnerChanged(Item item, int oldOwner, int newOwner) {
	if(newOwner != -1 && item.IsValid()) {
		SendItemAdded(newOwner, item);
		
		//WORKAROUND Well, for ammo we can call changed on pickup to "Notify" weapons about new alt-ammo
		if(item.SuperType == ST_Ammo)
			ItemAmountChanged(item, 0, item.Amount);
		
		if(item.SuperType == ST_Ammo)
			EmitSoundToClient(newOwner, "items/ammo_pickup.wav", _, _, _, _, 0.2);
		else
			EmitSoundToClient(newOwner, "items/itempickup.wav", _, _, _, _, 0.2);
	}
}

stock void ItemOwnerChangedPre(Item item, int oldOwner, int newOwner) {
	//WORKAROUND Same as above, just the other way around. If ammo is dropped we need to notify the client that he doesnt have it anymore.
	if(oldOwner != -1  && item.IsValid(true) && item.SuperType == ST_Ammo)
		ItemAmountChanged(item, item.Amount, -1);
}

stock void ItemDropped(Item item, int prevOwner) {
	if(item.IsValid()) {
		LoopIngameClients(i)
			if(i != prevOwner && item.IsInClientReach(i))
				SendItemAdded(i, item);
	}
}

/**
 * Swap the weapons from one slot to the other, vice versa
 *
 * @param client		Client index.
 * @param weaponA		Swap-Candidate 1
 * @param weaponB		Swap-Candidate 2
 */
stock void PerformWeaponSwap(int client, Go4TK_Inventory_Slots weaponA, Go4TK_Inventory_Slots WeaponB) {
	//Weapon was moved from one Equip slot to another. Simply swap fakeweapon slots
	if(weaponA == Weapon1 || weaponA == Weapon2 || weaponA == Weapon3 && (WeaponB == Weapon1 || WeaponB == Weapon2 || WeaponB == Weapon3)) {
		Debug("Shit got called: Old: %i, New: %i", weaponA, WeaponB);
		
		int slotAIndx = 1;
		if (weaponA == Weapon2) slotAIndx = 2;
		if (weaponA == Weapon3) slotAIndx = 3;
		
		int slotBIndx = 1;
		if (WeaponB == Weapon2) slotBIndx = 2;
		if (WeaponB == Weapon3) slotBIndx = 3;
		
		int newSlotWeap = GetPlayerFakeWeaponSlotWeapon(client, slotAIndx);
		int weapToReplace = GetPlayerFakeWeaponSlotWeapon(client, slotBIndx);
		
		Debug("We will be swapping %i(%i) with %i(%i)", newSlotWeap, slotAIndx, weapToReplace, slotBIndx);
		
		int curSlot = GetPlayerFakeWeaponSlot(client);
		//If we are moving weaponA into a slot which does not contain a weapon we cannot re-select the weapons
		//current slot (if it is selected) as it wont contain an item afterwards.
		if(weapToReplace == -1 && curSlot == slotAIndx)
			curSlot = slotBIndx;
		
		//SelectPlayerWeaponSlot(client, SLOT_FISTS);
		
		SetPlayerWeaponSlot(client, slotBIndx, newSlotWeap);
		SetPlayerWeaponSlot(client, slotAIndx, weapToReplace);
		
		SelectPlayerWeaponSlot(client, curSlot);
	}
}

stock void ItemSlotChangedPost(Item item, Go4TK_Inventory_Slots newSlot, Go4TK_Inventory_Slots oldSlot) {
	if(item.CurrentOwner != -1) {
		Debug("ITEM SLOT CHANGED! Owner: %i, newSlot: %i, oldSlot: %i", item.CurrentOwner, newSlot, oldSlot);
		int client = item.CurrentOwner;
		SendChangedItemSlot(client, item, newSlot, oldSlot);
	
		//Weapon was moved out of Equip slot, need to strip its Ammo, return it as Ammo_X and kill the weapon
		if((oldSlot == Weapon1 || oldSlot == Weapon2 || oldSlot == Weapon3) && (newSlot != Weapon1 && newSlot != Weapon2 && newSlot != Weapon3)) {
			int fakeWpSlot = 1;
			if(oldSlot == Weapon2) fakeWpSlot = 2;
			else if(oldSlot == Weapon3) fakeWpSlot = 3;
			
			int fakeWp = GetPlayerFakeWeaponSlotWeapon(client, fakeWpSlot);
			
			if(!IsValidEdict(fakeWp)) {
				Debug("[INV] SOMETHING IMPOSSIBLE HAPPENED!! 90u12");
				return; //TODO impossible?
			}
			
			int currAmmo = GetEntData(fakeWp, CBaseCombatWeapon_iClip1, 1);
			
			//If the Weapon is currently loaded we need to unload the primary ammo into the Inventory
			if(currAmmo > 0) {
				Item ammo = PlayerInv(client, Carrying).AddItem(GetWeapAmmoType(fakeWp), currAmmo);
				
				Go4TK_Playerinv pInv = Go4TK_Playerinv(client);
				
				//It is possible that by picking up all the primary ammo we've overfilled the inventory past its limit
				//so we drop the excess ammo.
			
				int overhead = pInv.invSize - pInv.invLimit;
				
				if(overhead > 0)
					pInv.DropItem(ammo, RoundToCeil(float(overhead) / ammo.TypeWeight));
			}
			
			//PlayerInv(client, Carrying).AddItem(GetWeapAmmoType(fakeWp), , client);
			//TODO possibly detect if theres another weapon equipped and switch to that ¯\_(ツ)_/¯
			SelectPlayerWeaponSlot(client, SLOT_FISTS);
			SetPlayerWeaponSlot(client, fakeWpSlot, -1);
		}//Weapon was moved INTO Equip slot, and wasnt in one beforehand. Create Weapon & set FakeWeaponSlot etc...
		else if((newSlot == Weapon1 || newSlot == Weapon2 || newSlot == Weapon3) && (oldSlot != Weapon1 && oldSlot != Weapon2 && oldSlot != Weapon3)) {
			GetWeapByItemType(item.Type, tmpWpStr, sizeof(tmpWpStr));
			
			int fakeWpSlot = 1;
			if(newSlot == Weapon2) fakeWpSlot = 2;
			else if(newSlot == Weapon3) fakeWpSlot = 3;
			
			int fakeWp = GivePlayerWeaponSlot(client, fakeWpSlot, tmpWpStr);
			
			if(!IsValidEdict(fakeWp)) {
				Debug("[INV] SOMETHING IMPOSSIBLE HAPPENED!! 78dsa");
				return; //TODO impossible?
			}
			
			SetEntData(fakeWp, CBaseCombatWeapon_iPrimaryReserveAmmoCount, 0, 2, true);
			SetEntData(fakeWp, CBaseCombatWeapon_iClip1, 0, 2, true);
			
			SelectPlayerWeaponSlot(client, fakeWpSlot);
		}//Helmet was put on
		else if(newSlot == Head) {
			SetEntProp(client, Prop_Send, "m_bHasHelmet", 1);
			SetPlayerAddon(client, Head, Helmet);
		}//Helmet was taken off
		else if(oldSlot == Head) {
			SetEntProp(client, Prop_Send, "m_bHasHelmet", 0);
			SetPlayerAddon(client, Head);
		}//Armor was put on
		else if(newSlot == Armor) {
			SetEntProp(client, Prop_Send, "m_ArmorValue", item.Durability);
			SetPlayerAddon(client, Armor, item.Type);
		}//Armor was taken off
		else if(oldSlot == Armor) {
			SetEntProp(client, Prop_Send, "m_ArmorValue", 0);
			SetPlayerAddon(client, Armor);
		}//Backpack put on...
		else if(newSlot == Storage || oldSlot == Storage) {
			SetPlayerAddon(client, Storage, newSlot == Storage ? item.Type : None);
		}//Boots put on...
		else if(newSlot == Boots || oldSlot == Boots) {
			//TODO Set the client maximum speed dependant on if the has boots, or not.
		}
	}
}

stock void InitGo4TKInventoryForClient(int client) {
	if(PLAYER_INV[client][Carrying] != null) {
		PLAYER_INV[client][Carrying].Clear();
	} else {
		PLAYER_INV[client][Carrying] = new ItemList();
	}
	
	PlayerAddons[client][0] = INVALID_ENT_REFERENCE;
	PlayerAddons[client][1] = INVALID_ENT_REFERENCE;
	PlayerAddons[client][2] = INVALID_ENT_REFERENCE;
	
	PLAYER_DROPPED_ITEMS[client] = 0;
	
	for (int i = 0; i < 7; i++)
		PLAYER_INV[client][i] = -1;
}

stock void HandlePlayerDeath(int client, bool override = false, bool dontDropItems = false) {
	if(!IsClientInGame(client) || !IsPlayerAlive(client) || override) {
		SetPlayerAddon(client, Head);
		SetPlayerAddon(client, Armor);
		
		//if(IsClientInGame(client))
		Debug("Handling death of client %i (%L)", client, client);
		
		float playerOrigin[3]; float clientAbs[3];
		
		if(IsValidClient(client))
			GetEntDataVector(client, CBaseEntity_vecOrigin, playerOrigin);
		else
			playerOrigin = plLastPosition[client];
			
		if(!TraceDown(playerOrigin, clientAbs, _, client))
			clientAbs = playerOrigin;
		
		if(IsValidClient(client)) {
			for(int offset = 0; offset < 128; offset += 4)
		        SetEntDataEnt2(client, CBasePlayer_hMyWeapons + offset, -1, true);
		    
			SetEntDataEnt2(client, CBasePlayer_hActiveWeapon, -1, true);
		}
		
		ItemList pInv = PlayerInv(client, Carrying);
		Item tmpItem;
		
		//First we drop the weapons, this will cause any ammo thats possibly in them to get transferred to the carried inventory
		for (int i = AsInt(Weapon1); i < AsInt(Weapon3); i++) {
			tmpItem = Item(PlayerInv(client, i));
			PlayerInv(client, i) = -1;
			
			if(tmpItem.IsValid())
				if(dontDropItems)
					tmpItem.Destroy();
				else
					tmpItem.Drop(clientAbs, true);
		}
		
		//Then we drop anything else thats currently carried by the player to empty out his storage
		while(pInv.Length > 0) {
			//Keep on getting the first thing in the inventory, delete it, and afterwards drop it
			tmpItem = pInv.GetItem(0);
		
			pInv.Erase(0);
			
			if(tmpItem.IsValid())
				if(dontDropItems)
					tmpItem.Destroy();
				else
					tmpItem.Drop(clientAbs, true);
		}
		
		//Afterwards we can finally drop anything else equipped, including his backpack as his storage is now empty and wont fail on size validations.
		for (int i = AsInt(Head); i < AsInt(Boots); i++) {
			tmpItem = Item(PlayerInv(client, i));
			PlayerInv(client, i) = -1;
			
			if(tmpItem.IsValid())
				if(dontDropItems)
					tmpItem.Destroy();
				else
					tmpItem.Drop(clientAbs, false);
		}
		
		//Everything's dropped, lets spawn the crate now.
		if(!dontDropItems && g_CurrentEntityCount <= 2010) {
			//Need to use a prop_physics_override because for some reason a non-solid dynamic prop is ignored by rays.
			int cratePos = CreateEntityByName("prop_physics_override");
			//Make sure that we actually can physically drop this, otherwise fallback to arbitrary drop method.
			if(IsValidEdict(cratePos)) {
				TeleportEntity(cratePos, clientAbs, NULL_VECTOR, NULL_VECTOR);
				DispatchKeyValue(cratePos, "model", "models/props/coop_cementplant/grenade_box/grenade_box_closed.mdl");
				DispatchKeyValue(cratePos, "fademaxdist", "5000");
				DispatchKeyValue(cratePos, "drawinfastreflection", "1");
				//DispatchKeyValue(wIndex, "solid", "2");
				DispatchKeyValue(cratePos, "disableshadows", "1");
				
				DispatchKeyValue(cratePos, "spawnflags", "4367");
				
				DispatchSpawn(cratePos);
				
				SetEntityMoveType(cratePos, MOVETYPE_NONE);
				
				DROPPED_ITEM_ITEMS[cratePos] = BODYBAG_FAKEID;
			}
		}
		
		ResetFakeWeaponClient(client);
	}
	
	static int previousPlayerCount;
	
	if(previousPlayerCount != alivePlayers) {
		if(previousPlayerCount >= HIDE_ARMOR_WHEN_ALIVE && alivePlayers < HIDE_ARMOR_WHEN_ALIVE) {
			LoopIngameClients(i)
				SetPlayerAddonHidden(i, Armor, false);
		}
		
		if(previousPlayerCount >= HIDE_BACKPACKS_WHEN_ALIVE && alivePlayers < HIDE_BACKPACKS_WHEN_ALIVE) {
			LoopIngameClients(i)
				SetPlayerAddonHidden(i, Storage, false);
		}
	
		previousPlayerCount = alivePlayers;
	}
}