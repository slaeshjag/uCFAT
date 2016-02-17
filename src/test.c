#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "fat.h"

FILE *fp;

const char *attribs = "RHSLDA";

int write_sector_call(uint32_t sector, uint8_t *data) {
	fseek(fp, sector * 512, SEEK_SET);
	if (ftell(fp) != sector * 512)
		return -1;
	(void)fwrite(data, 512, 1, fp);
	if (ftell(fp) != (sector + 1) * 512)
		return -1;
	return 0;
}


int read_sector_call(uint32_t sector, uint8_t *data) {
	fseek(fp, sector * 512, SEEK_SET);
	if (ftell(fp) != sector * 512)
		return -1;
	(void)fread(data, 512, 1, fp);
	if (ftell(fp) != (sector + 1) * 512)
		return -1;
	return 0;
}

static void print_filesize(uint32_t filesize) {
	if(filesize < 1024)
		printf("%u", filesize);
	else if(filesize < 1024*1024)
		printf("%uk", filesize/1024U);
	else
		printf("%uM", filesize/(1024U*1024U));
}

static void strtoupper(char *str) {
	while(*str) {
		*str = toupper(*str);
		str++;
	}
}

int main(int argc, char **argv) {
	int fd, i, files, j, k;
	uint8_t buff[512];
	char path[256] = "";
	char pathbuf[256];
	char cmdline[300] = {};
	uint32_t size, tmp;
	uint8_t stat;
	
	char *cmd, *filename;
	
	struct FATDirList list[8];
	
	if(argc < 2) {
		fprintf(stderr, "Usage: test /path/to/image.img\n");
		return 1;
	}
	
	sector_buff = buff;
	fp = fopen(argv[1], "r+");
	init_fat();
	
	for(;;) {
		printf("%s$ ", *path ? path : "/");
		fflush(stdout);
		do {
			if(!fgets(cmdline, 300, stdin))
				break;
		} while(cmdline[strlen(cmdline) - 1] != '\n');
		
		if(!(cmd = strtok(cmdline, " \r\n")))
			continue;
		
		if(!strcmp(cmd, "cd")) {
			if(!(filename = strtok(NULL, " \r\n")))
				continue;
			strtoupper(filename);
			
			if(!strcmp(filename, "."))
				continue;
			if(!strcmp(filename, "..") && strcmp(path, "/")) {
				goup:
				*strrchr(path, '/') = 0;
				continue;
			}
			
			strcat(path, "/");
			strcat(path, filename);
			
			stat = fat_get_stat(path);
			if(stat == 0xFF || !(stat & 0x10)) {
				fprintf(stderr, "Error: %s is not a directory\n", filename);
				goto goup;
			}
		} else if(!strcmp(cmd, "pwd")) {
			if(*path)
				puts(path);
			else
				puts("/");
		} else if(!strcmp(cmd, "ls")) {
			for(files = 0; (i = fat_dirlist(path, list, 8, files)); files += i) {
				for (j = i - 1; j >= 0; j--) {
					if(list[j].filename[0]) {
						stat = list[j].attrib;
						for(k = 5; k != ~0; k--) {
							if(stat & (0x1 << k))
								putchar(attribs[k]);
							else
								putchar('-');
						}
						if(stat & 0x10) {
							printf("\t\t");
						} else {
							sprintf((char *) pathbuf, "%s/%s", path, list[j].filename);
							fd = fat_open(pathbuf, O_RDONLY);
							printf("\t");
							print_filesize(fat_fsize(fd));
							printf("\t");
							fat_close(fd);
						}
						printf("%s\n", list[j].filename);
					}
				}
			}
		} else if(!strcmp(cmd, "cat")) {
			if(!(filename = strtok(NULL, " \r\n")))
				continue;
			strtoupper(filename);
			
			sprintf((char *) pathbuf, "%s/%s", path, filename);
			if((fd = fat_open((char *) pathbuf, O_RDONLY)) < 0)
				continue;
			
			for(size = fat_fsize(fd); (size/512); size -= 512) {
				fat_read_sect(fd);
				fwrite(sector_buff, 1, 512, stdout);
			}
			if(size) {
				fat_read_sect(fd);
				fwrite(sector_buff, 1, size, stdout);
			}
			
			fat_close(fd);
		} else if(!strcmp(cmd, "rm")) {
			if(!(filename = strtok(NULL, " \r\n")))
				continue;
			strtoupper(filename);
			
			sprintf((char *) pathbuf, "%s/%s", path, filename);
			stat = fat_get_stat(pathbuf);
			if(stat == 0xFF || (stat & 0x10)) {
				fprintf(stderr, "Error: %s does not exist or is a directory\n", filename);
				continue;
			}
			delete_file(pathbuf);
		} else if(!strcmp(cmd, "touch")) {
			if(!(filename = strtok(NULL, " \r\n")))
				continue;
			strtoupper(filename);
			
			create_file(path, filename, 0x20);
		} else if(!strcmp(cmd, "mkdir")) {
			if(!(filename = strtok(NULL, " \r\n")))
				continue;
			strtoupper(filename);
			
			create_file(path, filename, 0x10);
		} else if(!strcmp(cmd, "rmdir")) {
			if(!(filename = strtok(NULL, " \r\n")))
				continue;
			strtoupper(filename);
			
			sprintf((char *) pathbuf, "%s/%s", path, filename);
			
			stat = fat_get_stat(pathbuf) ;
			if(stat == 0xFF || !(stat & 0x10)) {
				fprintf(stderr, "Error: is not directory\n");
				continue;
			}
			
			for(files = 0; (i = fat_dirlist(pathbuf, list, 8, files)); files += i) {
				for (j = i - 1; j >= 0; j--) {
					if(list[j].filename[0]) {
						if(!strcmp("..", list[j].filename) || !strcmp(".", list[j].filename))
							continue;
						fprintf(stderr, "Error: directory not empty\n");
						goto no_rmdir;
					}
				}
			}
		
			sprintf((char *) pathbuf, "%s/%s", path, filename);
			delete_file(pathbuf);
			no_rmdir:;
		} else if(!strcmp(cmd, "edit")) {
			if(!(filename = strtok(NULL, " \r\n")))
				continue;
			strtoupper(filename);
			
			if((fd = fat_open((char *) pathbuf, O_WRONLY)) < 0)
				continue;
			
			size = 0;
			while((tmp = fread(buff, 1, 512, stdin)) == 512) {
				printf("writing 512\n");
				fat_write_sect(fd);
				size += tmp;
			}
			if(tmp > 0) {
				printf("writing %u\n", tmp);
				fat_write_sect(fd);
				size += tmp;
			}
			
			fat_close(fd);
			fat_set_fsize(pathbuf, size);
		} else if(!strcmp(cmd, "exit")) {
			return 0;
		} else {
			fprintf(stderr, "Unknown command %s\n", cmd);
		}
	}

	return 0;
}
