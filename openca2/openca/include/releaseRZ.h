#ifndef __RELEASE_RZ_H__
#define __RELEASE_RZ_H__

typedef enum callReleaseReason_e {
	RZ_Unknown = 0,			//未知错误
	RZ_OK,					//正常结束
	RZ_CallerGiveup,		//主叫放弃
	RZ_CallerForbidden,		//主叫禁呼
	RZ_CallerLimited,		//主叫限制
	RZ_CallerNoDail,		//主叫未拨号
	RZ_CalledRingNoAnswer,	//久叫不答
	RZ_CalledBusy,			//被叫忙
	RZ_CalledReject,		//被叫拒绝
	RZ_CalledUnreachable,	//被叫不可达
	RZ_CalledForbidden,		//被叫禁呼
	RZ_CalledInvalidate,	//被叫空号
	RZ_GWRestart,			//网关重启
	RZ_GWResponseError,		//网关应答错
	RZ_GWNoResponse,		//网关无应答
	RZ_NoTrunk,				//中继资源不足
	RZ_NoProtocolBridge,	//协议转换资源不足
	RZ_NetworkTimeout,		//网络超时
	RZ_NetworkError,		//网络错
	RZ_NoEnoughBandwidth,	//网络带宽不足
	RZ_ExceedLicenseLimit,	//超出许可
	RZ_PrepaidCardInUsing,	//预付卡正在使用
	RZ_PrepaidInvalidCard,	//预付卡不合法
	RZ_PrepaidCardInactive,	//预付卡未激活
	RZ_PrepaidCardExpire,	//预付卡已过期
	RZ_PrepaidCardLowBalance,	//预付卡金额不足
	RZ_PrepaidCardPinError,	//预付卡密码错
	RZ_ExtServerCallerError,//主叫号错（外部服务器）
	RZ_ExtServerCalledError,//被叫号错（外部服务器）
	RZ_ExtServerSystemBusy,	//外部服务器系统忙
	RZ_ExtServerDBError,	//外部服务器数据库错误
	RZ_ExtServerResponseError,	//外部服务器应答错
	RZ_ExtServerTimeout,	//外部服务器超时
	RZ_TeardownByExtServer,	//外部服务器中断呼叫
	RZ_SystemError,			//系统错误
	RZ_End
}; 

char* GetReleaseRZStrCh(callReleaseReason_e rz);
char* GetReleaseRZStrEn(callReleaseReason_e rz);
#endif  //__RELEASE_RZ_H__

