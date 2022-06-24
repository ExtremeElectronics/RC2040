#include <string.h>
#include <ctype.h>
#include "base64endecode.c"

#define DEBUG 0

//serial states
#define WAITFORSTART 0
#define WAITFORCMD 1
#define WAITFORDRIVE 2
#define WAITFORFN 3
#define CHECK 4
//#define RECEIVEDATA 5
#define DODATA 6
#define WAITFOREND 7
#define SENDEND 8
#define RECEIVEEND 9
#define TEST 10
#define EXIT 99
//#define CMDRM 11

//drives
#define DRIVEMAX 'P'
#define DRIVEMIN 'A'

//commands
#define CMDNONE 0
#define CMDLS 1
#define CMDCOPYTO 2
#define CMDCOPYFROM 3
#define CMDRM 4

//tokens
#define StartToken "&&&-magic-XXX"
#define EndToken "XXX-magic-&&&"

#define NOT_FOUND -1


//cpmtools
#include "cpmcp.c"
#include "cpmls.c"

char buffer[10*1024];
char linebuffer[1024];
char cfilename[20];
int state=0;
int oldstate=-1;
int scmd=0;
char drivel=' ';
struct cpmSuperBlock drive;

char format[40];
char *devopts=NULL;

struct cpmInode root;
char **gargv;
int gargc;
static char starlit[2]="*";
static char * star[]={starlit};
char serr[255];
const char * err;
char *image;



void WaitForStart(void){
    printf("\nStart?:\n");
    scanf("%1024s", linebuffer);
    if (DEBUG) printf("%s",linebuffer);
    strcpy(serr,"");
    if (strcmp(linebuffer,StartToken  ) == 0){
      state=WAITFORCMD;
    }
    if (strcmp(linebuffer,"EXIT"  ) == 0){
      state=EXIT;
    }

}

void WaitForCMD(void){
    printf("Command:\n");
    scanf("%1024s", linebuffer);
    if (DEBUG) printf("%s\n",linebuffer);
    if (strcmp(linebuffer,"LS")==0){
       scmd=CMDLS;
       state=WAITFORDRIVE;
    }
    if (strcmp(linebuffer,"RM")==0){
       scmd=CMDRM;
       state=WAITFORDRIVE;
    }
    if (strcmp(linebuffer,"COPYTO")==0){
       scmd=CMDCOPYTO;
       state=WAITFORDRIVE;
    }
    if (strcmp(linebuffer,"COPYFROM")==0){
       scmd=CMDCOPYFROM;
       state=WAITFORDRIVE;
    }   
    if (strcmp(linebuffer,"TEST")==0){
       scmd=TEST;
       state=TEST;
    }
    if (strcmp(linebuffer,"EXIT")==0){
       scmd=EXIT;
       state=EXIT;
       state=WAITFORSTART;
       scmd=CMDNONE;
    }

}

void WaitForDrive(void){
   printf("Drive:\n");
   scanf("%1024s", linebuffer);
   linebuffer[0]=toupper(linebuffer[0]);
   if (DEBUG)   printf("%s\n",linebuffer);
   if (linebuffer[0]>=DRIVEMIN && linebuffer[0]<=DRIVEMAX){
       drivel=linebuffer[0];
       if (scmd==CMDLS){
         state=CHECK;
     }else{
         state=WAITFORFN;
     }
   }else{
       sprintf(serr,"Drive out of range %c",drivel);
       state=CHECK;
   }  
}

void WaitForFN(void){
   printf("Filename:\n");
   scanf("%20s", linebuffer);
   if (DEBUG)   printf("FN:%s\n",linebuffer);
   int e=0;
   if(strlen(linebuffer)>12){
       sprintf(serr,"Filename too long");
       e=1;
   }
   int i, count;
   for (i=0, count=0; linebuffer[i]; i++) count += (linebuffer[i] == '.');
   if(count>1){
       sprintf(serr,"toomany .'s");
       e=1;
   }   
   if (e==0){
       int pos;
       char dot[] = ".";
       pos=strcspn(linebuffer,dot);
       if(pos==0){
         sprintf(serr,"%s no .",serr);
         e=1;
       }else{
            if(pos>8){
                sprintf(serr,"$s FN too long before .",err);
                e=1;
            }
            if(strlen(linebuffer)-pos>4){
                sprintf(serr,"%s FN too long [%i] after ." ,err,strlen(linebuffer)-pos);
                e=1;
            }
       }   
       if (e==0){
           strcpy(cfilename,linebuffer);
           if (DEBUG)  printf("filename:!%s!",cfilename);
       }else{
           if (DEBUG)   printf("Filename error %s",serr);
       }  
   }
   state=CHECK;
}

void Check(void){
  if(strlen(serr)==0){
      printf("OK:\n");
      if(scmd==CMDLS) state=DODATA;
      if(scmd==CMDCOPYTO) state=DODATA;
      if(scmd==CMDCOPYFROM) state=DODATA;
      if(scmd==TEST) state=TEST;
      if(scmd==CMDRM) state=DODATA;
  }else{
      printf("ERROR: %s\n",serr);
      state=WAITFORSTART;
      scmd=CMDNONE; 
  }
}


void ReceiveEnd(void){
   printf("File Received\n");
   state=WAITFORSTART;
   scmd=CMDNONE;
}

void CMDTestCPM(void){
   if (DEBUG) printf("TEST\n");
/*
   if ((err=Device_f_open(&drive.dev,idepath,FA_WRITE | FA_READ,devopts))!=0){
       printf("Device fail %s\n",err);
   }else{
      if (DEBUG)  printf("Device Opened\n");
   }
*/
   sprintf(format,"rc2040imgd");

   if (cpmReadSuper(&drive,&root,format)==-1){
       printf("cannot read superblock (%s) - stopping\n",boo);
       while(1);
    }

    if(DEBUG)printf("CPM Read Super\n");

    sprintf(cfilename,"TESTTEST.TXT");

    TestToCPM(&root,cfilename);
    state=RECEIVEEND;

    cpmUmount(&drive);  
    Device_f_sync();
//    Device_f_close(&drive.dev);
}

void DoData(void){
   char * data;
     char filespec[2+8+1+3+1];
     char * file;
     static char filearg[13]="";
     static char * fargc[]={filearg};

//Open image file
/*
    if ((err=Device_f_open(&drive.dev,idepath,FA_WRITE | FA_READ,devopts))!=0){
        printf("Device fail %s\n",err);
    }else{
       if (DEBUG)  printf("Device Opened\n");
    }
*/

// LS

    if(scmd==CMDLS){
       if (DEBUG) printf("LS\n");
       sprintf(format,"rc2040img%c",tolower(drivel));

       if (cpmReadSuper(&drive,&root,format)==-1){
          printf("cannot read superblock (%s) - stopping\n",boo);
          while(1);
       }

       if (DEBUG)  printf("\nLS");
       cpmglob(0,1,star,&root,&gargc,&gargv);
   
       if (DEBUG)  printf("After Glob\n");
       data=UUoldddir(gargv,gargc,&root);
   
       if (DEBUG)  printf("Data:%s",data);
  
       size_t input_size = strlen(data);
       printf("\n");
       send_base64_encode(data, input_size);
       printf("\n");
       state=SENDEND;
       scmd=CMDNONE;
   }
   
//COPYTO

//void ReceiveData(void){

    if(scmd==CMDCOPYTO){
        if (DEBUG) printf("COPYTO\n");
        sprintf(format,"rc2040img%c",tolower(drivel));

        if (cpmReadSuper(&drive,&root,format)==-1){
           printf("cannot read superblock (%s) - stopping\n",boo);
           while(1);
        }
      
        if(DEBUG)printf("CPM Read Super\n");

        if(DEBUG)printf("filename:%s\n",cfilename);

        Base64ToCPM(&root,cfilename);
        state=RECEIVEEND;
    }
   
   
//COPYFROM      
    if(scmd==CMDCOPYFROM){
        if (DEBUG) printf("COPYFROM\n");
        sprintf(format,"rc2040img%c",tolower(drivel));

        if (cpmReadSuper(&drive,&root,format)==-1){
            printf("cannot read superblock (%s) - stopping\n",boo);
            while(1);
        }
      
        if(DEBUG)printf("CPM Read Super\n");

        if(DEBUG)printf("filename:%s ",cfilename);

        sprintf(file,"%s",cfilename);
 
        sprintf(filearg,"%s",cfilename);
        cpmglob(0,1,fargc-1,&root,&gargc,&gargv);
      
        if (DEBUG) printf("CPM GLOB\n");

        sprintf(filespec,"00%s",cfilename);

        if(DEBUG)printf("filespec:%s %s",filespec,cfilename);

        cpmToBase64(&root,filespec);
      
        state=SENDEND;
        scmd=CMDNONE;
   }
   
   
//RM
    if(scmd==CMDRM){
   
        if (DEBUG) printf("RM\n");
        sprintf(format,"rc2040img%c",tolower(drivel));

        if (cpmReadSuper(&drive,&root,format)==-1){
            printf("cannot read superblock (%s) - stopping\n",boo);
            while(1);
        }

        if(DEBUG)printf("CPM Read Super\n");

        if(DEBUG)printf("filename:%s ",cfilename);
        sprintf(filespec,"00%s",cfilename);
        if(cpmUnlink(&root,filespec)==-1){
            printf("can not erase %s: %s\n",cfilename,boo);
        }

        state=SENDEND;
        scmd=CMDNONE;
  }

  cpmUmount(&drive);
  Device_f_sync();   
//  Device_f_close(&drive.dev);
   
}

void SendEnd(void){
    printf("\n\n\n");
    state=WAITFORSTART;
    scmd=CMDNONE;

}

void WaitForEnd(void){
    printf("End:\n");
    scanf("%1024s", buffer);

}


void serialfile(void){
  int exit=0;
  Device_share_open(&drive.dev,  devopts)  ;

  if (DEBUG) printf("\nSerial DeBUG ON\n");

   while (!exit) {

       if(state!=oldstate){
           if (DEBUG)  printf("State:%i CMD:%i\n",state,scmd);
       }

       switch (state){
         case WAITFORSTART:
           WaitForStart();
           break;
         case WAITFORCMD:
           WaitForCMD();
           break;
         case WAITFORDRIVE:
           WaitForDrive();
           break;
         case WAITFORFN:
           WaitForFN();
           break;
         case CHECK:
           Check();
           break;
         case DODATA:
           DoData();
           break;
         case WAITFOREND:
           WaitForEnd();
           break;
         case SENDEND:
           SendEnd();
           break;
         case RECEIVEEND:
           ReceiveEnd();
           break;
         case TEST:
           CMDTestCPM();   
         case EXIT:
           state=WAITFORSTART;
           exit=1;  
         default:
          break;
     }
  }
  printf ("\n\r EXIT FILE MODE \n\r");

}
