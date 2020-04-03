#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "constants.h"

void ff_sigint_handler(int signal __attribute__((unused)))
{
    char message[] = "Interrupt signal received, terminating process!\n";
    write(STDOUT_FILENO, message, sizeof(message));
    exit(1);
}
