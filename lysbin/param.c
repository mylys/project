#include <sys/socket.h>
#include <netinet/in.h>
#include <iconv.h>
#include <stdio.h>    
#include <stdlib.h>   
#include <sys/types.h>
#include <string.h>   
#include <malloc.h>   
//#include "cJSON.h"
//#include "xmlparse.h"

typedef struct 
{
    char ip[32];
    char port[32];
    char info[64];


}Tconfig;
Tconfig ProcessConfig;
/***********************************************
 *���ܣ� ��ʽ��ascii�ַ���������ַ����е�\t \n \r
 *���룺 char *Src_Dst       �����ַ���  ���     
 *����ֵ��
 *����  
 **********************************************/
char * sides_trim(char* Src_Dst)//ȥ��ͷβ
{
 int    i, iLen;
 char   *ptr;

 if ( Src_Dst != NULL )
 {
   ptr = Src_Dst;

   while ( *ptr )
   {
     if( *ptr == '\r' || *ptr == '\n' || *ptr == ' ' || *ptr == '\t' )
     {
       ptr++;
     }
     else
     {   
       break;
     }   
   }   

   memmove( Src_Dst , ptr , strlen( ptr ) + 1 ) ; //ȥ��ͷ��

   ptr = Src_Dst;
   iLen = strlen(ptr);
   ptr = Src_Dst + iLen - 1;

   while( ptr >= Src_Dst )
   {   
     if( *ptr == '\r' || *ptr == '\n' || *ptr == ' ' || *ptr == '\t' )
     {   
       --ptr;
     }   
     else
     {
       break;
     }
   }

   *(ptr + 1) = 0 ;

   return Src_Dst;//ȥ��β��
 }

 return NULL;

}
/***********************************************
 *���ܣ� ��ʽ��ascii�ַ���������ַ����е�\t \n \r
 *���룺 char *source       �����ַ���  ���     
 *����ֵ��
 *����  
 **********************************************/
int all_trim(char *source)
{
  int ilen,i;
  char *ps=NULL,*pd=NULL;//,*tmp=NULL
  char tmp[4096]={0};
  ilen=strlen(source);
  ps=source;
  pd=tmp;
  
  for(i=0;i<ilen;i++)
  {
    if(*ps==' '||*ps=='\t'||*ps=='\n'||*ps=='\r') 
    {
      ps++;continue;
    }
    memcpy(pd,ps,1);
    ps++;
    pd++;
  }
  memset(source,0,strlen(source));
  memcpy(source,tmp,pd-tmp);
  return strlen(source);
}


/***********************************************
 *���ܣ� ��ȡ�ַ���[]�е�����
 *���룺 const char *line    ��������
 *       char  *Section      �������
 *����ֵ��
 *����  
 **********************************************/
static int  getConfigRoot(const char *line,char  *Section)
{
  char  *sStart,*sEnd,*noItem;
  
  sStart = strstr(line,"[");
  sEnd = strstr(line,"]");
  noItem = strstr(line,"=");

  if (sStart == NULL || sEnd == NULL)//����ƥ�䵽��ʼ�ͽ���
  {
    return -1;
  }

  if (noItem != NULL)//�����޵Ⱥ�
  {
    return -1;
  }
  
  memset(Section,0,sizeof(Section));

  strncpy(Section,sStart + 1,sEnd - sStart - 1); 

  return 0;
}

/***********************************************
 *���ܣ� ��ȡ�ַ���=�����ߵ�����
 *���룺 const char *line    ��������
 *       char  *Section1     �������
 *       char  *Section2     �������
 *����ֵ��
 *����  
 **********************************************/
static int  getConfigSubLevel(const char *line,char  *Section1,char *Section2)//��ȡ����
{
  char       *sStart;

  sStart = strstr(line,"=");
  if (sStart == NULL)
  {
    return -1;
  }

  strncpy(Section1,line,sStart - line);//ȡ��key
  strncpy(Section2,sStart + 1,strlen(line) - (sStart - line));//ȡ��value

  sides_trim(Section1);
  sides_trim(Section2);
  
  return 0;
}

int getConfigValue(char *sInConfigFiLe,char *sInRootValue, char *sInMatch,char *sOutMatch)
{
  int iRet,iFlag;
  char sLineBuf[1024]={0};
  char sLineBuf_format[1024]={0};
  char sRootBuf[1024]={0};
  char sSubValueBuf[1024]={0};
  char sKeyBuf[1024]={0};
  FILE *fp=NULL;

  if( (fp = fopen(sInConfigFiLe,"r"))==NULL )
  {
      printf("cant open [%s]\n",sInConfigFiLe);
      return 0;
  }

  while(!feof(fp))
  {
    memset(sLineBuf,0x00,sizeof(sLineBuf));
    memset(sRootBuf,0x00,sizeof(sRootBuf));
    memset(sLineBuf_format,0x00,sizeof(sLineBuf_format));
   
    fgets(sLineBuf,sizeof(sLineBuf),fp);
    //formatstrasc(sLineBuf,sLineBuf_format);
    sides_trim(sLineBuf);
    if(strlen(sLineBuf)<=1)
        continue;

    iRet=getConfigRoot(sLineBuf,sRootBuf);//[]������ƥ��
    if(iRet!=0)
    {
        //printf("[%s][%d][%s] get sub config  \n",__FILE__,__LINE__,__FUNCTION__);
        iFlag=0;//ÿ���ҵ��µ����ö���Ҫ�����±�ǩ��־
    }    
    else if(strcmp(sRootBuf,sInRootValue)==0)//���ݲ�ͬ���������ƣ����ò�ͬ��iFlag
    {
        printf("[%s][%d][%s] get RootConfig[%s]  \n",__FILE__,__LINE__,__FUNCTION__,sInRootValue);
        iFlag=1;
        continue;
    }
        
    if(iFlag=1)
    {    
        memset(sOutMatch,0,sizeof(sOutMatch));
        memset(sSubValueBuf,0,sizeof(sSubValueBuf));
        iRet=getConfigSubLevel(sLineBuf,sKeyBuf,sSubValueBuf);//ȡ�����á�
        if(iRet!=0)
        {
          printf("get Config SubLevel error[%d]! \n",iRet);
          continue;
        }
        else if(strcmp(sKeyBuf,sInMatch)==0)
        {
            strcpy(sOutMatch,sSubValueBuf);
            printf("[%s][%d][%s] get sSubValueBuf[%s]  \n",__FILE__,__LINE__,__FUNCTION__,sSubValueBuf);
            return 0;
        }
        else
          continue;
    }
    else
        continue;
  }
  printf("[%s][%d][%s] CONFIG[%s][%s] NOT FOUND  \n",__FILE__,__LINE__,__FUNCTION__,sInRootValue,sInMatch);
  return -1;
}

int main(int argc,char **argv)
{
  int  i,iLen,iRet,iSockFd,iFlag;
  char sConfigFiLe[200] ={0};


  char sSndBuf[2048]  ={0};
  char sRcvBuf[2048]  ={0};

  if(argc!=2)
  {
      printf("usage :%s a.txt\n",argv[0]);
      return 0; 
  }
  if(strlen(argv[1])==0||strlen(argv[1])>150)
  {
      printf("wrong input fiLename\n");
      return 0;
  }
  
  strcat(sConfigFiLe,argv[1]);

  getConfigValue(sConfigFiLe,"PROCESSCONFIG", "IP",ProcessConfig.ip);
  getConfigValue(sConfigFiLe,"PROCESSCONFIG", "PORT",ProcessConfig.port);
  getConfigValue(sConfigFiLe,"PROCESSCONFIG", "INFO",ProcessConfig.info);

}








