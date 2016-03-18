/* 
 * Quick code to demonstrate how COW memory and relationships between
 * parent and child process works.
 *
 * This will allocate and initialise memory in different stages, and user
 * is prompted to investigate throughout.
 *
 * To run:
 * gcc -Wall cow_memory_parent_child.c -o cow_memory_parent_child
 * ./cow_memory_parent_child
 *
 * Erik Ljungstrom, 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#define MALLOC_SIZE 1024*1024*10

int errno;

int main(int argc, char *argv[]){
    pid_t my_pid = getpid();
    pid_t child_pid;
    int child_status;
    char buf;
    void *data;
    int pipefd[2];
    
    if (pipe(pipefd) < 0){
        perror("pipe");
        return 0;
    }
    
    data = malloc(MALLOC_SIZE);
    if (data == NULL){
        fprintf(stderr, "Couldn't allocate memory...\n");
        return 1;
    }

    printf("At this point, we have asked the kernel to allocate us %dMB."
           " Check VIRT and RES in: top -d 9999 -p %d\nPress Enter to continue\n",
           MALLOC_SIZE/1024/1024, my_pid);
    getchar();

    bzero(data, MALLOC_SIZE/2);
    printf("Yeah, it's NOT using %dMB. This is how memory can become"
           " overallocated.\n", MALLOC_SIZE/1024/1024);
    printf("But now we've written data to half the allocated memory, check the RES column top again\n");
    getchar();
    bzero(data, MALLOC_SIZE);
    printf("Now we've written data to the entire memory allocation, check top again\n"); 
    getchar();
    
    child_pid = fork();


    if (child_pid >= 0){

        if (child_pid == 0){
            void *data2;
            while (read(pipefd[0], &buf, 1) > 0){
                if (buf == '1'){
                    data2 = malloc(MALLOC_SIZE);
                    printf("Child process memory address: %p\n", data2);
                }
                else if (buf == '2'){
                    bzero(data2, MALLOC_SIZE);
                }
                else if (buf == 'q'){
                    free(data2);
                    free(data);
                    exit(0);
                }
                
            }

        }
        else {
            printf("We now have a child process with PID %d run and keep running: top -d 9999 -p %d,%d"
                   "Also check: pmap -X %d (parent) and pmap -X %d (child) and take note of the"
                   " memory addresses. See how they are the same?\n",
                   child_pid, my_pid, child_pid, my_pid, child_pid);
            getchar();

            printf("Yes, the child process hasn't allocated any memory, but we have"
                   " a copy-on-write copy of the parent's memory\n");

            write(pipefd[1], "1", 1);
            printf("Now the child process has asked the kernel for another %d MB, check"
                   " top again, and look at the VIRT column\n", MALLOC_SIZE/1024/1024);
            getchar();
            write(pipefd[1], "2", 1);
            printf("Now the child has written data to the memory it asked the kernel for"
                   " check top again.\n");
            getchar();

            if ((data = realloc(data, (MALLOC_SIZE+(MALLOC_SIZE/2)))) == NULL){
                fprintf(stderr, "Could not extend memory allocation\n");
                write(pipefd[1], "q", 1);
                exit(1);
            }
            bzero(data, MALLOC_SIZE+(MALLOC_SIZE/2));
            printf("The parent process has now extended and initialised the original allocation by %d MB. Check top."
                   " This has not affected the memory consumption of the child process. Also check the"
                   " pmap -X commands from earlier - see the addresses aren't the same anymore?"
                   " Since we reinitialised the memory they previously shared, the kernel gave"
                   " the child its own, separate memory address\n",
                   ((MALLOC_SIZE+(MALLOC_SIZE/2)-MALLOC_SIZE))/1024/1024);
            
            getchar();

            free(data);
            printf("The parent process has now free'd its memory, check top"
                   " The child still has a reference to the memory it initially shared with the parent,"
                   " so the child's memory footprint is still 2*%dMB\n", MALLOC_SIZE/1024/1024);
            getchar();
            printf("Check pmap -X %d (child) and pmap -X %d (parent), and you will see another memory reference (first column"
                   " in the first bit of output which isn't in the output of the second command.\n",
                   child_pid, my_pid);
            getchar();

            printf("\nPress Enter to quit (or for the heck of it, press 'o' then Enter to create an orphaned process or 'z' for a zombie)");
            int input = getchar();
            if (input == 'o'){
                kill(my_pid, SIGKILL);
            }
            else if (input == 'z'){
                write(pipefd[1], "q", 1);
                printf("There's now a zombie with pid %d\nPress Enter to quit and kill them both", child_pid);
                getchar();
                getchar();
            }
            else {
                printf("\n");
                write(pipefd[1], "q", 1);
            }
        }
        
    }
    else {
        fprintf(stderr, "Unable to fork()...%s\n", strerror(errno));
        return 1;
    }

    wait(&child_status);
    return 0;
}
