#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

void handle_file(const char * path_at, const char * path_to, const char * subdir, const char * file)
{
	char * name_from = (char *) malloc(0x1000);
	char * name_to = (char *) malloc(0x1000);
	strcpy(name_from, path_at);
	strcat(name_from, subdir);
	strcat(name_from, "/");
	strcat(name_from, file);
	strcpy(name_to, path_to);
	strcat(name_to, subdir);
	strcat(name_to, "/");
	strcat(name_to, file);
	int i = strlen(name_from);
	int discard = 0;
	while (i >= 0 && name_from[i] != L'/' && name_from[i] != L'.') i--;
	if (i >= 0 && name_from[i] == L'.') {
		if (strcmp(name_from + i, ".cpp") == 0) discard = 0;
		else if (strcmp(name_from + i, ".h") == 0) discard = 0;
		else if (strcmp(name_from + i, ".mm") == 0) discard = 0;
		else discard = 1;
	} else discard = 1;
	if (strlen(subdir) >= 18 && memcmp(subdir, "/PlatformDependent", 18) == 0) discard = 2;
	if (discard == 1) {
		printf("Ignored (not source)");
	} else if (discard == 2) {
		printf("Ignored (platform dependent)");
	} else {
		int fd_from = open(name_from, O_RDONLY, 0666);
		int fd_to = open(name_to, O_RDWR, 0666);
		if (fd_from != -1) {
			bool sync = false;
			if (fd_to == -1) {
				auto len = strlen(name_to);
				for (i = 0; i < len; i++) if (name_to[i] == '/') {
					name_to[i] = 0;
					if (strlen(name_to)) mkdir(name_to, 0777);
					name_to[i] = '/';
				}
				fd_to = open(name_to, O_RDWR | O_CREAT | O_EXCL, 0666);
				printf("Created...");
				sync = true;
			}
			if (fd_to != -1) {
				struct stat fd_from_stat, fd_to_stat;
				fstat(fd_from, &fd_from_stat);
				fstat(fd_to, &fd_to_stat);
				if (!sync) {
					if (fd_to_stat.st_mtim.tv_sec < fd_from_stat.st_mtim.tv_sec) sync = true;
				}
				if (sync) {
					char * buffer = (char *) malloc(fd_from_stat.st_size);
					read(fd_from, buffer, fd_from_stat.st_size);
					ftruncate(fd_to, 0);
					lseek(fd_to, 0, SEEK_SET);
					write(fd_to, buffer, fd_from_stat.st_size);
					free(buffer);
					printf("Updated");
				} else {
					printf("Skipped");
				}
				close(fd_to);
			} else {
				printf("Sync failed (can not open/create dest)");
			}
			close(fd_from);
		} else {
			printf("Sync failed (can not open source)");
		}
	}
	free(name_from);
	free(name_to);
}
void enumerate_objects(const char * path_at, const char * path_to, const char * subdir)
{
	char * path_at_full = (char *) malloc(0x1000);
	char * new_subdir = (char *) malloc(0x1000);
	strcpy(path_at_full, path_at);
	strcat(path_at_full, subdir);
	struct dirent ** objs;
	int num_files = scandir(path_at_full, &objs, 0, alphasort);
	if (num_files >= 0) {
		for (int i = 0; i < num_files; i++) {
			if (objs[i]->d_type == DT_REG) {
				printf("Handling a file %s...", objs[i]->d_name);
				handle_file(path_at, path_to, subdir, objs[i]->d_name);
				printf("\n");
			} else if (objs[i]->d_type == DT_DIR) {
				if (objs[i]->d_name[0] != L'.') {
					strcpy(new_subdir, subdir);
					strcat(new_subdir, "/");
					strcat(new_subdir, objs[i]->d_name);
					printf("Watching files at directory %s...\n", new_subdir);
					enumerate_objects(path_at, path_to, new_subdir);
				}
			}
			free(objs[i]);
		}
		free(objs);
	}
	free(path_at_full);
	free(new_subdir);
}
void enumerate_objects(const char * path_at, const char * path_to) { enumerate_objects(path_at, path_to, ""); }

int main(void)
{
	if (chdir("/home/manwe/Документы/VirtualUI") == -1) {
		printf("chdir() failed\n");
		return 1;
	}
	enumerate_objects("EngineRuntime", "EngineRuntime-Linux");
	return 0;
}