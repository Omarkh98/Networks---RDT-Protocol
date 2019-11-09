#include <stdio.h>
#include <stdlib.h>

#define BIDIRECTIONAL 0 

void StartTimer(int AorB, float Increment);   // Function to Start Timer
void StopTimer(int AorB);   // Fuction to Stop Timer.
void ToLayer3(int AorB, struct Packet Pkt);   // ToLayer 3 Function.
void ToLayer5(int AorB, char DataSent[20]);  // ToLayer 5 Function
int CheckSumCalc(struct Packet Pkt);  // Check Sum Function, to Check for errors.

int A_App = 0;
int A_Trans = 0;
int B_App = 0;
int B_Trans = 0;

struct  Message {   // 20-byte Chunk from Layer 5.
	char Data[20];
};

struct Packet {
	int SequenceNumber;  // Sequence #.
	int AckNumber;      // Acknowledgment #.
	int CheckSum;      // Check Sum to check for errors. 
	char PayLoad[20];
};

float TimeOut = 25.0;
int WindowSize;         
int SendBufferSize = 2000; 
int ReceiveBufferSize = 0; 
float Time;

int Base; 
int NextSequence; 
int ExpectedSequence; 

struct Packet* Pkt;
struct Packet Ack;

int CheckSumCalc(struct Packet Pkt) {   // Check For errors with CheckSum.

	int i;
	int CheckSum = 0;
	char Byte;

	for (i = 0; i < sizeof(Pkt.SequenceNumber); i++)  // Check Sequence Numebr.
	{
		Byte = *((char *)&Pkt.SequenceNumber + i);
		CheckSum += Byte;
	}

	for (i = 0; i < sizeof(Pkt.AckNumber); i++)   // Check Acknowledgment Number.
	{
		Byte = *((char *)&Pkt.AckNumber + i);
		CheckSum += Byte;
	}

	for (i = 0; i < sizeof(Pkt.PayLoad); i++)   // Chekc Payload or The size of the Message.
	{
		Byte = *((char *)&Pkt.PayLoad + i);
		CheckSum += Byte;
	}
	return CheckSum;
}

// Entity A ROUTINES.

void A_Output(struct Message Msg) {  // Message containing data, sent from Entity A to Entity B.  Routine # 1;
	
	A_App++;

	printf("A_Output Routine received data from layer5 : %.*s \n", 20, Msg.Data);
	printf("A_Output Routine Next Sequence Number is : %d\n", NextSequence);
	printf("A_Output Routine Time is : %f\n", Time);

	Pkt[NextSequence].SequenceNumber = NextSequence;
	memcpy(Pkt[NextSequence].PayLoad, Msg.Data, 20);
	Pkt[NextSequence].CheckSum = CheckSumCalc(Pkt[NextSequence]);
	
	printf("A_Output Routine packet has been created with payload: %.*s \n", 20, Pkt[NextSequence].PayLoad);
	printf("A_Output Routine packet has been added to buffer if needed to resend\n");

	while ((NextSequence < (Base + WindowSize)) && (Pkt[NextSequence].SequenceNumber != -1))
	{
		ToLayer3(0, Pkt[NextSequence]);
		A_Trans++;

		if (Base == NextSequence)
		{
			//stoptimer(0);
			printf("A_Output Routine Timer has started for base %d\n", Base);
			StartTimer(0, TimeOut);

		}

		printf("A_Output Routine Packet %d has been sent.\n", Pkt[NextSequence].SequenceNumber);
		NextSequence++;
	}

	printf("A_Output Routine Returning back\n");
	return;

}

void B_Output(struct Message Msg) {    // At BIDIRECTIONAL = 1, Message containing data, sent from Entity B to Entity A. Routine # 2.

}

void A_Input(struct Packet Pkt) {   // Message from Entity B Sent over to Entity A. (Packet is Probably Corrupted).  Routine # 3.
	
	printf("A_Input Routine Base is : %d\n", Base);
	printf("A_Input Routine Next Sequence is : %d\n", NextSequence);


	printf("A_Input Routine ACK has been received : %d\n", Pkt.AckNumber);

	int CheckS = CheckSumCalc(Pkt);
	printf("A_Input Routine has Calculated the Checksum : %d\n", CheckS);
	printf("A_Input Routine has Received the Checksum : %d\n", Pkt.CheckSum);
	printf("A_Input Routine Time is : %f\n", Time);

	if (CheckS != Pkt.CheckSum) {
		printf("A_Input Routine the Checksum recieved does not match ACK is probably corrupted\n");
		return;
	}

	printf("A_Input Routine Packet % d has been received successfully at Entity B\n", Pkt.AckNumber);

	if (Base != Pkt.AckNumber + 1) { //to avoid starting timer again if it has already been started
		Base = Pkt.AckNumber + 1;

		if (Base == NextSequence) //stop timer if nothing more to send
		{
			StopTimer(0);
			printf("A_Input Routine Timer has stopped.\n");
		}
		else{

			if (Pkt.AckNumber != 0){
				StopTimer(0);
				printf("A_Input Routine Timer has started for base %d\n", Base);
				StartTimer(0, TimeOut);
			}
		}
	}

	printf("A_Input Routine is Returning\n");
	return;
}

void A_TimerInterrupt() {  // When timer of Entity A expires generate a timer interrupt.  Routine #4.
	
	printf("A_TimeInterrupt Routine has Timer started for base %d\n", Base);
    
	StartTimer(0, TimeOut);
	printf("A_TimeInterrupt Routine NextSequence: %d\n", NextSequence);
	printf("A_TimeInterrupt Routine Base: %d\n", Base);
	printf("A_TimeInterrupt Routine Time: %f\n", Time);

	for (int i = 0; i< (NextSequence - Base); i++)
	{
		printf("A_TimeInterrupt Routine is Resending %.*s \n", 20, Pkt[Base + i].PayLoad);
		printf("A_TimeInterrupt Routine Time: %f\n", Time);
		ToLayer3(0, Pkt[Base + i]);
		A_Trans++;
	}

	return;
}

void A_Init() { // Called once to initialize any required variables for Entity A. Routine #5.
	
	Base = 1;
	NextSequence = 1;
	Pkt = malloc((sizeof(struct Packet)) * SendBufferSize);

	for (int i = 0; i< SendBufferSize; i++)
	{
		Pkt[i].SequenceNumber = -1;
	}
}

// Entity B ROUTINES.

void B_Input(struct Packet Pkt) {   // Message from Entity A Sent over to Entity B. (Packet is Probably Corrupted). Routine #6.

	B_Trans++;
	printf(" B_Input Routine Expectedseqnum is : %d\n", ExpectedSequence);
	printf(" B_Input Routine Packet received: %.*s \n", 20, Pkt.PayLoad);
	printf(" B_Input Routine SequenceNumber: %d\n", Pkt.SequenceNumber);
	printf(" B_Input Routine Time: %f\n", Time);

	int CheckS = CheckSumCalc(Pkt);
	printf("B_Input Routine Calculated Checksum is : %d\n", CheckS);
	printf("B_Input Routine Received Checksum is : %d\n", Pkt.CheckSum);

	if ((Pkt.CheckSum == CheckS) && (Pkt.SequenceNumber == ExpectedSequence))
	{
		printf(" B_Input Routine Sequence number: %d \n", Pkt.SequenceNumber);
		printf(" B_Input Routine Checksum matches.\n");

		ToLayer5(1, Pkt.PayLoad);
		B_App++;

		printf("B_Input Routine Data has been sent to layer5. Count: %d\n", B_App);

		Ack.AckNumber = ExpectedSequence;
		Ack.CheckSum = CheckSumCalc(Ack);

		printf("B_Input Routine is Sending an ACK %d\n", Ack.AckNumber);
		ToLayer3(1, Ack);
		ExpectedSequence++;
		return;
	}
	// Else 

	printf("B_Input Routine CheckSum Or the Sequence Number do not match.\n");
	printf("B_Input Routine is Resending the ACK %d\n", Ack.AckNumber);
	ToLayer3(1, Ack);
	return;
}

void B_TimerInterrupt() {   // When timer of Entity B expires.  Routine #7.
	printf(" B_TimerInterrupt Routine : B doesn't have a timer.\n");
}

void B_Init() {   // Called once to initialize any required variables for Entity B. Routine #8.
	ExpectedSequence = 1;
	Ack.AckNumber = 0;
	Ack.CheckSum = CheckSumCalc(Ack);
}

//////////////////////////////////////////////////////////////////////////////////
// Network Emulator//
/////////////////////////////////////////////////////////////////////////////////

struct event {
	float evtime;           /* event time */
	int evtype;             /* event type code */
	int eventity;           /* entity where event occurs */
	struct Packet *pktptr;     /* ptr to packet (if any) assoc w/ this event */
	struct event *prev;
	struct event *next;
};
struct event *evlist = NULL;   /* the event list */

							   /* possible events: */
#define  TIMER_INTERRUPT 0
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

main()
{
	struct event *eventptr;
	struct Message  msg2give;
	struct Packet  pkt2give;

	int i, j;
	char c;

	init();
	A_Init();
	B_Init();

	while (1) {
		eventptr = evlist;            /* get next event to simulate */
		if (eventptr == NULL)
			goto terminate;
		evlist = evlist->next;        /* remove this event from event list */
		if (evlist != NULL)
			evlist->prev = NULL;
		if (TRACE >= 2) {
			printf("\nEVENT time: %f,", eventptr->evtime);
			printf("  type: %d", eventptr->evtype);
			if (eventptr->evtype == 0)
				printf(", timerinterrupt  ");
			else if (eventptr->evtype == 1)
				printf(", fromlayer5 ");
			else
				printf(", fromlayer3 ");
			printf(" entity: %d\n", eventptr->eventity);
		}
		time = eventptr->evtime;        /* update time to next event time */
		if (nsim == nsimmax)
			break;                        /* all done with simulation */
		if (eventptr->evtype == FROM_LAYER5) {
			generate_next_arrival();   /* set up future arrival */
									   /* fill in msg to give with string of same letter */
			j = nsim % 26;
			for (i = 0; i<20; i++)
				msg2give.Data[i] = 97 + j;
			if (TRACE>2) {
				printf("          MAINLOOP: data given to student: ");
				for (i = 0; i<20; i++)
					printf("%c", msg2give.Data[i]);
				printf("\n");
			}
			nsim++;
			if (eventptr->eventity == A)
				A_Output(msg2give);
			else
				B_Output(msg2give);
		}
		else if (eventptr->evtype == FROM_LAYER3) {
			pkt2give.SequenceNumber = eventptr->pktptr->SequenceNumber;
			pkt2give.AckNumber = eventptr->pktptr->AckNumber;
			pkt2give.CheckSum = eventptr->pktptr->CheckSum;
			for (i = 0; i<20; i++)
				pkt2give.PayLoad[i] = eventptr->pktptr->PayLoad[i];
			if (eventptr->eventity == A)      /* deliver packet by calling */
				A_Input(pkt2give);            /* appropriate entity */
			else
				B_Input(pkt2give);
			free(eventptr->pktptr);          /* free the memory for packet */
		}
		else if (eventptr->evtype == TIMER_INTERRUPT) {
			if (eventptr->eventity == A)
				A_TimerInterrupt();
			else
				B_TimerInterrupt();
		}
		else {
			printf("INTERNAL PANIC: unknown event type \n");
		}
		free(eventptr);
	}

terminate:
	printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n", time, nsim);

	system("pause");
}

init()                         /* initialize the simulator */
{
	int i;
	float sum, avg;
	float jimsrand();
	printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
	printf("Enter the number of messages to simulate: ");
	scanf_s("%d", &nsimmax);
	printf("Enter  packet loss probability [enter 0.0 for no loss]:");
	scanf_s("%f", &lossprob);
	printf("Enter packet corruption probability [0.0 for no corruption]:");
	scanf_s("%f", &corruptprob);
	printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
	scanf_s("%f", &lambda);
	printf("Enter TRACE:");
	scanf_s("%d", &TRACE);

	srand(9999);              /* init random number generator */
	sum = 0.0;                /* test random number generator for students */
	for (i = 0; i<1000; i++)
		sum = sum + jimsrand();    /* jimsrand() should be uniform in [0,1] */
	avg = sum / 1000.0;
	if (avg < 0.25 || avg > 0.75) {
		printf("It is likely that random number generation on your machine\n");
		printf("is different from what this emulator expects.  Please take\n");
		printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
		exit(0);
	}

	ntolayer3 = 0;
	nlost = 0;
	ncorrupt = 0;

	time = 0.0;                    /* initialize time to 0.0 */
	generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
	double mmm = (double)RAND_MAX;//2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
	float x;                   /* individual students may need to change mmm */
	x = rand() / mmm;            /* x should be uniform in [0,1] */
	return(x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

generate_next_arrival()
{
	double x, log(), ceil();
	struct event *evptr;
	//   char *malloc();
	float ttime;
	int tempint;

	if (TRACE>2)
		printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

	x = lambda*jimsrand() * 2;  /* x is uniform on [0,2*lambda] */
								/* having mean of lambda        */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime = time + x;
	evptr->evtype = FROM_LAYER5;
	if (BIDIRECTIONAL && (jimsrand()>0.5))
		evptr->eventity = B;
	else
		evptr->eventity = A;
	insertevent(evptr);
}


insertevent(p)
struct event *p;
{
	struct event *q, *qold;

	if (TRACE>2) {
		printf("            INSERTEVENT: time is %lf\n", time);
		printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
	}
	q = evlist;     /* q points to header of list in which p struct inserted */
	if (q == NULL) {   /* list is empty */
		evlist = p;
		p->next = NULL;
		p->prev = NULL;
	}
	else {
		for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
			qold = q;
		if (q == NULL) {   /* end of list */
			qold->next = p;
			p->prev = qold;
			p->next = NULL;
		}
		else if (q == evlist) { /* front of list */
			p->next = evlist;
			p->prev = NULL;
			p->next->prev = p;
			evlist = p;
		}
		else {     /* middle of list */
			p->next = q;
			p->prev = q->prev;
			q->prev->next = p;
			q->prev = p;
		}
	}
}
printevlist()
{
	struct event *q;
	int i;
	printf("--------------\nEvent List Follows:\n");
	for (q = evlist; q != NULL; q = q->next) {
		printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype, q->eventity);
	}
	printf("--------------\n");
}



////////////////////////////////////////////  
void StopTimer(int AorB) {                 // Stopping Timer at Entity A or B.
	struct event *q, *qold;
	if (TRACE>2)
		printf("          STOP TIMER: stopping timer at %f\n", time);
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == TIMER_INTERRUPT  && q->eventity == AorB)) {
			/* remove this event */
			if (q->next == NULL && q->prev == NULL)
				evlist = NULL;         /* remove first and only event on list */
			else if (q->next == NULL) /* end of list - there is one in front */
				q->prev->next = NULL;
			else if (q == evlist) { /* front of list - there must be event after */
				q->next->prev = NULL;
				evlist = q->next;
			}
			else {     /* middle of list */
				q->next->prev = q->prev;
				q->prev->next = q->next;
			}
			free(q);
			return;
		}
	printf("Warning: unable to cancel your timer. It wasn't running.\n");
}
///////////////////////////////////////////


///////////////////////////////////////////
void StartTimer(int AorB, float Increment){  // Start counting time taken to send packet from Entity A over to Entity B, (takes around 5 time-units).

	struct event *q;
	struct event *evptr;
	// char *malloc();

	if (TRACE>2)
		printf("          START TIMER: starting timer at %f\n", time);
	/* be nice: check to see if timer is already started, if so, then  warn */
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == TIMER_INTERRUPT  && q->eventity == AorB)) {
			printf("Warning: attempt to start a timer that is already started\n");
			return;
		}

	/* create future event for when timer goes off */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime = time + Increment;
	evptr->evtype = TIMER_INTERRUPT;
	evptr->eventity = AorB;
	insertevent(evptr);
}
////////////////////////////////////////////


////////////////////////////////////////////
void ToLayer3(int AorB, struct Packet Pkt) {  // Send packet to the network from one Entity, on its way to the other.
	struct Packet *mypktptr;
	struct event *evptr, *q;
	// char *malloc();
	float lastime, x, jimsrand();
	int i;

	ntolayer3++;

	/* simulate losses: */
	if (jimsrand() < lossprob) {
		nlost++;
		if (TRACE>0)
			printf("TOLAYER3: packet being lost\n");
		return;
	}
	/* make a copy of the packet student just gave me since he/she may decide */
	/* to do something with the packet after we return back to him/her */
	mypktptr = (struct Packet *)malloc(sizeof(struct Packet));
	mypktptr->SequenceNumber = Pkt.SequenceNumber;
	mypktptr->AckNumber = Pkt.AckNumber;
	mypktptr->CheckSum = Pkt.CheckSum;
	for (i = 0; i<20; i++)
		mypktptr->PayLoad[i] = Pkt.PayLoad[i];
	if (TRACE>2) {
		printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->SequenceNumber,
			mypktptr->AckNumber, mypktptr->CheckSum);
		for (i = 0; i<20; i++)
			printf("%c", mypktptr->PayLoad[i]);
		printf("\n");
	}
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
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == FROM_LAYER3  && q->eventity == evptr->eventity))
			lastime = q->evtime;
	evptr->evtime = lastime + 1 + 9 * jimsrand();
	/* simulate corruption: */
	if (jimsrand() < corruptprob) {
		ncorrupt++;
		if ((x = jimsrand()) < .75)
			mypktptr->PayLoad[0] = 'Z';   /* corrupt payload */
		else if (x < .875)
			mypktptr->SequenceNumber = 999999;
		else
			mypktptr->AckNumber = 999999;
		if (TRACE>0)
			printf("          TOLAYER3: packet being corrupted\n");
	}

	if (TRACE>2)
		printf("          TOLAYER3: scheduling arrival on other side\n");
	insertevent(evptr);
}
////////////////////////////////////////////

////////////////////////////////////////////
void ToLayer5(int AorB, char DataSent[20]) { // Sent data will be passed up to Layer 5.
	int i;
	if (TRACE>2) {
		printf("TOLAYER5: data received: ");
		for (i = 0; i<20; i++)
			printf("%c", DataSent[i]);
		printf("\n");
	}
}
///////////////////////////////////////////
