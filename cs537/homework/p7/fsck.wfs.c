#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

// Assuming 'change' and 'date' are defined elsewhere in your code
extern int change(int);

void del_dir(const char *npath) {
    DIR *ptr = opendir(npath);
    if (ptr == NULL) {
        perror("Failed to open directory");
        return;
    }

    struct dirent *ent;
    while ((ent = readdir(ptr)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        char pathname[PATH_MAX];
        snprintf(pathname, sizeof(pathname), "%s/%s", npath, ent->d_name);

        if (ent->d_type == DT_REG) { 
            if (remove(pathname) != 0) {
                perror("Failed to remove file");
            }
        } else if (ent->d_type == DT_DIR) {
            del_dir(pathname);
            if (remove(pathname) != 0) {
                perror("Failed to remove directory");
            }
        }
    }

    closedir(ptr);
}

void readdirect(const char *path) {
    DIR *ptr = opendir(path);
    if (ptr == NULL) {
        perror("Failed to open directory");
        return;
    }

    struct dirent *ent;
    while ((ent = readdir(ptr)) != NULL) {
        char pathname[PATH_MAX];
        snprintf(pathname, sizeof(pathname), "%s/%s", path, ent->d_name);

            del_dir(pathname);
            if (remove(pathname) != 0) {
                perror("Failed to remove directory");
        }
    }

    closedir(ptr);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s path\n", argv[0]);
        return 0;
    }
    FILE* fp = fopen(argv[1], "rb");
   if (fp == NULL) {
     printf("Failed to open file after fsck.wfs\n");
     return 0;
   }

  fclose(fp);

  if (remove(argv[1]) != 0) {
     perror("Failed to remove directory");
	 return 0;
  }
  //readdirect(argv[1]);
  exit(0);
  return 0;
}
