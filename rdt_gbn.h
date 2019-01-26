#include "chksum.h"
#include <stdio.h>

FILE *fp;
// current expected seq for B
int b_seq_no;

/* defines for sender */
/* define window size */
#define N 5
int window_base = 0;
int nextseq = 0;
/* ring buffer for all packets in window */
int packets_base = 0;
struct pkt packets[N];

/* extra buffer used when window is full */
struct extra_buf {
    struct msg message;
    struct extra_buf *next;
};

struct extra_buf *list_head = NULL;
struct extra_buf *list_end = NULL;
int extra_buf_size = 0;

void push_msg(struct msg *m) {
    int i;
    /*allocate memory*/
    struct extra_buf *n = (struct extra_buf *) malloc(sizeof(struct extra_buf));
    if (n == NULL) {
        printf("no enough memory\n");
        fp = fopen("output_gbn.doc","a");
        fprintf(fp,"no enough memory\n");
        fclose(fp);
        return;
    }
    n->next = NULL;
    /*copy packet*/
    for (i = 0; i < 20; ++i) {
        n->message.data[i] = m->data[i];
    }

    /* if list empty, just add into the list*/
    if (list_end == NULL) {
        list_head = n;
        list_end = n;
        ++extra_buf_size;
        return;
    }
    /* otherwise, add at the end*/
    list_end->next = n;
    list_end = n;
    ++extra_buf_size;
}

struct extra_buf *pop_msg() {
    struct extra_buf *p;
    /* if the list is empty, return NULL*/
    if (list_head == NULL) {
        return NULL;
    }

    /* retrive the first extra_buf*/
    p = list_head;
    list_head = p->next;
    if (list_head == NULL) {
        list_end = NULL;
    }
    --extra_buf_size;

    return p;
}

void log(int AorB, char *msg, struct pkt *p, struct msg *m) {
    char ch = (AorB == A) ? 'A' : 'B';
    if (AorB == A) {
        if (p != NULL) {
            printf("[%c] %s. Window[%d,%d) Packet[seq=%d,ack=%d,check=%d,data=%c..]\n", ch, msg,
                   window_base, nextseq, p->seqnum, p->acknum, p->checksum, p->payload[0]);
        } else if (m != NULL) {
            printf("[%c] %s. Window[%d,%d) Message[data=%c..]\n", ch, msg, window_base, nextseq, m->data[0]);
        } else {
            printf("[%c] %s.Window[%d,%d)\n", ch, msg, window_base, nextseq);
        }
    } else {
        if (p != NULL) {
            printf("[%c] %s. Expected[%d] Packet[seq=%d,ack=%d,check=%d,data=%c..]\n", ch, msg,
                   b_seq_no, p->seqnum, p->acknum, p->checksum, p->payload[0]);
        } else if (m != NULL) {
            printf("[%c] %s. Expected[%d] Message[data=%c..]\n", ch, msg, b_seq_no, m->data[0]);
        } else {
            printf("[%c] %s.Expected[%d]\n", ch, msg, b_seq_no);
        }
    }
}

void logfile(int AorB, char *msg, struct pkt *p, struct msg *m) {
    char ch = (AorB == A) ? 'A' : 'B';
    fp = fopen("output_gbn.doc","a");
    if (AorB == A) {
        if (p != NULL) {
            fprintf(fp,"[%c] %s. Window[%d,%d) Packet[seq=%d,ack=%d,check=%d,data=%c..]\n", ch, msg,
                   window_base, nextseq, p->seqnum, p->acknum, p->checksum, p->payload[0]);
        } else if (m != NULL) {
            fprintf(fp,"[%c] %s. Window[%d,%d) Message[data=%c..]\n", ch, msg, window_base, nextseq, m->data[0]);
        } else {
            fprintf(fp,"[%c] %s.Window[%d,%d)\n", ch, msg, window_base, nextseq);
        }
    } else {
        if (p != NULL) {
            fprintf(fp,"[%c] %s. Expected[%d] Packet[seq=%d,ack=%d,check=%d,data=%c..]\n", ch, msg,
                   b_seq_no, p->seqnum, p->acknum, p->checksum, p->payload[0]);
        } else if (m != NULL) {
            fprintf(fp,"[%c] %s. Expected[%d] Message[data=%c..]\n", ch, msg, b_seq_no, m->data[0]);
        } else {
            fprintf(fp,"[%c] %s.Expected[%d]\n", ch, msg, b_seq_no);
        }
    }
    fclose(fp);
}

/* helper functions for the window */
int window_isfull() {
    if (nextseq >= window_base + N)
        return 1;
    else
        return 0;
}

struct pkt *get_free_packet() {
    int cur_index = 0;

    /* check is window full */
    if (nextseq - window_base >= N) {
        log(A, "Alloc packet failed. The window is full already", NULL, NULL);
        logfile(A, "Alloc packet failed. The window is full already", NULL, NULL);
        return NULL;
    }

    /* get next free packet */
    cur_index = (packets_base + nextseq - window_base) % N;

    return &(packets[cur_index]);
}

struct pkt *get_packet(int seqnum) {
    int cur_index = 0;
    if (seqnum < window_base || seqnum >= nextseq) {
        log(A, "Seqnum is not within the window", NULL, NULL);
        logfile(A, "Seqnum is not within the window", NULL, NULL);
        return NULL;
    }

    cur_index = (packets_base + seqnum - window_base) % N;
    return &(packets[cur_index]);
}

/* Every time there is a new packet come,
 * a) we append this packet at the end of the extra buffer.
 * b) Then we check the window is full or not; If the window is full, we just leave the packet in the extra buffer;
 * c) If the window is not full, we retrieve one packet at the beginning of the extra buffer, and process it.
 */
void A_output(struct msg message);

void free_packets(int acknum) {
    int count;
    if (acknum < window_base || acknum >= nextseq)
        return;

    packets_base += (acknum - window_base + 1);
    count = acknum - window_base + 1;
    packets_base = packets_base % N;
    window_base = acknum + 1;

    //when the window size decrease, check the
    //extra buffer, if we need any message need to sned
    while (count > 0) {
        struct extra_buf *n = pop_msg();
        if (n == NULL) {
            break;
        }
        A_output(n->message);
        free(n);
        count--;
    }
}


/* called from layer 5, passed the data to be sent to other side */

/* Every time there is a new packet come,
 * a) we append this packet at the end of the extra buffer.
 * b) Then we check the window is full or not; If the window is full, we just leave the packet in the extra buffer;
 * c) If the window is not full, we retrieve one packet at the beginning of the extra buffer, and process it.
 */
void A_output(struct msg message) {
    printf("================================ Inside A_output===================================\n");
    fp = fopen("output_gbn.doc","a");
    fprintf(fp,"================================ Inside A_output===================================\n");
    fclose(fp);
    int i;
    int checksum = 0;
    struct pkt *p = NULL;
    struct extra_buf *n;

    /* append message to buffer */
    /*Step (a)*/
    push_msg(&message);

    /* If the last packet have not been ACKed, just drop this message */
    /*Step (b)*/
    if (window_isfull()) {
        log(A, "Window is full already, save message to extra buffer", NULL, &message);
        logfile(A, "Window is full already, save message to extra buffer", NULL, &message);
        return;
    }

    /* pop a message from the extra buffer */
    /* Step(c)*/
    n = pop_msg();
    if (n == NULL) {
        printf("No message need to process\n");
        fp = fopen("output_gbn.doc","a");
        fprintf(fp,"No message need to process\n");
        fclose(fp);
        return;
    }
    /* get a free packet from the buf */
    p = get_free_packet();
    if (p == NULL) {
        log(A, "BUG! The window is full already", NULL, &message);
        logfile(A, "BUG! The window is full already", NULL, &message);
        return;
    }
    //++A_from_layer5;
    log(A, "Receive an message from layer5", NULL, &message);
    logfile(A, "Receive an message from layer5", NULL, &message);
    /* copy data from msg to pkt */
    for (i = 0; i < 20; i++) {
        p->payload[i] = n->message.data[i];
    }
    /* after used, free the extra_buf */
    free(n);

    /* set current seqnum */
    p->seqnum = nextseq;
    /* we are send package, do not need to set acknum */
    p->acknum = DEFAULT_ACK;

    /* calculate check sum including seqnum and acknum */
    checksum = calc_checksum(p);
    /* set check sum */
    p->checksum = checksum;

    /* send pkg to layer 3 */
    tolayer3(A, *p);
    //++A_to_layer3;

    /* we only start the timer for the oldest packet */
    if (window_base == nextseq) {
        starttimer(A, TIMEOUT);
    }

    ++nextseq;

    log(A, "Send packet to layer3", p, &message);
    logfile(A, "Send packet to layer3", p, &message);

    printf("================================ Outside A_output===================================\n");
    fp = fopen("output_gbn.doc","a");
    fprintf(fp,"================================ Outside A_output===================================\n");
    fclose(fp);
}

/* need be completed only for extra credit */
void B_output(struct msg message) {

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
    printf("================================ Inside A_input===================================\n");
    fp = fopen("output_gbn.doc","a");
    fprintf(fp,"================================ Inside A_input===================================\n");
    fclose(fp);
    log(A, "Receive ACK packet from layer3", &packet, NULL);
    logfile(A, "Receive ACK packet from layer3", &packet, NULL);

/* check checksum, if corrupted, do nothing */
    if (packet.checksum != calc_checksum(&packet)) {
        log(A, "ACK packet is corrupted", &packet, NULL);
        logfile(A, "ACK packet is corrupted", &packet, NULL);
        return;
    }

/* Duplicate ACKs, do nothing */
    if (packet.acknum < window_base) {
        log(A, "Receive duplicate ACK", &packet, NULL);
        logfile(A, "Receive duplicate ACK", &packet, NULL);
        return;
    } else if (packet.acknum >= nextseq) {
        log(A, "BUG: receive ACK of future packets", &packet, NULL);
        logfile(A, "BUG: receive ACK of future packets", &packet, NULL);
        return;
    }

/* go to the next seq, and stop the timer */
    free_packets(packet.acknum);

/* stop timer of the oldest packet, and if there
   is still some packets on network, we need start
   another packet  */
    stoptimer(A);
    if (window_base != nextseq) {
        starttimer(A, TIMEOUT);
    }

    log(A, "ACK packet process successfully accomplished!!", &packet, NULL);
    logfile(A, "ACK packet process successfully accomplished!!", &packet, NULL);
    printf("================================ Outside A_input===================================\n");
    fp = fopen("output_gbn.doc","a");
    fprintf(fp,"================================ Outside A_input===================================\n");
    fclose(fp);
}

/* called when A's timer goes off */
void A_timerinterrupt(void) {
    int i;
    log(A, "Time interrupt occur", NULL, NULL);
    logfile(A, "Time interrupt occur", NULL, NULL);
    /* if current package no finished, we resend it */
    for (i = window_base; i < nextseq; ++i) {
        struct pkt *p = get_packet(i);
        tolayer3(A, *p);
        //++A_to_layer3;
        log(A, "Timeout! Send out the package again", p, NULL);
        logfile(A, "Timeout! Send out the package again", p, NULL);
    }

    /* If there is still some packets, start the timer again */
    if (window_base != nextseq) {
        starttimer(A, TIMEOUT);
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void) {

}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
    printf("================================ Inside B_input===================================\n");
    fp = fopen("output_gbn.doc","a");
    fprintf(fp,"================================ Inside B_input===================================\n");
    fclose(fp);
    log(B, "Receive a packet from layer3", &packet, NULL);
    logfile(B, "Receive a packet from layer3", &packet, NULL);

/* check checksum, if corrupted, just drop the package */
    if (packet.checksum != calc_checksum(&packet)) {
        log(B, "Packet is corrupted", &packet, NULL);
        logfile(B, "Packet is corrupted", &packet, NULL);
        return;
    }

/* normal package, deliver data to layer5 */
    if (packet.seqnum == b_seq_no) {
        ++b_seq_no;
        tolayer5(B, packet.payload);
        log(B, "Send packet to layer5", &packet, NULL);
        logfile(B, "Send packet to layer5", &packet, NULL);
    }
/* duplicate package, do not deliver data again.
   just resend the latest ACK again */
    else if (packet.seqnum < b_seq_no) {
        log(B, "Duplicated packet detected", &packet, NULL);
        logfile(B, "Duplicated packet detected", &packet, NULL);
    }
/* disorder packet, discard and resend the latest ACK again */
    else {
        log(B, "Disordered packet received", &packet, NULL);
        logfile(B, "Disordered packet received", &packet, NULL);
    }

/* send back ack */
    if (b_seq_no - 1 >= 0) {
        packet.acknum = b_seq_no - 1;    /* resend the latest ACK */
        packet.checksum = calc_checksum(&packet);
        tolayer3(B, packet);
        log(B, "Send ACK packet to layer3", &packet, NULL);
        log(B, "Send ACK packet to layer3", &packet, NULL);
    }
    printf("================================ Outside B_input(packet) =========================\n");
    fp = fopen("output_gbn.doc","a");
    fprintf(fp,"================================ Outside B_input(packet) =========================\n");
    fclose(fp);
}

/* called when B's timer goes off */
void B_timerinterrupt(void) {
    printf("  B_timerinterrupt: B doesn't have a timer. ignore.\n");
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void) {
    b_seq_no = 0;
}