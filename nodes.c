#include<stdio.h>
#include<stdlib.h>

#include<sys/types.h>
#include<sys/socket.h>

#include<netinet/in.h>
#include<unistd.h>
#include<sys/wait.h>
#include<arpa/inet.h>

#include<pthread.h>

struct msg_buf
{
    long mesg_type;
    char mesg_text[100];
};

struct point
{
    double x;
    double y;
};

struct l_i_args
{
    int n;
    double *x;
    double *y;
    double a;
    double *r;
};

void * pthread_lagrange_interpolation(void *arg)
{
    //int n, double *x, double *y, double a
    struct l_i_args *li_arg = (struct l_i_args *)arg;
    int n = li_arg->n;
    double *x = li_arg->x;
    double *y = li_arg->y;
    double a = li_arg->a;
    double *r = li_arg->r;

    double numer = 1,denomi = 1,k = 0;
    int i, j;

    for(i=0;i<n;i++)
    {
        numer = 1;
        denomi = 1;
        for(j=0;j<n;j++)
        {
            if(j!=i)
            {
                numer = numer * (a - x[j]);
                denomi = denomi * (x[i] - x[j]);
            }
        }
        k = k + y[i]*(numer/denomi);
    }

    *r = k;
}

int socket_create()
{
    int socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    return socket_descriptor;
}

int socket_connect(int descriptor)
{
    int ret_val = -1;
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9001);
    server_addr.sin_addr.s_addr = inet_addr("192.168.0.9");

    ret_val = connect(descriptor, (struct sockaddr *)&server_addr, sizeof(server_addr));
    return ret_val;
}

void sm4_dec(double *tokens, int term)
{
    int i;
    //double *y = malloc(sizeof(double) * term);
    system("./decrypt.sh");
    FILE *fptr;
    fptr = fopen("dec_msg", "rb");

    for(i=0;i<term;i++)
    {
        fread(&tokens[i], sizeof(double), 1, fptr);
    }
    fclose(fptr);
}

// void read_decryption(double *dec_y, int term)
// {
//     int i;
//     FILE *fp;
//     fp = fopen("dec_msg", "rb");
//     for(i=0;i<term;i++)
//     {
//         fread(&dec_y[i], sizeof(double), 1, fp);
//     }
//     fclose(fp);
// }
int main()
{
    // create a socket, 0 means using TCP by default
    int net_socket;
    net_socket = socket_create();

    if(socket_connect(net_socket) == -1)
    {
        perror(" connect() error \n");
        exit(-1);
    }
    
    char connect_info[256] = "connect to server\n";
    send(net_socket, connect_info, sizeof(connect_info), 0);
    printf("client reached the server\n");
    
    int count;
    int term_buf[1];
    count = recv(net_socket, term_buf, sizeof(term_buf), 0);
    int term = term_buf[0];
    printf("\n%d devices in total\n", term);

    double *dec_y = malloc(sizeof(double) * term);
    sm4_dec(dec_y, term);

    int i;
    for(i=0;i<term;i++)
    {
        printf("%f ", dec_y[i]);
    }
    printf("dec finished\n");
    pid_t *pids = malloc(sizeof(pid_t) * term);
    for(i=0;i<term;i++)
    {
        pids[i] = fork();
        sleep(1);
        if(pids[i] < 0)
        {
            perror("fork() error!\n");
            exit(-1);
        }
        else if(pids[i] == 0)
        {
            // just in the newly created child process
            // xy saves x-coordinate and the constant coefficient
            double xy[2];
            int c_rev;
            c_rev = recv(net_socket, xy, sizeof(xy), 0);
            printf("process %d received x: %f, constant %f\n", i, xy[1], xy[0]);
            printf("%d bytes in total\n", c_rev);
            
            sleep(3);
            FILE *fptr;
            if(i == 0)
            {
                fptr = fopen("data.bin", "wb");
                if(fptr == NULL)
                {
                    perror("fopen error\n");
                    exit(-1);
                }
                struct point p;
                p.x = xy[1];
                //p.y = xy[2];
                p.y = dec_y[i];

                fwrite(&p, sizeof(p), 1, fptr);
                fclose(fptr);
            }
            else
            {
                fptr = fopen("data.bin", "ab");
                if(fptr == NULL)
                {
                    perror("fopen error\n");
                    exit(-1);
                }
                struct point p;
                p.x = xy[1];
                //p.y = xy[2];
                p.y = dec_y[i];

                fwrite(&p, sizeof(p), 1, fptr);
                fclose(fptr);
            }
            exit(0);
        }
    }

    pid_t child_id;
    int status;
    int n = term;
    while(n>0)
    {
        child_id = wait(&status);
        //printf("Child with PID %ld exited with status 0x%x.\n", (long)child_id, status);
        n--;
    }
    
    FILE *fptr_read;
    fptr_read = fopen("data.bin", "rb");
    if(fptr_read == NULL)
    {
        perror("fopen error\n");
        exit(-1);
    }
    struct point p_read;
    double *x = malloc(sizeof(double) * term);
    double *y = malloc(sizeof(double) * term);

    for(i=0;i<term;i++)
    {
        fread(&p_read, sizeof(p_read), 1, fptr_read);
        x[i] = p_read.x;
        y[i] = p_read.y;
        printf("x%d: %f, y%d: %f\n", i, x[i], i, y[i]);
    }
    fclose(fptr_read);
    // close the socket
    close(net_socket);

    // simulate group authentication by multithreading
    pthread_t *tid = malloc(sizeof(pthread_t) * term);
    struct l_i_args *arg = malloc(sizeof(struct l_i_args) * term);
    double *result = malloc(sizeof(double) * term);

    for(i = 0; i < term; i++)
    {
        arg[i].n = term;
        arg[i].x = x;
        arg[i].y = y;
        arg[i].a = 0;
        arg[i].r = &result[i];
        pthread_create(&tid[i], NULL, pthread_lagrange_interpolation,(void *)(&arg[i]));
    }
    
    for(i = 0; i < term; i++)
    {
        pthread_join(tid[i], NULL);
    }

    for(i = 0; i < term; i++)
    {
        printf("thread%d %f\n", i, result[i]);
    }

    free(pids);
    free(x);
    free(y);
    free(tid);
    free(arg);
    free(result);

    return 0;
}