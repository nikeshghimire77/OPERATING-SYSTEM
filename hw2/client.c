#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <sys/socket.h>       
#include <sys/types.h>        
#include <arpa/inet.h>        
#include <unistd.h>           
#include <string.h>

// define required constants
#define W_CODE_LEN 4
#define MAX_PROCESS 3
#define MAX_BITS 3


int PORT;
char * ADDRESS;

// Structure to store information given as input
typedef struct input
{
	int src_id;
	int dest_pid;
	int value;
} input;

// Structure to store information received from server
typedef struct output
{
	int src_id;
	int em[W_CODE_LEN*MAX_BITS];
	int w_code[W_CODE_LEN];
	int value;
} output;

/*
	Decoder routine.
	Accepts pointer to array representing encoded message and walsh code.
	Returns the decoded integer value.

	Step-1: Decoded Message = Encoded message * walsh code
	(since walsh code is in binary form, instead of multiplication, we use inverse value if 0, else we use the same value)

	Step-2: Sum every 4 digits of encoded message and store in binaryNum

	Step-3: Divide binaryNum with length of code i.e 4

	Step-4: Convert to binary representation from bipolar form

	Step-5: Convert to decimal from binary representation.

	Step-6: return the decoded message.
*/
int decode(int * em, int * w)
{
	int i, j, n;
	int binaryNum[MAX_BITS] = {0,0,0};
	
	int x[12];

	for (i = 0; i < W_CODE_LEN*MAX_BITS; i++) x[i] = em[i];
	for (i = 0; i < MAX_BITS; i++)
	{
		for ( j = 0; j < W_CODE_LEN; j++)
		{
			// DM = EM*W
			x[i*W_CODE_LEN+j] = (w[j] == 1)? em[i*W_CODE_LEN+j] : -em[i*W_CODE_LEN+j];			
			// genarate correponding binary representation
			binaryNum[i] += x[i*W_CODE_LEN+j];
		}
		// Since original message was spread using codes of length W_CODE_LEN
		binaryNum[i] /= W_CODE_LEN;

		// Correct conversion from bipolar representation
		if (binaryNum[i] == -1) binaryNum[i] = 0;
	}
	
	// Decoded value equals value represented by the binary number
	n = binaryNum[0]*4 + binaryNum[1]*2 + binaryNum[2]*1;
	return n;	
}

/*
	Helper function to print the desired output to screen.
	Accepts data element of structure type output.

	Conversion of signal and code to string values so that output to STDOUT can be done in a single operation.
	This prevents two threads to print to the STDOUT at the same time
*/
void print_to_screen(output data)
{
	char wsignal[W_CODE_LEN*MAX_BITS*2];
	char code[W_CODE_LEN*2];

	char map[] = {'0', '1', '2', '3'};

	int i, j = 0;

	// convert em values to string
	for (i = 0; i < W_CODE_LEN*MAX_BITS; i++)
	{
		if (data.em[i] < 0)
		{
			wsignal[j] = '-';
			wsignal[j+1] = map[-data.em[i]];
			j += 2;
		}

		else
		{
			wsignal[j] = map[data.em[i]];
			j++;
		}
	}
	wsignal[j] = '\0';

	// convert w_code values to string
	j = 0;
	for(i = 0; i < W_CODE_LEN; i++)
	{
		if (data.w_code[i] < 0)
		{
			code[j] = '-';
			code[j+1] = map[-data.w_code[i]];
			j += 2;
		}

		else
		{
			code[j] = map[data.w_code[i]];
			j++;
		}
	}
	code[j] = '\0';

	// Print to screen
	printf("Child %d\nSignal: %s\nCode: %s\nReceived Value: %d\n", data.src_id, wsignal, code, data.value);
}

/*
	Routine executed by each thread.
	Accepts a pointer to structure input variable.
	
	Establishes a connection with server and sends the input information to the server.
	Sending buffer contains 3 values : sender id, message, destination id.

	Receives a message from the server.
	Received value is the encoded message followed by a 4-length walsh code.
	This information is used to decode the message using decoder routine and then required data is printed to the screen.
*/
void *thread_client(void *vargp) 
{ 
    input *client = (input *)vargp; 

    struct sockaddr_in servaddr;
    int conn_s = -1;
    int send_buffer[3];
    // char receive_buffer[1024];
    int receive_buffer[16];

    // Set all bytes in socket address structthread_client, and fill in the relevant data members
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);

    //  Set the remote IP address 
    if ( inet_aton(ADDRESS, &servaddr.sin_addr) <= 0 )
    {
		perror("Invalid remote IP address.");
		exit(EXIT_FAILURE);
    }

    // Create an unbounded socket
	if ((conn_s = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		perror("Failed to create socket");
		return NULL;
	}

	// connect() to the remote server 	
	if (connect(conn_s, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)) < 0) 
	{
		perror("Failed to connect to server");
		return NULL;
	}

	// initialize send buffer with assosciated values
	send_buffer[0] = htonl(client->src_id); 
	send_buffer[1] = htonl(client->value); 
	send_buffer[2] = htonl(client->dest_pid);
	

	// printf("Client %d sends %d to process %d\n", client->src_id, client->value, client->dest_pid);
    printf("Child %d, sending value: %d to child process %d\n", client->src_id, client->value, client->dest_pid); 
	write(conn_s, send_buffer, sizeof(send_buffer));

	// erase the send buffer
	memset(&send_buffer, 0, sizeof(send_buffer));

	// read data sent by the server
	read(conn_s, receive_buffer, sizeof(receive_buffer));
	
	// read values from buffer to our data structure
	output data;
	int i;
	data.src_id = client->src_id;
	for (i = 0; i < W_CODE_LEN*MAX_BITS; i++) data.em[i] = receive_buffer[i];
	for (i = 0; i < W_CODE_LEN; i++) data.w_code[i] = receive_buffer[W_CODE_LEN*MAX_BITS + i];
	
	// call routine for decoding and printing to screen
	data.value = decode(data.em, data.w_code);
	printf("\n");
	print_to_screen(data);

	return NULL;
}


void main(int argc, char* argv[])
{
	char * remote_port;
	char * remote_address;
	char * endptr;

	input client[MAX_PROCESS];

	// usage for client is: executable ipaddress port 
	if (argc != 3)
	{
		printf("usage:\n");
		printf("\t%s <server ip> <server port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	remote_address = argv[1];
	remote_port = argv[2];

	// convert port to integer value
	PORT = strtol(remote_port, &endptr, 0);
    if ( *endptr ) 
    {
		perror("Invalid port supplied");
		exit(EXIT_FAILURE);
    }

    ADDRESS = remote_address;


	int i;
	for (i = 0; i < MAX_PROCESS; i++)
	{
		client[i].src_id = i+1;
		scanf("%d", &client[i].dest_pid);
		scanf("%d", &client[i].value);
	}

	int rc;

	pthread_t t_client1, t_client2, t_client3;
	
	// create the 1st thread 
    if ( rc = pthread_create(&t_client1, NULL, thread_client, (void*)&client[0]) )
    {
    	perror("Error creating thread");
    	exit(EXIT_FAILURE);	
    }
		sleep(2);
    // create the second thread
    if ( rc = pthread_create(&t_client2, NULL, thread_client, (void*)&client[1]) )
    {
    	perror("Error creating thread");
    	exit(EXIT_FAILURE);	
    }
    sleep(2);
    // create the third thread
    if ( rc = pthread_create(&t_client3, NULL, thread_client, (void*)&client[2]) )
    {
    	perror("Error creating thread");
    	exit(EXIT_FAILURE);	
    }
     
    // wait for threads to finish execution
    pthread_join(t_client1, NULL);
    pthread_join(t_client2, NULL);
    pthread_join(t_client3, NULL);
}