#include <sys/socket.h>  // datatype, macros, function needed for creating a socket
#include <netinet/in.h> //for constrants and structure needed for internet domain address
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h> //contents four variables, atoi();
#include <unistd.h> //read, write, close functions
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 1000

int cou=0;
int maincount=0;

static _Atomic unsigned int cli_count = 0;  //this can be accessed by muliple threads concurrently
static int uid = 10;

/* Client structure */
typedef struct{
        struct sockaddr_in address;
        int sockfd;
        int uid;
        char name[32];
} client_t; //struct variable

client_t *clients[MAX_CLIENTS]; //structure type array

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_overwrite_stdout() {
    printf("\r%s", "> "); // '\r' returns the left most position and just print >
    fflush(stdout);//clear the output stream and move the buffer data to console
}

void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') { //if find any '\n' or enter
      arr[i] = '\0'; //string might be terminated
      break;
    }
  }
}

void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* Add clients to queue */
void queue_add(client_t *cl){
        pthread_mutex_lock(&clients_mutex);

        for(int i=0; i < MAX_CLIENTS; ++i){
                if(!clients[i]){  //if clients[i] doesnot exist
                        clients[i] = cl; //put the cl value in clients[i]
                        break;
                }
        }

        pthread_mutex_unlock(&clients_mutex);
}

/* Remove clients from queue */
void queue_remove(int uid){
        pthread_mutex_lock(&clients_mutex);

        for(int i=0; i < MAX_CLIENTS; ++i){
                if(clients[i]){
                        if(clients[i]->uid == uid){
                                clients[i] = NULL;
                                break;
                        }
                }
        }

        pthread_mutex_unlock(&clients_mutex);
}

/* Send message to all clients except sender */
void send_message(char *s, int uid){
        pthread_mutex_lock(&clients_mutex);

        for(int i=0; i<MAX_CLIENTS; ++i){
                if(clients[i]){
                        if(clients[i]->uid != uid){
                                if(write(clients[i]-> sockfd, s, strlen(s)) < 0){
                                        perror("ERROR: write to descriptor failed");
                                        break;
                                }
                        }
                }
        }

        pthread_mutex_unlock(&clients_mutex);
}

/* Handle all communication with the client */
void *handle_client(void *arg){
        char buff_out[BUFFER_SZ];
        char name[32];
        int leave_flag = 0;

        cli_count++;
        client_t *cli = (client_t *)arg;

        //recv(cli->sockfd,buff_out,sizeof(buff_out),0);
        // Name
        if(recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) <  2 || strlen(name) >= 32-1){ //if recv from client socket sockfd is <=0
                printf("Didn't enter the name and left,user id - %d\n",cli->uid);
                leave_flag = 1;
                cou=-1;
                maincount=maincount+cou;
                //adding
                //exit(1);
                printf("                                           Number of clients :%d\n",maincount);
        } else{

                strcpy(cli->name, name); //save the recived client name from client socket and save it into the client structure in server
                sprintf(buff_out, "%s has joined\n", cli->name); //print this to all client also
                printf("%s", buff_out);
                send_message(buff_out, cli->uid); //send the msg to all
        }

        bzero(buff_out, BUFFER_SZ);

        while(1){
                if (leave_flag) {
                        break;
                }

                int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
                if (receive > 0){
                        if(strlen(buff_out) > 0){
                                send_message(buff_out, cli->uid); //send the msg to all cients except the client itself

                                str_trim_lf(buff_out, strlen(buff_out));
                                printf("%s \n", buff_out);//print the msg on server itself
                        }
                } else if (receive == 0 || strcmp(buff_out, "exit") == 0){
                        cou=-1;
                        maincount=maincount+cou;
                        sprintf(buff_out, "%s has left\n", cli->name);
                        printf("%s", buff_out);
                        printf("                                           Number of clients :%d\n",maincount);


                        send_message(buff_out, cli->uid);
                        leave_flag = 1;
                } else {
                        printf("ERROR: -1\n");
                        leave_flag = 1;
                }

                bzero(buff_out, BUFFER_SZ);
        }

  /* Delete client from queue and yield thread */
        close(cli->sockfd);
  queue_remove(cli->uid);
  free(cli);
  cli_count--;
  pthread_detach(pthread_self()); //if client disconnect thread is detach by itself

        return NULL;
}

int main(int argc, char **argv){
        char str[100];
        if(argc != 2){
                printf("Usage: %s <port>\n", argv[0]);
                return EXIT_FAILURE;
        }

        char *ip = "127.0.0.1"; //loop back ip address defined
        int port = atoi(argv[1]); //converting the user given port number string to integer
        //recv(cli->sockfd,str,strlen(str),0);
        int option = 1; //used in setsockopt
        int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr;  //sockaddr_in type server address
  struct sockaddr_in cli_addr; //sockaddr_in type client address
  pthread_t tid; //create a thread name tid

  /* Socket settings */
  listenfd = socket(AF_INET, SOCK_STREAM, 0);//creating a socket
  serv_addr.sin_family = AF_INET; //domain name giving in the server address
  serv_addr.sin_addr.s_addr = inet_addr(ip);//convert the ip address/loop back address into standard internet dot notation
  serv_addr.sin_port = htons(port); //convert a short integer to host byte order to network byte order

  /* Ignore pipe signals */
        signal(SIGPIPE, SIG_IGN); //ignors the signal SIGPIPE, passing SIG_IGN as signal handeler except SIGKILL or SIGSTOP
        //if a process whants to write but there is no other process attached for reading, this is an error and it will generate SIGPIPE

        if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){ //SOL_SOCKET is a socket layer, within that we are passing two options SO_REUSEPORT=multiple server can add on the same port, SO_REUSEADDR=
                perror("ERROR: setsockopt failed");
    return EXIT_FAILURE;
        }

        /* Bind */
  if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR: Socket binding failed");
    return EXIT_FAILURE;
  }


  /* Listen */
  if (listen(listenfd, 10) < 0) {
    perror("ERROR: Socket listening failed");
    return EXIT_FAILURE;
        }

        printf("=== THE CHATROOM SERVER ===\n");


        while(1){

                printf("                                           Number of clients :%d\n",maincount);
                socklen_t clilen = sizeof(cli_addr);
                connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

        /*      if(connfd>0)
                {
                //      recv(cli->sockfd, buff_out, sizeof(buff_out), 0);
                        cou=1;
                        maincount=maincount+cou;
                }
        */
        if(connfd<0)
        {
                printf("server accept fail");
                exit(0);
        }
        else
        {
                printf("sever accept the client\n");
                cou=1;
                maincount=maincount+cou;
        }



                /* Check if max clients is reached */
                if((cli_count + 1) == MAX_CLIENTS){
                        printf("Max clients reached. Rejected: ");
                        print_client_addr(cli_addr);
                        printf(":%d\n", cli_addr.sin_port);
                        close(connfd);
                        continue;
                }

                /* Client settings */
                client_t *cli = (client_t *)malloc(sizeof(client_t));
                cli->address = cli_addr;
                cli->sockfd = connfd;
                cli->uid = uid++;

                /* Add client to the queue and fork thread */
                queue_add(cli);
                pthread_create(&tid, NULL, &handle_client, (void*)cli);

                /* Reduce CPU usage */
                sleep(1);

        }

        return EXIT_SUCCESS;
}
