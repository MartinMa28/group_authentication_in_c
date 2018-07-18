#include<stdio.h>
#include<stdlib.h>

#include<sys/types.h>
#include<sys/socket.h>

#include<netinet/in.h>
#include<unistd.h>
#include<sys/wait.h>

struct msg_buf
{
    long mesg_type;
    char mesg_text[100];
};

int main()
{
    // create a socket, 0 means using TCP by default
    int net_socket;
    net_socket = socket(AF_INET, SOCK_STREAM, 0);

    // specify an address for the socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9002);
    server_addr.sin_addr.s_addr = htonl(0xC0A80009);   
    //server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int connection_status = connect(net_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    // check for error with the connection
    if(connection_status == -1)
        printf("There was an error making a connection to the remote socket.\n");
    
    char connect_info[256] = "connect to server\n";
    send(net_socket, connect_info, sizeof(connect_info), 0);
    printf("client reached the server\n");
    
    int count;
    int term_buf[1];
    count = recv(net_socket, term_buf, sizeof(term_buf), 0);
    int term = term_buf[0];
    printf("\n%d devices in total\n", term);

    int i;
    pid_t *pids = malloc(sizeof(pid_t) * term);
    for(i=0;i<term;i++)
    {
        pids[i] = fork();
        if(pids[i] < 0)
        {
            perror("fork() error!\n");
            exit(-1);
        }
        else if(pids[i] == 0)
        {
            // just in the newly created child process
            double xy[3];
            int c_rev;
            c_rev = recv(net_socket, xy, sizeof(xy), 0);
            printf("process %d received x: %f, y: %f, constant %f\n", i, xy[1], xy[2], xy[0]);
            printf("%d bytes in total\n", c_rev);
            sleep(5);
            exit(0);
        }
    }

    pid_t child_id;
    int status;
    int n = term;
    while(n>0)
    {
        child_id = wait(&status);
        printf("Child with PID %ld exited with status 0x%x.\n", (long)child_id, status);
        n--;
    }
    // int rand_buf[10];
    // int count;
    // int i;

    // int child = fork();
    // if(child < 0)
    // {
    //     perror("fork error!");
    // }
    // else if(child == 0)
    // {
    //     // in the children process
    //     count = recv(net_socket, rand_buf, sizeof(rand_buf), 0);
    //     for(i=0;i<10;i++)
    //     {
    //         printf("%d\n", rand_buf[i]);
    //     }
    //     printf("%d bytes in total\n", count);
    // }
    // else
    // {
    //     // in the parent process, in which case, child == the PID of the children process
    //     count = recv(net_socket, rand_buf, sizeof(rand_buf), 0);
    //     for(i=0;i<10;i++)
    //     {
    //         printf("%d\n", rand_buf[i]);
    //     }
    //     printf("%d bytes in total\n", count);
    // }
    
    // close the socket
    close(net_socket);

    return 0;
}


