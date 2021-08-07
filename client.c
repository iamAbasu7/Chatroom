#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LENGTH 1000

// Global variables
volatile sig_atomic_t flag = 0; //volatile tells the compiler that the value of the variable can be changed anytime, without assigning any work. this is a signal handeler
int sockfd = 0;
char name[20];

void str_overwrite_stdout() {
  printf("%s", "> ");//this is the character using before any msg
  fflush(stdout); //clear the output buffer and move the buffer data to console
}
//added new
void  INThandler(int sig)
{


     signal(sig, SIG_IGN); //catch the ctrl+c command from user
     exit(0);
     /*
     printf("OOPS!, did you hit Ctrl-C?\n"
            "Do you really want to quit? [y/n] ");
     c = getchar();
     if (c == 'y' || c == 'Y')
          exit(0);
     else
          signal(SIGINT, INThandler);
     getchar(); // Get new line character*/
}




void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') { //if there is a '\n or enter given by the user
      arr[i] = '\0'; //it means your msg is end ('\0' the string termination)
      break;
    }
  }
}

void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
}

void send_msg_handler() {
  char message[LENGTH] = {};
        char buffer[LENGTH + 32] = {};

  while(1) {
        str_overwrite_stdout();// >  Just for this
    fgets(message, LENGTH, stdin); //read character from a file stream
    str_trim_lf(message, LENGTH); //"\n" means your message is end

    if (strcmp(message, "exit") == 0) { //check your written msg is exit or not, if yes break the loop
                        break;
    } else {
      sprintf(buffer,"%s: %s\n",name,message); //print it in the buffer
      send(sockfd, buffer, strlen(buffer), 0);//send the buffer
    }

                bzero(message, LENGTH); //clear the msg
    bzero(buffer, LENGTH + 32); //clear the buffer
  }
  catch_ctrl_c_and_exit(2); //check for ctrl +c command
}

void recv_msg_handler() {
        char message[LENGTH] = {};
  while (1) {
                int receive = recv(sockfd, message, LENGTH, 0); //recieve msg from server socket= sockfd
    if (receive > 0) {
      printf("%s", message); //print the msg getting from server
      str_overwrite_stdout(); //

    } else if (receive == 0) {
            printf("Server is disconnected\n");
                        exit(1);
    } else {
                        // -1
                }
                memset(message, 0, sizeof(message));
  }
}

int main(int argc, char **argv){
        char str[100];
        if(argc != 2){
                printf("Usage: %s <port>\n", argv[0]);
                return EXIT_FAILURE;
        }

        char *ip = "127.0.0.1";
        int port = atoi(argv[1]);

        signal(SIGINT, catch_ctrl_c_and_exit);//if we get any interrupt signal we cansider it as exit

/*      printf("Please enter your name: ");//message to give the name
  fgets(name, 32, stdin);
  str_trim_lf(name, strlen(name));*/


/*      if (strlen(name) > 32 || strlen(name) < 2){
                printf("Name must be less than 30 and more than 2 characters.\n");
                return EXIT_FAILURE;
        }
*/
        struct sockaddr_in server_addr; //server structure decleared

        /* Socket settings */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip);
  server_addr.sin_port = htons(port);


  //send(sockfd,"Client Connected",sizeof("Client Connected"),0);
  // Connect to Server
  int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)); //CONNECTION WITH THE SERVER
  if (err == -1) {
                printf("ERROR: connect\n");
                return EXIT_FAILURE;
        }
  else
  {
          printf("Accepted\n");
  }
        // Send name
        printf("Please enter your name: ");//message to give the name
        //added new
        signal(SIGINT, INThandler); //SIGINT is a inturrupt signal for ctrl+c
/*      while(1)
        {
                pause();
        }*/

        fgets(name, 32, stdin); //taking the client name from the client in name
  str_trim_lf(name, strlen(name));//checking for the enter or '\n' signal



  if (strlen(name) > 32 || strlen(name) < 2){  //checking name characters
                printf("Name must be less than 30 and more than 2 characters.\n");
                return EXIT_FAILURE;
        }

        send(sockfd, name, 32, 0);

        //adding new
        printf("=== WELCOME TO THE CHATROOM  %s ===\n",name);

        pthread_t send_msg_thread;
  if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0){
                printf("ERROR: pthread\n");
    return EXIT_FAILURE;
        }

        pthread_t recv_msg_thread;
  if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0){
                printf("ERROR: pthread\n");
                return EXIT_FAILURE;
        }

        while (1){
                if(flag){
                                printf("\nDisconnected from server\n");
                                break;
                            }
                }

        close(sockfd);

        return EXIT_SUCCESS;
}
