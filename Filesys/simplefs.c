#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "simplefs.h"

#define BLOCKSIZE 1024  // Размер блока - 1 KB
#define MAX_FILES 52    // Максимальное количество файлов
#define FILENAME_LEN 32 // Длина имени файла (включая '\0')
#define FAT_ENTRIES 128 * 1024 // Размер FAT таблицы (128KB)

int vdisk_fd; // global virtual disk file descriptor
              // will be assigned with the sfs_mount call
              // any function in this file can use this.
              
typedef struct {
    int disk_size;
    int num_blocks;
    int root_dir_start;
    int fat_start;
    int data_start;
} SuperBlock;

typedef struct {
    char filename[FILENAME_LEN];
    int size;
    int first_block;
    int is_used;
} DirectoryEntry;

typedef struct {
    int next_block;
} FATEntry;

typedef struct {
    int file_index;
    int mode;
    int current_position;
} OpenFileEntry;

SuperBlock super_block;
DirectoryEntry root_dir[MAX_FILES];
FATEntry fat_table[FAT_ENTRIES];
OpenFileEntry open_files[10]; // Таблица открытых файлов

// This function is simply used to a create a virtual disk
// (a simple Linux file including all zeros) of the specified size.
// You can call this function from an app to create a virtual disk.
// There are other ways of creating a virtual disk (a Linux file)
// of certain size.
// size = 2^m Bytes
int create_vdisk (char *vdiskname, int m)
{
    char command[BLOCKSIZE];
    int size;
    int num = 1;
    int count;
    size  = num << m;
    count = size / BLOCKSIZE;
    printf ("%d %d\n", m, size);
    sprintf (command, "dd if=/dev/zero of=%s bs=%d count=%d", vdiskname, BLOCKSIZE, count);
    printf ("executing command = %s\n", command);
    system (command);
    return (0);
}



// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk.
int read_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = read (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
        printf ("read error\n");
        return -1;
    }
    return (0);
}

// write block k into the virtual disk.
int write_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = write (vdisk_fd, block, BLOCKSIZE);            
    if (n != BLOCKSIZE) {
        printf ("write error\n");
        return (-1);
    }
    return 0;
}


/**********************************************************************
   The following functions are to be called by applications directly.
***********************************************************************/

int sfs_format(char *vdiskname) {
    vdisk_fd = open(vdiskname, O_RDWR);
    if (vdisk_fd == -1) return -1;
    
    for (int j = 0; j < 10; j++) {
         open_files[j].file_index = -1;
    }

    // Инициализация суперблока
    super_block.disk_size = lseek(vdisk_fd, 0, SEEK_END);
    super_block.num_blocks = super_block.disk_size / BLOCKSIZE;
    super_block.root_dir_start = 1;
    super_block.fat_start = 8;
    super_block.data_start = 1032;
 
    // Инициализация корневого каталога
    memset(root_dir, 0, sizeof(root_dir));
    for (int i = 0; i < MAX_FILES; i++) {
        root_dir[i].is_used = 0;
    }

    // Инициализация FAT таблицы
    for (int i = 0; i < FAT_ENTRIES; i++) {
        fat_table[i].next_block = -1;
    }

    // Запись структуры на диск
    write_block(&super_block, 0);
    for (int i = 1; i < 8; i++) {
        if(write_block(&root_dir[0], i) == -1) return -1;
    }

    for (int i = 8; i < 1032; i++) {
        if(write_block(&fat_table[0], i)== -1) return -1;
    }
    printf("The %s disk has been successfully formatted\n" , vdiskname);
    close(vdisk_fd);
    return 0;
}


int sfs_mount (char *vdiskname)
{
    // simply open the Linux file vdiskname and in this
    // way make it ready to be used for other operations.
    // vdisk_fd is global; hence other function can use it.
    vdisk_fd = open(vdiskname, O_RDWR);
    if(vdisk_fd == -1) return -1;
    printf("The %s disk has been successfully mounted\n" , vdiskname);
    return(0);
}

int sfs_umount ()
{
    fsync (vdisk_fd);
    close (vdisk_fd);
    printf("The disk has been successfully umounted\n");
    return (0);
}


int sfs_create(char *filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (root_dir[i].is_used == 0) {
            root_dir[i].is_used = 1;
            strncpy(root_dir[i].filename, filename, FILENAME_LEN);
            root_dir[i].size = 0;
            root_dir[i].first_block = -1;
            printf("The %s file has been successfully created\n" , filename);
            return 0;
        }
    }
    return -1;
}



int sfs_open(char *filename, int mode) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (root_dir[i].is_used && strcmp(root_dir[i].filename, filename) == 0) {
            for (int j = 0; j < 10; j++) {
                if (open_files[j].file_index == -1) {
                    open_files[j].file_index = i;
                    open_files[j].mode = mode;
                    open_files[j].current_position = (mode == 0) ? 0 : root_dir[i].size;
                    printf("The %s file has been successfully openned, mode: %d \n" , filename, mode);
                    return j; // Вернём индекс как файловый дескриптор
                }
            }
        }
    }
    return -1;
}


int sfs_close(int fd) {
    if (fd < 0 || fd >= 10 || open_files[fd].file_index == -1) {
        return -1;
    }
    open_files[fd].file_index = -1; // Освобождаем запись
    printf("The file has been successfully closed\n");
    return 0;
}


int sfs_getsize(int fd) {
    if (fd < 0 || fd >= 10 || open_files[fd].file_index == -1) {
        return -1;
    }
    int file_index = open_files[fd].file_index;
    return root_dir[file_index].size;
}


int sfs_read(int fd, void *buf, int n) {
    if (fd < 0 || fd >= 10 || open_files[fd].file_index == -1 || open_files[fd].mode != 0) {
        return -1;
    }

    int file_index = open_files[fd].file_index;
    int bytes_to_read = (root_dir[file_index].size - open_files[fd].current_position < n) 
                        ? root_dir[file_index].size - open_files[fd].current_position 
                        : n;
    
    int read_bytes = 0;
    int current_block = root_dir[file_index].first_block;
    int offset = open_files[fd].current_position;

    while (offset >= BLOCKSIZE && current_block != -1) {
        current_block = fat_table[current_block].next_block;
        offset -= BLOCKSIZE;
    }

    char block[BLOCKSIZE];
    while (bytes_to_read > 0 && current_block != -1) {
        read_block(block, current_block);
	printf("Read block %d: %s\n", current_block, block);
        int to_copy = (bytes_to_read < BLOCKSIZE - offset) ? bytes_to_read : BLOCKSIZE - offset;
        memcpy((char *)buf + read_bytes, block + offset, to_copy);

        read_bytes += to_copy;
        bytes_to_read -= to_copy;
        offset = 0;

        current_block = fat_table[current_block].next_block;
    }

    open_files[fd].current_position += read_bytes;
    return read_bytes;
}


int sfs_append(int fd, void *buf, int n) {
    if (fd < 0 || fd >= 10 || open_files[fd].file_index == -1 || open_files[fd].mode != 1) {
        return -1;
    }

    int file_index = open_files[fd].file_index;
    int bytes_to_write = n;
    int written_bytes = 0;
    int current_block = root_dir[file_index].first_block;

    if (current_block == -1) {
        for (int i = 0; i < FAT_ENTRIES; i++) {
            if (fat_table[i].next_block == -1) {
                root_dir[file_index].first_block = i;
                current_block = i;
                fat_table[i].next_block = 0;
                break;
            }
        }
    }

    int offset = root_dir[file_index].size % BLOCKSIZE;
    char block[BLOCKSIZE];

    if (offset > 0) {
        read_block(block, current_block);
    } else {
        memset(block, 0, BLOCKSIZE);
    }

    while (bytes_to_write > 0) {
        int to_copy = (bytes_to_write < BLOCKSIZE - offset) ? bytes_to_write : BLOCKSIZE - offset;
        memcpy(block + offset, (char *)buf + written_bytes, to_copy);

        written_bytes += to_copy;
        bytes_to_write -= to_copy;
        offset = 0;

        write_block(block, current_block);
        printf("Write block %d: %s\n", current_block, block);
        if (bytes_to_write > 0) {
            int new_block = -1;
            for (int i = 0; i < FAT_ENTRIES; i++) {
                if (fat_table[i].next_block == -1) {
                    new_block = i;
                    fat_table[current_block].next_block = new_block;
                    fat_table[new_block].next_block = 0;
                    current_block = new_block;
                    memset(block, 0, BLOCKSIZE);
                    break;
                }
            }
            if (new_block == -1) return -1; // Ошибка: нет свободных блоков
        }
    }

    root_dir[file_index].size += written_bytes;
    open_files[fd].current_position = root_dir[file_index].size;
    return written_bytes;
}


int sfs_delete(char *filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (root_dir[i].is_used && strcmp(root_dir[i].filename, filename) == 0) {
            int current_block = root_dir[i].first_block;
            while (current_block != -1) {
                int next_block = fat_table[current_block].next_block;
                fat_table[current_block].next_block = -1; // Освобождаем блок
                current_block = next_block;
            }
            root_dir[i].is_used = 0;
                printf("The %s file has been successfully deleted\n", filename);
            return 0;
        }
    }
    return -1;
}


