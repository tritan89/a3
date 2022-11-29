//
// Created by Langer on 25/11/2022.
//



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

void * print_files(char*disk, int start);


int main(int argc, char *argv[]) {
    int fd;
    struct stat sb;

    fd = open(argv[1], O_RDWR);
    fstat(fd, &sb);
    char * disk_img = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // disk_img points to the starting pos of your mapped memory
    if (disk_img == MAP_FAILED) {
        printf("Error: failed to map memory\n");
        exit(1);
    }

    print_files(disk_img, 0x2600);

    munmap(disk_img, sb.st_size); // the modifed the memory data would be mapped to the disk image
    close(fd);


}

void * print_files(char*disk, int start){

    int addr = start;
    int perm = 0x2600;

    while( addr < 0x4200){

        char filename[9];
        for (int i = 0; i < 8; ++i) {
            filename[i]= disk[addr+i];

        }
        filename[8] = '\0';
        int attr = disk[addr+11];

        if ((int)filename[0]!=0xE5 && (int)filename[0] != 0x00 && filename[0]!='.' && attr != 0x0F && (attr & 0x08) != 0x08 ){

            printf("%s\n", filename);
            if((attr & 0x10) == 0x10) {

                int logical_cluster = disk[addr+26] +(disk[27] <<8);
                print_files(disk,perm+logical_cluster+31);

            }
        }
        addr+=0x20;
    }


    return NULL;
}