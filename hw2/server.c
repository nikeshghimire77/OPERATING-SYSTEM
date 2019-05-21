#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
	
#define TRUE 1 
#define FALSE 0 
#define W_CODE_LEN 4
#define MAX_CLIENTS 3
#define MAX_BITS 3
#define PORT 8888

// Hadamard matrix
int W[W_CODE_LEN][W_CODE_LEN] = {{-1,-1,-1,-1}, {-1,1,-1,1}, {-1,-1,1,1}, {-1,1,1,-1}};

// structure to maintain client list
typedef struct client_list_t
{
	int sock_fd;
	int value;
	int em[12];
	int src_id;
	
} client_list_t;

/*
	Encoder routine.
	Accepts pointer to array to store encoded message, message to encode, walsh code to use for encoding.

	Step-1: Convert the integer message to its binary representation.

	Step-2: Generate encoded message by spreading bip1olar representaion of number with walsh code 
	EM[i] = Bipolar repr*walsh_code[i]
	(since number is in binary representation, inverse the walsh_code[i] if 0 else use the value of walsh_code[i])
*/
void encode(int *em, int n, int *w)
{
	int i = 2, j = 0;
	int binaryNum[MAX_BITS];

	// convert given digit to binary representation
	while (n > 0)
	{
		binaryNum[i] = n % 2; 
        
        n = n / 2; 
        i--;       
	}

	// Generate encoded message
	for (i = 0; i < MAX_BITS; i++)
	{
		for (j = 0; j < W_CODE_LEN; j++)
		{
			em[i*W_CODE_LEN+j] = binaryNum[i] == 1 ? w[j] : -w[j]; 
		}
	}
}
	
void main(int argc , char *argv[]) 
{
	// usage for server is: executable port 
	if (argc != 2)
	{
		printf("usage:\n");
		printf("\t%s <server port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *remote_port = argv[1];
	char *endptr;
	
	// convert remote_port to integer value
	int port = strtol(remote_port, &endptr, 0);
    if ( *endptr ) 
    {
		perror("Invalid port supplied");
		exit(EXIT_FAILURE);
    }


	int opt = TRUE; 
	int master_socket , addrlen , new_socket , client_socket[30] , activity, i , valread , sd, max_clients = MAX_CLIENTS; 
	int max_sd;

	struct sockaddr_in address; 
		 
	int receive_buffer[MAX_CLIENTS];

	//set of socket descriptors 
	fd_set readfds;  
	
	//initialise all client_socket[] to 0 so not checked 
	for (i = 0; i < MAX_CLIENTS; i++) 
	{ 
		client_socket[i] = 0; 
	} 
		
	//create a master socket 
	if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
	{ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	//set master socket to allow multiple connections 
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) 
	{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	} 
	
	
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons(port);
		
	//bind the socket to remote port 
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	}  
		
	//specify maximum of 3 pending connections for the master socket 
	if (listen(master_socket, 3) < 0) 
	{ 
		perror("listen"); 
		exit(EXIT_FAILURE); 
	} 
		
	//accept the incoming connection 
	addrlen = sizeof(address); 

	// maintain a client list
	client_list_t client_list[MAX_CLIENTS];
	int count = 0;
	int flag = 1;

	while(flag) 
	{ 
		//clear the socket set 
		FD_ZERO(&readfds); 
	
		//add master socket to set 
		FD_SET(master_socket, &readfds); 
		max_sd = master_socket; 
			
		//add child sockets to set 
		for ( i = 0 ; i < MAX_CLIENTS ; i++) 
		{ 
			//socket descriptor 
			sd = client_socket[i]; 
				
			//if valid socket descriptor then add to read list 
			if(sd > 0) 
				FD_SET( sd , &readfds); 
				
			//highest file descriptor number, need it for the select function 
			if(sd > max_sd) 
				max_sd = sd; 
		}
	
		//wait for an activity on one of the sockets , timeout is NULL, so wait indefinitely. 
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL); 
	
		if ((activity < 0) && (errno!=EINTR)) 
		{ 
			printf("select error"); 
		} 
			
		//If something happened on the master socket, then its an incoming connection. 
		if (FD_ISSET(master_socket, &readfds)) 
		{ 
			if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) 
			{ 
				perror("accept"); 
				exit(EXIT_FAILURE); 
			} 
				
			//add new socket to array of sockets 
			for (i = 0; i < MAX_CLIENTS; i++) 
			{ 
				//if position is empty 
				if( client_socket[i] == 0 ) 
				{ 
					client_socket[i] = new_socket; 					
					break; 
				} 
			} 
		}
			
		//else its some IO operation on some other socket 
		for (i = 0; i < MAX_CLIENTS; i++) 
		{ 
			sd = client_socket[i]; 
				
			if (FD_ISSET( sd , &readfds)) 
			{ 
				//Check if it was for closing , and also read the 
				//incoming message 
				if ((valread = read( sd , receive_buffer, sizeof(receive_buffer))) == 0) 
				{ 
					//Somebody disconnected
					//Close the socket and mark as 0 in list for reuse 
					close( sd ); 
					client_socket[i] = 0; 
				} 
					
				else
				{ 		
					int dest_id = ntohl(receive_buffer[2])-1;

					client_list[dest_id].src_id = ntohl(receive_buffer[0])-1;
					client_list[client_list[dest_id].src_id].sock_fd = sd;
					client_list[dest_id].value = ntohl(receive_buffer[1]);
					
					printf("Here is a message from child %d: Value = %d, Destination = %d\n", client_list[dest_id].src_id+1, client_list[dest_id].value, dest_id+1);
					count++;
				} 
			} 
		}

		if (count == MAX_CLIENTS)
		{
			int i,j;

			// message will represent the combination of encoded message and corresponding walsh code
			int message[W_CODE_LEN*MAX_CLIENTS+W_CODE_LEN];
			memset(&message, 0, sizeof(message));

			for(i = 0; i < count; i++)
			{
				// encode the received value.
				encode(client_list[i].em, client_list[i].value, W[client_list[i].src_id+1]);
				// prepare message
				// signal = EM1 + EM2 + EM3
				for(j = 0; j < 12; j++) message[j] += client_list[i].em[j];
			}
			
			for (i = 0; i < count; i++)
			{
				// add correspond code to message
				for (j = 0; j < W_CODE_LEN; j++)
				{
					message[W_CODE_LEN*MAX_CLIENTS+j] = W[client_list[i].src_id+1][j];
				}

				// send message to client
				send(client_list[i].sock_fd, message, sizeof(message), 0);
			}

			// close sockets
			for (i = 0; i < count; i++)
			{
				close(client_list[i].sock_fd);
				client_socket[i] = 0;
			}
			// Reset count
			count = 0;
			// set flag to exit loop
			flag = 0;
		} 
	}  
} 
