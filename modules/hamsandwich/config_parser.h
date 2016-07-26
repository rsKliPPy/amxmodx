#ifndef _CONFIG_PARSER_H
#define _CONFIG_PARSER_H

#include "amxxmodule.h"
#include <amtl/am-string.h>

class HamDataInfo : public ITextListener_SMC
{
	/* ITextListener_SMC */
public:
	void ReadSMC_ParseStart() override;
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name) override;
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value) override;
	SMCResult ReadSMC_LeavingSection(const SMCStates *states) override;

	/* HamDataInfo */
private:
	size_t StringToIndex(const char *str);

private:
	ke::AString m_Key;
	int m_Index;
	bool m_isBase;
	bool m_isPev;
};


extern HamDataInfo g_HamDataManager;
extern IGameConfigManager *g_ConfigManager;

int ReadConfig(void);

#endif