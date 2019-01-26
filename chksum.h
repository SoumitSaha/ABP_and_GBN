/********* FUNCTION PROTOTYPES. DEFINED IN THE LATER PART******************/
void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);

#define BIDIRECTIONAL 0 /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */


/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg
{
    char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt
{
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];
};

#define TIMEOUT 20.0
#define DEFAULT_ACK 5
#define WAIT_FOR_PKG 3
#define WAIT_FOR_ACK 4

#define   A    0
#define   B    1

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
