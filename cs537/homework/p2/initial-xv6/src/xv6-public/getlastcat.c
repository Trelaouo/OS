#include "types.h"
#include "stat.h"
#include "user.h"
#include <stddef.h>

int main (int argc, char *argv[])
{
    char buf[512];
    int catResult = getlastcat(&buf[0]);

    if(catResult == 0){
        printf(1, "XV6_TEST_OUTPUT Last catted filename: %s\n", buf);
    }
    
    exit();
}


