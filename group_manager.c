#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>
#include<pthread.h>

#include<sys/types.h>
#include<sys/socket.h>

#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>


// the structure of each term (node) of the polynomial
struct Node
{
    int coef;
    int exp;
    struct Node *next;
};

// n -- the term of the polynomial
// x -- x array
// y -- y array
// a -- the number to be computed in the polynomail
double lagrange_interpolation(int n, double *x, double *y, double a)
{
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

    return k;
}

// create new node
struct Node * create_node(int coef, int exp, struct Node **temp, struct Node **cur)
{
    struct Node *r;           // r points to a term node
    r = *cur;
    if(*temp == NULL)
    {
        r = malloc(sizeof(struct Node));
        r->coef = coef;
        r->exp = exp;
        *temp = r;
        r->next = malloc(sizeof(struct Node));
        r = r->next;
        r->next = NULL;
    }
    else
    {
        r->coef = coef;
        r->exp = exp;
        r->next = malloc(sizeof(struct Node));
        r = r->next;
        r->next = NULL;
    }

    return r;
}

double compute(struct Node *node, double x)
{
    double result = 0;
    while(node->next != NULL)
    {
        result += node->coef * pow(x,(double)node->exp);
        node = node->next;
    }
    return result;
}

double * compute_array(struct Node *node, int n, double *x)
{
    int i;
    double *y = malloc(sizeof(double) * n);

    for(i=0;i<n;i++)
    {
        y[i] = compute(node, x[i]);
    }

    return y;
}

void show(struct Node *node)
{
    while(node->next != NULL)
    {
        printf(" %dx^%d ", node->coef, node->exp);
        node = node->next;
        if(node->next != NULL)
            printf("+");
    }
    printf("\n");
}

double* generate_normal_distribution(int n)
{
    int i;
    int m = n + n % 2;
    double* values = (double*)calloc(m,sizeof(double));
    double average, deviation;
 
    if ( values )
    {
        for ( i = 0; i < m; i += 2 )
        {
            double x,y,rsq,f;
            do {
                x = 2.0 * rand() / (double)RAND_MAX - 1.0;
                y = 2.0 * rand() / (double)RAND_MAX - 1.0;
                rsq = x * x + y * y;
            }while( rsq >= 1. || rsq == 0. );
            f = sqrt( -2.0 * log(rsq) / rsq );
            values[i]   = x * f;
            values[i+1] = y * f;
        }
    }
    return values;
}

int socket_create()
{
    int socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    return socket_descriptor;
}

int socket_bind(int descriptor)
{
    int ret_val = -1;
    int local_port = 9001;
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(local_port);
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    
    ret_val = bind(descriptor, (struct sockaddr *)&server_addr, sizeof(server_addr));
    return ret_val;
}

int socket_accept(int descriptor)
{
    //struct sockaddr_in client_addr;
    int client_addr_len = sizeof(struct sockaddr_in);

    int new_socket = accept(descriptor, (struct sockaddr *)NULL, (socklen_t *)&client_addr_len);
    return new_socket;
}

void sm4_enc(double *y, int term)
{
    int i;

    FILE *fptr;
    fptr = fopen("msg", "wb");
    
    for(i=0;i<term;i++)
    {
        fwrite(&y[i], sizeof(double), 1, fptr);
    }
    fclose(fptr);
    
    system("./encrypt.sh");
}

void read_cipher(double *y, int term)
{
    int i;
    FILE *fptr;
    fptr = fopen("msg", "rb");

    for(i=0;i<term;i++)
    {
        fread(&y[i], sizeof(double), 1, fptr);
    }
    fclose(fptr);
}

int main()
{
    int term = 0;
    printf("enter the size of the group:");
    scanf("%d", &term);

    srand(time(NULL));

    int i;
    int coef;
    struct Node *poly = NULL;
    struct Node *cur = NULL;

    for(i=0;i<term;i++)
    {
        coef = rand() % 10;
        cur = create_node(coef, term-1-i, &poly, &cur);
    }
    show(poly);

    double *x = generate_normal_distribution(term);
    double *y = compute_array(poly, term, x);

    for(i=0;i<term;i++)
    {
        printf("x: %f, y: %f\n", x[i], y[i]);
    }

    // encrypt all of tokens (y)
    // sm4_enc(y, term);
    // read_cipher(y, term);
    // for(i=0;i<term;i++)
    // {
    //     printf("%f ", y[i]);
    // }
    // printf("\n");

  
    int server_socket = socket_create();
    
    if(socket_bind(server_socket) == -1)
    {
        perror(" bind() error \n");
        exit(-1);
    }

    listen(server_socket, 5);    // server socket listens on other sockets from client side

    int client_socket;
    client_socket = socket_accept(server_socket);

    char server_recv[256];
    int count = recv(client_socket, server_recv, sizeof(server_recv), 0);
    printf("%s", server_recv);

    int term_buf[1];
    term_buf[0] = term;
    send(client_socket, term_buf, sizeof(term_buf), 0);

    double xy[3];
    double const_coef = compute(poly, 0);
    for(i=0;i<term;i++)
    {
        xy[0] = const_coef;
        xy[1] = x[i];
        xy[2] = y[i];
        count = send(client_socket, xy, sizeof(xy), 0);
        printf("Group manager sent a pair of x:%f and y:%f to a node, %d bytes in total.\n", xy[1], xy[2], count);
    }
    close(server_socket);
    close(client_socket);
    free(poly);
    free(x);
    free(y);
    return 0;
}
