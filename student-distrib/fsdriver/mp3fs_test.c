#include "mp3fs_test.h"

int launch_mp3fs_driver_test(){
	int temp_return;
	int fd;
	struct dirent* temp_dirent;
	uint8_t buf[128];
	int temp_size;
	DIR* temp_dir;

	int fd_in;
	char temp_input;
	
	if ((temp_return = mount("","/","mp3fs",0,""))){
		printf("mp3fs mount failed\n");
		return 0;
	}

	// open the root directory
	if (!(temp_dir = opendir("/"))){
		printf("open mp3fs root dir failed");
		return 0;
	}

	if ((fd_in = open("/dev/stdin", O_RDONLY, 0)) < 0){
		printf("open stdin failed\n");
		return 0;
	}

	while ((temp_dirent = readdir(temp_dir))){
		// iterate through all files
		if ((fd = open(temp_dirent -> filename, O_RDONLY,0))){
			while ((temp_size = read(fd, buf, 128))>0){
				write(0, buf, temp_size);
			}
			printf("\n\nPress enter to continue");
			close(fd);
			if (read(fd_in, &temp_input, 1)<=0){
				printf("read key failed\n");
				return 0;
			}
		}
	}
	return 1;
}
