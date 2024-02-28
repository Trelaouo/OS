#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

const char name[] = "[1mNAME[0m\n";
const char description[] = "[1mDESCRIPTION[0m\n";

int main(int argc, char *argv[])
{
    // Invalid argument
    if (argc <= 1)
    {
        printf("wapropos what?\n");
        return 0;
    }

    // Variable declaration
    DIR *dir;
    char *keyword = argv[1]; // get the user input section
    char path[256];
    struct dirent *entry;
    int found = 0;
    FILE *fp = NULL;

    // open file from man1 to man9
    for(int i = 1; i <= 9; i++){

        // Find the file name one by one
        snprintf(path, sizeof(path), "./man_pages/man%d/", i);

        // Check if file open correctly
        if ((dir = opendir(path)) == NULL) {
            printf("error opening dir\n");
            exit(1);
        }

        while ((entry = readdir(dir)) != NULL) {
            // printf("1");
            char *filename = entry->d_name;
            char fullFilename[256];
            //struct stat fileStat;
            snprintf(fullFilename,sizeof(fullFilename),"./man_pages/man%d/%s", i, filename);

            // Check if the file is opened
            fp = fopen(fullFilename, "r");
            if(fp == NULL){
                printf("Cannot open file\n");
                fclose(fp);
                exit(1);
            }

            char buffer[256];
            char output[256];
            int match = 0;

            // Search the word line by line
            while(fgets(buffer, sizeof(buffer), fp) != NULL) {

                // print("1");
                // If name is matched
                if (strcmp(buffer, name) == 0)
                {
                    // printf("1");
                    char line[256];
                    while (fgets(line, sizeof(line), fp) != NULL)
                    {
                        if (line[0]=='\n')
                        {
                            break;
                        }

                        // If correctly searched the user input keyword
                        if (strstr(line, keyword) != NULL)
                        {
                            match = 1;
                        }
                        strcpy(output, line);
                    }
                }

                // If description is matched
                if (strcmp(buffer, description) == 0)
                {
                    // printf("%i", 1);
                    char line[256];
                    while (fgets(line, sizeof(line), fp) != NULL)
                    {
                        if (line[0]=='\n')
                        {
                            break;
                        }

                        // If correctly searched the user input keyword
                        if (strstr(line, keyword) != NULL)
                        {
                            match = 1;
                        }
                    }
                }
            }
            // If the word match
            if (match == 1)
            {
                found = 1;
                char *separator  = strchr(output, '-');

                // print
                for (int j = 0; j < (strlen(filename) - 2); j++)
                {
                    printf("%c", filename[j]);
                }
                printf(" (%d) %s", i, separator);
            }
            fclose(fp);
        }

        // Close dir
        closedir(dir);
    }

    // If no such found
    if (found == 0)
    {
        printf("nothing appropriate\n");
    }
    return 0;
}
