#include "mp3fs_test.h"

int launch_mp3fs_driver_test(){
	int temp_return;
	int fd;
	struct dirent* temp_dirent;
	uint8_t buf[128];
	int temp_size;
	DIR* temp_dir;

	int fd_in, fd_out;
	char temp_input;

	temp_return = mount("","/","mp3fs",0,"");
	if (temp_return != 0 && temp_return != -EEXIST) {
		printf("mp3fs mount failed\n");
		return 0;
	}

	// open the root directory
	if (!(temp_dir = opendir("/"))){
		printf("open mp3fs root dir failed: %d\n", (int)temp_dir);
		return 0;
	}

	if ((fd_in = open("/dev/stdin", O_RDONLY, 0)) < 0){
		printf("open stdin failed: %d\n", fd_in);
		closedir(temp_dir);
		return 0;
	}
	if ((fd_out = open("/dev/stdout", O_RDONLY, 0)) < 0){
		printf("open stdout failed: %d\n", fd_out);
		close(fd_in);
		closedir(temp_dir);
		return 0;
	}


	while ((temp_dirent = readdir(temp_dir))){
		// iterate through all files
		if ((fd = open(temp_dirent -> filename, O_RDONLY,0))){
			while ((temp_size = read(fd, buf, 128))>0){
				write(fd_out, buf, temp_size);
			}
			if (write(fd, buf, 128) != -EROFS){
				printf("mp3fs write failed\n");
				return 0;
			}
			printf("\n \nreading file %s \nusing fd number %d \n",temp_dirent -> filename, fd);
			printf("Press enter to continue");
			close(fd);

			if (read(fd_in, &temp_input, 1)<=0){
				printf("read key failed\n");
				return 0;
			}
			clear();
		}
	}
	close(fd_in);
	close(fd_out);
	closedir(temp_dir);
	return 1;
}
