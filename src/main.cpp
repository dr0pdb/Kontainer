#include <iostream>
#include <sched.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <fstream>

/**
 * Utility function to allocated stack memory.
 * @return
 */
char* allocate_stack_memory() {
    const int stack_size = 65536; // 64 KB
    auto *stack = new (std::nothrow) char[stack_size];

    if (!stack) {
        printf("Cannot allocate memory \n");
        exit(EXIT_FAILURE);
    }

    return stack + stack_size; // stack grows in opposite direction.
}

/**
 * Wrapper method to replace the current process with a new one using exec system call.
 * @tparam P
 * @param params
 * @return
 */
template <typename... P>
int run(P... params) {
    char *args[] = {(char *)params..., (char *)0};
    return execvp(args[0], args);
}

/**
 * Starting point of the containerized application(bash in our case).
 * @param args
 * @return
 */
int child_function(void *args) {
    clearenv(); // Clear the environment variables.
    printf("Hello world from the child with pid: %d\n", getpid());
    fflush(stdout);

    run("/bin/sh");
    return EXIT_SUCCESS;
}

/**
 * Entry point of the application.
 * @param argc : argument count.
 * @param argv : command line arguments.
 * @return
 */
int main(int argc, char** argv) {
    printf("Hello world from parent with pid: %d\n", getpid());
    fflush(stdout);

    clone(child_function, allocate_stack_memory(), CLONE_NEWPID | CLONE_NEWUTS | SIGCHLD, 0);
    wait(nullptr);

    return EXIT_SUCCESS;
}
