#include <string.h>
#include <ctype.h>
#include "base64endecode.c"
#include <stdint.h>
#include <unistd.h>

#define DEBUG 0
//include from RC2040
#include "RC2040.h"

//serial states
#define WAITFORSTART 0
#define WAITFORCMD 1
#define WAITFORDRIVE 2
#define WAITFORFN 3
#define WAITFORADDRESS 4
#define CHECK 5

//#define RECEIVEDATA 5
#define DODATA 6
#define WAITFOREND 7
#define SENDEND 8
#define RECEIVEEND 9
#define TEST 10
#define EXIT 99

//drives
#define DRIVEMAX 'P'
#define DRIVEMIN 'A'

//commands
#define CMDNONE 0
#define CMDLS 1
#define CMDCOPYTO 2
#define CMDCOPYFROM 3
#define CMDRM 4
#define CMDWHO 5
#define CMDWATCH 6
#define CMDTRACE 7
#define CMDDUMP 8
#define CMDDISSEMBLE 9
#define CMDPROGRAM 10

//tokens
#define StartToken "&&&-magic-XXX"
#define EndToken "XXX-magic-&&&"

#define NOT_FOUND -1

//cpmtools
#include "cpmcp.c"
#include "cpmls.c"

extern uint16_t watch;
extern  int trace;

//was 20*
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

unsigned int Address;
unsigned int Size;

extern void DumpMemoryUSB(unsigned int FromAddr, int dumpsize);
extern void DessembleMemoryUSB(unsigned int FromAddr, int dumpsize);

/* Copy from Base64 chunks to RAM
 *
 *
*/

void Base64ToMEM(int Address){

      int chunk,x,y;
      char encbuffer[16*1024+4];  //faster?
      y=0;
      
      do{
              printf("Chunksize:\n");
              scanf("%d", &chunk);
              if (DEBUG) printf("Chunk:%i",chunk);
              if(chunk>0){
                  printf("Data:\n");
                  scanf("%10000s",encbuffer);
                  if (DEBUG) printf("Data:%s",encbuffer);
                  size_t decode_size = strlen(encbuffer);
                  char * decoded_data = base64_decode(encbuffer, decode_size, &decode_size);
                  if (DEBUG) printf("%s\n\r",decoded_data);
                  if (DEBUG) printf("Write to %i %i %i \n",Address,chunk,decode_size);
                  for(x=0;x<decode_size;x++){
                     //write to ram
                     mem_write0(Address+x+y,decoded_data[x]);
                     printf("%i ",decoded_data[x]);
                  }
                  y=y+x;                     
                  free(decoded_data);

                }
                printf("OK:\n");
      } while (chunk>0);
}

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
    if (strcmp(linebuffer,"WHO")==0){
        printf("\nRC2040\n\n");
        state=WAITFORSTART;
        scmd=CMDNONE;
    }
    if (strcmp(linebuffer,"WATCH")==0){
        state=WAITFORADDRESS;
        scmd=CMDWATCH;
    }
    if (strcmp(linebuffer,"TRACE")==0){
        state=WAITFORADDRESS;
        scmd=CMDTRACE;
    }
    if (strcmp(linebuffer,"DUMP")==0){
        state=WAITFORADDRESS;
        scmd=CMDDUMP;
    }
    if (strcmp(linebuffer,"PROGRAM")==0){
        state=WAITFORADDRESS;
        scmd=CMDPROGRAM;
    }

    if (strcmp(linebuffer,"DISSEMBLE")==0){
        state=WAITFORADDRESS;
        scmd=CMDDISSEMBLE;
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


void WaitForAddress(void){
    printf("Address:\n");
    scanf("%x", &Address);
    if (DEBUG)   printf("%s\n",linebuffer);
    if (Address<=0xffff && Address>=0){
        state=CHECK;
    }else{
        sprintf(serr,"Address out of range %i",Address);
        state=CHECK;
    }
}

void WaitForSize(void){
    printf("Size:\n");
    scanf("%x", &Size);
    if (DEBUG)   printf("%s\n",linebuffer);
    if (Size+Address<=0xffff && Size>=0){
        state=CHECK;
    }else{
        sprintf(serr,"Size/Address out of range %i",Address);
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
        if(scmd==CMDWATCH) state=DODATA;
        if(scmd==CMDTRACE) state=DODATA;
        if(scmd==CMDDUMP) state=DODATA;
        if(scmd==CMDDISSEMBLE) state=DODATA;
        if(scmd==CMDPROGRAM) state=DODATA;        
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
/*
void CMDTestCPM(void){
    if (DEBUG) printf("TEST\n");
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
}
*/
void DoData(void){
   char * data;
     char filespec[2+8+1+3+1];
     char * file;
     static char filearg[13]="";
     static char * fargc[]={filearg};

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
  
//WATCH
    if(scmd==CMDWATCH){
      if (DEBUG) printf("WATCH\n");
      watch=Address;
      state=SENDEND;
      scmd=CMDNONE; 
    }

//TRACE
    if(scmd==CMDTRACE){
      if (DEBUG) printf("TRACE\n");
      trace=Address;
      state=SENDEND;
      scmd=CMDNONE; 
    }

//DODUMP
  if(scmd==CMDDUMP){
    if (DEBUG) printf("DUMP\n");
    DumpMemoryUSB(Address, 512);
    state=SENDEND;
    scmd=CMDNONE;
  }
  
//PROGRAM  
  if(scmd==CMDPROGRAM){
    if (DEBUG) printf("PROGRAM\n");
    Base64ToMEM(Address);
    state=RECEIVEEND;
    scmd=CMDNONE;

  }
  
//Disemble  
  if(scmd==CMDDISSEMBLE){
    DessembleMemoryUSB(Address, 64);
    state=SENDEND;
    scmd=CMDNONE;
  }

  cpmUmount(&drive);
  Device_f_sync();   
   
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
//         case TEST:
//           CMDTestCPM();   
//           break;
         case WAITFORADDRESS:
           WaitForAddress();  
           break;
         case EXIT:
           state=WAITFORSTART;
           exit=1;  
         default:
           break;
     }
  }
  printf ("\n\r EXIT FILE MODE \n\r");

}



