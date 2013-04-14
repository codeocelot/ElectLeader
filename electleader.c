// Chang & Roberts
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define DEBUG 0


int mod(int x, int y) {
    int r = x%y;
    return r<0?r+y:r;
}





// 	send(ELECTION(myID, 0,0)) // right
// 	send(ELECTION(myID, 0,0)) // left

// 	recv(ELECTION) // left
// 	recv(ELECTION) // right

// 	// left
// 	if (leftID > myID && leftCounter <= 2**leftStep):
// 		send(ELECTION(leftID,leftPhase,leftStep+1)) // to the right
// 	if (leftID > myID && leftCounter == 2**leftStep):
// 		send(REPLY(leftID,leftPhase)) // to the left
// 	if (myID == leftID):
// 		I am a leader

// 	// right
// 	if (rightID > myID && rightCounter <= 2**rightStep):
// 		send(ELECTION(rightID,rightPhase,rightStep+1)) // to the left
// 	if (rightID > myID && rightCounter == 2**rightStep):
// 		send(REPLY(rightID,rightPhase)) // to the right
// 	if (myID == rightID):
// 		I am a leader

// 	// reply from left (leftID, rightPhase):
// 	if (myID != leftID):
// 		send(REPLY(leftID, leftPhase)) // to right
// 	else if (already recieved REPLY(leftPhase, leftCounter)):
// 		send(ELECTION(leftID,leftPhase,1)) // to right
// 		send(ELECTION(leftID,leftPhase,1)) // to left
// 	// reply from left (rightID, rightPhase):
// 	if (myID != rightID):
// 		send(REPLY(rightID, rightPhase)) // to right
// 	else if (already recieved REPLY(rightPhase, rightPhase)):
// 		send(ELECTION(rightID,rightPhase,1)) // to right
// 		send(ELECTION(rightID,rightPhase,1)) // to left
// 	return 0;
// }


// // mpiexec -n NUM ./electleader PNUM
// // unique_id = (myrank+1)*PNUM mod NUM
// // a unique identifier to each of the NUM nodes as long as PNUM is relatively prime to NUM

int main(int argc, char* argv[]) {
	int rank,size;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	int NUM = size;
	int PNUM = *argv[argc-1];
	int myID = mod(((rank+1)*PNUM),NUM);

	int sendLeft[] = {myID, 0, 0};
	int sendRight[] = {myID, 0, 0};
	int recvLeft[3] = {0,0,0}; //left
	int recvRight[3] = {0,0,0}; //right
	MPI_Request requestLeft,requestRight;
	MPI_Status statusLeft,statusRight;
	int elected = 0;
	int leader = 0;

	int mrecved=0;
	int msent=0;

	// initial election call
	// send left
	// printf("sendLeft:%i\n",sendLeft[0]);
	MPI_Isend(&sendLeft,3,MPI_INT,mod(rank-1,NUM),123,MPI_COMM_WORLD,&requestLeft);
	msent++;
	// send right
	MPI_Isend(&sendRight,3,MPI_INT,mod(rank+1,NUM),123,MPI_COMM_WORLD,&requestRight);
	msent++;
	MPI_Wait(&requestLeft,&statusLeft);
	MPI_Wait(&requestRight,&statusRight);

	MPI_Recv(&recvLeft,3,MPI_INT,mod(rank-1,NUM),123,MPI_COMM_WORLD,&statusLeft);
	MPI_Recv(&recvRight,3,MPI_INT,mod(rank+1,NUM),123,MPI_COMM_WORLD,&statusRight);

	// printf("myID: %i, received from left: %i, %i, %i\n", myID, recvLeft[0],recvLeft[1],recvLeft[2]);
	// printf("myID: %i, received from right: %i, %i, %i\n", myID, recvRight[0],recvRight[1],recvRight[2]);

	int leftReply = 0;
	int rightReply = 0;

	// send a reply to right
	if (recvLeft[0] > myID) {
		recvLeft[2]=-1;
		MPI_Isend(&recvLeft,3,MPI_INT,mod(rank-1,NUM),123,MPI_COMM_WORLD,&requestLeft);
		msent++;
	} 
	// send left 
	if (recvRight[0] > myID) {
		recvRight[2]=-1;
		MPI_Isend(&recvRight,3,MPI_INT,mod(rank+1,NUM),123,MPI_COMM_WORLD,&requestRight);
		msent++;
	} 

	int lflag,rflag;

	while (!elected) {
		top:
		MPI_Irecv(&recvLeft,3,MPI_INT,mod(rank-1,NUM),123,MPI_COMM_WORLD, &requestLeft);
		MPI_Test(&requestLeft,&lflag,&statusLeft);
		if (lflag){
			mrecved++;
				// printf("131 myID: %i, received from left: %i, %i, %i\n", myID, recvLeft[0],recvLeft[1],recvLeft[2]);

				if (recvLeft[2] != -1) {
					// send election
					if (recvLeft[0] > myID && recvLeft[2] < pow(2,recvLeft[1])) {
						recvLeft[2]++;// inc leftStep
						// send to right
						// printf("myID: %i sent a election(103) message right from %i\n",myID, recvLeft[0]);
						MPI_Isend(&recvLeft,3,MPI_INT,mod(rank+1,NUM),123,MPI_COMM_WORLD,&requestLeft);
						msent++;
					}
					// send reply	
					else if (recvLeft[0] > myID && recvLeft[2] == pow(2,recvLeft[1])) {
						// send(REPLY(leftID,leftPhase)) // to the left
						recvLeft[2] = -1;
						// printf("myID: %i sent a reply(110) message to %i\n",myID, recvLeft[0]);
						MPI_Isend(&recvLeft,3,MPI_INT,mod(rank-1,NUM),123,MPI_COMM_WORLD,&requestRight);
						msent++;
					}
					else if (myID == recvLeft[0]){
						// printf("myID: %i I am a leader!\n",myID);
						leader=1;
						recvLeft[0] =-1;
						recvLeft[1] =-1;
						recvLeft[2] =-1;
						MPI_Send(&recvLeft,3,MPI_INT,mod(rank-1,NUM),123,MPI_COMM_WORLD);
						goto done;
					}
					else {
						// begin new elections!
						// printf("myid: %i in new elections\n",myID);
						goto top;
					}
		
				}
				else {
							// left REPLY
				if (recvLeft[2] == -1) {
					if (recvLeft[0] == -1 && recvLeft[1] == -1) {
						// forward to the next node
						MPI_Send(&recvLeft,3,MPI_INT,mod(rank+1,NUM),123,MPI_COMM_WORLD);
						goto done;
					}
					if (myID != recvLeft[0]) {
						// send reply to right
						// printf("myID: %i sent a reply (142) right from %i\n",myID, recvLeft[0]);
						MPI_Isend(&recvLeft, 3, MPI_INT,mod(rank+1,NUM),123,MPI_COMM_WORLD,&requestLeft);
						msent++;
					}
					else {
						leftReply = 1;
						// printf("myID: %i, leftReply is 1\n",myID);
					}
					// else if (already received REPLY(j,k))
						//send(ELECTION(j,k+1,1)) to left and right (start a new round)
				}
			}
		}
		else {
			MPI_Cancel(&requestLeft);
		}

		MPI_Irecv(&recvRight,3,MPI_INT,mod(rank+1,NUM),123,MPI_COMM_WORLD, &requestRight);
		MPI_Test(&requestRight, &rflag, &statusRight);
		if (rflag){
			mrecved++;
					// printf("116: myID: %i, received from right: %i, %i, %i\n", myID, recvRight[0],recvRight[1],recvRight[2]);
				// right ELECTION, if d!=-1
				if (recvRight[2] != -1) {
					// send election to left
					if (recvRight[0] > myID && recvRight[2] < pow(2,recvRight[1])) {
						recvRight[2]++;// inc leftStep
						// printf("myID: %i sent an election (123) left from  %i\n",myID, recvRight[0]);
						MPI_Isend(&recvRight,3,MPI_INT,mod(rank-1,NUM),123,MPI_COMM_WORLD,&requestRight);
						msent++;
					}
					// send reply
					else if (recvRight[0] > myID && recvRight[2] == pow(2,recvRight[1])) {
						//send(REPLY(rightID,rightPhase)) // to the right
						recvRight[2]=-1;
						// printf("myID: %i sent a reply (130) right from %i\n",myID, recvRight[0]);
						MPI_Isend(&recvRight,3,MPI_INT,mod(rank+1,NUM),123,MPI_COMM_WORLD,&requestLeft);
						msent++;
					}
					else if (myID == recvRight[0]) {
						// I am the leader
						leader=1;
						// printf("myID: %i I am a leader!\n",myID);
						recvRight[0] =-1;
						recvRight[1] =-1;
						recvRight[2] =-1;
						MPI_Send(&recvRight,3,MPI_INT,mod(rank+1,NUM),123,MPI_COMM_WORLD);
						goto done;
					}
					else {
						// new elections!
						// printf("myid: %i in new elections\n",myID);
						goto top;
					}
				}
				else {
							// right REPLY
				if (recvRight[2] == -1) {
					if (recvRight[0] == -1 && recvRight[1] == -1) {
						// forward to the next node
						MPI_Send(&recvRight,3,MPI_INT,mod(rank-1,NUM),123,MPI_COMM_WORLD);
						goto done;
					}
					if (myID != recvRight[0]) {
						// send reply to right
						// printf("myID: %i sent a reply(152) left from %i\n",myID, recvLeft[0]);
						MPI_Isend(&recvRight, 3, MPI_INT,mod(rank-1,NUM),123,MPI_COMM_WORLD,&requestRight);
						msent++;
					}
					else {
						// printf("myID: %i, rightReply is 1\n",myID);
						rightReply = 1;
					}
					// else if (already received REPLY(j,k))
						//send(ELECTION(j,k+1,1)) to left and right (start a new round)
				}
			}
		} else {
			MPI_Cancel(&requestRight);
		}


		if (leftReply && rightReply) {
			leftReply = 0;
			rightReply = 0;
			recvLeft[0] = myID;
			recvLeft[1]++;
			recvLeft[2]=1;
			MPI_Send(&recvLeft,3,MPI_INT,mod(rank-1,NUM),123,MPI_COMM_WORLD);
			msent++;
			recvRight[0] = myID;
			recvRight[1]++;
			recvRight[2]=1;
			MPI_Send(&recvRight,3,MPI_INT,mod(rank+1,NUM),123,MPI_COMM_WORLD);
			msent++;
		}
	}
	done: 
	printf("rank=%d, id=%d, leader=%i, mrecved=%d, msent=%d\n",rank, myID, leader, mrecved, msent);

	int stats[2] = {0,0}; // tsent, trecvd
	MPI_Request requestStats;
	MPI_Status statusStats;
	if (!leader) {
		MPI_Irecv(&stats,2,MPI_INT,mod(rank-1,NUM),111,MPI_COMM_WORLD, &requestStats);
		MPI_Wait(&requestStats,&statusStats);
		stats[0] += msent;
		stats[1] += mrecved;
		MPI_Isend(&stats, 2, MPI_INT, mod(rank+1,NUM),111,MPI_COMM_WORLD,&requestStats);
		MPI_Wait(&requestStats,&statusStats);
		// printf("myID: %i stats1 %i stats2 %i \n",myID, stats[0],stats[1]);
	}

	if (leader) {
		stats[0] += msent;
		stats[1] += mrecved;
		MPI_Isend(&stats, 2, MPI_INT, mod(rank+1,NUM),111,MPI_COMM_WORLD,&requestStats);
		MPI_Wait(&requestStats, &statusStats);
		MPI_Irecv(&stats,2,MPI_INT,mod(rank-1,NUM),111,MPI_COMM_WORLD, &requestStats);
		MPI_Wait(&requestStats, &statusStats);
		// printf("leader reports: %i %i\n",stats[0],stats[1]);
		printf("rank=%d, id=%d, trecved=%d, tsent=%d\n",rank,myID,stats[0],stats[1]);
	}

	MPI_Finalize();
	return 0;
}