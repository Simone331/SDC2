#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

// macros for error handling
#include "common.h"

#define N 100   // child process count
#define M 10    // thread per child process count
#define T 3     // time to sleep for main process

#define FILENAME	"accesses.log"

/*
 * data structure required by threads
 */
typedef struct thread_args_s {
    unsigned int child_id;
    unsigned int thread_id;
} thread_args_t;

/*
 * parameters can be set also via command-line arguments
 */
int n = N, m = M, t = T;

/* TODO: declare as many semaphores as needed to implement
 * the intended semantics, and choose unique identifiers for
 * them (e.g., "/mysem_critical_section") */
sem_t *wait_for_children;
sem_t *wait_for_main;
sem_t *critical_section;

/* TODO: declare a shared memory and the data type to be placed 
 * in the shared memory, and choose a unique identifier for
 * the memory (e.g., "/myshm") 
 * Declare any global variable (file descriptor, memory 
 * pointers, etc.) needed for its management
 */
int shm_fd;
int *shm_ptr;



/*
 * Ensures that an empty file with given name exists.
 */
void init_file(const char *filename) {
    printf("[Main] Initializing file %s...", FILENAME);
    fflush(stdout);
    int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd<0) handle_error("error while initializing file");
    close(fd);
    printf("closed...file correctly initialized!!!\n");
}

/*
 * Create a named semaphore with a given name, mode and initial value.
 * Also, tries to remove any pre-existing semaphore with the same name.
 */
sem_t *create_named_semaphore(const char *name, mode_t mode, unsigned int value) {
    printf("[Main] Creating named semaphore %s...", name);
    fflush(stdout);
    sem_unlink(name); // remove any pre-existing semaphore with the same name
    sem_t *sem = sem_open(name, O_CREAT | O_EXCL, mode, value);
    if (sem == SEM_FAILED) handle_error("error while creating named semaphore");
    printf("done!!!\n");
    return sem;
}



void parseOutput() {
    // identify the child that accessed the file most times
    int* access_stats = calloc(n, sizeof(int)); // initialized with zeros
    printf("[Main] Opening file %s in read-only mode...", FILENAME);
	fflush(stdout);
    int fd = open(FILENAME, O_RDONLY);
    if (fd < 0) handle_error("error while opening output file");
    printf("ok, reading it and updating access stats...");
	fflush(stdout);

    size_t read_bytes;
    int index;
    do {
        read_bytes = read(fd, &index, sizeof(int));
        if (read_bytes > 0)
            access_stats[index]++;
    } while(read_bytes > 0);
    printf("ok, closing it...");
	fflush(stdout);

    close(fd);
    printf("closed!!!\n");

    int max_child_id = -1, max_accesses = -1;
    for (i = 0; i < n; i++) {
        printf("[Main] Child %d accessed file %s %d times\n", i, FILENAME, access_stats[i]);
        if (access_stats[i] > max_accesses) {
            max_accesses = access_stats[i];
            max_child_id = i;
        }
    }
    printf("[Main] ===> The process that accessed the file most often is %d (%d accesses)\n", max_child_id, max_accesses);
    free(access_stats);
}

void* thread_function(void* x) {
    thread_args_t *args = (thread_args_t*)x;

    /* TODO: protect the critical section using semaphore(s) as needed */
    ret=sem_wait(critical_section);
    if (ret<0) handle_error("error while waiting on semaphore");
    printf("[Child#%d-Thread#%d] Entering critical section...\n", args->child_id, args->thread_id);
    // open file, write child identity and close file
    int fd = open(FILENAME, O_WRONLY | O_APPEND);
    if (fd < 0) handle_error("error while opening file");
    printf("[Child#%d-Thread#%d] File %s opened in append mode!!!\n", args->child_id, args->thread_id, FILENAME);	

    write(fd, &(args->child_id), sizeof(int));
    printf("[Child#%d-Thread#%d] %d appended to file %s opened in append mode!!!\n", args->child_id, args->thread_id, args->child_id, FILENAME);	

    close(fd);
    printf("[Child#%d-Thread#%d] File %s closed!!!\n", args->child_id, args->thread_id, FILENAME);

    printf("[Child#%d-Thread#%d] Leaving critical section...\n", args->child_id, args->thread_id);
    ret=sem_post(critical_section);
    if (ret<0) handle_error("error while posting semaphore");
    printf("[Child#%d-Thread#%d] completed\n", args->child_id, args->thread_id);
    free(args);
    pthread_exit(NULL);
}

void mainProcess() {
    /* TODO: the main process waits for all the children to start,
     * it notifies them to start their activities, and sleeps
     * for some time t. Once it wakes up, it notifies the children
     * to end their activities, and waits for their termination.
     * Finally, it calls the parseOutput() method and releases
     * any shared resources. */
   // wait for all the children to start
    printf("[Main] %d children created, wait for all children to be ready...\n", n);
    int i, ret;
    for (i = 0; i < n; i++) {
        ret = sem_wait(main_waits_for_children);
		if(ret) {
		    handle_error("sem_wait failed");
		}
	}
    printf("[Main] All the children are now ready!!!\n");
     // notify children to start their activities
    printf("[Main] Notifying children to start their activities...\n");
    for (i = 0; i < n; i++) {
        ret = sem_post(children_wait_for_main);
		if(ret) {
		    handle_error("sem_post failed");
		}
	}
    printf("[Main] Children have been notified to start their activities!!!\n");
    // sleep for some time t
    printf("[Main] Sleeping for %d seconds...\n", t);
    sleep(t);
    printf("[Main] Woke up from sleep!!!\n");
    // notify children to end their activities
    printf("[Main] Notifying children to end their activities...\n");
    *shm_ptr = 1; // set termination event in shared memory
    printf("[Main] Children have been notified to end their activities!!!\n");
    // wait for their termination
    printf("[Main] Waiting for all children to terminate...\n");
    int &child_status;
    for (i = 0; i < n; i++) {
        ret= wait(&child_status);
        if (ret < 0) handle_error("error while waiting for child");
        if (WEXITSTATUS(child_status) == EXIT_SUCCESS)
            printf("[Main] Child %d terminated correctly!!!\n", ret);
        else
            printf("[Main] Child %d terminated with errors!!!\n", ret);
    }
    printf("[Main] All children have terminated!!!\n");
    // call the parseOutput() method
    parseOutput();
    // clean up and release any shared resources
    fflush(stdout);
    printf("[Main] Releasing shared resources...\n");
    ret = sem_close(wait_for_children);
    if (ret < 0) handle_error("error while closing semaphore");
    ret = sem_unlink("/wait_for_children");
    if (ret < 0) handle_error("error while unlinking semaphore");
    ret = sem_close(wait_for_main);
    if (ret < 0) handle_error("error while closing semaphore");
    ret = sem_unlink("/wait_for_main");
    if (ret < 0) handle_error("error while unlinking semaphore");
    ret = sem_close(critical_section);
    if (ret < 0) handle_error("error while closing semaphore");
    ret = sem_unlink("/critical_section");
    if (ret < 0) handle_error("error while unlinking semaphore");
    ret = close(shm_fd);
    if (ret < 0) handle_error("error while closing shared memory file descriptor");
    ret = munmap(shm_ptr, sizeof(int));
    if (ret < 0) handle_error("error while unmapping shared memory");
    ret = shm_unlink("/myshm");
    if (ret < 0) handle_error("error while unlinking shared memory");
    printf("[Main] Shared resources released!!!\n");
    printf("[Main] Terminating...\n");
 





}

void childProcess(int child_id) {
    /* TODO: each child process notifies the main process that it
     * is ready, then waits to be notified from the main in order
     * to start. As long as the main process does not notify a
     * termination event [hint: use sem_getvalue() here], the child
     * process repeatedly creates m threads that execute function
     * thread_function() and waits for their completion. When a
     * notification has arrived, the child process notifies the main
     * process that it is about to terminate, and releases any
     * shared resources before exiting. */
}

int main(int argc, char **argv) {
    // arguments
    if (argc > 1) n = atoi(argv[1]);
    if (argc > 2) m = atoi(argv[2]);
    if (argc > 3) t = atoi(argv[3]);

    // initialize the file
    init_file(FILENAME);

    /* TODO: initialize any semaphore needed in the implementation, and
     * create N children where the i-th child calls childProcess(i); 
     * initialize the shared memory (create it, set its size and map it in the 
     * memory), then the main process executes function mainProcess() once 
     * all the children have been created */

    shm_unlink("/myshm"); // remove any pre-existing shared memory with the same name
    shm_fd = shm_open("/myshm", O_CREAT | O_EXCL | O_RDWR, 0600);
    if (shm_fd < 0) handle_error("error while creating shared memory");
    if (ftruncate(shm_fd, sizeof(int)) < 0) handle_error("error while setting size of shared memory");
    shm_ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) handle_error("error while mapping shared memory");
    *shm_ptr = 0; // initialize the shared memory

    wait_for_children = create_named_semaphore("/wait_for_children", 0600, 0);
    wait_for_main = create_named_semaphore("/wait_for_main", 0600, 0);
    critical_section = create_named_semaphore("/critical_section", 0600, 1);


    printf("[Main] Creating %d children...\n", n);
     for (int i=0; i<n; i++){
        pid_t pid = fork();
        if (pid < 0) handle_error("error on fork");
        if (pid == 0) {
            printf("[Child#%d] Hello, I'm a child!!!\n", i);
            childProcess(i);
            exit(EXIT_SUCCESS); 
        }
     }
    

    exit(EXIT_SUCCESS);
}