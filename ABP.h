#include "chksum.h"

#define TIMEOUT 20.0
#define DEFAULT_ACK 5
#define WAIT_FOR_PKG 3
#define WAIT_FOR_ACK 4

int A_seqnum = 0;
int B_seqnum = 0;

int A_from_layer5 = 0;
int A_to_layer3 = 0;
int B_from_layer3 = 0;
int B_to_layer5 = 0;

int A_state = WAIT_FOR_PKG;
struct pkt cur_packet;

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message){
    printf("================================ Inside A_output===================================\n");
    int i;
    int checksum = 0;

    ++A_from_layer5;
    log(A, "Receive an message from layer5", NULL, &message);

    /* If the last packet have not been ACKed, just drop this message */
    if(A_state != WAIT_FOR_PKG){
        log(A, "Drop this message, last message have not finished", NULL, &message);
        return;
    }
    /* set current package to not finished */
    A_state = WAIT_FOR_ACK;

    /* copy data from meg to pkt */
    for (i=0; i<20; i++){
        cur_packet.payload[i] = message.data[i];
    }
    /* set current seqnum */
    cur_packet.seqnum = A_seqnum;
    /* we are send package, do not need to set acknum */
    cur_packet.acknum = DEFAULT_ACK;

    /* calculate check sum including seqnum and acknum */
    checksum = calc_checksum(&cur_packet);
    /* set check sum */
    cur_packet.checksum = checksum;

    /* send pkg to layer 3 */
    tolayer3(A, cur_packet);
    ++A_to_layer3;
    log(A, "Send packet to layer3", &cur_packet, &message);

    /* start timer */
    starttimer(A, TIMEOUT);

    printf("================================ Outside A_output===================================\n");
}

/* need be completed only for extra credit */
void B_output(struct msg message){

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet){
    printf("================================ Inside A_input===================================\n");
    log(A, "Receive ACK packet from layer3", &packet, NULL);

    /* check checksum, if corrupted, do nothing */
    if(packet.checksum != calc_checksum(&packet)){
        log(A, "ACK packet is corrupted", &packet, NULL);
        return;
    }

    /* Delay ack? if ack is not expected, do nothing */
    if(packet.acknum != A_seqnum){
        log(A, "ACK is not expected", &packet, NULL);
        return;
    }

    /* go to the next seq, and stop the timer */
    A_seqnum = (A_seqnum + 1) % 2;
    stoptimer(A);
    /* set the last package have received correctly */
    A_state = WAIT_FOR_PKG;

    log(A, "ACK packet process successfully accomplished!!", &packet, NULL);

    printf("================================ Outside A_input===================================\n");
}

/* called when A's timer goes off */
void A_timerinterrupt(void){
    log(A, "Time interrupt occur", &cur_packet, NULL);
    /* if current package not finished, we resend it */
    if(A_state == WAIT_FOR_ACK){
        log(A, "Timeout! Send out the package again", &cur_packet, NULL);
        tolayer3(A, cur_packet);
        ++A_to_layer3;
        /* start the timer again */
        starttimer(A, TIMEOUT);
    }
    else{
        log(A, "BUG??? Maybe forgot to stop the timer", &cur_packet, NULL);
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void){

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet){
    printf("================================ Inside B_input===================================\n");
    log(B, "Receive a packet from layer3", &packet, NULL);
    ++B_from_layer3;

    /* check checksum, if corrupted, just drop the package */
    if(packet.checksum != calc_checksum(&packet)){
        log(B, "Packet is corrupted", &packet, NULL);
        return;
    }

    /* duplicate package, do not deliver data again.
    just resend the ACK again */
    if(packet.seqnum != B_seqnum){
        log(B, "Duplicated packet detected", &packet, NULL);
    }
        /* normal package, deliver data to layer5 */
    else{
        B_seqnum = (B_seqnum + 1)%2;
        tolayer5(B, packet.payload);
        ++B_to_layer5;
        log(B, "Send packet to layer5", &packet, NULL);
    }

    /* send back ack */
    packet.acknum = packet.seqnum;
    packet.checksum = calc_checksum(&packet);

    tolayer3(B, packet);
    log(B, "Send ACK packet to layer3", &packet, NULL);
    printf("================================ Outside B_input(packet) =========================\n");
}

/* called when B's timer goes off */
void B_timerinterrupt(void){
    printf("  B_timerinterrupt: B doesn't have a timer. ignore.\n");
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void){

}
