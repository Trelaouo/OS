#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
int main(int argc , char* argv[]) {

    // If invalid argument provide
    if(argc < 2 || argc > 3){
        printf("What manual page do you want?\nFor example, try 'wman wman'\n");
        return 0;
    }

    // If user doesn't enter specific file
    if(argc == 2){

        // Variable declaration
        char *page = argv[1];
        struct stat sb;
        FILE *fp = NULL;

        // Search through all of the files from man1 to man9
        for(int i = 1; i <= 9; i++){
            char path[256];
            snprintf(path, sizeof(path), "./man_pages/man%d/%s.%d", i, page, i);

            // Check if file open correctly
            int fileExist = stat(path, &sb);
            if(fileExist == 0) {
                fp = fopen(path, "r");

                // If can open file
                if (fp != NULL) {

                    // put fgets() into a variable buffer and print out
                    char buffer[256];
                    while(fgets(buffer, sizeof(buffer), fp) != NULL) {
                        printf("%s", buffer);
                    }
                    fclose(fp);
                    return 0;
                }

                // If cannot open file, then exit 1
                else{
                    printf("cannot open file\n");
                    //printf("No manual entry for %s\n", page);
                    exit(1);
                }
            }

        }

        // If no such files

            //printf("cannot open file\n");
            printf("No manual entry for %s\n", page);
            return 0;
    }

        // If user enter the specific file
    else if(argc == 3){

        // Check if the man page section is valid
        int section = atoi(argv[1]);
        if (section < 1 || section > 9){
            printf("invalid section\n");
            exit(1); // If not, then exit 1
        }

        // Variable declaration
        char *page = argv[2];
        FILE *fp = NULL;
        struct stat sb;

        // Search through the specific file that user enters
        char path[256];
        snprintf(path, sizeof(path), "./man_pages/man%d/%s.%d", section, page, section);

        // Check if file open correctly
        int fileExist = stat(path, &sb);
        if(fileExist == 0) {

            fp = fopen(path, "r");
            if (fp != NULL) {

                // put fgets() into a variable buffer and print out
                char buffer[256];
                while(fgets(buffer, sizeof(buffer), fp) != NULL) {
                    printf("%s", buffer);
                }
                fclose(fp);
                return 0;
            }

                // If cannot open file, then exit 1
            else {
                printf("cannot open file\n");
                //printf("No manual entry for %s\n", page);
                exit(1);
            }
        }

        // If no such files
        printf("No manual entry for wman in section %i\n", section);
        return 0;
    }
}
