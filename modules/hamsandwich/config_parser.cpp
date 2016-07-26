// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X, based on AMX Mod by Aleksander Naszko ("OLO").
// Copyright (C) The AMX Mod X Development Team.
//
// This software is licensed under the GNU General Public License, version 3 or higher.
// Additional exceptions apply. For full license details, see LICENSE.txt or visit:
//     https://alliedmods.net/amxmodx-license

//
// Ham Sandwich Module
//

#include "amxxmodule.h"

#include "ham_const.h"
#include "hooklist.h"
#include "offsets.h"
#include "config_parser.h"


extern hook_t hooklist[];

HamDataInfo g_HamDataManager;
IGameConfigManager *g_ConfigManager;


size_t HamDataInfo::StringToIndex(const char *str)
{
	char *endptr;

	if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X'))
		return (size_t)strtoul(str, &endptr, 16);

	return (size_t)strtoul(str, &endptr, 10);
}

void HamDataInfo::ReadSMC_ParseStart()
{
	m_Key = "";
	m_Index = -1;
	m_isBase = false;
	m_isPev = false;
}

SMCResult HamDataInfo::ReadSMC_NewSection(const SMCStates *states, const char *name)
{
	if (!m_Key.length())
	{
		m_Key = name;

		if (!stricmp(name, "pev"))
			m_isPev = true;
		else if (!stricmp(name, "base"))
			m_isBase = true;
	}

	return SMCResult_Continue;
}

SMCResult HamDataInfo::ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
{
	if (m_Key.length())
	{
#ifdef _WIN32
		if (!stricmp(key, "windows"))
#elif defined(__linux__)
		if (!stricmp(key, "linux"))
#elif defined(__APPLE__)
		if (!stricmp(key, "mac"))
#endif
		{
			if (m_isBase)
				Offsets.SetBase(StringToIndex(value));
			else if (m_isPev)
				Offsets.SetPev(StringToIndex(value));
			else
				m_Index = StringToIndex(value);
		}
	}

	return SMCResult_Continue;
}

SMCResult HamDataInfo::ReadSMC_LeavingSection(const SMCStates *states)
{
	if (m_Key.length() && m_Index != -1)
	{
		for (size_t i = 0; i < HAM_LAST_ENTRY_DONT_USE_ME_LOL; i++)
		{
			if (!stricmp(m_Key.chars(), hooklist[i].name))
			{
				hooklist[i].isset = 1;
				hooklist[i].vtid = m_Index;

				break;
			}
		}
	}

	m_Key = "";
	m_Index = -1;
	m_isBase = false;
	m_isPev = false;

	return SMCResult_Continue;
}


int ReadConfig(void)
{
	IGameConfig *hamConfig;
	char szError[256] = "";

	if (!g_ConfigManager->LoadGameConfigFile("hamdata.games", &hamConfig, szError, sizeof(szError)) && *szError)
	{
		MF_Log("Could not read hamdata.games gamedata: %s", szError);
		return -1;
	}
	
	g_ConfigManager->CloseGameConfigFile(hamConfig);

	return 1;
}
