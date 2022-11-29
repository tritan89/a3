
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>




char* get_label(char* disk, char* label);
char* get_os_name(char*disk, char* os );
int get_free_size(char* disk);
int num_files(char*disk, int start);

int main(int argc, char *argv[]) {
    int fd;
    struct stat sb;

    fd = open(argv[1], O_RDWR);
    fstat(fd, &sb);
    int disk_size = sb.st_size;

    char * disk_img = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // disk_img points to the starting pos of your mapped memory
    if (disk_img == MAP_FAILED) {
        printf("Error: failed to map memory\n");
        exit(1);
    }
    char os_name[9];
    get_os_name(disk_img,os_name);

    int freesize = get_free_size(disk_img);

    char label[12];
    get_label(disk_img,label);

    int files;
    files = num_files(disk_img,0x2600);

    char numFats = disk_img[16];
    int fat_sectors = disk_img[22] + (disk_img[23] << 8);

    printf("OS Name: %s\n", os_name);
    printf("Label of the disk: %s\n", label);
    printf("Total size of the disk: %d\n", disk_size);
    printf("Free size of the disk: %d\n\n", freesize);

    printf("==============\nThe number of files in the disk (including all files in the root directory and files in all subdirectories)\n%d\n==============\n", files);
    printf("Number of FAT copies: %d\n", numFats);
    printf("Sectors per FAT: %d\n", fat_sectors);
    munmap(disk_img, sb.st_size); // the modifed the memory data would be mapped to the disk image
    close(fd);


}


char* get_label(char* disk, char* label){
    // check boot sector then check volume check bit 43 and its 11 bytes long
    for (int i = 0; i < 11; i++) {
        label[i]= disk[i + 43];

    }
    int addr = 0x2600;
    if (label[0] == ' '){
        //go into first logo
        while (addr< 0x4200){
            int attr = disk[addr +11];
            if (attr == 0x08){
                for (int i = 0; i < 11; ++i) {
                    label[i]= disk[addr+i];

                }
                break;
            }
            addr += 0x20;
        }
    }
    //got to look from 19 -32 in the root dir

    label[11] = '\0';
    return label;
}
char* get_os_name(char* disk, char* os ){

    for (int i = 0; i < 8; i++) {
        os[i]= disk[i + 3];

    }
    os[8]= '\0';
    return os;

}

int get_free_size(char* disk){
    //start at beginning of root dir
    //start at secot 3
    int total_sectors = disk[19] +(disk[20] <<8) ; // get 16bit value

    int free_size=0;
    for (int i = 3; i <  total_sectors - 31; ++i) {// ignore last 31 sectors
        int free;
        if (i%2 != 0){
            // if odd then hi 4 are in 3*n/2
            int hi_4 = disk[512 + (3*i/2)] & 0xF0;
            int low_8 = disk[512 +1+(3*i/2)] & 0xFF;
            free = (hi_4 >>4) + (low_8 << 4);

        }
        else{
            // if even then the low 4 are in 1+3n/2
            int low_4 = disk[512 + 1+(3*i/2)] & 0x0F;
            int hi_8 = disk[512 +(3*i/2)] & 0xFF;
            free = (low_4 << 8) + hi_8;
        }
        if (free == 0x00){
            free_size++;
        }

    }
    return free_size*512;


}

int num_files(char*disk, int start){
    int numfiles=0;
    int addr = start;
    //char* filename;
    int perm = 0x2600;
    int temp;
    while( addr < 0x4200){

        char filename[9];
        for (int i = 0; i < 8; ++i) {
            filename[i]= disk[addr+i];

        }
        filename[8] = '\0';
        int attr = disk[addr+11];

        if ((int)filename[0]!=0xE5 && (int)filename[0] != 0x00 && filename[0]!='.' && attr != 0x0F && (attr & 0x08) != 0x08 ){
            numfiles++;

            if((attr & 0x10) == 0x10) {
                //printf("%s\n", filename);
                int logical_cluster = disk[addr + 26] +(disk[27] <<8);
                temp = num_files(disk,perm+logical_cluster+31);
                numfiles+= temp;
                numfiles--;
            }
        }
        addr+=0x20;
    }
    return numfiles;

}



