using System;
using System.IO;

namespace AMXXRelease
{
	/// <summary>
	/// Summary description for ModCstrike.
	/// </summary>
	public class ModCstrike : AMod
	{
		public ModCstrike()
		{
			AddModules();
			AddPlugins();
		}

		public override sealed string GetName()
		{
			return "cstrike";
		}

		private void AddPlugins()
		{
			AddPlugin("miscstats");
			AddPlugin("stats_logging");
			AddPlugin("statsx");
			AddPlugin("restmenu");
			
			Plugin csstats = new Plugin("csstats");
			csstats.outdir = "data";
			m_Plugins.Add(csstats);
		}

		public override sealed bool CopyExtraFiles(ABuilder ab, string basedir, string source)
		{

			if ((int)System.Environment.OSVersion.Platform == 128)
			{
			} else {
				File.Copy(source + "\\dlls\\csx\\WinCSX\\Release\\WinCSX.exe",
					  basedir + "\\data\\WinCSX.exe",
					  true);
			}

			return true;
		}

		private void AddModules()
		{
			Module csx = new Module("csx");
			Module cstrike = new Module("cstrike");

			m_Modules.Add(csx);
			m_Modules.Add(cstrike);
		}
	}
}
