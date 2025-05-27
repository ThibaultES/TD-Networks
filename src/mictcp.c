#include <mictcp.h>
#include <api/mictcp_core.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_LENGTH 20

mic_tcp_sock my_sockets[1];
int PE = 0;
int PA = 0;
unsigned long TIMEOUT = 10;
int failBuffer[BUFFER_LENGTH] = { 0 }; // 1 for fail, 0 for success
int ACCEPT_RATE = 10;

pthread_mutex_t mutex;
pthread_cond_t cond;

/*
 * Enables to create a socket between the application and MIC-TCP
 * Returns the descriptor (int) of the socket, or -1 in case of failure
 */
int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Call to the function: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Necessary call */
   if(result == -1) {
    return -1;
   }
   set_loss_rate(30); ////////////////////////////////////    LOSS RATE    ///////////////////////////////////////////

   return 0;
}




/*
 * Enables to link an adress to a socket
 * Returns 0 if succes, -1 in case of failure
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");


   return 0;
}

/*
 * Put the socket in a state of accepting the connection
 * Returns 0 if succes, -1 in case of failure
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");

    pthread_mutex_lock(&mutex);

    //printf("Waiting for the state to become SYN_RECEIVED\n");
    while(my_sockets[socket].state != SYN_RECEIVED) {
        //printf("WAIT\n");
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);


    // Creating the synack PDU
    mic_tcp_pdu synack;
    synack.payload.size = 0;

    synack.header.dest_port = my_sockets[socket].local_addr.port;
    synack.header.source_port = my_sockets[socket].remote_addr.port;

    synack.header.ack = 1;
    synack.header.syn = 1;
    synack.header.fin = 0;

    IP_send(synack, my_sockets[socket].remote_addr.ip_addr); // only once 
    //printf("Just sent the SYNACK\n");

    //printf("Waiting for the state to become ESTABLISHED\n");

    pthread_mutex_lock(&mutex);
    while(my_sockets[socket].state != ESTABLISHED) {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    printf("- Connection established -\n");

    return 0;
}

/*
 * Enables to require the establishment of a connection
 * Returns 0 if connection is successfully established, -1 in case of failure
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");

    my_sockets[socket].remote_addr = addr;
    //mic_tcp_sock my_sock = my_sockets[socket];

    if(my_sockets[socket].state != IDLE) {
        //printf("Already connected or establishing a connection\n");
        return -1;
    }

    mic_tcp_pdu syn;
    //syn.payload.data = ""; // admissible loss rate in percent
    //syn.payload.size = 0;

    syn.header.source_port = my_sockets[socket].local_addr.port;
    syn.header.dest_port = my_sockets[socket].remote_addr.port; // = addr.port

    syn.header.ack = 0;
    syn.header.syn = 1;
    syn.header.fin = 0;

    char lossRate[3];
    sprintf(lossRate, "%d", ACCEPT_RATE);
    syn.payload.data = lossRate;
    syn.payload.size = 3;


    mic_tcp_pdu acksyn;
    acksyn.payload.size = 0;

    int received = -1;
    my_sockets[socket].state = SYN_SENT;
    //printf("-> STATE = SYN_SENT\n");

    while(received == -1) {
        IP_send(syn, addr.ip_addr);
        //printf("- Just sent a SYN -\n");
        received = IP_recv(&acksyn, &my_sockets[socket].local_addr.ip_addr, &addr.ip_addr, TIMEOUT);
        //printf("- SYN ACK Timeout -\n");
        if(acksyn.header.syn == 1 && acksyn.header.ack == 1) {
            //printf("- Receided SYN ACK -\n");
            break;
        }
        received = -1;
    }

    mic_tcp_pdu ack;
    ack.payload.size = 0;

    ack.header.source_port = my_sockets[socket].local_addr.port;
    ack.header.dest_port = my_sockets[socket].remote_addr.port;

    ack.header.ack = 1;
    ack.header.syn = 0;
    ack.header.fin = 0;

    IP_send(ack, addr.ip_addr);

    int ack_received = 0;
    while (ack_received != -1) {
        //printf("- Waiting -\n");
        ack_received = IP_recv(&acksyn, &my_sockets[socket].local_addr.ip_addr, &addr.ip_addr, 5*TIMEOUT);
        if(acksyn.header.syn == 1 && acksyn.header.ack == 1) {
            IP_send(ack, addr.ip_addr);
            //printf("- Resending ACK -\n");
        }
    }

    //printf("- Sent last ACK -\n");
    my_sockets[socket].state = ESTABLISHED;
    //printf("-> STATE = ESTABLISHED\n");
    printf("- Connection established -\n");

    return 0;
}



// Functions to store and manipulate a circular buffer of informations on last 5 losses
void pushBuffer(int* buffer, int newData)
{
    int N = BUFFER_LENGTH;
    for (int i = 0 ; i < N - 1 ; i++) {
        buffer[i] = buffer[i+1];
    }
    buffer[N-1] = newData;
}

int isGoodBuffer(int* buffer)
{
    int nbFail = 0;
    for(int i = 0 ; i < BUFFER_LENGTH; i++) {
        nbFail += buffer[i];
    }
    if((nbFail*100 / BUFFER_LENGTH) < ACCEPT_RATE) {
        return 1;
    }
    else {
        return -1;
    }
}


// ------------------- V1 : MIC-TCP data transfer without loss recovery --------------------

/*
 * Enables to request the sending of an applicative data
 * Returns the size of sent datas, -1 in case of failure
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Call to the function: "); printf(__FUNCTION__); printf("\n");



    // Get back the socket
    mic_tcp_sock sock = my_sockets[mic_sock];

    // Constructing the PDU
    mic_tcp_pdu pdu;
    pdu.payload.data = mesg;
    pdu.payload.size = mesg_size;

    pdu.header.source_port = sock.local_addr.port;
    pdu.header.dest_port = sock.remote_addr.port;
    
    pdu.header.seq_num = PE;
    PE = (PE + 1) % 2;


    // Not an ACK, FIN or SYN
    pdu.header.ack = 0;
    pdu.header.syn = 0;
    pdu.header.fin = 0;

    // Constructing the ack PDU
    mic_tcp_pdu ack_pdu;
    ack_pdu.payload.size = 0;

    int sent_data = -1;

    // Sending the PDU, and checking an ACK comes with a timer
    int received = -1;
    while( received == -1 ) {
        sent_data = IP_send(pdu, sock.remote_addr.ip_addr);
        received = IP_recv(&ack_pdu, &sock.local_addr.ip_addr, &sock.remote_addr.ip_addr, TIMEOUT);
        if(received != -1) {
            // We received a PDU
            
            if(ack_pdu.header.ack == 1) {
                // the PDU is an ACK
                if(ack_pdu.header.ack_num == PE) {
                    // The ACK has a correct ack number
                    pushBuffer(failBuffer, 1);
                    
                    break;
                }
                pushBuffer(failBuffer, 0);
            }
        }
        else { // timer timed out
            pushBuffer(failBuffer, 0);

            if(isGoodBuffer(failBuffer) > 0) {
                PE = (PE + 1) % 2;
                break;
            }
        }
    received = -1;
    }

    return sent_data;
}

/*
 * Enables the receiving application to request the recovery of datas stocked in the reception
 * buffers of the socket
 * Returns the number of read datas, -1 in case of failure
 * NB : this function calls the function app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    printf("[MIC-TCP] Call to the function: "); printf(__FUNCTION__); printf("\n");
    
    // Constructing a payload to receive datas in 
    mic_tcp_payload payload;
    payload.data = mesg;
    payload.size = max_mesg_size;
    
    // Ask for the datas in the reception buffer
    int written_size = app_buffer_get(payload);
    
    return written_size;
}



/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    return 1;
}



/*
 * Enables to treat a PDU MIC-TCP received (update sequence numbers and ack
 * numbers, ...) then inserts datas from the pdu (msg) into the reception buffer of the socket.
 * This function calls the function app_buffer_put(). It is called by initialize_components().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_ip_addr local_addr, mic_tcp_ip_addr remote_addr)
{
    printf("[MIC-TCP] Call to the function: "); printf(__FUNCTION__); printf("\n");

    if(my_sockets[0].state == ESTABLISHED){
        //printf("> Connected <\n");
        if(pdu.header.ack == 0) { // not an ack
            //printf("> Received ack <\n");

            if(pdu.header.seq_num == PA) {
                app_buffer_put(pdu.payload);
                PA = (PA + 1) % 2;
            } 

            // Creating the ack PDU
            mic_tcp_pdu ack_pdu;
            ack_pdu.payload.size = 0;
            ack_pdu.header.dest_port = pdu.header.source_port;
            ack_pdu.header.source_port = pdu.header.dest_port;
            ack_pdu.header.ack = 1;
            ack_pdu.header.ack_num = PA; // the old or new PA


            IP_send(ack_pdu, remote_addr);
            //printf("> Well answered to message <\n");
        }
    }

    if(my_sockets[0].state == IDLE) {
        //printf("State = IDLE\n");

        if(pdu.header.syn == 1){ // A syn

            pthread_mutex_lock(&mutex);

            my_sockets[0].state = SYN_RECEIVED;

            my_sockets[0].remote_addr.ip_addr = remote_addr;

            int lossRate = atoi(pdu.payload.data);
            char answer;
            //while(answer != 'y'){
            printf("Loss Rate is %d : do you agree ? [y/n]\n", lossRate);
            scanf("%c", &answer);
            //}
            if(answer == 'y') {
                printf("Loss rate accepted, continuing the connection.\n");
            }
            else{
                printf("Accepted anyway.\n");
            }
           
            //stock remote address in the socket
            //release mutex, broadcast
            pthread_cond_broadcast(&cond);

            pthread_mutex_unlock(&mutex);
        }
    }

 
    if(my_sockets[0].state == SYN_RECEIVED) {
        //printf("State = Syn received\n");

        if(pdu.header.ack == 1) {
            my_sockets[0].state = ESTABLISHED;

            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
        }
    }


}
