#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#include "common.h"

void send_over_pipe(int write_desc, char* buf, size_t data_len) {

	/**
     * COMPLETARE QUI
     *
     * Obiettivi:
     * - scrivere sulla pipe tutti i data_len byte contenuti nel buffer buf
     * - gestire eventuali interruzioni ed errori
     * - assicurarsi che tutti i byte siano stati scritti
     */
     int byte_sent=0 , ret;
     while (byte_sent<data_len){
        ret= write(write_desc, buf+ byte_sent, data_len-byte_sent);
        if (ret == -1 && errno== EINTR) continue;
        ERROR_HELPER(ret, "errore nella write"); 
        byte_sent+=ret;
     }

}

int read_from_pipe(int read_desc, char* buf, size_t buf_len) {

	/**
	 * COMPLETARE QUI
     *
     * Obiettivi:
     * - leggere un messaggio dalla pipe e salvarlo nel buffer buf
     * - il valore di ritorno di questa funzione deve essere pari al numero di
     *   byte letto dalla pipe (incluso il delimitatore di fine messaggio)
     * - gestire eventuali chiusure della pipe da parte dell'altro processo come se fosse un errore
     * - gestire come un errore il caso in cui il messaggio sia troppo grande rispetto al buffer a disposizione
     * - gestire eventuali interruzioni ed errori
     *
     * Il delimitatore di fine messaggio è il terminatore di riga '\n'.
	 **/
    int b_read=0 , ret;
     while (1){
        ret= read(read_desc, buf+b_read, 1);
        
        if (ret == -1 && errno== EINTR) continue;
        ERROR_HELPER(ret, "errore nella read"); 
        
        if(ret==0) ERROR_HELPER(-1, "pipe chiusa"); 
        
        if (buf[b_read++] == '\n') break;
        if (b_read == buf_len) ERROR_HELPER(-1, "Messaggio troppo lungo");
     }
  return b_read;

}

void child_loop(int write_desc, int read_desc) {
    char buf[BUF_SIZE];

    int i;
    for (i = 0; i < NUM_MSG; ++i) {
        int num = rand() % MAX_NUM;
        sprintf(buf, "%d\n", num);
        send_over_pipe(write_desc, buf, strlen(buf));

        int bytes_read = read_from_pipe(read_desc, buf, sizeof(buf));
        buf[bytes_read-1] = '\0';
        printf("[Figlio] ricevuto msg #%d: %s\n", i, buf);
    }
}

void parent_loop(int read_desc, int write_desc) {
    char buf[BUF_SIZE];

    int i;
    for (i = 0; i < NUM_MSG; ++i) {
        int bytes_read = read_from_pipe(read_desc, buf, sizeof(buf));
        buf[bytes_read-1] = '\0';
        printf("[Padre] ricevuto msg #%d: %s\n", i, buf);

        int num = atoi(buf);
        sprintf(buf, "%d\n", 2*num);
        send_over_pipe(write_desc, buf, strlen(buf));
    }
}

int main(int argc, char* argv[]) {

    srand(SEED);

	/**
     * COMPLETARE QUI
     *
     * Obiettivi:
	 * - creare due pipe
     * - creare un processo figlio
     * - chiudere i descrittori di pipe non utilizzati nel padre e nel figlio
     * - il processo figlio chiama child_loop(), che prende come argomenti i
     *   descrittori di pipe per scrivere/leggere (vedere signature funzione)
     * - il processo padre chiama parent_loop(), che prende come argomenti i
     *   descrittori di pipe per leggere/scrivere (vedere signature funzione)
	 * - il padre deve attendere il termine del figlio prima di terminare a sua
     *   volta
	 * - chiudere i descrittori di pipe in fase di uscita
     * - gestire eventuali errori
     */
    int ret, first_pipe[2], second_pipe[2];
    ret= pipe(first_pipe);
    ERROR_HELPER(ret, "pipe first_pipe");
    ret= pipe(second_pipe);
    ERROR_HELPER(ret, "pipe second_pipe");
    pid_t pid= fork();
    if (pid== -1){
        ERROR_HELPER(-1, "fork");
    } else if (pid == 0){
        ret= close(first_pipe[0]);
        ERROR_HELPER(ret, "close first_pipe[0] child");
        ret= close(second_pipe[1]);
        ERROR_HELPER(ret, "close second_pipe[1] child");
        child_loop(first_pipe[1], second_pipe[0]);
         ret = close(first_pipe[1]);
        ERROR_HELPER(ret, "[figlio] impossibile chiudere first_pipe[1]");

        ret = close(second_pipe[0]);
        ERROR_HELPER(ret, "[figlio] impossibile chiudere second_pipe[0]");
    }else{
        ret= close(first_pipe[1]);
        ERROR_HELPER(ret, "close first_pipe[1] parent");
        ret= close(second_pipe[0]);
        ERROR_HELPER(ret, "close second_pipe[0] parent");
        
        parent_loop(first_pipe[0], second_pipe[1]);
        ret= wait(0);
        ERROR_HELPER(ret, "wait del figlio");
        
        
        ret= close(first_pipe[0]);
        ERROR_HELPER(ret, "close first_pipe[0] parent");
        ret= close(second_pipe[1]);
        ERROR_HELPER(ret, "close second_pipe[1] parent"); 
        
    }
    


    return 0;
}
