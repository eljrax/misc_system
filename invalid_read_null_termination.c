/* This code does very little besides demonstrates an invalid
 * read of one byte.
 *
 * Solely used to demonstrate debuggers and memory profilers
 * and get talking about null terminated strings
 *
 * gcc -g invalid_read_null_termination.c -o invalid_read_null_termination
 * ./invalid_read_null_termination
 * valgrind ./invalid_read_null_termination
 *
 * Hint: str[BUF_SIZE-1] = '\0';
 *
 * Erik Ljungstrom, 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 4 

int main(int argc, char *argv[]){

    char *data = malloc(BUF_SIZE);
    char str[BUF_SIZE];
    int i;
    for (i = 0 ;  i < BUF_SIZE ; i++)
        str[i] = 'a';

    str[BUF_SIZE] = '\0';
    printf("%s\n", str);
    strncpy(data, str, BUF_SIZE);
    /* Invalid read incoming! */ 
    printf("%s\n", data);
    free(data);
}
