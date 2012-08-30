
#ifndef APPSERVER_BUSSOP_HEAD_DEFINE_
#define APPSERVER_BUSSOP_HEAD_DEFINE_

t_int appserver_proc_realtimebussop(t_pcstr s_pack_rmbussop, t_pcstr s_bussoptype, t_int& i_ret, string& s_resultdes, string & s_bussopid, string & s_tid);
extern t_int appserver_proc_webpack_cancel(t_pcstr s_pack_cancelbussop, t_pcstr s_msgtype, t_int& i_ret, string& s_resultdes);
extern t_int appserver_proc_pesqpack(t_pcstr s_rpack);
extern t_int appserver_proc_ftppack(t_pcstr s_rpack);
extern t_int appserver_proc_sumpack(t_pcstr s_rpack);

#endif
