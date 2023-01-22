#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#define MAX_LINES 1000
#include <semaphore.h>

int read_done[MAX_LINES];
int upper_done[MAX_LINES];
int replace_done[MAX_LINES];
int countlines=0;
char lines[MAX_LINES][1024]; // array to store the lines read by the threads
int line_number = 0;     // global variable shared by all read threads
pthread_mutex_t line_number_mutex; // mutex to protect access to line_number
sem_t file_sem;         // semaphore to synchronize access to the file
int current_position;
int current_position_write;
pthread_mutex_t upper_index_mutex;
pthread_mutex_t lines_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lines_read_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t uuu = PTHREAD_MUTEX_INITIALIZER;
sem_t upper_sem;
sem_t replace_sem;
sem_t write_sem;
int upper_index =0;
pthread_cond_t condRead= PTHREAD_MUTEX_INITIALIZER;
int linesReady=0;
pthread_mutex_t replace_index_mutex = PTHREAD_MUTEX_INITIALIZER; // lock for replace_index
pthread_cond_t data_replace = PTHREAD_COND_INITIALIZER; // condition variable for data replace
int replace_index = 0; // next array index to be replaced
int write_index = 0; // next array index to be written
pthread_mutex_t write_index_mutex = PTHREAD_MUTEX_INITIALIZER;

void* read_thread_func(void* arg) {
    //char* filename = (char*)arg; // cast the argument passed to the thread to a char*sem_wait(&file_sem);
     FILE* file = fopen("asd.txt", "r"); // open the file for reading. And unfortunatly here i couldnt take argument of the name. So i have to deal 
                                         //the file name is always gonna be "asd.txt"
    while (1) {
        int my_line_number;


        sem_wait(&file_sem);
        // locking for the index number
        pthread_mutex_lock(&line_number_mutex);

        // check if there are more lines to read
      if (line_number >= countlines) {
            pthread_mutex_unlock(&line_number_mutex);
            sem_post(&file_sem);
            break;
        }
        // get next line number then increase the global for the other threads
        my_line_number = line_number++;
        pthread_mutex_unlock(&line_number_mutex);

        // release the lock
      

       
    pthread_mutex_lock(&lines_read_mutex);
    // move the file pointer to the appropriate line and lock the line so they wont try to read at the same time
    fseek(file, current_position, SEEK_SET);
 //Add the readed line to the array
    if (fgets(lines[my_line_number], 1024, file) == NULL)
        break;
      current_position = ftell(file);

      linesReady=1;

         
      printf("Read Thread %ld: read line %d: %s", pthread_self(), my_line_number, lines[my_line_number]); 
      read_done[my_line_number]=1;

     pthread_mutex_unlock(&lines_read_mutex);



    sem_post(&file_sem);
        

    }

    // close the file
    fclose(file);


    return NULL;
}


void* upper_thread_func(void* arg) {
    while (1) {
        int my_index;

        while(1){  //i tried to use cond here but didnt work somehow then needed to replace more bruteforce way.
        if(read_done[my_index]&&!replace_done[my_index]){  //if the reading is done and replacing isnt done we upper casing.
        break;                                             //I know this isnt what we wanted but couldnt make pthread_cond_wait work.
        }
}
        sem_wait(&upper_sem);

        // acquire the lock on the upper_index variable
        pthread_mutex_lock(&upper_index_mutex);
        if (upper_index >= countlines) {
            pthread_mutex_unlock(&upper_index_mutex);

            sem_post(&upper_sem);
            break;
        }
        my_index = upper_index++;
        pthread_mutex_unlock(&upper_index_mutex);
        // acquire the lock on the lines array

        for (int i = 0; lines[my_index][i]; i++) {
            lines[my_index][i] = toupper(lines[my_index][i]);  //Chancing the upper case
        }
        printf("Upper Thread %ld: line: %d uppercased line %s", pthread_self(),my_index, lines[my_index]);
        upper_done[my_index]=1;

        sem_post(&upper_sem);  //Opening the sem.


    }
    return NULL;
}

void* replace_thread_func(void* arg) {
    while (1) {
        int my_index_replace;

        while (1) {
           if(read_done[my_index_replace]&&upper_done[my_index_replace])  ///Same Brute Force way. as upper thread
           break;
        }
        sem_wait(&replace_sem);
        pthread_mutex_lock(&replace_index_mutex);  //Locking the thread for index
        if (replace_index >= countlines) {
            pthread_mutex_unlock(&replace_index_mutex);

            break;
        }
        my_index_replace = replace_index++;  //Acquring the index
        pthread_mutex_unlock(&replace_index_mutex);  ///then unlocking the thread
        for (int i = 0; lines[my_index_replace][i]; i++) {
            if (lines[my_index_replace][i] == ' ') {
                lines[my_index_replace][i] = '_';  //Doing replacing.
            }
        }
        replace_done[my_index_replace]=1;
        printf("Replace Thread %ld: replaced line %s", pthread_self(), lines[my_index_replace]);
     sem_post(&replace_sem);

    }
    return NULL;
}



void* write_thread_func(void* arg) {
    FILE* fp = fopen("output.txt", "w");   //Opening the file
    if (fp == NULL) {
        printf("Error opening file\n");
        return NULL;
    }
    while (1) {
        int my_index;

        while (1) {
        if(upper_done[my_index]&&replace_done[my_index])   //this is not brute force actually. We are checking here if the upper and replace thread is done
                                                            //at the index.
        break;

        }
        sem_wait(&write_sem);     
        pthread_mutex_lock(&write_index_mutex);    //Lock for the index and only 1 can enter cause of sem wait.
        if (write_index >= countlines) {
            pthread_mutex_unlock(&write_index_mutex);

            break;
        }
        my_index = write_index++;
        pthread_mutex_unlock(&write_index_mutex);

        fseek(fp, current_position_write, SEEK_SET); //Getting the cursor of the file 

        fprintf(fp, "%s", lines[my_index]); //Writing to the file
        current_position_write = ftell(fp);
        printf("Write Thread %ld: wrote line %s", pthread_self(), lines[my_index]);  
          sem_post(&write_sem);
    }
    fclose(fp);
    return NULL;
}



pthread_t th[8];
pthread_t tu[8];
pthread_t tr[8];
pthread_t tw[8];

int main(int argc, char* argv[]) {
char* fpp = NULL;
    int num_threads[4] = {0};      //Something went wrong here couldnt get parameters as input. I Dont know why. Just have to deal with it as 8 threads.
                                    //All the time

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            fpp = argv[i+1];
            i++;
        } else if (strcmp(argv[i], "-n") == 0) {
            for (int j = 0; j < 4; j++) {
                num_threads[j] = atoi(argv[i+1+j]);
            }
            i += 4;
        }
    }

    // check that file_name and num_threads are not NULL
   // if (fpp== NULL || num_threads[0] == 0) {
   //     printf("Usage: project3.out -d <file_name> -n <num_read> <num_upper> <num_replace> <num_write>\n");
   //     return 1;
   // }
//pthread_t tr[num_threads[0]];
//pthread_t tu[num_threads[1]];
//pthread_t tr[num_threads[2]];
//pthread_t tw[num_threads[3]];

FILE *fp=fopen("asd.txt","r");  //opening file names "asd.txt"
char ch;



while(!feof(fp)){
  ch=fgetc(fp);
  if(ch=='\n')
  countlines++;          //Get the line number first of all
}

int i;
    pthread_mutex_init(&line_number_mutex, NULL);
    pthread_mutex_init(&upper_index_mutex, NULL);
    sem_init(&file_sem, 0, 1);
    sem_init(&upper_sem, 0, 1);
    sem_init(&replace_sem, 0, 1);
    sem_init(&write_sem, 0, 1);
        for (i = 0; i < 8; i++) {
        if (pthread_create(tr + i, NULL, read_thread_func, NULL) != 0) {
            perror("Failed to create thread");       //creating the threads here.
            return 1;
        }
        printf("Thread %ld has started\n", pthread_self());
         if (pthread_create(tu + i, NULL, upper_thread_func, NULL) != 0) {
            perror("Failed to create thread");
            return 1;
        }
        printf("Upper_Thread %ld has started\n", pthread_self());
        if (pthread_create(tr + i, NULL, replace_thread_func, NULL) != 0) {
            perror("Failed to create thread");
            return 1;
        }
        printf("Replace_Thread %ld has started\n", pthread_self());
        if (pthread_create(tw + i, NULL, write_thread_func, NULL) != 0) {
            perror("Failed to create thread");
            return 1;
        }
        printf("Write_Thread %ld has started\n", pthread_self());
    }

    for (i = 0; i < 8; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            return 2;
        }
        printf("Thread %ld has finished execution\n", pthread_self());
        if (pthread_join(tu[i], NULL) != 0) {
            return 2;
        }
        printf("Upper_Thread %ld has finished execution\n", pthread_self());
        if (pthread_join(tr[i], NULL) != 0) {
            return 2;
        }
        printf("Upper_Thread %ld has finished execution\n", pthread_self());
        if (pthread_join(tw[i], NULL) != 0) {
            return 2;
        }
        printf("Write_Thread %ld has finished execution\n", pthread_self());

    }

  /*   for (i = 0; i < 8; i++) {
        if (pthread_create(tu + i, NULL, upper_thread_func, NULL) != 0) {
            perror("Failed to create thread");
            return 1;
        }
        printf("Upper_Thread %d has started\n", i);
    }
    for (i = 0; i < 8; i++) {
        if (pthread_join(tu[i], NULL) != 0) {
            return 2;
        }
        printf("Upper_Thread %d has finished execution\n", i);
    }
*/
      pthread_mutex_destroy(&line_number_mutex);
       pthread_mutex_destroy(&upper_index_mutex);
       pthread_mutex_destroy(&upper_index_mutex);
      pthread_mutex_destroy(&uuu);
        pthread_mutex_destroy(&replace_index_mutex);
           pthread_mutex_destroy(&data_replace);
             pthread_mutex_destroy(&write_index_mutex);
      sem_destroy(&file_sem);
      sem_destroy(&upper_sem);
      sem_destroy(&write_sem);
      sem_destroy(&replace_sem);





}
