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
 *功能： 格式化ascii字符串，清楚字符串中的\t \n \r
 *输入： char *Src_Dst       整个字符串  输出     
 *返回值：
 *例：  
 **********************************************/
char * sides_trim(char* Src_Dst)//去除头尾
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

   memmove( Src_Dst , ptr , strlen( ptr ) + 1 ) ; //去除头部

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

   return Src_Dst;//去除尾部
 }

 return NULL;

}
/***********************************************
 *功能： 格式化ascii字符串，清楚字符串中的\t \n \r
 *输入： char *source       整个字符串  输出     
 *返回值：
 *例：  
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
 *功能： 获取字符串[]中的内容
 *输入： const char *line    输入内容
 *       char  *Section      输出内容
 *返回值：
 *例：  
 **********************************************/
static int  getConfigRoot(const char *line,char  *Section)
{
  char  *sStart,*sEnd,*noItem;
  
  sStart = strstr(line,"[");
  sEnd = strstr(line,"]");
  noItem = strstr(line,"=");

  if (sStart == NULL || sEnd == NULL)//必须匹配到开始和结束
  {
    return -1;
  }

  if (noItem != NULL)//必须无等号
  {
    return -1;
  }
  
  memset(Section,0,sizeof(Section));

  strncpy(Section,sStart + 1,sEnd - sStart - 1); 

  return 0;
}

/***********************************************
 *功能： 获取字符串=号两边的内容
 *输入： const char *line    输入内容
 *       char  *Section1     输出内容
 *       char  *Section2     输出内容
 *返回值：
 *例：  
 **********************************************/
static int  getConfigSubLevel(const char *line,char  *Section1,char *Section2)//读取配置
{
  char       *sStart;

  sStart = strstr(line,"=");
  if (sStart == NULL)
  {
    return -1;
  }

  strncpy(Section1,line,sStart - line);//取出key
  strncpy(Section2,sStart + 1,strlen(line) - (sStart - line));//取出value

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

    iRet=getConfigRoot(sLineBuf,sRootBuf);//[]中内容匹配
    if(iRet!=0)
    {
        //printf("[%s][%d][%s] get sub config  \n",__FILE__,__LINE__,__FUNCTION__);
        iFlag=0;//每次找到新的配置都需要重置新标签标志
    }    
    else if(strcmp(sRootBuf,sInRootValue)==0)//根据不同的配置名称，设置不同的iFlag
    {
        printf("[%s][%d][%s] get RootConfig[%s]  \n",__FILE__,__LINE__,__FUNCTION__,sInRootValue);
        iFlag=1;
        continue;
    }
        
    if(iFlag=1)
    {    
        memset(sOutMatch,0,sizeof(sOutMatch));
        memset(sSubValueBuf,0,sizeof(sSubValueBuf));
        iRet=getConfigSubLevel(sLineBuf,sKeyBuf,sSubValueBuf);//取出配置。
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








