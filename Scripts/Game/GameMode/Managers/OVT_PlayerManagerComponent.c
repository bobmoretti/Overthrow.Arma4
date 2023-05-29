class OVT_PlayerManagerComponentClass: OVT_ComponentClass
{
};

class OVT_PlayerData : Managed
{
	[NonSerialized()]
	int id;
	
	string name;
	vector location;	
	vector home;
	vector camp;	
	int money;
	bool initialized;
	bool isOfficer;
	
	bool IsOffline()
	{
		return id == -1;
	}
}

class OVT_PlayerManagerComponent: OVT_Component
{	
	[Attribute()]
	ResourceName m_rMessageConfigFile;
	
	protected ref SCR_SimpleMessagePresets m_Messages;
	
	static OVT_PlayerManagerComponent s_Instance;
	static OVT_PlayerManagerComponent GetInstance()
	{
		if (!s_Instance)
		{
			BaseGameMode pGameMode = GetGame().GetGameMode();
			if (pGameMode)
				s_Instance = OVT_PlayerManagerComponent.Cast(pGameMode.FindComponent(OVT_PlayerManagerComponent));
		}

		return s_Instance;
	}
	
	protected ref map<int, string> m_mPersistentIDs;
	protected ref map<string, int> m_mPlayerIDs;
	ref map<string, ref OVT_PlayerData> m_mPlayers;
	
	void OVT_PlayerManagerComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_mPersistentIDs = new map<int, string>;
		m_mPlayerIDs = new map<string, int>;
		m_mPlayers = new map<string, ref OVT_PlayerData>;
		
		LoadMessageConfig();
	}
	
	OVT_PlayerData GetPlayer(string persId)
	{
		if(m_mPlayers.Contains(persId)) return m_mPlayers[persId];
		return null;
	}
	
	protected void LoadMessageConfig()
	{
		Resource holder = BaseContainerTools.LoadContainer(m_rMessageConfigFile);
		if (holder)		
		{
			SCR_SimpleMessagePresets obj = SCR_SimpleMessagePresets.Cast(BaseContainerTools.CreateInstanceFromContainer(holder.GetResource().ToBaseContainer()));
			if(obj)
			{
				m_Messages = obj;
			}
		}
	}
	
	void HintMessageAll(string tag, int townId = -1, int playerId = -1)
	{
		SCR_SimpleMessagePreset preset = m_Messages.GetPreset(tag);
		int index = m_Messages.m_aPresets.Find(preset);
		Rpc(RpcDo_HintMessage, index, townId, playerId);
		DoHintMessage(index, townId, playerId);
	}
	
	protected string GetMessageText(int index, int townId = -1, int playerId = -1)
	{
		string text = "";
		
		if(playerId > -1)
		{
			//Add player name
			string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
			text += name + " ";
		}	
		
		SCR_SimpleMessagePreset preset = m_Messages.m_aPresets[index];		
		if(preset.m_UIInfo){
			text += preset.m_UIInfo.GetDescription();
		}
		return text;
	}
	
	protected string GetMessageTitle(int index, int townId = -1, int playerId = -1)
	{
		SCR_SimpleMessagePreset preset = m_Messages.m_aPresets[index];		
		string text = "";
				
		string townName;
		if(townId > -1)
		{
			//Add town name
			OVT_TownManagerComponent towns = OVT_Global.GetTowns();
			OVT_TownData town = towns.m_Towns[townId];
			text += towns.GetTownName(townId) + " ";
		}			
		string title = preset.m_UIInfo.GetName();
		if(title != "")
		{ 
			//Append anything from UIInfo
			text += title;
		}
		return text;
	}
	
	protected void DoHintMessage(int index, int townId = -1, int playerId = -1)
	{
		string text = GetMessageText(index, townId, playerId);
		string title = GetMessageTitle(index, townId, playerId);
		SCR_HintManagerComponent.GetInstance().ShowCustom(text, title, 10, true);
	}
	
	void SendMessageAll(string tag, int townId = -1, int playerId = -1)
	{
		SCR_SimpleMessagePreset preset = m_Messages.GetPreset(tag);
		int index = m_Messages.m_aPresets.Find(preset);
		Rpc(RpcDo_SendMessage, index, townId, playerId);
		DoRcvMessage(index, townId, playerId);
	}
	
	protected void DoRcvMessage(int index, int townId = -1, int playerId = -1)
	{
		string text = GetMessageText(index, townId, playerId);
	}
	
	string GetPersistentIDFromPlayerID(int playerId)
	{
		if(!m_mPersistentIDs.Contains(playerId)) {
			string persistentId = EPF_Utils.GetPlayerUID(playerId);
#ifdef WORKBENCH
			//Force only two players in workbench to test reconnection
			if(playerId > 2)
			{
				persistentId = "LOCAL_UID_2";
			}
#endif
			SetupPlayer(playerId, persistentId);
			return persistentId;
		}
		return m_mPersistentIDs[playerId];
	}
	
	int GetPlayerIDFromPersistentID(string id)
	{
		if(!m_mPlayerIDs.Contains(id)) return -1;
		return m_mPlayerIDs[id];
	}
	
	void SetupPlayer(int playerId, string persistentId)
	{
		m_mPersistentIDs[playerId] = persistentId;
		m_mPlayerIDs[persistentId] = playerId;
		
		OVT_PlayerData player = GetPlayer(persistentId);		
					
		if(!player)
		{
			player = new OVT_PlayerData;
			m_mPlayers[persistentId] = player;			
		}
		
		player.name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		
		player.id = playerId;
		
		if(Replication.IsServer())	
			Rpc(RpcDo_RegisterPlayer, playerId, persistentId);
	}
	
	void TeleportPlayer(int playerId, vector pos)
	{
		int localId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(SCR_PlayerController.GetLocalControlledEntity());
		if(playerId == localId){
			RpcDo_TeleportPlayer(playerId, pos);
			return;
		}
		
		Rpc(RpcDo_TeleportPlayer, playerId, pos);
	}
	
	//RPC Methods
	
	override bool RplSave(ScriptBitWriter writer)
	{
		//Send JIP Players
		writer.WriteInt(m_mPlayers.Count());
		for(int i=0; i<m_mPlayers.Count(); i++)
		{
			OVT_PlayerData player = m_mPlayers.GetElement(i);
			writer.WriteString(m_mPlayers.GetKey(i));
			writer.WriteInt(player.id);
			writer.WriteInt(player.money);
			writer.WriteVector(player.home);
			writer.WriteVector(player.camp);
			writer.WriteString(player.name);
			writer.WriteBool(player.isOfficer);
		}		
		return true;
	}
	
	override bool RplLoad(ScriptBitReader reader)
	{		
		
		int length, playerId;
		string persId;
		
		//Recieve JIP players
		if (!reader.ReadInt(length)) return false;
		for(int i=0; i<length; i++)
		{
			if (!reader.ReadString(persId)) return false;
			if (!reader.ReadInt(playerId)) return false;
			OVT_PlayerData player = GetPlayer(persId);
			if(!player)
			{
				player = new OVT_PlayerData;
				m_mPlayers[persId] = player;
				player.id = playerId;		
			}
			m_mPersistentIDs[playerId] = persId;
			m_mPlayerIDs[persId] = playerId;
			
			if (!reader.ReadInt(player.money)) return false;
			if (!reader.ReadVector(player.home)) return false;
			if (!reader.ReadVector(player.camp)) return false;
			if (!reader.ReadString(player.name)) return false;
			if (!reader.ReadBool(player.isOfficer)) return false;
		}
		return true;
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_TeleportPlayer(int playerId, vector pos)
	{
		int localId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(SCR_PlayerController.GetLocalControlledEntity());
		if(playerId != localId) return;
		
		string persId = OVT_Global.GetPlayers().GetPersistentIDFromPlayerID(playerId);
		vector spawn = OVT_Global.FindSafeSpawnPosition(pos);
		SCR_Global.TeleportPlayer(spawn);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_RegisterPlayer(int playerId, string s)
	{
		SetupPlayer(playerId, s);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_HintMessage(int msg, int townId, int playerId)
	{
		DoHintMessage(msg, townId, playerId);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SendMessage(int msg, int townId, int playerId)
	{
		DoRcvMessage(msg, townId, playerId);
	}
	
	void ~OVT_PlayerManagerComponent()
	{
		if(m_mPersistentIDs)
		{
			m_mPersistentIDs.Clear();
			m_mPersistentIDs = null;
		}
		if(m_mPlayerIDs)
		{
			m_mPlayerIDs.Clear();
			m_mPlayerIDs = null;
		}
	}
}