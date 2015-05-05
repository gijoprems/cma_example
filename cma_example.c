/* Cross Memory Attach example application */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/uio.h>

/*********************************************************************************/

#define MAX_NR_PROC    (10)
#define MSG_LEN        (32)

#define CMA_TEST_STR   "cma_test"

/*********************************************************************************/

/* This is just to demonstarate how to use process_vm_readv/writev for fast data
 * transfers among processes. This is not a full implementation of the broadcast
 * operation; instead, it uses fork to spawn child processes and assumes only "read"
 * operation is performed in them. And the parent process sends out a predfined
 * string to all child processes, for which they all wait.
 *
 * An ideal implementation of a broadcast operation would be to implement a client
 * -server model using Unix sockets for control messages and CMA for data transfer.
 * In that case, the server listens for connections from client processes and this
 * forms a process group. The server(root process) gets the remote memory address
 * published by other processes in the group via exchanging control messages (Making
 * a /proc entry would be an addition for reducing the control messages overhead.
 */ 

int main(void)
{
    int i;
    int status;
    pid_t wait_pid;
    pid_t pid;
    pid_t ppid;
    pid_t child_pid[MAX_NR_PROC];    
    char *local_buf;
    char *buf[MAX_NR_PROC];
    size_t nr_bytes;
    struct iovec local[1];
    struct iovec remote[1];

    local_buf = (char *)malloc(MSG_LEN);
    if (local_buf == NULL) {
        fprintf(stderr, "%s(%d) \n", strerror(errno), errno);
        exit(2);
    }

    memset(local_buf, '\0', MSG_LEN);

    for (i = 0; i < MAX_NR_PROC; i++) {
        if ((child_pid[i] = fork()) == 0) { /* child */
            ppid = getppid();
            pid = getpid();
            fprintf(stdout, "child process(%d) \n", pid);
            buf[i] = (char *)malloc(sizeof(MSG_LEN));

            /* invalid local address will cause issues in cma */
            if (buf[i] == NULL) {
                fprintf(stderr, "child proc: %s(%d) \n",
                    strerror(errno), errno);
                continue;
            }
            
            /* iovec, for process_vm_readv */
            local[0].iov_base = (char *)buf[i];
            local[0].iov_len = MSG_LEN;
            remote[0].iov_base = (char *)local_buf;
            remote[0].iov_len = MSG_LEN;
            
            while(strstr(buf[i], CMA_TEST_STR) == NULL) {
                if ((nr_bytes = process_vm_readv(ppid, local,
                        1, remote, 1, 0)) == -1)
                    fprintf(stderr, "read error, %s(%d) \n",
                        strerror(errno), errno);
                else
                    fprintf(stdout, "child(%d) read (%ld) bytes \n",
                        pid, nr_bytes);

                usleep(50 * 1000);
            }

            /* frees the child heap */
            exit(2);
        }
    }

    do {
        /* effectively writing to the "shared" memeory */
        strncpy(local_buf, CMA_TEST_STR, strlen(CMA_TEST_STR));
        /* wait till all are done */
        while((wait_pid = wait(&status)) > 0);
    }while(0);

    if (local_buf)
        free(local_buf);

    return 0;
}

/*********************************************************************************/
