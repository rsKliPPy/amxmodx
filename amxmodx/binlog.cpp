#include <time.h>
#include "amxmodx.h"
#include "binlog.h"

#if defined BINLOG_ENABLED

BinLog g_BinLog;
int g_binlog_level = 0;
int g_binlog_maxsize = 0;

bool BinLog::Open()
{
	const char *data = get_localinfo("amxmodx_datadir", "addons/amxmodx/data");
	char path[255];
	build_pathname_r(path, sizeof(path)-1, "%s/binlogs", data);
	
	if (!DirExists(path))
	{
		mkdir(path
#if defined __linux__
			, 0755
#endif
			);
		if (!DirExists(path))
			return false;
	}

	char file[255];
	build_pathname_r(file, sizeof(file)-1, "%s/binlogs/lastlog", data);

	unsigned int lastcntr = 0;
	FILE *lastlog = fopen(file, "rb");
	if (lastlog)
	{
		if (fread(&lastcntr, sizeof(int), 1, lastlog) != 1)
			lastcntr = 0;
		fclose(lastlog);
	}
	lastlog = fopen(file, "wb");
	if (lastlog)
	{
		lastcntr++;
		fwrite(&lastcntr, sizeof(int), 1, lastlog);
		fclose(lastlog);
	}
	build_pathname_r(file, sizeof(file)-1, "%s/binlogs/binlog%04d.blg", data, lastcntr);
	m_logfile.assign(file);

	/**
	* it's now safe to create the binary log
	*/
	FILE *fp = fopen(m_logfile.c_str(), "wb");
	if (!fp)
		return false;

	int magic = BINLOG_MAGIC;
	short vers = BINLOG_VERSION;
	char c = sizeof(time_t);
	fwrite(&magic, sizeof(int), 1, fp);
	fwrite(&vers, sizeof(short), 1, fp);
	fwrite(&c, sizeof(char), 1, fp);

	WritePluginDB(fp);
	fclose(fp);

	m_state = true;

	WriteOp(BinLog_Start, -1);

	return true;
}

void BinLog::Close()
{
	WriteOp(BinLog_End, -1);
	m_state = false;
}

void BinLog::WriteOp(BinLogOp op, int plug, ...)
{
	if (!m_state)
		return;

	FILE *fp = fopen(m_logfile.c_str(), "ab");
	if (!fp)
		return;

	if (g_binlog_maxsize)
	{
		fseek(fp, 0, SEEK_END);
		if (ftell(fp) > (g_binlog_maxsize * (1024 * 1024)))
		{
			fclose(fp);
			Close();
			Open();
			fp = fopen(m_logfile.c_str(), "ab");
			if (!fp)
				return;
		}
	}

	unsigned char c = static_cast<char>(op);
	time_t t = time(NULL);
	float gt = gpGlobals->time;
	fwrite(&c, sizeof(char), 1, fp);
	fwrite(&t, sizeof(time_t), 1, fp);
	fwrite(&gt, sizeof(float), 1, fp);
	fwrite(&plug, sizeof(int), 1, fp);

	va_list ap;
	va_start(ap, plug);

	switch (c)
	{
	case BinLog_Registered:
		{
			const char *title = va_arg(ap, const char *);
			const char *vers = va_arg(ap, const char *);
			c = (char)strlen(title);
			fwrite(&c, sizeof(char), 1, fp);
			fwrite(title, sizeof(char), c+1, fp);
			c = (char)strlen(vers);
			fwrite(&c, sizeof(char), 1 ,fp);
			fwrite(vers, sizeof(char), c+1, fp);
			break;
		}
	case BinLog_NativeCall:
		{
			int native = va_arg(ap, int);
			int params = va_arg(ap, int);
			fwrite(&native, sizeof(int), 1, fp);
			fwrite(&params, sizeof(int), 1, fp);
			break;
		}
	case BinLog_NativeRet:
		{
			cell retval = va_arg(ap, cell);
			fwrite(&retval, sizeof(cell), 1, fp);
			break;
		}
	case BinLog_NativeError:
		{
			int err = va_arg(ap, int);
			const char *msg = va_arg(ap, const char *);
			short len = (short)strlen(msg);
			fwrite(&err, sizeof(int), 1, fp);
			fwrite(&len, sizeof(short), 1, fp);
			fwrite(msg, sizeof(char), len+1, fp);
			break;
		}
	case BinLog_CallPubFunc:
		{
			int num = va_arg(ap, int);
			fwrite(&num, sizeof(int), 1, fp);
			break;
		}
	case BinLog_SetLine:
		{
			int line = va_arg(ap, int);
			fwrite(&line, sizeof(int), 1, fp);
			break;
		}
	case BinLog_FormatString:
		{
			int param = va_arg(ap, int);
			int maxlen = va_arg(ap, int);
			const char *str = va_arg(ap, const char *);
			short len = (short)strlen(str);
			fwrite(&param, sizeof(int), 1, fp);
			fwrite(&maxlen, sizeof(int), 1, fp);
			fwrite(&len, sizeof(short), 1, fp);
			fwrite(str, sizeof(char), len+1, fp);
			break;
		}
	case BinLog_NativeParams:
		{
			cell *params = va_arg(ap, cell *);
			cell num = params[0] / sizeof(cell);
			fwrite(&num, sizeof(cell), 1, fp);
			for (cell i=1; i<=num; i++)
				fwrite(&(params[i]), sizeof(cell), 1, fp);
			break;
		}
	case BinLog_GetString:
		{
			cell addr = va_arg(ap, cell);
			const char *str = va_arg(ap, const char *);
			short len = (short)strlen(str);
			fwrite(&addr, sizeof(cell), 1, fp);
			fwrite(&len, sizeof(short), 1, fp);
			fwrite(str, sizeof(char), len+1, fp);
			break;
		}
	case BinLog_SetString:
		{
			cell addr = va_arg(ap, cell);
			int maxlen = va_arg(ap, int);
			const char *str = va_arg(ap, const char *);
			short len = (short)strlen(str);
			fwrite(&addr, sizeof(cell), 1, fp);
			fwrite(&maxlen, sizeof(int), 1, fp);
			fwrite(&len, sizeof(short), 1, fp);
			fwrite(str, sizeof(char), len+1, fp);
			break;
		}
	};

	va_end(ap);
	fclose(fp);
}

void BinLog::WritePluginDB(FILE *fp)
{
	int num = g_plugins.getPluginsNum();
	fwrite(&num, sizeof(int), 1, fp);

	CPluginMngr::CPlugin *pl;
	char c;
	unsigned char len;
	for (CPluginMngr::iterator iter = g_plugins.begin(); iter; ++iter)
	{
		pl = &(*iter);
		if (pl->isValid())
			c = 1;
		else
			c = 0;
		if (c && pl->isDebug())
			c = 2;
		fwrite(&c, sizeof(char), 1, fp);
		len = (char)strlen(pl->getName());
		fwrite(&len, sizeof(char), 1, fp);
		len++;
		fwrite(pl->getName(), sizeof(char), len, fp);
		int natives, publics;
		AMX *amx = pl->getAMX();
		amx_NumNatives(amx, &natives);
		amx_NumPublics(amx, &publics);
		fwrite(&natives, sizeof(int), 1, fp);
		fwrite(&publics, sizeof(int), 1, fp);
		char name[34];
		for (int i=0; i<natives; i++)
		{
			amx_GetNative(amx, i, name);
			len = (char)strlen(name);
			fwrite(&len, sizeof(char), 1, fp);
			len++;
			fwrite(name, sizeof(char), len, fp);
		}
		for (int i=0; i<publics; i++)
		{
			amx_GetPublic(amx, i, name);
			len = (char)strlen(name);
			fwrite(&len, sizeof(char), 1, fp);
			len++;
			fwrite(name, sizeof(char), len, fp);
		}
	}
}

#endif //BINLOG_ENABLED
