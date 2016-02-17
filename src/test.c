#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "fat.h"

FILE *fp;

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


int main(int argc, char **argv) {
	int fd, i, files, j;
	uint8_t buff[512];
	char path[256] = "/";
	char cmdline[300] = {};
	uint32_t size;
	
	char *cmd, *filename;
	
	struct FATDirList list[8];
	
	if(argc < 2) {
		fprintf(stderr, "Usage: test /path/to/image.img\n");
	}
	
	sector_buff = buff;
	fp = fopen(argv[1], "r+");
	init_fat();
	
	for(;;) {
		fputs("> ", stdout);
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
			
			if(!strcmp(filename, "."))
				continue;
			if(!strcmp(filename, "..") && strcmp(path, "/")) {
				*strrchr(path, '/') = 0;
				*(strrchr(path, '/') + 1) = 0;
				continue;
			}
				
			strcat(path, filename);
			strcat(path, "/");
		} else if(!strcmp(cmd, "pwd")) {
			puts(path);
		} else if(!strcmp(cmd, "ls")) {
			for(files = 0; (i = fat_dirlist(path, list, 8, files)); files += i) {
				for (j = i - 1; j >= 0; j--) {
					if(list[j].filename[0])
						printf("%s (0x%X)\n", list[j].filename, list[j].attrib);
				}
			}
		} else if(!strcmp(cmd, "cat")) {
			if(!(filename = strtok(NULL, " \r\n")))
				continue;
			
			sprintf((char *) buff, "%s%s", path, filename);
			printf("%s\n", buff);
			if((fd = fat_open((char *) buff, O_RDONLY)) < 0)
				continue;
			
			printf("will do\n");
			
			for(size = fat_fsize(fd); !(size%512); size -= 512) {
				fat_read_sect(fd);
				fwrite(sector_buff, 1, 512, stdout);
			}
			if(size) {
				fat_read_sect(fd);
				fwrite(sector_buff, 1, size, stdout);
			}
			
			fat_close(fd);
		} else {
			fprintf(stderr, "Unknown command %s\n", cmd);
		}
	}
	
	//delete_file("/ARNE/LOLOL.ASD");
	//delete_file("/ARNE");
	//create_file(0, "TESTCASE", 0x10);
	//create_file("/TESTCASE", "ARNE", 0x10);
	//create_file("/TESTCASE/ARNE", "TEST.BIN", 0x0);

	//fd = fat_open("/TESTCASE/ARNE/TEST.BIN", O_RDWR);
	//size = fat_fsize(fd);
	//fprintf(stderr, "fd=%i, size=%i\n", fd, size);
	//for (i = 0; i < 129; i++) {
	//	memset(sector_buff, i, 512);
	//	fat_write_sect(fd);
	//}

	//fat_close(fd);

	//create_file(0, "ARNE", 0x10);
	//create_file(0, "ARNE", 0x10);
	//create_file("/ARNE", "LOLOL.ASD", 0x10);
	//create_file("/ARNE", "LOLOL.ASD", 0x10);
	
	//delete_file("/TESTCASE/ARNE/TEST.BIN");
	//delete_file("/TESTCASE/ARNE");
	//delete_file("/TESTCASE");

	return 0;
}
