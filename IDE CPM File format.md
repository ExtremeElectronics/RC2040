## CF file format

The CF format was designed for a CompactFlash card by Grant Searle.
It encapsulates multiple partitions with a 1K header file indicating the positions sizes and geometry of CP/M drives A-P into a single image.

## img file format

The img file format is similar to the above, but the geometry is assumed to be a standard therefore the first 1k is not needed.
But of course you still need to know the geometry (identity) of the drives so this is extracted and given to the system as a 1K identity (.id) file

The advantage of the img format is there are a number of external tools that use this format.

## Tools

### CPM Disk manager

A GUI CP/M img file manager. Although it (currently) has some bugs it greatly eases transferring files from a PC to an img file

https://github.com/abaffa/cpm_disk_manager

Note make sure your source files have the read only flag unset, it crashes CP/M disk manager.
![Screenshot 2022-04-15 152810](https://user-images.githubusercontent.com/102665314/163608884-c9f2418c-757b-41b6-bf59-3e38db019dcd.jpg)

### CPM tools

You can use this suite of CLI tools on both Windows and Linux. You will need a diskdef file, but you can use them with either .cf or .img files

http://www.moria.de/~michael/cpmtools/

The Disk Def files can be found in their own folder.
