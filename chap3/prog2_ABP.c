#include <stdio.h>
#include <stdlib.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
********************************************************************* */

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
                           /* and write a routine called B_output */

/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define  A               0
#define  B               1

/** Host's possible state */
#define WAIT_CALL        0  /** waiting for call from above */
#define WAIT_ACK         1  /** waiting for ACK */

#define MSG_LEN 20
#define TIMEOUT_INTERVAL 20

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[MSG_LEN];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
  int seqnum;
  int ack;
  __uint16_t checksum;
  char payload[MSG_LEN];
};

struct event {
  float evtime;           /* event time */
  int evtype;             /* event type code */
  int eventity;           /* entity where event occurs */
  struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
  struct event *prev;
  struct event *next;
};

struct host {
  int state;              /** State of host */
  int seqnum;             /** Sequence number */
  int exp_seqnum;         /** Expected sequence number */
  struct pkt sndpkt;      /** packet to send */
};

void init();
float jimsrand();
void generate_next_arrival();
void insertevent(struct event *);
void printevlist();
void stoptimer(int);
void starttimer(int, float);
void tolayer3(int, struct pkt);
void tolayer5(int, char[MSG_LEN]);

void calc_checksum(struct pkt *p);
int check_checksum(struct pkt *p);

struct event *evlist = NULL;   /* the event list */
int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */ 
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */   
int ntolayer3;             /* number sent into layer 3 */
int nlost;                 /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

struct host host_A;
struct host host_B;

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
  host_A.state = WAIT_CALL;
  host_A.seqnum = 0;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) {
  if (host_A.state == WAIT_ACK) {
    if (TRACE > 2) {
      printf("--A: waiting for ack, message ignored.\n");
    }
    return;
  }

  host_A.state = WAIT_ACK;

  // make packet to send
  host_A.sndpkt.seqnum = host_A.seqnum;
  for (int i = 0; i < MSG_LEN; ++i) {
    host_A.sndpkt.payload[i] = message.data[i];
  }
  calc_checksum(&host_A.sndpkt);

  // send to layer 3
  tolayer3(A, host_A.sndpkt);

  // start timer
  starttimer(A, TIMEOUT_INTERVAL);
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
  // check if packet is not corrupt
  int not_corrupt = check_checksum(&packet);

  if (not_corrupt == 1 && packet.ack == 1) {
    if (TRACE > 2) {
      printf("--ACK received correctly.\n");
    }
    nsim++;
    host_A.state = WAIT_CALL;
    host_A.seqnum = 1 - host_A.seqnum;
    stoptimer(A);
  } else if (not_corrupt == 0) {
    if (TRACE > 2) {
      printf("--corrupted packet received.\n");
    }
  } else if (packet.ack == 0) {
    if (TRACE > 2) {
      printf("--NACK received, sending packet again\n");
    }
    stoptimer(A);
    tolayer3(A, host_A.sndpkt);
    starttimer(A, TIMEOUT_INTERVAL);
  }
}

/* called when A's timer goes off */
void A_timerinterrupt() {
  tolayer3(A, host_A.sndpkt);
  starttimer(A, TIMEOUT_INTERVAL);
}

/****************************************************************************/

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
  host_B.exp_seqnum = 0;
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
  // check if packet is not corrupt
  int not_corrupt = check_checksum(&packet);

  if (not_corrupt == 1 && packet.seqnum == host_B.exp_seqnum) {
    if (TRACE > 2) {
      printf("--packet received correctly.\n");
    }
    // 0 -> 1 or 1 -> 0
    host_B.exp_seqnum = 1 - host_B.exp_seqnum;
    // send payload to layer 5
    tolayer5(B, packet.payload);
    // make ACK
    host_B.sndpkt.ack = 1;
    calc_checksum(&host_B.sndpkt);
    // send ACK to layer 3
    tolayer3(B, host_B.sndpkt);
  } else {
    if (not_corrupt == 1) {
      if (TRACE > 2) {
        printf("--duplicate packet received. expected: %d, real: %d\n", host_B.exp_seqnum, packet.seqnum);
      }
      // make ACK and send to A
      host_B.sndpkt.ack = 1;
    } else {
      if (TRACE > 2) {
        printf("--corrupted packet received.\n");
      }
      // make NACK and send to A
      host_B.sndpkt.ack = 0;
    }
    calc_checksum(&host_B.sndpkt);
    tolayer3(B, host_B.sndpkt);
  }
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* need be completed only for extra credit */
void B_output(struct msg message) {}

/* called when B's timer goes off */
void B_timerinterrupt() {}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

int main(void) {
  struct event *eventptr;
  struct msg msg2give;
  struct pkt pkt2give;
  
  init();
  A_init();
  B_init();
  
  while (1) {
    eventptr = evlist;            /* get next event to simulate */
    if (eventptr == NULL) {
      break;
    }

    evlist = evlist->next;        /* remove this event from event list */
    if (evlist != NULL) {
      evlist->prev = NULL;
    }

    if (nsim == nsimmax) {
	    break;                        /* all done with simulation */
    }

    if (TRACE >= 2) {
      printf("\n%c, ", eventptr->eventity == 0 ? 'A' : 'B');
      printf("EVENT time: %f,", eventptr->evtime);
      printf(" type: %d", eventptr->evtype);
      if (eventptr->evtype == 0) {
	      printf(", timerinterrupt  ");
      } else if (eventptr->evtype == 1) {
        printf(", fromlayer5 ");
      } else {
	      printf(", fromlayer3 ");
      }
      printf("\n");
    }

    time = eventptr->evtime;        /* update time to next event time */
    
    if (eventptr->evtype == FROM_LAYER5) {
      generate_next_arrival();   /* set up future arrival */
      /* fill in msg to give with string of same letter */    
      int j = nsim % 26;
      for (int i = 0; i < MSG_LEN; i++) {
        msg2give.data[i] = 97 + j;
      }

      if (TRACE > 2) {
        printf("--MAINLOOP: data given to student: ");
        for (int i = 0; i < MSG_LEN; ++i) {
          putchar(msg2give.data[i]);
        }
        putchar('\n');
	    }

      if (eventptr->eventity == A) {
        A_output(msg2give);  
      } else {
        B_output(msg2give);
      }
    } else if (eventptr->evtype == FROM_LAYER3) {
      pkt2give.seqnum = eventptr->pktptr->seqnum;
      pkt2give.ack = eventptr->pktptr->ack;
      pkt2give.checksum = eventptr->pktptr->checksum;
      for (int i= 0; i < MSG_LEN; i++)   {
        pkt2give.payload[i] = eventptr->pktptr->payload[i];
      }

	    if (eventptr->eventity == A) {      /* deliver packet by calling */
  	    A_input(pkt2give);                /* appropriate entity */
      } else {
  	    B_input(pkt2give);
      }
	    free(eventptr->pktptr);          /* free the memory for packet */
    } else if (eventptr->evtype == TIMER_INTERRUPT) {
      if (eventptr->eventity == A)  {
	      A_timerinterrupt();
      } else {
	      B_timerinterrupt();
      }
    } else {
	    printf("--INTERNAL PANIC: unknown event type.\n");
    }
    free(eventptr);
  }

  printf("--Simulator terminated at time %f, after sending %d msgs from layer5\n",time,nsim);

  return 0;
}

/* initialize the simulator */
void init()
{
  float sum, avg;
  
  printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
  printf("Enter the number of messages to simulate: ");
  scanf("%d", &nsimmax);
  printf("Enter packet loss probability [enter 0.0 for no loss]: ");
  scanf("%f", &lossprob);
  printf("Enter packet corruption probability [0.0 for no corruption]: ");
  scanf("%f", &corruptprob);
  printf("Enter average time between messages from sender's layer5 [ > 0.0]: ");
  scanf("%f", &lambda);
  printf("Enter TRACE: ");
  scanf("%d", &TRACE);

  srand(9999);              /* init random number generator */
  sum = 0.0;                /* test random number generator for students */
  for (int i = 0; i < 1000; i++) {
     sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
  }
  avg = sum / 1000.0;

  if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(0);
  }

  ntolayer3 = 0;
  nlost = 0;
  ncorrupt = 0;

  time=0.0;                    /* initialize time to 0.0 */
  generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
  double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */ 
  x = rand() / mmm;          /* x should be uniform in [0,1] */
  return x;
}  

/*************** EVENT HANDLINE ROUTINES *************/
/*  The next set of routines handle the event list   */
/*****************************************************/
 
void generate_next_arrival()
{
  double x,log(),ceil();
  struct event *evptr;

  if (TRACE > 2) {
    printf("--GENERATE NEXT ARRIVAL: creating new arrival\n");
  }
 
  x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
                               /* having mean of lambda        */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtime = time + x;
  evptr->evtype = FROM_LAYER5;

  if (BIDIRECTIONAL && jimsrand() > 0.5) {
    evptr->eventity = B;
  } else {
    evptr->eventity = A;
  }

  insertevent(evptr);
} 

void insertevent(struct event *p) {
  struct event *q, *qold;

  if (TRACE > 2) {
    printf("----INSERTEVENT: current time is %lf\n", time);
    printf("----INSERTEVENT: future time will be %lf\n", p->evtime); 
  }

  q = evlist;     /* q points to header of list in which p struct inserted */
  if (q == NULL) {   /* list is empty */
    evlist = p;
    p->next = NULL;
    p->prev = NULL;
  } else {
    for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next) {
      qold = q;
    }
    if (q == NULL) {   /* end of list */
      qold->next = p;
      p->prev = qold;
      p->next = NULL;
    } else if (q == evlist) { /* front of list */
      p->next = evlist;
      p->prev = NULL;
      p->next->prev = p;
      evlist = p;
    } else {     /* middle of list */
      p->next = q;
      p->prev = q->prev;
      q->prev->next = p;
      q->prev = p;
    }
  }
}

void printevlist()
{
  printf("--------------\nEvent List Follows:\n");
  for(struct event *q = evlist; q != NULL; q = q->next) {
    printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype, q->eventity);
  }
  printf("--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
/* A or B is trying to stop timer */
void stoptimer(int AorB) {
  if (TRACE > 2) {
    printf("--STOP TIMER: stopping timer at %f\n", time);
  }

  /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
  for (struct event *q = evlist; q != NULL ; q = q->next) {
    if (q->evtype == TIMER_INTERRUPT && q->eventity == AorB) { 
      /* remove this event */
      if (q->next == NULL && q->prev == NULL) {
        evlist = NULL; /* remove first and only event on list */
      } else if (q->next == NULL) { /* end of list - there is one in front */
        q->prev->next = NULL;
      } else if (q == evlist) { /* front of list - there must be event after */
        q->next->prev = NULL;
        evlist = q->next;
      } else { /* middle of list */
        q->next->prev = q->prev;
        q->prev->next = q->next;
      }
      free(q);
      return;
    }
  }

  printf("--Warning: unable to cancel your timer. It wasn't running.\n");
}

/* A or B is trying to stop timer */
void starttimer(int AorB, float increment) {
  if (TRACE > 2) {
    printf("--START TIMER: starting timer at %f\n",time);
  }

  /* be nice: check to see if timer is already started, if so, then  warn */
  /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
  for (struct event *q = evlist; q != NULL; q = q->next) {
    if (q->evtype == TIMER_INTERRUPT && q->eventity == AorB) {
      printf("--Warning: attempt to start a timer that is already started\n");
      return;
    }
  }

  struct event *evptr;
  /* create future event for when timer goes off */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtime = time + increment;
  evptr->evtype = TIMER_INTERRUPT;
  evptr->eventity = AorB;

  insertevent(evptr);
} 

/************************** TOLAYER3 ***************/
void tolayer3(int AorB, struct pkt packet) {
  float lastime, x, jimsrand();

  ntolayer3++;

  /* simulate losses: */
  if (jimsrand() < lossprob) {
    nlost++;
    if (TRACE > 0) {
	    printf("--TOLAYER3: packet being lost\n");
    }
    return;
  }

  struct pkt *mypktptr;
  /* make a copy of the packet student just gave me since he/she may decide */
  /* to do something with the packet after we return back to him/her */ 
  mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
  mypktptr->seqnum = packet.seqnum;
  mypktptr->ack = packet.ack;
  mypktptr->checksum = packet.checksum;

  for (int i = 0; i < MSG_LEN; i++) {
    mypktptr->payload[i] = packet.payload[i];
  }

  if (TRACE > 2) {
    printf("--TOLAYER3: seq: %d, ack %d, check: %d, payload: ", 
      mypktptr->seqnum, mypktptr->ack, mypktptr->checksum);
    for (int i = 0; i < MSG_LEN; ++i) {
      putchar(mypktptr->payload[i]);
    }
    putchar('\n');
  }

  struct event *evptr;
  /* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype = FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */

  /* finally, compute the arrival time of packet at the other end.
    medium can not reorder, so make sure packet arrives between 1 and 10
    time units after the latest arrival time of packets
    currently in the medium on their way to the destination */
  lastime = time;
  /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
  for (struct event *q = evlist; q != NULL ; q = q->next) {
    if (q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity) {
      lastime = q->evtime;
    }
  }
  evptr->evtime = lastime + 1 + 9*jimsrand();
 
  /* simulate corruption: */
  if (jimsrand() < corruptprob)  {
    ncorrupt++;

    if ((x = jimsrand()) < .75) {
       mypktptr->payload[0]='Z';   /* corrupt payload */
    } else if (x < .875) {
       mypktptr->seqnum = 999999;
    } else {
       mypktptr->ack = 999999;
    }

    if (TRACE > 0) {
	    printf("--TOLAYER3: packet being corrupted\n");
    }
  }

  if (TRACE > 2) {
    printf("--TOLAYER3: scheduling arrival on other side\n");
  }

  insertevent(evptr);
}

void tolayer5(int AorB, char datasent[MSG_LEN]) {
  if (TRACE > 2) {
    printf("--TOLAYER5: data received: ");
    for (int i = 0; i < MSG_LEN; ++i) {
      putchar(datasent[i]);
    }
    putchar('\n');
  }
}

/** calculate checksum and put it in the checksum field */
void calc_checksum(struct pkt *p) {
  int sum = p->ack + p->seqnum;

  int count = MSG_LEN;
  char *msg = p->payload;

  while (count > 1) {
    sum += *((__uint16_t *)msg);
    msg += 2;
    count -= 2;
  }

  if (count > 0) {
    sum += *msg;
  }

  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  p->checksum = (~sum);
}

/** check if packet is corrupt */
int check_checksum(struct pkt *p) {
  int sum = p->ack + p->seqnum;

  int count = MSG_LEN;
  char *msg = p->payload;

  while (count > 1) {
    sum += *((__uint16_t *)msg);
    msg += 2;
    count -= 2;
  }

  if (count > 0) {
    sum += *msg;
  }

  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  if ((sum + p->checksum) == (1 << 16) - 1) {
    return 1;
  }

  return 0;
}