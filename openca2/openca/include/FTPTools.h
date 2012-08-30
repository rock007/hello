/*****************************************************************************
 1:��½FTP�����
FTPTools ftpTools (char * pm_sFtpServer,int pm_iFtpPort, LogFile * pLogFile=NULL);
ftpTools.login(char * pm_sFtpUser,char *pm_sFtpPwd);

2:��������Ŀ¼
ftpTools.cd(char * pm_sServerPath);

3:�趨����Ŀ¼
ftpTools.Lcd(char * pm_sLocalPath);

4:�ϴ��ļ�
ftpTools.upload(char *pm_sSrcFilePath,char * pm_sSrcFileName);

5:�����ļ�
ftpTools.download(char * pm_sServerFileName,char * pm_sDesPath);

֧��ȫ·���ļ�����PASVģʽ
��logfile����־
******************************************************************************/

#ifndef FTPTOOLS_H_
#define FTPTOOLS_H_
#define BUFFER  1024   /* buffer max size */ 
#define PATH    128    /* path buffer max size */ 
#define MAXFILE 10     /* transfer file max size */ 
#define LOGPATH "log"  /* log file path */ 
#define TMPPATH "tmp"  /* temp file path */ 
#define ETCPATH "etc"  /* configuration/task file path */
#include<string>
#include<stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include "logfile.h"

using namespace std;

typedef struct sockaddr_in SOCKINF; 
typedef struct tm          TIMEINF; 
typedef struct dirent      DIRINF; 
typedef struct stat        STAT; 
 

class FTPTools
{
	private:
	  //string  ftpServer;
	  char 	  *ftpServer;
	  int     ftpPort;
	  //string  serverPath;
	  char 	  *serverPath;
	  //string  localPath;
	  char	  *localPath;
	  int     errorno;
	  int     SockID,         
              SockSer;       
      SOCKINF SockAddr; 
      LogFile *plog;
      /*
       * �ڲ�����
       */
       int RecvMessage(int Sock,char *Message,int Length,char *Check);
       void SendCommand(int Sock,char *Command,int Length);
       int SendToRemote(int Sock,char *SrcFilePath,char *SrcFileName,char *DesFilePath); 
       int GetFromRemote(int Sock,char *FileName);
       int GetRemoteFileList(int Sock,char *FileList);
       void DeleteAD(char *Str);
       short GetPortFromBuffer(char *Buffer);
       void SafeExit(int Code) ;



 
		
	public:
	  /*
      *���캯��
      *@param pm_sFtpServer ������IP��ַ
      *@param pm_iFtpPort   �������˿ں�
      */	 
	  FTPTools(char *pm_sFtpServer,int pm_iFtpPort, LogFile *pLogFile=NULL);
		
	  /*
      *��½FTP������
      *@param pm_sFtpUser  Ftp�ʺ�
      *@param pm_sFtpPwd   Ftp����
      *@return 0 �ɹ� -1 ʧ��
      *        
      */
      int login(char *pm_sFtpUser,char *pm_sFtpPwd);
      
      /*
      *�ļ�Ŀ¼
      *@param pm_sServerPath  Ftp�����Ŀ¼
      *@return 0 �ɹ� returnCode �������
      */
      int cd(char *pm_sServerPath);
      
      /*
       *�趨����·��
       * @param pm_sLocalPath ����·�� 
       */
      void Lcd(char *pm_sLocalPath);
       
     /*
      *�ϴ��ļ�
      *@param pm_sSrcFileName  �����ļ���
      *@return 0 �ɹ� returnCode �������
      *        
      */
      int upload(char *pm_sSrcFileName);
      
      /*
      *�����ļ�
      *@param pm_sServerFileName  �����ļ���
      *@return 0 �ɹ� returnCode �������
      *        
      */
      int download(char *pm_sServerFileName);
      
      /*
      *�ر�����
      */
      void bye();
		
		
	  virtual ~FTPTools();
};


int ftp_upload_file(char * ftp_server_ip, int ftp_server_port, char * username, char * password, 
	char * filename, 
	char * server_path = NULL, char * local_path = NULL, 
	LogFile * pLogFile = NULL);


#endif /*FTPTOOLS_H_*/
