#include "csapp.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int clientfd;  //file descriptor to communicate with the server
    char *host, *port;
    size_t n;

    char buffer[MAXLINE]; //MAXLINE = 8192 defined in csapp.h

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
    	exit(0);
    }

    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port); //wrapper function that calls getadderinfo, socket and connect functions for client side

    bool flag = false;

    // while option 2 is not selected
    while (!flag){
        //getting an option from the user
        printf("\n(1) Take a quiz\n");
        printf("(2) Terminate\n");
        printf("(3) Create a quiz\n");
        printf("Select an Option [1,2, or 3]: ");
    
        //get number from user
        bzero(buffer,MAXLINE);
        Fgets(buffer,MAXLINE,stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        //sending the number received from the user to the server
        n = write(clientfd,buffer,strlen(buffer));

        
        // option 1: take a quiz
        if (strcmp(buffer, "1")==0){
            // receive quiz count from server
            bzero(buffer,MAXLINE);
            n = read(clientfd,buffer,MAXLINE);
            // convert count back to integer
            int count = atoi(buffer);

            // print all quizzes (from most recent)
            printf("All available quizzes:\n");
            printf("----------------------\n");
            for (int i = count; i>0; i--){
                printf("Quiz [%d]\n", i);
            }

            // ask user which quiz they would like to take
            printf("\nWhich quiz would you like to take (ex: 1): ");
            // send to server
            bzero(buffer,MAXLINE);
            Fgets(buffer,MAXLINE,stdin);
            buffer[strcspn(buffer, "\n")] = '\0';
            n = write(clientfd,buffer,strlen(buffer));

            // read in all questions and answers given by server
            for (int i=1; i<=5; i++){
                int len;
                // read length of question
                n = read(clientfd, &len, sizeof(len));
                // read question
                bzero(buffer, MAXLINE);
                n = read(clientfd, buffer, len);
                buffer[len] = '\0';

                printf("Question %d: %s\n", i, buffer);
                
                for (int j=1; j<=4; j++){
                    // read length of answer
                    n = read(clientfd, &len, sizeof(len));
                    // read answer
                    bzero(buffer, MAXLINE);
                    n = read(clientfd, buffer, len);
                    buffer[len] = '\0';

                    printf("\tAnswer %d: %s\n", j, buffer);
                }
                printf("Choose your answer (1,2,3 or 4): ");
                // ask for answer and give to server
                bzero(buffer,MAXLINE);
                Fgets(buffer,MAXLINE,stdin);
                buffer[strcspn(buffer, "\n")] = '\0';
                n = write(clientfd,buffer,strlen(buffer));
            }
            // recieve score and display
            bzero(buffer,MAXLINE);
            n = read(clientfd,buffer,MAXLINE);
            printf("You got %s questions correct.\n", buffer);
        }

        // terminate
        else if (strcmp(buffer, "2")==0){
            // leave program
            flag=true;

            // receive message from server
            printf("Message From Server:\n");
            // resetting the buffer
            bzero(buffer,MAXLINE);
            // read in from server to client
            n = read(clientfd,buffer,MAXLINE);
            // print out buffer
            Fputs(buffer,stdout);
            
        }
        // create a quiz
        else if (strcmp(buffer, "3")==0){

            for (int i=0; i<5; i++){
                // ask for question
                printf("\nEnter question %d: ", i+1);
                bzero(buffer,MAXLINE);
                Fgets(buffer,MAXLINE,stdin);
                n = write(clientfd,buffer,strlen(buffer));
                for (int j=0; j<4; j++){
                    // ask for options
                    printf("\n\tEnter option %d: ", j+1);
                    bzero(buffer,MAXLINE);
                    Fgets(buffer,MAXLINE,stdin);
                    n = write(clientfd,buffer,strlen(buffer));
                }
                // ask for correct answer
                printf("\n\tEnter correct answer [1,2,3, or 4]: ");
                bzero(buffer,MAXLINE);
                Fgets(buffer,MAXLINE,stdin);
                n = write(clientfd,buffer,strlen(buffer));
            }
            // receive and print quiz confirm message from server
            printf("Message From Server:\n");
            bzero(buffer,MAXLINE);
            n = read(clientfd,buffer,MAXLINE);
            Fputs(buffer,stdout);
        }
        // handle invalid input
        else {
            // resetting the buffer
            bzero(buffer,MAXLINE);
            // read in from server to client
            n = read(clientfd,buffer,MAXLINE);
            // print out buffer
            Fputs(buffer,stdout);
        }
    }

    Close(clientfd);
    return 0;
}