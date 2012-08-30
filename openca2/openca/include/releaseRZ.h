#ifndef __RELEASE_RZ_H__
#define __RELEASE_RZ_H__

typedef enum callReleaseReason_e {
	RZ_Unknown = 0,			//δ֪����
	RZ_OK,					//��������
	RZ_CallerGiveup,		//���з���
	RZ_CallerForbidden,		//���н���
	RZ_CallerLimited,		//��������
	RZ_CallerNoDail,		//����δ����
	RZ_CalledRingNoAnswer,	//�ýв���
	RZ_CalledBusy,			//����æ
	RZ_CalledReject,		//���оܾ�
	RZ_CalledUnreachable,	//���в��ɴ�
	RZ_CalledForbidden,		//���н���
	RZ_CalledInvalidate,	//���пպ�
	RZ_GWRestart,			//��������
	RZ_GWResponseError,		//����Ӧ���
	RZ_GWNoResponse,		//������Ӧ��
	RZ_NoTrunk,				//�м���Դ����
	RZ_NoProtocolBridge,	//Э��ת����Դ����
	RZ_NetworkTimeout,		//���糬ʱ
	RZ_NetworkError,		//�����
	RZ_NoEnoughBandwidth,	//���������
	RZ_ExceedLicenseLimit,	//�������
	RZ_PrepaidCardInUsing,	//Ԥ��������ʹ��
	RZ_PrepaidInvalidCard,	//Ԥ�������Ϸ�
	RZ_PrepaidCardInactive,	//Ԥ����δ����
	RZ_PrepaidCardExpire,	//Ԥ�����ѹ���
	RZ_PrepaidCardLowBalance,	//Ԥ��������
	RZ_PrepaidCardPinError,	//Ԥ���������
	RZ_ExtServerCallerError,//���кŴ��ⲿ��������
	RZ_ExtServerCalledError,//���кŴ��ⲿ��������
	RZ_ExtServerSystemBusy,	//�ⲿ������ϵͳæ
	RZ_ExtServerDBError,	//�ⲿ���������ݿ����
	RZ_ExtServerResponseError,	//�ⲿ������Ӧ���
	RZ_ExtServerTimeout,	//�ⲿ��������ʱ
	RZ_TeardownByExtServer,	//�ⲿ�������жϺ���
	RZ_SystemError,			//ϵͳ����
	RZ_End
}; 

char* GetReleaseRZStrCh(callReleaseReason_e rz);
char* GetReleaseRZStrEn(callReleaseReason_e rz);
#endif  //__RELEASE_RZ_H__

