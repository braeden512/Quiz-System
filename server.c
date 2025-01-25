#include "csapp.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_LENGTH 25
#define MAX_SIZE 1000
#define MAX_QUESTIONS 5
#define MAX_TEXT_LENGTH 256
#define MAX_CHOICES 4

// question struct
typedef struct {
    char text[MAX_TEXT_LENGTH];
    char choices[MAX_CHOICES][MAX_TEXT_LENGTH];
    int correct_choice;
    int user_choice;
} Question;

// quiz struct
typedef struct {
    Question questions[MAX_QUESTIONS];
    char name[MAX_TEXT_LENGTH];
    int id;
} Quiz;

// reader writer lock
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

// prototypes
void newQuiz(Quiz quizzes[], int *count, int connfd);
void serverFunction(int connfd);
void loadQuizzes(Quiz quizzes[], int *count);
void saveQuizzes(Quiz quizzes[], int count);
int gradeQuiz(Quiz quiz);

// handles one client connection
void serverFunction(int connfd) {
    char buffer[MAXLINE]; //MAXLINE = 8192 defined in csapp.h
    //messages
    char successMessage[MAXLINE] = "Quiz added successfully!\n";
    char closeMessage[MAXLINE] = "Connection closed!\n";
    char invalidOption[MAXLINE] = "Invalid option. Please try again.\n";
    size_t n;
    bool flag = false;
    // quiz count
    int count = 0;
    Quiz quizzes[1000];

    // while option 3 not selected
    while (!flag) {
        // resetting buffer
        bzero(buffer, MAXLINE);
        // read input from client
        n = read(connfd, buffer, MAXLINE);
        // make sure no newline character
        buffer[strcspn(buffer, "\n")] = '\0';
        
        // reload the quizzes from file to array
        // every time the client puts in an option
        loadQuizzes(quizzes, &count);

        // option 1: take quiz
        if (strcmp(buffer, "1")==0) {
            // write lock
            pthread_rwlock_wrlock(&rwlock);
            
            // convert count to string
            char count_str[10];
            sprintf(count_str, "%d", count);
            // send count to client
            n= write(connfd, count_str, strlen(count_str));

            // read in the quiz the user wants to take
            bzero(buffer, MAXLINE);
            n = read(connfd, buffer, MAXLINE);
            buffer[strcspn(buffer, "\n")] = '\0';

            int quiz_num = (atoi(buffer)-1);
            Quiz correct_quiz;
            // find the correct quiz and serve it to the client
            for (int i=0; i<count; i++){
                if (quizzes[i].id == quiz_num){
                    correct_quiz = quizzes[i];
                    break;
                }
            }

            for (int i=0; i<5; i++){
                int len = strlen(correct_quiz.questions[i].text);
                // send length of question
                n = write(connfd, &len, sizeof(len));
                // send question
                n= write(connfd, correct_quiz.questions[i].text, len);
                
                for (int j=0; j<4; j++){
                    len = strlen(correct_quiz.questions[i].choices[j]);
                    // send length of answer
                    n = write(connfd, &len, sizeof(len));
                    // send answer
                    n= write(connfd, correct_quiz.questions[i].choices[j], len);
                }
                // recieve answer from client
                bzero(buffer, MAXLINE);
                n = read(connfd, buffer, MAXLINE);
                buffer[strcspn(buffer, "\n")] = '\0';
                correct_quiz.questions[i].user_choice = atoi(buffer);
            }


            // grade the quiz and send it back to the client
            int score = gradeQuiz(correct_quiz);
            char score_str[MAXLINE];
            sprintf(score_str, "%d", score);
            strcat(score_str, "/5");
            // send to the server
            n= write(connfd, score_str, strlen(score_str));
            // unlock write lock
            pthread_rwlock_unlock(&rwlock);
        }
        // option 2: terminate
        else if (strcmp(buffer, "2") == 0) {
            // leave the program
            flag = true;

            // send close message back to client
            n= write(connfd, closeMessage, strlen(closeMessage));
        }
        // option 3: add quiz
        else if (strcmp(buffer, "3")==0) {
            // write lock
            pthread_rwlock_wrlock(&rwlock);
            // create a new quiz (only in struct)
            // passing address updates value of count
            newQuiz(quizzes, &count, connfd);
            // save changes in csv file
            saveQuizzes(quizzes, count);
            // unlock write lock
            pthread_rwlock_unlock(&rwlock);
            // send a success message to the client
            n= write(connfd, successMessage, strlen(successMessage));
        }
        else {
            // send an invalid message to the client
            n= write(connfd, invalidOption, strlen(invalidOption));
        }
    }
}

// add new record to employee struct
void newQuiz(Quiz quizzes[], int *count, int connfd) {
    char buffer[MAXLINE]; //MAXLINE = 8192 defined in csapp.h 
    size_t n;
    
    for (int i=0; i<5; i++){
        // read in each question
        bzero(buffer, MAXLINE);
        n = read(connfd, buffer, MAXLINE);
        buffer[strcspn(buffer, "\n")] = '\0';
        // update struct
        strcpy(quizzes[*count].questions[i].text, buffer);

        for (int j=0; j<4; j++){
            // read in each answer choice
            bzero(buffer, MAXLINE);
            n = read(connfd, buffer, MAXLINE);
            buffer[strcspn(buffer, "\n")] = '\0';
            strcpy(quizzes[*count].questions[i].choices[j], buffer);
        }

        // read in the correct answer
        bzero(buffer, MAXLINE);
        n = read(connfd, buffer, MAXLINE);
        buffer[strcspn(buffer, "\n")] = '\0';
        quizzes[*count].questions[i].correct_choice = atoi(buffer);
    }

    quizzes[*count].id = (*count);
    (*count)++;
}

int main(int argc, char *argv[]) {
    int listenfd;
    int connfd; //file descriptor to communicate with the client
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; /* Enough space for any address */
    
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
    }

    listenfd = Open_listenfd(argv[1]); //wrapper function that calls getadderinfo, socket, bind, and listen functions in the server side

    //Server runs in an infinite loop.
    //To stop the server process, it needs to be killed using the Ctrl+C key.
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);

        // wait for the connection from the client.
    	connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname,
                                   MAXLINE,client_port, MAXLINE, 0);

        // connect message
        printf("Connected to (%s, %s)\n", client_hostname, client_port);

        // fork a child process to handle the client
        pid_t pid = fork();
        // child process
        if (pid == 0) {
            Close(listenfd);
            serverFunction(connfd);
            Close(connfd);
            // disconnect message
            printf("Client at %s:%s has disconnected.\n", client_hostname, client_port);
            exit(EXIT_SUCCESS);
        }
        // parent process
        else if (pid > 0) {
            Close(connfd);
        }
        else {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
    }

    exit(0);
}
// load all of the quizzes from the csv file to the quizzes struct
void loadQuizzes(Quiz quizzes[], int *count) {
    char line[1000];
    *count = 0;

    // open the file as read
    FILE *file = fopen("book.csv", "r");
    if (file == NULL) {
        perror("File open failed");
        return;
    }

    int question_count = 0;
    
    // adding the records from records.csv to the employees structure
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%[^,],%[^,],%[^,],%[^,],%[^,],%d,%d",
            quizzes[*count].questions[question_count].text,
            quizzes[*count].questions[question_count].choices[0],
            quizzes[*count].questions[question_count].choices[1],
            quizzes[*count].questions[question_count].choices[2],
            quizzes[*count].questions[question_count].choices[3],
            &quizzes[*count].questions[question_count].correct_choice,
            &quizzes[*count].id);

        question_count++;
        if (question_count == 5){
            question_count = 0;
            (*count)++;
        }
    }
    // close file
    fclose(file);
}



// save all of the quizzes from the quizzes struct to the csv file
void saveQuizzes(Quiz quizzes[], int count) {
    FILE *file = fopen("book.csv", "w");
    if (file == NULL) {
        perror("File open failed");
        return;
    }

    // write each quiz to the file
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < MAX_QUESTIONS; j++) {
            Question *question = &quizzes[i].questions[j];
            
                fprintf(file, "%s,%s,%s,%s,%s,%d,%d\n",
                    question->text,
                    question->choices[0],
                    question->choices[1],
                    question->choices[2],
                    question->choices[3],
                    question->correct_choice,
                    quizzes[i].id);
        }
    }
    fclose(file);
}

// grades the quiz based on user vs correct answers
int gradeQuiz(Quiz quiz) {
    int score=0;
    for (int i=0; i<5; i++){
        if (quiz.questions[i].user_choice == quiz.questions[i].correct_choice) {
            score++;
        }
    }
    return score;
}


