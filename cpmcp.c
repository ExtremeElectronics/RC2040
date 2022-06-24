/* #includes */ /*{{{C}}}*//*{{{*/
#include "config.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <utime.h>

//#include "getopt_.h"
#include "cpmfs.h"

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

//#define DEBUG 0

//const char cmd[]="cpmcp";
static int text=0;
static int preserve=0;

/**
 * Return the user number.
 * @param s CP/M filename in 0[0]:aaaaaaaa.bbb format.
 * @returns The user number or -1 for no match.
 */
static int userNumber(const char *s){
  if (isdigit(*s) && *(s+1)==':') return (*s-'0');
  if (isdigit(*s) && isdigit(*(s+1)) && *(s+2)==':') return (10*(*s-'0')+(*(s+1)-'0'));
  return -1;
}

/**
 * Copy one file from CP/M to UNIX.
 * @param root The inode for the root directory.
 * @param src  The CP/M filename in 00aaaaaaaabbb format.
 * @param dest The UNIX filename.
 * @returns 0 for success, 1 for error.
 */
static int cpmToUnix(struct cpmInode *root, const char *src, const char *dest) /*{{{*/
{
  struct cpmInode ino;
  int exitcode=0;

  if (cpmNamei(root,src,&ino)==-1) { printf("can not open `%s': %s\n",src,boo); exitcode=1; }
  else
  {
    struct cpmFile file;
    FILE *ufp;

    cpmOpen(&ino,&file,O_RDONLY);
    if ((ufp=fopen(dest,text ? "w" : "wb"))==(FILE*)0) { printf(" can not create %s: %s\n",dest,strerror(errno)); exitcode=1; }
    else
    {
      int crpending=0;
      int ohno=0;
      int res;
      char buf[4096];

      while ((res=cpmRead(&file,buf,sizeof(buf)))>0)
      {
        int j;

        for (j=0; j<res; ++j)
        {
          if (text)
          {
            if (buf[j]=='\032') goto endwhile;
            if (crpending)
            {
              if (buf[j]=='\n') 
              {
                if (putc('\n',ufp)==EOF) { printf("can not write %s: %s\n",dest,strerror(errno)); exitcode=1; ohno=1; goto endwhile; }
                crpending=0;
              }
              else if (putc('\r',ufp)==EOF) { printf(" can not write %s: %s\n",dest,strerror(errno)); exitcode=1; ohno=1; goto endwhile; }
              crpending=(buf[j]=='\r');
            }
            else
            {
              if (buf[j]=='\r') crpending=1;
              else if (putc(buf[j],ufp)==EOF) { printf(" can not write %s: %s\n",dest,strerror(errno)); exitcode=1; ohno=1; goto endwhile; }
            }
          }
          else if (putc(buf[j],ufp)==EOF) { printf(" can not write %s: %s\n",dest,strerror(errno)); exitcode=1; ohno=1; goto endwhile; }
        }
      }
      endwhile:
      if (res==-1 && !ohno) { printf("can not read %s (%s)\n",src,boo); exitcode=1; ohno=1; }
      if (fclose(ufp)==EOF && !ohno) { printf("can not close %s: %s\n",dest,strerror(errno)); exitcode=1; ohno=1; }
      if (preserve && !ohno && (ino.atime || ino.mtime))
      {
        struct utimbuf ut;

        if (ino.atime) ut.actime=ino.atime; else time(&ut.actime);
        if (ino.mtime) ut.modtime=ino.mtime; else time(&ut.modtime);
//        if (utime(dest,&ut)==-1) { printf("can change timestamps of %s: %s\n",dest,strerror(errno)); exitcode=1; ohno=1; }
      }
    }
    cpmClose(&file);
  }
  return exitcode;
}
/*}}}*/



/**
 * Copy one file from CP/M to serial (via base64encoding).
 * @param root The inode for the root directory.
 * @param src  The CP/M filename in 00aaaaaaaabbb format.
 * @returns 0 for success, 1 for error.
 */
int cpmToBase64(const struct cpmInode *root, const char *src) /*{{{*/
{
  struct cpmInode ino;
  int exitcode=0;

  if (cpmNamei(root,src,&ino)==-1) { printf("can not open `%s': %s\n",src,boo); exitcode=1; }
  else
  {
      struct cpmFile file;
      FILE *ufp;

      cpmOpen(&ino,&file,O_RDONLY);
      int crpending=0;
      int ohno=0;
      size_t res;
      char buf[3*1024]; //must be a multiple of 3 and 4

      printf("\n%s\n",src);
      while ((res=cpmRead(&file,buf,sizeof(buf)))>0)
      {
         send_base64_encode(buf, res);      
      }
      endwhile:
      printf("\n");
      if (res==-1 && !ohno) { printf("can not read %s (%s)\n",src,boo); exitcode=1; ohno=1; }
      if (fclose(ufp)==EOF && !ohno) { printf("can not close %s\n",strerror(errno)); exitcode=1; ohno=1; }
      cpmClose(&file);
  }
  return exitcode;
}
/*}}}*/


void TestToCPM( struct cpmInode *root,  const char *filename){
  
    int chunk;
      char *dest=(char*)0;
      struct cpmInode ino;
      char cpmname[2+8+1+3+1]; /* 00foobarxy.zzy\0 */
      char encbuffer[4096+4];

      snprintf(cpmname,sizeof(cpmname),"00%s",filename);

      if (cpmCreat(root,cpmname,&ino,0666)==-1) /* just cry */ /*{{{*/
      {
           printf(" can not create %s: %s\n",cpmname,boo);
      }
      else
      {
          struct cpmFile file;
          int ohno=0;

          cpmOpen(&ino,&file,O_WRONLY);

          printf("Data:\n");

          sprintf(encbuffer,"The Quick Brown Fox Jumps Over The lazy dog");
          size_t chunk = strlen(encbuffer);
          if (cpmWrite(&file,encbuffer,chunk)!=(ssize_t)chunk)
          {
                printf("can not write %s: %s\n",dest,boo);
                ohno=1;
          }

          printf("OK:\n");
          if (cpmClose(&file)==EOF && !ohno) /* I just can't hold back the tears */ /*{{{*/
          {
            printf("can not close %s: %s\n",dest,boo);
          }
   }

}

void Base64ToCPM( struct cpmInode *root,  const char *filename){

      int chunk;
      char *dest=(char*)0;
      struct cpmInode ino;
      char cpmname[2+8+1+3+1]; /* 00foobarxy.zzy\0 */
      char encbuffer[4096+4]; 

      snprintf(cpmname,sizeof(cpmname),"00%s",filename);
      if (DEBUG) printf("Create %s\n\r",cpmname);
      if (cpmCreat(root,cpmname,&ino,0666)==-1) /* just cry */ /*{{{*/
      {
           printf(" can not create %s: %s\n",cpmname,boo);
          //exitcode=1;
      }
      else
      {
          struct cpmFile file;
          int ohno=0;

          cpmOpen(&ino,&file,O_WRONLY);
          do{
              printf("Chunksize:\n");
              scanf("%d", &chunk);
              if (DEBUG) printf("Chunk:%i",chunk);
              if(chunk>0){
                  printf("Data:\n");
                  scanf("%4100s",encbuffer);
                  if (DEBUG) printf("Data:%s",encbuffer);
                  size_t decode_size = strlen(encbuffer);
                  char * decoded_data = base64_decode(encbuffer, decode_size, &decode_size);
                  if (DEBUG) printf("%s\n\r",decoded_data);       
                  if (DEBUG) printf("Write %i %i %s\n",chunk,decode_size,dest);
//                  sprintf(encbuffer,"The Quick Brown Fox Jumps Over The lazy dog");
//                  size_t chunk = strlen(encbuffer);
                  if (cpmWrite(&file,decoded_data,decode_size)!=(ssize_t)decode_size)
//                  if (cpmWrite(&file,encbuffer,chunk)!=(ssize_t)chunk)
                  {
                      printf("Can not write %i %i %s: %s\n",chunk,decode_size,dest,boo);
                      ohno=1;
                      chunk=0;
                  }
                  free(decoded_data);

                }  
                printf("OK:\n");
          } while (chunk>0);
          
          if (cpmClose(&file)==EOF && !ohno) /* I just can't hold back the tears */ /*{{{*/
          {
              printf("can not close %s: %s\n",dest,boo);
              //exitcode=1;
          }
      }

}














static void usage(void) /*{{{*/
{
  printf("Usage: [-f format] [-p] [-t] image user:file file\n");
  printf("       [-f format] [-p] [-t] image user:file ... directory\n");
  printf("       [-f format] [-p] [-t] image file user:file\n");
  printf("       [-f format] [-p] [-t] image file ... user:\n");
  exit(1);
}
/*}}}*/

int cpmain(int argc, char *argv[])
{
  /* variables */ /*{{{*/
  const char *err;
  const char *image;
  const char *format;
  const char *devopts=NULL;
  int c,readcpm=-1,todir=-1;
  struct cpmInode root;
  struct cpmSuperBlock super;
  int exitcode=0;
  int gargc;
  char **gargv;
  /*}}}*/

  /* parse options */ /*{{{*/
/*  if (!(format=getenv("CPMTOOLSFMT"))) format=FORMAT;
  while ((c=getopt(argc,argv,"T:f:h?pt"))!=EOF) switch(c)
  {
    case 'T': devopts=optarg; break;
    case 'f': format=optarg; break;
    case 'h':
    case '?': usage(); break;
    case 'p': preserve=1; break;
    case 't': text=1; break;
  }
*/  
  /*}}}*/
  /* parse arguments */ /*{{{*/
  if ((optind+2)>=argc) usage();
  image=argv[optind++];

  if (userNumber(argv[optind])>=0) /* cpm -> unix? */ /*{{{*/
  {
    int i;
    struct stat statbuf;

    for (i=optind; i<(argc-1); ++i) if (userNumber(argv[i])==-1) usage();
    todir=((argc-optind)>2);
    if (stat(argv[argc-1],&statbuf)==-1) { if (todir) usage(); }
    else if (S_ISDIR(statbuf.st_mode)) todir=1; else if (todir) usage();
    readcpm=1;
  }
  /*}}}*/
  else if (userNumber(argv[argc-1])>=0) /* unix -> cpm */ /*{{{*/
  {
    int i;

    todir=0;
    for (i=optind; i<(argc-1); ++i) if (userNumber(argv[i])>=0) usage();
    if ((argc-optind)>2 && *(strchr(argv[argc-1],':')+1)!='\0') usage();
    if (*(strchr(argv[argc-1],':')+1)=='\0') todir=1;
    readcpm=0;
  }
  /*}}}*/
  else usage();
  /*}}}*/
  /* open image file */ /*{{{*/
  if ((err=Device_open(&super.dev,image,readcpm ? O_RDONLY : O_RDWR, devopts)))
  {
    printf(" cannot open %s (%s)\n",image,err);
    exit(1);
  }
  if (cpmReadSuper(&super,&root,format)==-1)
  {
    printf("cannot read superblock (%s)\n",boo);
    exit(1);
  }
  /*}}}*/
  if (readcpm) /* copy from CP/M to UNIX */ /*{{{*/
  {
    int i;
    char *last=argv[argc-1];
    
    cpmglob(optind,argc-1,argv,&root,&gargc,&gargv);
    /* trying to copy multiple files to a file? */
    if (gargc>1 && !todir) usage();
    for (i=0; i<gargc; ++i)
    {
      char dest[_POSIX_PATH_MAX];

      if (todir)
      {
        strcpy(dest,last);
        strcat(dest,"/");
        strcat(dest,gargv[i]+2);
      }
      else strcpy(dest,last);
      if (cpmToUnix(&root,gargv[i],dest)) exitcode=1;
    }
  }
  /*}}}*/
  else /* copy from UNIX to CP/M */ /*{{{*/
  {
    int i;

    for (i=optind; i<(argc-1); ++i)
    {
      /* variables */ /*{{{*/
      char *dest=(char*)0;
      FILE *ufp;
      /*}}}*/

      if ((ufp=fopen(argv[i],"rb"))==(FILE*)0) /* cry a little */ /*{{{*/
      {
        printf(" can not open %s: %s\n",argv[i],strerror(errno));
        exitcode=1;
      }
      /*}}}*/
      else
      {
        struct cpmInode ino;
        char cpmname[2+8+1+3+1]; /* 00foobarxy.zzy\0 */
        struct stat st;

        stat(argv[i],&st);

        if (todir)
        {
          if ((dest=strrchr(argv[i],'/'))!=(char*)0) ++dest; else dest=argv[i];
          snprintf(cpmname,sizeof(cpmname),"%02d%s",userNumber(argv[argc-1]),dest);
        }
        else
        {
          snprintf(cpmname,sizeof(cpmname),"%02d%s",userNumber(argv[argc-1]),strchr(argv[argc-1],':')+1);
        }
        if (cpmCreat(&root,cpmname,&ino,0666)==-1) /* just cry */ /*{{{*/
        {
          printf(" can not create %s: %s\n",cpmname,boo);
          exitcode=1;
        }
        /*}}}*/
        else
        {
          struct cpmFile file;
          int ohno=0;
          char buf[4096+1];

          cpmOpen(&ino,&file,O_WRONLY);
          do
          {
            unsigned int j;

            for (j=0; j<(sizeof(buf)/2) && (c=getc(ufp))!=EOF; ++j)
            {
              if (text && c=='\n') buf[j++]='\r';
              buf[j]=c;
            }
            if (text && c==EOF) buf[j++]='\032';
            if (cpmWrite(&file,buf,j)!=(ssize_t)j)
            {
              printf(" can not write %s: %s\n",dest,boo);
              ohno=1;
              exitcode=1;
              break;
            }
          } while (c!=EOF);
          if (cpmClose(&file)==EOF && !ohno) /* I just can't hold back the tears */ /*{{{*/
          {
            printf("%s: can not close %s: %s\n",cmd,dest,boo);
            exitcode=1;
          }
          /*}}}*/
          if (preserve && !ohno)
          {
            struct utimbuf times;
            times.actime=st.st_atime;
            times.modtime=st.st_mtime;
            cpmUtime(&ino,&times);
          }
        }
        fclose(ufp);
      }
    }
  }
  /*}}}*/
  cpmUmount(&super);
  exit(exitcode);
}
