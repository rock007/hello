/**********************************************************************************
 * Mgcp_process_mgr.h, defination & implementation of the ProcessMgr class.
 * Note: The implementation of the member functions of ProcessMgr are all in the
 * nt_syspara_mgr.h
 * Author : Zhu Lei (IAG)
 * NewTone Commnunications Corp.
 **********************************************************************************/

#ifndef __MGCP_PROCESS_MGR_H__
#define __MGCP_PROCESS_MGR_H__

#include "openca_typedef.h"
#include <stdlib.h>
#include <strings.h>

class ProcessMgr
{
private:
	pcb_s*	processes;
	int		numOfProcesses;
public:
	ProcessMgr() {processes = NULL; numOfProcesses = 0;}
	
	ProcessMgr(pcb_s* p, int n) {processes = p; numOfProcesses = n;}
	
	~ProcessMgr() {}
	
	int SetpData(pcb_s* p, int n)
	{
		int result = ((processes != NULL) || (p == NULL)) ? 0 : 1;
		processes = p;
		numOfProcesses = n;
		return result;
	}	
	
	time_t GetHB(int whose)
	{
		if (!processes || whose >= numOfProcesses)
			return 0;
		
		return (processes[whose].hb);
	}

	pid_t GetPid(int whose)
	{
		if (!processes || whose >= numOfProcesses)
			return 0;
		
		return (processes[whose].pid);
	}

	char* GetPName(int whose)
	{
		if (!processes || whose >= numOfProcesses)
			return 0;
		
		return (processes[whose].pname);
	}

	int GetTermId(int whose)
	{
		if (!processes || whose >= numOfProcesses)
			return 0;
		
		return (processes[whose].termid);
	}

	int GetExitReason(int whose)
	{
		if (!processes || whose >= numOfProcesses)
			return 0;
		
		return (processes[whose].reason);
	}

	int IfChangeTerm(int whose)
	{
		if (!processes || whose >= numOfProcesses)
			return 0;
		
		return processes[whose].bChangeTerm;	
	}

	int SetTermId(int whose, int term_id)
	{
		if (!processes || whose >= numOfProcesses)
			return 0;
		
		processes[whose].termid = term_id;

		return term_id;
	}

	int ChangeTerm(int whose, int term_id)
	{
		int oldterm;

		if (!processes || whose >= numOfProcesses)
			return 0;
		
		oldterm = processes[whose].termid;
		processes[whose].bChangeTerm = 1;
		processes[whose].termid = term_id;
		return oldterm;
	}

	int TermChanged(int whose)
	{
		if (!processes || whose >= numOfProcesses)
			return 0;
		
		if (processes[whose].bChangeTerm == 0)
			return 0;
		
		processes[whose].bChangeTerm = 0;

		return 1;		

	}

	int SetHB(int whose,time_t hb)
	{
		if (!processes || whose >= numOfProcesses)
			return 0;
		
		processes[whose].hb = hb;
		return 1;
	}

	int SetPid(int whose ,pid_t pid, char* name = NULL)
	{
		if (!processes || whose >= numOfProcesses)
			return 0;
		
		processes[whose].pid = pid;

		if (pid == 0)
			return 1;

		if (name == NULL) {
			strncpy(processes[whose].pname, "Unknown", sizeof(processes[whose].pname)-1);
		}
		else {
			strncpy(processes[whose].pname, name, sizeof(processes[whose].pname)-1);
		}
		return 1;
	}
	
	int SetExitReason(int whose ,openca_process_exit_reason_e rs)
	{
		if (!processes || whose >= numOfProcesses)
			return 0;
		
		if (rs == exit_reason_unknown) {
			time_t now;
			time(&now);
			SetHB(whose, now);
		}
		processes[whose].reason = rs;
		return 1;
	}

	int LogPid(int whose,pid_t pid)
	{
		return SetPid(whose, pid);
	}
	
	int LogHB(int whose,time_t hb)
	{
		return SetHB(whose,hb);
	}
	
	int ValidPid(pid_t pid)
	{
		int i;
		for (i=0; i < numOfProcesses; i++)
		{
			if (processes[i].pid == pid)
				return i;
		}
		return -1;
	}
};

#endif  //__MGCP_PROCESS_MGR_H__
