#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


// Function to replace a substring within a string
void moveString(char *str, const char *oldSubstr, const char *newSubstr) {
    char *pos = str;
    int old = strlen(oldSubstr);
    int new = strlen(newSubstr);

    while ((pos = strstr(pos, oldSubstr)) != NULL) {
        // move characters
        memmove(pos + new, pos + old, strlen(pos + old) + 1);

        // copy the new substring
        strncpy(pos, newSubstr, new);

        // move the position to the end of the new substring
        pos += new;
    }
}
int main(int argc, char *argv[]) {
    // Check for the correct number of arguments
    if (argc != 2) {
        printf("Improper number of arguments\nUsage: ./wgroff <file>\n");
        return 0;
    }

    // Open the input file
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("File doesn't exist\n");
        return 0;
    }

    // Variable declaration
    int lineNumber = 0;
    char line[256];

    // the args
    char section[30];
    char command[30];
    char date[30];

    char header[100];
    int sectionNum = 0;
    FILE *file;
    int count = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        count++;

        // Skip lines starting with '#'
        if (line[0] == '#') {
            continue;
        }
        lineNumber++;

        // If the line number is 1, it should contain .TH
        if (lineNumber == 1) {
            if (sscanf(line, ".TH %s %s %s", command, section, date) != 3) {
                printf(("Improper formatting on line %d\n"), count);
                fclose(fp);
                return 0;
            }

            // Check if the section is valid
            for (int i = 0; section[i] != '\0'; i++) {
                if (!isdigit(section[i]) || section[i] == '.') {
                    printf("Improper formatting on line 1\n");
                    fclose(fp);
                    if (file != NULL) {
                        fclose(file);
                    }
                    return 0;
                }
            }


            // If the section is valid, convert it to an int
            sectionNum = atoi(section);
            if (sectionNum < 1 || sectionNum > 9) {
                printf(("Improper formatting on line %d\n"), count);
                //printf("2");
                fclose(fp);
                return 0;
            }

            if (strlen(date) != 10 || date[4] != '-' || date[7] != '-') {
                printf(("Improper formatting on line %d\n"), count);
                fclose(fp);
                return 0;
            }

            // Check if the date is valid
            int year;
            int month;
            int day;
            if (sscanf(date, "%d-%d-%d", &year, &month, &day) != 3 ||year < 0 || month < 1 || month > 12 || day < 1 || day > 31) {
                printf("Improper formatting on line 1\n");
                //printf("4");
                fclose(fp);
                return 0;
            }

            char *newFilePath = malloc(256);
            snprintf(newFilePath, 256, "./%s.%s", command, section);
            if ((file = fopen(newFilePath, "w")) == NULL) {
                printf("Open file failed\n");
                return 0;
            }
            free(newFilePath);
            snprintf(header, sizeof(header), "%s(%s)", command, section);
            fprintf(file, "%-40s%40s\n", header, header);
        }

        else if (strncmp(line, ".SH ", 4) == 0) {
            fprintf(file, "\n");
            char sh[100];
            sscanf(line, ".SH %s", sh);
            int len = strlen(sh);
            for (int i = 0; i < len; i++) {
                sh[i] = toupper(sh[i]);
            }
            fprintf(file, "\033[1m%s\033[0m\n", sh);
        }
        else {
            moveString(line, "/fB", "\033[1m");
            moveString(line, "/fI", "\033[3m");
            moveString(line, "/fU", "\033[4m");
            moveString(line, "/fP", "\033[0m");
            moveString(line, "//", "/");
            fprintf(file, "       %s", line);
        }
    }
    char lastLine[80];
    int start = (80 - strlen(date)) / 2;
    //printf("%d\n",start);
    for (int i = 0; i < start ; i++) {
        lastLine[i] = ' ';
    }
    strncat(lastLine, date, 10);
    for (int i = start + 10; i < 80; i++) {
        lastLine[i] = ' ';
    }
    fprintf(file, "%s\n", lastLine);
    return 0;

}
