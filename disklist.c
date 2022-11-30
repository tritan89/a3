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
#define timeOffset 14 //offset of creation time in directory entry
#define dateOffset 16 //offset of creation date in directory entry

void * print_files(char*disk, int start);
void print_date_time(char * directory_entry_startPos);
int get_file_size( char* disk, int addr);

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
    int logical_cluster;
    int size;

    while( disk[addr] != 0x00){
        logical_cluster = 0;
        memcpy(&logical_cluster,disk+addr+26,2);
        char filename[20];
        for (int i = 0; i < 11; ++i) {
            filename[i]= disk[addr+i];

        }
        filename[11] = '\0';
        int attr = disk[addr+11];

        if ((int)filename[0]!=0xE5 && (int)filename[0] != 0x00 && filename[0]!='.' && attr != 0x0F && (attr & 0x08) != 0x08 && logical_cluster != 0x00 && logical_cluster != 0x01){
            size = get_file_size(disk,addr);
            if((attr & 0x10) == 0x10) {
                printf("D %s\n",filename);
                print_files(disk,(logical_cluster+31)*512);

            } else{

                printf("FILE: %s\n",filename);
                printf("SIZE: %d\n",size);
                print_date_time(&disk[addr]);
            }
        }
        addr+=0x20;
    }




    return NULL;
}



void print_date_time(char * directory_entry_startPos){

    int time, date;
    int hours, minutes, day, month, year;

    time = *(unsigned short *)(directory_entry_startPos + timeOffset);
    date = *(unsigned short *)(directory_entry_startPos + dateOffset);

    //the year is stored as a value since 1980
    //the year is stored in the high seven bits
    year = ((date & 0xFE00) >> 9) + 1980;
    //the month is stored in the middle four bits
    month = (date & 0x1E0) >> 5;
    //the day is stored in the low five bits
    day = (date & 0x1F);

    printf("%d-%02d-%02d ", year, month, day);
    //the hours are stored in the high five bits
    hours = (time & 0xF800) >> 11;
    //the minutes are stored in the middle 6 bits
    minutes = (time & 0x7E0) >> 5;

    printf("%02d:%02d\n", hours, minutes);

    return ;
}

int get_file_size( char* disk, int addr){
    int size;
    memcpy(&size, disk+addr+28,4 );
    return size;



}
