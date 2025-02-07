//------------------------------------------------------------------------------------------------
class OVT_JobListEntryHandler : SCR_ButtonBaseComponent
{
	OVT_Job m_Job;
	OVT_JobConfig m_JobConfig;	
	
	void Populate(OVT_Job job, OVT_JobConfig config)
	{
		m_Job = job;
		m_JobConfig = config;
		
		TextWidget title = TextWidget.Cast(m_wRoot.FindAnyWidget("Title"));
		title.SetText(m_JobConfig.m_sTitle);
		
		TextWidget location = TextWidget.Cast(m_wRoot.FindAnyWidget("Location"));
		if(job.townId == -1)
		{
			OVT_BaseData base = OVT_Global.GetOccupyingFaction().m_Bases[job.baseId];
			OVT_TownData town = OVT_Global.GetTowns().GetNearestTown(base.location);
			int townID = OVT_Global.GetTowns().GetTownID(town);
			location.SetText("#OVT-BaseNear " + OVT_Global.GetTowns().GetTownName(townID));
		}else{
			location.SetText(OVT_Global.GetTowns().GetTownName(job.townId));
		}
		
		TextWidget reward = TextWidget.Cast(m_wRoot.FindAnyWidget("Reward"));
		reward.SetText("$" + m_JobConfig.m_iReward.ToString());
	}
}