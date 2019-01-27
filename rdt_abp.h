#include "chksum.h"
#include <queue>
FILE *fp;
int a_seq_no;
int b_seq_no;

int A_state = WAIT_FOR_PKG;
struct pkt cur_packet;

void printStat(){

}

void log(int AorB, char *msg, struct pkt *p, struct msg *m){
    char ch = (AorB == A)?'A':'B';
    if(p != NULL) {
        printf("[%c] %s. Packet->seq=%d,ack=%d,check=%d,data=%c\n", ch,
               msg, p->seqnum, p->acknum, p->checksum, p->payload[0]);
    } else if(m != NULL) {
        printf("[%c] %s. Message->data=%c\n", ch, msg, m->data[0]);
    } else {
        printf("[%c] %s.\n", ch, msg);
    }
}

void logfile(int AorB, char *msg, struct pkt *p, struct msg *m){
    fp = fopen("output_abp.doc","a");
    char ch = (AorB == A)?'A':'B';
    if(p != NULL) {
        fprintf(fp,"[%c] %s. Packet->seq=%d,ack=%d,check=%d,data=%c\n", ch,
               msg, p->seqnum, p->acknum, p->checksum, p->payload[0]);
    } else if(m != NULL) {
        fprintf(fp,"[%c] %s. Message->data=%c\n", ch, msg, m->data[0]);
    } else {
        fprintf(fp,"[%c] %s.\n", ch, msg);
    }
    fclose(fp);
}

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message){
    printf("================================ Inside A_output===================================\n");
    fp = fopen("output_abp.doc","a");
    fprintf(fp,"================================ Inside A_output===================================\n");
    fclose(fp);
    int checksum;
    log(A, "Receive an message from layer5", NULL, &message);
    logfile(A, "Receive an message from layer5", NULL, &message);
    // If the last packet have not been ACKed, just drop this message
    if(A_state != WAIT_FOR_PKG){
        log(A, "Drop this message, last message have not finished", NULL, &message);
        logfile(A, "Drop this message, last message have not finished", NULL, &message);
        return;
    }
    // set current package to not finished
    A_state = WAIT_FOR_ACK;

    // copy data from meg to pkt
    for (int i=0; i<20; i++){
        cur_packet.payload[i] = message.data[i];
    }
    // set current seqnum
    cur_packet.seqnum = a_seq_no;
    // we are send package, do not need to set acknum
    cur_packet.acknum = DEFAULT_ACK;

    // calculate check sum including seqnum and acknum
    checksum = calc_checksum(&cur_packet);
    // set check sum
    cur_packet.checksum = checksum;

    // send pkg to layer 3
    tolayer3(A, cur_packet);
    log(A, "Send packet to layer3", &cur_packet, &message);
    logfile(A, "Send packet to layer3", &cur_packet, &message);


    // start timer
    starttimer(A, TIMEOUT);

    printf("================================ Outside A_output===================================\n");
    fp = fopen("output_abp.doc","a");
    fprintf(fp,"================================ Outside A_output===================================\n");
    fclose(fp);
}

/* need be completed only for extra credit */
void B_output(struct msg message){

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet){
    printf("================================ Inside A_input===================================\n");
    fp = fopen("output_abp.doc","a");
    fprintf(fp,"================================ Inside A_input===================================\n");
    fclose(fp);
    log(A, "Receive ACK packet from layer3", &packet, NULL);
    logfile(A, "Receive ACK packet from layer3", &packet, NULL);

    // check checksum, if corrupted, do nothing
    if(packet.checksum != calc_checksum(&packet)){
        log(A, "ACK packet is corrupted", &packet, NULL);
        logfile(A, "ACK packet is corrupted", &packet, NULL);
        return;
    }

    // Delay ack? if ack is not expected, do nothing
    if(packet.acknum != a_seq_no){
        log(A, "ACK is not expected", &packet, NULL);
        logfile(A, "ACK is not expected", &packet, NULL);
        return;
    }

    // go to the next seq, and stop the timer
    a_seq_no = (a_seq_no + 1) % 2;
    stoptimer(A);
    // set the last package have received correctly
    A_state = WAIT_FOR_PKG;

    log(A, "ACK packet process successfully accomplished!!", &packet, NULL);
    logfile(A, "ACK packet process successfully accomplished!!", &packet, NULL);

    printf("================================ Outside A_input===================================\n");
    fp = fopen("output_abp.doc","a");
    fprintf(fp,"================================ Outside A_input===================================\n");
    fclose(fp);
}

/* called when A's timer goes off */
void A_timerinterrupt(void){
    log(A, "Time interrupt occur", &cur_packet, NULL);
    logfile(A, "Time interrupt occur", &cur_packet, NULL);
    /* if current package not finished, we resend it */
    if(A_state == WAIT_FOR_ACK){
        log(A, "Timeout! Send out the package again", &cur_packet, NULL);
        logfile(A, "Timeout! Send out the package again", &cur_packet, NULL);
        tolayer3(A, cur_packet);
        // start the timer again
        starttimer(A, TIMEOUT);
    }
    else{
        log(A, "BUG??? Maybe forgot to stop the timer", &cur_packet, NULL);
        logfile(A, "BUG??? Maybe forgot to stop the timer", &cur_packet, NULL);
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void){
    a_seq_no = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet){
    printf("================================ Inside B_input===================================\n");
    fp = fopen("output_abp.doc","a");
    fprintf(fp,"================================ Inside B_input===================================\n");
    fclose(fp);
    log(B, "Receive a packet from layer3", &packet, NULL);
    logfile(B, "Receive a packet from layer3", &packet, NULL);

    // check checksum, if corrupted, just drop the package
    if(packet.checksum != calc_checksum(&packet)){
        log(B, "Packet is corrupted", &packet, NULL);
        logfile(B, "Packet is corrupted", &packet, NULL);
        return;
    }

    // duplicate package, do not deliver data again. just resend the ACK again
    if(packet.seqnum != b_seq_no){
        log(B, "Duplicated packet detected", &packet, NULL);
        logfile(B, "Duplicated packet detected", &packet, NULL);
    }

    // normal package, deliver data to layer5
    else{
        b_seq_no = (b_seq_no + 1)%2;
        tolayer5(B, packet.payload);
        log(B, "Send packet to layer5", &packet, NULL);
        logfile(B, "Send packet to layer5", &packet, NULL);
    }

    // send back ack
    packet.acknum = packet.seqnum;
    packet.checksum = calc_checksum(&packet);

    tolayer3(B, packet);
    log(B, "Send ACK packet to layer3", &packet, NULL);
    logfile(B, "Send ACK packet to layer3", &packet, NULL);
    printf("================================ Outside B_input(packet) =========================\n");
    fp = fopen("output_abp.doc","a");
    fprintf(fp,"================================ Outside B_input(packet) =========================\n");
    fclose(fp);
}

/* called when B's timer goes off */
void B_timerinterrupt(void){
    printf("  B_timerinterrupt: B doesn't have a timer. ignore.\n");
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void){
    b_seq_no = 0;
}
