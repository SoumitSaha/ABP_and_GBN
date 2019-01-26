#define   A    0
#define   B    1
#include "structures.h"

int calc_checksum(struct pkt *p)
{
    int i;
    int checksum = 0;
    if(p == NULL){
        return checksum;
    }
    //Adding all the characters in payload
    for (i=0; i<20; i++){
        checksum += (unsigned char)p->payload[i];
    }
    // checksum = payload
    checksum += p->seqnum;
    //checksum = payload + seqnum
    checksum += p->acknum;
    //checksum = payload + seqnum + acknum
    return checksum;
}

void log(int AorB, char *msg, struct pkt *p, struct msg *m){
    char ch = (AorB == A)?'A':'B';
    if(p != NULL) {
        printf("[%c] %s. Packet[seq=%d,ack=%d,check=%d,data=%c..]\n", ch,
               msg, p->seqnum, p->acknum, p->checksum, p->payload[0]);
    } else if(m != NULL) {
        printf("[%c] %s. Message[data=%c..]\n", ch, msg, m->data[0]);
    } else {
        printf("[%c] %s.\n", ch, msg);
    }
}
