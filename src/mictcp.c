#include <mictcp.h>
#include <api/mictcp_core.h>


mic_tcp_sock my_sockets[1];



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
   set_loss_rate(0);

   return 0;
}




/*
 * Enables to link an adress to a socket
 * Returns 0 if succes, -1 in case of failure
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");


   return 1;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 1;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    my_sockets[socket].remote_addr = addr;
    return 1;
}




// ------------------- V1 : MIC-TCP data transfer without loss recovery -------------------

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

    // Sending the PDU
    int sent_data = IP_send(pdu, sock.remote_addr.ip_addr);

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

    app_buffer_put(pdu.payload);
}
