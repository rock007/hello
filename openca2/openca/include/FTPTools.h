/*****************************************************************************
 1:登陆FTP服务端
FTPTools ftpTools (char * pm_sFtpServer,int pm_iFtpPort, LogFile * pLogFile=NULL);
ftpTools.login(char * pm_sFtpUser,char *pm_sFtpPwd);

2:进入服务端目录
ftpTools.cd(char * pm_sServerPath);

3:设定本地目录
ftpTools.Lcd(char * pm_sLocalPath);

4:上传文件
ftpTools.upload(char *pm_sSrcFilePath,char * pm_sSrcFileName);

5:下载文件
ftpTools.download(char * pm_sServerFileName,char * pm_sDesPath);

支持全路径文件名，PASV模式
用logfile记日志
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
       * 内部方法
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
      *构造函数
      *@param pm_sFtpServer 服务器IP地址
      *@param pm_iFtpPort   服务器端口号
      */	 
	  FTPTools(char *pm_sFtpServer,int pm_iFtpPort, LogFile *pLogFile=NULL);
		
	  /*
      *登陆FTP服务器
      *@param pm_sFtpUser  Ftp帐号
      *@param pm_sFtpPwd   Ftp密码
      *@return 0 成功 -1 失败
      *        
      */
      int login(char *pm_sFtpUser,char *pm_sFtpPwd);
      
      /*
      *文件目录
      *@param pm_sServerPath  Ftp服务端目录
      *@return 0 成功 returnCode 错误代码
      */
      int cd(char *pm_sServerPath);
      
      /*
       *设定本地路径
       * @param pm_sLocalPath 本地路径 
       */
      void Lcd(char *pm_sLocalPath);
       
     /*
      *上传文件
      *@param pm_sSrcFileName  本地文件名
      *@return 0 成功 returnCode 错误代码
      *        
      */
      int upload(char *pm_sSrcFileName);
      
      /*
      *下载文件
      *@param pm_sServerFileName  下载文件名
      *@return 0 成功 returnCode 错误代码
      *        
      */
      int download(char *pm_sServerFileName);
      
      /*
      *关闭连接
      */
      void bye();
		
		
	  virtual ~FTPTools();
};


int ftp_upload_file(char * ftp_server_ip, int ftp_server_port, char * username, char * password, 
	char * filename, 
	char * server_path = NULL, char * local_path = NULL, 
	LogFile * pLogFile = NULL);


#endif /*FTPTOOLS_H_*/
