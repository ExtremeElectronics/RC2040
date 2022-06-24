/* #includes */ 
#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "device.h"
//sd card reader
#include "f_util.h"
#include "ff.h"
#include "hw_config.h"

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif


/* sync back to image without closing image file handle */
void Device_f_sync(void){
    f_sync(&fild);
}

/* use already open device FIL handle */
const char *Device_share_open(struct Device *self, const char *deviceOpts){
    self->opened=1;
////    self->fd=*fil;
    return ((char*)0);
}

/* Device_open           -- Open an image file     */          
const char *Device_f_open(struct Device *self, const char *filename, int mode,const char *deviceOpts)
{

    self->opened=f_open(&fild,filename,mode)==FR_OK;
//    self->fd=fil;
//  printf("Device Open?: %s %i \n\r",filename, self->opened);
    return ((!self->opened)?strerror(errno):(char*)0);
}
  
/* Device_setGeometry    -- Set disk geometry                       */ 
const char *Device_setGeometry(struct Device *self, int secLength, int sectrk, int tracks, off_t offset,const char *libdskGeometry)
{
    self->secLength=secLength;
    self->sectrk=sectrk;
    self->tracks=tracks;
    self->offset=offset;
//  printf(" secLen:%i secTrk:%i tracks:%i offset:%i \n\r",self->secLength=secLength, self->sectrk=sectrk,  self->tracks=tracks,  self->offset=offset);
    return NULL;
}

/* Device_close          -- Close an image file                     */ 
const char *Device_f_close(struct Device *self)
{
    self->opened=0;
    return ((f_close(&fild)!=FR_OK)?strerror(errno):(char*)0);
}

/* Device_readSector     -- read a physical sector                  */ 
const char *Device_readSector( struct Device *self, int track, int sector, char *buf)
{
    int res;
    UINT len;
    assert(self);
    assert(sector>=0);
    assert(sector<self->sectrk);
    assert(track>=0);
    assert(track<self->tracks);
    assert(buf);
    if (f_lseek(&fild,(off_t)(((sector+track*self->sectrk)*self->secLength)+self->offset))!=FR_OK)  {
        return strerror(errno);
    }
    res=f_read(&fild, buf, self->secLength,&len);
    if (len != self->secLength) {
        if (res!=FR_OK) {
            return strerror(errno);
        }else{
            printf("len %d\n",self->secLength-res);
            memset(buf+res,0,self->secLength-res); /* hit end of disk image */
        }
    }
  return ( char*)0;
}

/* Device_writeSector    -- write physical sector                */
const char *Device_writeSector(struct Device *self, int track, int sector,const char *buf)
{
    UINT len;
    assert(sector>=0);
    assert(sector<self->sectrk);
    assert(track>=0);
    assert(track<self->tracks);
    if (f_lseek(&fild,(off_t)(((sector+track*self->sectrk)*self->secLength)+self->offset))!=FR_OK){
        return strerror(errno);
    }
    f_write(&fild, buf, self->secLength,&len);
    f_sync(&fild);
    if (len == self->secLength) return (char*)0;
    return strerror(errno);
}

