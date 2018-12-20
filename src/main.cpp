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

#define CGROUP_FOLDER "/sys/fs/cgroup/pids/container/"
#define concat(a,b) (a"" b)

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
 * Sets up environment variables for the containerized application.
 */
void setup_variables() {
    clearenv();
    setenv("TERM", "xterm-256color", 0);
    setenv("PATH", "/bin/:/sbin/:usr/bin:/usr/sbin", 0);
}

/**
 * Sets up the root folder for the containerized application.
 * Also change directory to the root folder.
 * @param folder
 */
void setup_root(const char* folder){
    chroot(folder);
    chdir("/");
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
 * update a given file with a string value.
 */
void write_rule(const char* path, const char* value) {
    int fp = open(path, O_WRONLY | O_APPEND );
    write(fp, value, strlen(value));
    close(fp);
}

/**
 * Setup process restrictions using cgroups.
 */
void setup_process_restrictions() {
    // create a folder inside cgroups for the container.
    mkdir( CGROUP_FOLDER, S_IRUSR | S_IWUSR);

    //get pid as a char pointer.
    const char* pid  = std::to_string(getpid()).c_str();

    write_rule(concat(CGROUP_FOLDER, "cgroup.procs"), pid); // register the process.
    write_rule(concat(CGROUP_FOLDER, "notify_on_release"), "1"); // notify on process end in order to cleanup.
    write_rule(concat(CGROUP_FOLDER, "pids.max"), "5"); // limit the number of processes created in the container.
}

/**
 * Starting point of the containerized application(bash in our case).
 * @param args
 * @return
 */
int child_function(void *args) {
    printf("Hello world from the child with pid: %d\n", getpid());
    fflush(stdout);

    setup_process_restrictions(); // use cgroups to setup restrictions.
    setup_variables(); // clear environment variables.
    setup_root("./root"); // change the root of the application.

    mount("proc", "/proc", "proc", 0, 0); // mount procfs in the /root/proc directory.

    auto run_bash = [](void *args) ->int { run("/bin/sh"); };

    // We need to run bash in another process in order to clean up mounted procfs later.
    clone(run_bash, allocate_stack_memory(), SIGCHLD, 0);
    wait(nullptr);

    umount("/proc");
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
