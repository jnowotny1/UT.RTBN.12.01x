// eFile.c
// Runs on either TM4C123 or MSP432
// High-level implementation of the file system implementation.
// Daniel and Jonathan Valvano
// August 29, 2016
#include <stdint.h>
#include "eDisk.h"

uint8_t Buff[512]; // temporary buffer used during file I/O
uint8_t Directory[256], FAT[256];
int32_t bDirectoryLoaded =0; // 0 means disk on ROM is complete, 1 means RAM version active

// Return the larger of two integers.
int16_t max(int16_t a, int16_t b){
  if(a > b){
    return a;
  }
  return b;
}
//*****MountDirectory******
// if directory and FAT are not loaded in RAM,
// bring it into RAM from disk
void MountDirectory(void){ 
// if bDirectoryLoaded is 0, 
//    read disk sector 255 and populate Directory and FAT
//    set bDirectoryLoaded=1
// if bDirectoryLoaded is 1, simply return
// **write this function**
	uint8_t result_ReadDisk;
	if (bDirectoryLoaded == 1){
		return;
	} else {
		result_ReadDisk = eDisk_ReadSector(Buff, 255);
		for (int i=0; i<=255; i++){
			Directory[i] = Buff[i];
			FAT[i] = Buff[i+256];
		}
		bDirectoryLoaded = 1;
	}
}

// Return the index of the last sector in the file
// associated with a given starting sector.
// Note: This function will loop forever without returning
// if the file has no end (i.e. the FAT is corrupted).
uint8_t lastsector(uint8_t start){
// **write this function**
	uint8_t next_sector;
  if (start != 255){
		next_sector = FAT[start];
		while (next_sector != 255){
			start = next_sector;
			next_sector = FAT[start];
		}
		return start;
	} else {
	return 255;
	}
}

// Return the index of the first free sector.
// Note: This function will loop forever without returning
// if a file has no end or if (Directory[255] != 255)
// (i.e. the FAT is corrupted).
uint8_t findfreesector(void){
// **write this function**
  int16_t free_sector = -1;
	int16_t last_sector;
	uint8_t i = 0;
	
	last_sector = lastsector(Directory[i]);
	while (last_sector != 255){
			free_sector = max(free_sector, last_sector);
			i++;
			last_sector = lastsector(Directory[i]);
	}
	return ((uint8_t)free_sector + 1);
}

// Append a sector index 'n' at the end of file 'num'.
// This helper function is part of OS_File_Append(), which
// should have already verified that there is free space,
// so it always returns 0 (successful).
// Note: This function will loop forever without returning
// if the file has no end (i.e. the FAT is corrupted).
uint8_t appendfat(uint8_t num, uint8_t n){
// **write this function**
  uint8_t i, m;
	i = Directory[num];									// i = file number
	if (i == 255){											// if i = 255, then file is empty
		Directory[num] = n;									// place n in Directory
		return 0;													
	} else {
		m = FAT[i];									// if i != 0, get next sector from FAT and put in m
		while (m != 255){										// if m != 255, then put m into i
			i = m;
			m = FAT[i];							// and get the next sector from FAT
		}
		FAT[i] = n;													// if m = 255, then m is empty
		return 0;
	}
}

//********OS_File_New*************
// Returns a file number of a new file for writing
// Inputs: none
// Outputs: number of a new file
// Errors: return 255 on failure or disk full
uint8_t OS_File_New(void){
// **write this function**
  uint8_t i = 0;
	if (bDirectoryLoaded == 0){							// Bring DIR and FAT from ROM to RAM if needed
		MountDirectory();
	}			
	while (Directory[i] != 255){						// if Directory[0] = 255, then 0 is first file
		i++;
		if (i == 255){
			return 255;													// if i = 255, the disk is full
		}
	}
  return i;
}

//********OS_File_Size*************
// Check the size of this file
// Inputs:  num, 8-bit file number, 0 to 254
// Outputs: 0 if empty, otherwise the number of sectors
// Errors:  none
uint8_t OS_File_Size(uint8_t num){
// **write this function**
  uint8_t size_count = 0;
	uint8_t next_sector = 0;
	
	next_sector = Directory[num];	
	if (next_sector == 255){
		return 0;
	}
	size_count++;
	while (FAT[next_sector] != 255){									// search the FAT for 255 while incrementing size_count per each FAT entry
		next_sector = FAT[next_sector];
		size_count++;
	}
  return size_count; 
}

//********OS_File_Append*************
// Save 512 bytes into the file
// Inputs:  num, 8-bit file number, 0 to 254
//          buf, pointer to 512 bytes of data
// Outputs: 0 if successful
// Errors:  255 on failure or disk full
uint8_t OS_File_Append(uint8_t num, uint8_t buf[512]){
// **write this function**
	uint8_t free_sector;
	uint8_t result_WriteSector;
	if (bDirectoryLoaded == 0){							// Bring DIR and FAT from ROM to RAM if needed
		MountDirectory();
	}
	free_sector = findfreesector();
	if (free_sector != 255){
		result_WriteSector = eDisk_WriteSector(buf, free_sector);
		if (result_WriteSector != 0){
			return 255;
		}
		appendfat(num,free_sector);
		return 0;
	}
  return 255; 
}

//********OS_File_Read*************
// Read 512 bytes from the file
// Inputs:  num, 8-bit file number, 0 to 254
//          location, logical address, 0 to 254
//          buf, pointer to 512 empty spaces in RAM
// Outputs: 0 if successful
// Errors:  255 on failure because no data
uint8_t OS_File_Read(uint8_t num, uint8_t location,
                     uint8_t buf[512]){
// **write this function**
  uint8_t next_sector = 0;
	uint8_t current_sector = 0;
	uint8_t sector_count = 0;

	if (bDirectoryLoaded == 0){							// Bring DIR and FAT from ROM to RAM if needed
		MountDirectory();
	}	

	current_sector = Directory[num];

	while (location != sector_count){
		next_sector = FAT[current_sector];
		current_sector = next_sector;
		if (next_sector != 255){
			sector_count++;
			} else {
				return 255;
			}
	}
  return eDisk_ReadSector(buf, current_sector); // replace this line
}

//********OS_File_Flush*************
// Update working buffers onto the disk
// Power can be removed after calling flush
// Inputs:  none
// Outputs: 0 if success
// Errors:  255 on disk write failure
uint8_t OS_File_Flush(void){
// **write this function**
	uint8_t result_FileFlush;
	for (int i=0; i<=255; i++){
		Buff[i] = Directory[i];
		Buff[i+256] = FAT[i];
	}
	result_FileFlush = eDisk_WriteSector(Buff, 255);
	if (result_FileFlush == 0){
		return 0;
	} else {
		return 255;
	}
}

//********OS_File_Format*************
// Erase all files and all data
// Inputs:  none
// Outputs: 0 if success
// Errors:  255 on disk write failure
uint8_t OS_File_Format(void){
// call eDiskFormat
// clear bDirectoryLoaded to zero
// **write this function**
	uint8_t result_DiskFormat;
	bDirectoryLoaded = 0;
  result_DiskFormat = eDisk_Format();
	for (int i=0; i<256; i++){
		Directory[i] = 0xFF;
		FAT[i] = 0xFF;
	}
	if (result_DiskFormat == 0){
		return 0;
	}else{
		return 255;
	}
}

