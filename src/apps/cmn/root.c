#include <common.h>
#include <osa/osadap.h>
#include "resource_mgr.h"


char *pString[] =
{
	"^R^ count=%05d",
	"^G^ count=%05d",
	"^B^ count=%05d",
	"^Y^ count=%05d",
	"^C^ count=%05d",
	"^P^ count=%05d",
	"^r^ count=%05d",
	"^g^ count=%05d",
	"^b^ count=%05d",
	"^y^ count=%05d",
	"^c^ count=%05d",
	"^p^ count=%05d",
	NULL
};

void tTask1(void)
{
	int cnt = 0;
	int i   = 0;

	while (1)
	{
		waitSem(lSMidTest01, WAIT_FOREVER);
		dbgprint(pString[i++], cnt++);

		if(pString[i] == NULL)
			i = 0;
		suspendTask(100);
		postSem(lSMidTest02);
	}
}

void tTask2(void)
{
	int cnt = 0;
	int i   = 0;

	while (1)
	{
		waitSem(lSMidTest02, WAIT_FOREVER);
		dbgprint(pString[i++], cnt++);

		if(pString[i] == NULL)
			i = 0;
		suspendTask(200);
		postSem(lSMidTest01);
	}
}

void tTask3(void)
{
	static int q_data = 0;

	while (1)
	{
		//dbgprint("^p^ send Q data : %d", q_data);
		sendMsg(lQMidTest01, &q_data, sizeof(int));
		suspendTask(1000);
		q_data++;
	}
}

void tTask4(void)
{
	int recvData = -1;

	while (1)
	{
		recvMsg(lQMidTest01, &recvData, sizeof(int), WAIT_FOREVER);
		//dbgprint("^c^ receive Q data : %d", recvData);
	}
}

void tTask5(void)
{
	while (1)
	{
		//dbgprint("^y^%s run", __func__);
		suspendTask(1000);
	}
}


int root (void)
{
	createSemas();
	createMsgQs();
	createTasks();

	suspendTask(OSADAP_WAIT_FOREVER);
	while(1)
		;
	return 0;
}
