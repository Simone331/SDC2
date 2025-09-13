#include <pthread.h>
#include <semaphore.h>
#include "common.h"

#define BUFFER_LENGTH 5
#define THREAD_COUNT 4
#define ITERATION_COUNT 10
#define PROD_ROLE 0
#define CONS_ROLE 1

typedef struct thread_args_s {
    int idx;
    int role;
} thread_args_t;

int buffer[BUFFER_LENGTH];
int write_index, read_index;

/**
 * COMPLETARE QUI
 *
 * Obiettivi:
 * - dichiarare i semafori necessari
 *
 */
sem_t producer_sem;
sem_t consumer_sem;
sem_t empty_sem;
sem_t filled_sem;

/***/


void producer_routine(int idx) {

    /**
     * COMPLETARE QUI
     *
     * Obiettivi:
     * - completare l'implementazione secondo il paradigma
     *   multi-prod/multi-cons inserendo operazioni sui
     *   semafori ove necessario
     * - gestire gli errori
     *
     */

     int i, ret;
     for (i = 0; i < ITERATION_COUNT; i++) {
        int value = idx*ITERATION_COUNT + i;
        ret= sem_wait(&empty_sem);
        if(ret) handle_error("error waiting on empty sem");
        ret= sem_wait(&producer_sem);
        if(ret) handle_error("error waiting on producer sem");

        buffer[write_index] = value;
        write_index = (write_index + 1) % BUFFER_LENGTH;
        ret= sem_post(&producer_sem);
        if(ret) handle_error("error posting on producer sem");
        ret= sem_post(&filled_sem);
        if(ret) handle_error("error posting on filled sem");

        printf("[Thread #%d] inserito %d nel buffer\n", idx, value);
     }
}

void consumer_routine(int idx) {

    /**
     * COMPLETARE QUI
     *
     * Obiettivi:
     * - completare l'implementazione secondo il paradigma
     *   multi-prod/multi-cons inserendo operazioni sui
     *   semafori ove necessario
     * - gestire gli errori
     *
     */

     int i, ret;
     for (i = 0; i < ITERATION_COUNT; i++) {
        ret= sem_wait(&filled_sem);
        if(ret) handle_error("error waiting on filled sem");
        ret=sem_wait(&consumer_sem);
        if(ret) handle_error("error waiting on consumer sem");

        int value = buffer[read_index];
        read_index = (read_index + 1) % BUFFER_LENGTH;
        ret=sem_post(&consumer_sem);
        if(ret) handle_error("error posting on consumer sem");
        ret=sem_post(&empty_sem);
        if(ret) handle_error("error posting on empty sem");

        printf("[Thread #%d] letto %d dal buffer\n", idx, value);
     }
}


void *thread_routine(void *args) {

    thread_args_t *thread_args = (thread_args_t*)args;
    if (thread_args->role == PROD_ROLE) {
        producer_routine(thread_args->idx);
    } else if (thread_args->role == CONS_ROLE) {
        consumer_routine(thread_args->idx);
    } else {
        printf("[Thread #%d] ruolo sconosciuto: %d\n", thread_args->idx, thread_args->role);
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {

    int ret;

    // inizializzazioni
    read_index = write_index = 0;

    /**
     * COMPLETARE QUI
     *
     * Obiettivi
     * - inizializzare i semafori dichiarati
     * - gestire gli errori
     */
    ret= sem_init(&consumer_sem,0,1);
    if(ret ) handle_error("error initializing consumer sem");
    ret= sem_init(&producer_sem,0,1);
    if(ret ) handle_error("error initializing producer sem");
    ret= sem_init(&empty_sem,0,BUFFER_LENGTH);
    if(ret ) handle_error("error initializing empty sem");
    ret= sem_init(&filled_sem,0,0);
    if(ret ) handle_error("error initializing filled sem"); 

    /***/

    /**
     * COMPLETARE QUI
     *
     * Obiettivi: creazione thread
     * - ogni thread deve invocare la funzione thread_routine()
     * - gli argomenti per il thread devono essere specificati
     *   all'interno di una struttura thread_args_t
     * - il campo role può assumere come valori PROD_ROLE o CONS_ROLE
     * - ogni thread ha un valore di idx diverso tra 0 e THREAD_COUNT-1
     * - ci dovranno essere THREAD_COUNT/2 thread con ruolo produttore
     *   ed altrettanti con ruolo consumatore (THREAD_COUNT è pari)
     * - gestire glie errori
     * - N.B.: si tenga presente che successivamente il programma
     *   dovrà attendere la terminazione di ogni thread
     * 
     */
    pthread_t threads[THREAD_COUNT];
    int i = 0;
    for(i=0; i<THREAD_COUNT; i++){
        thread_args_t *args = calloc(sizeof(thread_args_t));
        if(args == NULL) handle_error("error allocating memory for thread args");
        args->idx = i;
        if(i < THREAD_COUNT/2){
            args->role = PROD_ROLE;
        } else {
            args->role = CONS_ROLE;
        }
        ret = pthread_create(&threads[i], NULL, thread_routine, (void*)args);
        if(ret) handle_error_en(ret, "error creating thread");
    }

    /***/


    /**
     * COMPLETARE QUI
     *
     * Obiettivi:
     * - attendere la terminazione dei thread lanciati in precedenza
     * - gestire gli errori
     */
    for(i=0; i<THREAD_COUNT; i++){
        ret = pthread_join(threads[i], NULL);
        if(ret) handle_error_en(ret, "error joining thread");
    }
    /***/

    /**
     * COMPLETARE QUI
     *
     * Obiettivi:
     * - rilasciare i semafori inizializzati in precedenza
     * - gestire gli errori
     */
    ret= sem_destroy(&consumer_sem);
    if(ret ) handle_error("error destroying consumer sem");
    ret= sem_destroy(&producer_sem);
    if(ret ) handle_error("error destroying producer sem");
    ret= sem_destroy(&empty_sem);
    if(ret ) handle_error("error destroying empty sem");
    ret= sem_destroy(&filled_sem);
    if(ret ) handle_error("error destroying filled sem");

    /***/

    printf("[Main Thread] operazioni completate\n");

    return EXIT_SUCCESS;
}
