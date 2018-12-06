/*---------------------------------------------------------------------------------------------------------
 * File name : client.c
 * Author    : Steve Antony Xavier Kennedy
 *
 
 Command line inputs: <IP address> <port number>
 eg:
 ./client 127.0.0.1 9999

 --------------------------------------------------------------------------------------------------------*/
 

// Header section
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/stat.h>
 
#define BUFFER (2048) 
//#define DEBUG
#define SIZE_STR (40)
/**************************************************************************************
*                                   Global declaration
**************************************************************************************/
typedef enum
{
	SUCCESS = 1,
	ERROR = -1,
	EMPTY = -2,
	LOGIN_FAILED = -3,
}status;

typedef struct 
{
	char name[10];
	char ip[10];
	char port[10];
}dfs_info;

dfs_info DFS[4] = {0};

typedef struct
{
	char username[50];
	char password[50];
}credential;

credential user;

int socket_server[4] = {0};
struct sockaddr_in server_addr[4] = {0};

struct timeval tv;// for socket timeout

int validity[4] = {0};


int chunk_layout[4][4][2] = {{{0,3},{0,1},{1,2},{2,3}},
							 {{0,1},{1,2},{2,3},{0,3}},
							 {{1,2},{2,3},{0,3},{0,1}},
							 {{2,3},{0,3},{0,1},{1,2}}};   

char filename_command[40];
char sub_folder[40];

struct FRAME
{
	char file_data[BUFFER];
	int length;
};

struct FRAME frame;
/*****************************************************************
*						Local function prototypes
*****************************************************************/
status read_dfc_conf_file();
status establish_connection();
status GET();
status LIST();
status PUT();
int MD5HASH();
status login_to_server();
/***************************************************************
*                      Main Function
**************************************************************/

int main(int argc, char *argv[])
{
	char command[20];
	char filename[20];
	char input_choice[30];



	read_dfc_conf_file();


	//establish connection with the servers
	establish_connection();

	

	while(1)
	{
		
		validity[0] = 0;
		validity[1] = 0;
		validity[2] = 0;
		validity[3] = 0;
		
		memset(command,0,sizeof(command));
		memset(filename,0,sizeof(filename));
		memset(input_choice,0,sizeof(input_choice));

		printf("Enter your command:\n");
		printf("1. LIST \n2. GET <filename>\n3. PUT <filename>\n\n");
		scanf(" %[^\n]%*c", input_choice);

		sscanf(input_choice,"%s%s%s",command,filename_command,sub_folder);

		for(int server = 0; server<4;server++)
		{
			send(socket_server[server], command, 20 , 0);
		}

		#ifdef DEBUG
		printf("filename %s\n",filename_command );
		#endif

		if(strcmp(command,"LIST")==0)
		{
			login_to_server();
			LIST();
		}

		else if( (strcmp(command,"GET")==0))
		{
			login_to_server();
			GET();
		}

		else if( (strcmp(command,"PUT")==0))
		{
			login_to_server();
			PUT();
		}
		else
		{
			printf("Invalid command\n");
		}
	}

}

status read_dfc_conf_file()
{
	FILE *read_dfc_conf;
	char filename[20];
	char filename_input[10];
	memset(filename,0,sizeof(filename));
	memset(filename_input,0,sizeof(filename_input));

	printf("Enter your configuration filename\n");
	scanf("%s",filename_input);
	sprintf(filename,"DFC/%s",filename_input);

	read_dfc_conf = fopen(filename,"r");
	if(read_dfc_conf == NULL)
	{
		printf("Couldn't find %s\n",filename);
		return ERROR;
	}
	else
	{
		int32_t file_size = 0;
		struct stat file;

		stat(filename, &file);
		file_size = file.st_size;

		char *config_details = malloc(file_size);
		if(config_details == NULL)
		{
			printf("Error on malloc for config_details\n");
			return ERROR;
		}
		else
		{
			fread(config_details,1,file_size,read_dfc_conf);

			char *search = NULL;
			search  =  strstr(config_details,"Server");
			if(search!=NULL)
			{	
				for(int server=0;server<4;server++)
				{
					if(server!=0)
						search  =  strstr(search,"Server");

					if(search!=NULL)
					{
						sscanf(search,"%*s %s %s %s",DFS[server].name, DFS[server].ip, DFS[server].port);

						#ifdef DEBUG
						printf("\n\nserver_name %s\n",DFS[server].name);
						printf("ip   %s\n",DFS[server].ip);
						printf("port %s\n",DFS[server].port);
						#endif
						
						search = strstr(search," ");
						if(search == NULL)
						{
							printf("error");
							return ERROR;
						}
					}
					else
					{
						printf("Missing server configuration data\n");
						return ERROR;
					}

				}
			}
			else
			{
				printf("Missing server configuration data\n");
				return ERROR;
			}
			
			search = strstr(config_details,"Username");
			if(search == NULL)
			{
				printf("missing Username");
				return ERROR;
			}
			sscanf(search,"%*s%s",user.username);
			printf("username %s\n",user.username);

			search = strstr(config_details,"Password");
			if(search == NULL)
			{
				printf("missing Password");
				return ERROR;
			}
			sscanf(search,"%*s%s",user.password);

			printf("password %s\n",user.password);

		}

	}

	return SUCCESS;
	
}

status establish_connection()
{
	for(int server=0;server<4;server++)
	{

		socket_server[server] = socket(AF_INET,SOCK_STREAM,0);// setting the server socket
		

		server_addr[server].sin_family = AF_INET;
		server_addr[server].sin_addr.s_addr	= inet_addr(DFS[server].ip);
		server_addr[server].sin_port = htons(atoi(DFS[server].port));
		printf("sock val %d\n",socket_server[server]);
		connect(socket_server[server],(struct sockaddr *) &server_addr[server], sizeof(server_addr[server]));
		
		
		int option = 1;
		setsockopt(socket_server[server], SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

				       
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		setsockopt(socket_server[server], SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));

		
	}
	return SUCCESS;

}

status PUT()
{

		//-------------- splitting file into 4 chunks -------------------------------

		FILE *fp_original_file;
		int x = 0;
		int chunk_size = 0, count = 0;

		#ifdef DEBUG
		printf("file to transfer %s\n", filename_command );
		#endif
		fp_original_file = NULL;

		fp_original_file = fopen(filename_command,"r");
		if(fp_original_file == NULL)
		{
			printf("Could not open the requested file %s",filename_command);
			return ERROR;
		}

		int file_size = 0;
		struct stat file_put;
		stat(filename_command, &file_put);
		file_size = file_put.st_size;

		
		x = MD5HASH();
		chunk_size = 0;
		chunk_size = file_size / 4;

		#ifdef DEBUG
		printf("Total File size : %d\n",file_size);
		#endif

		char chunk_filename[SIZE_STR];
		int packets;

		for(int Chunk_num = 0;Chunk_num < 4 ; Chunk_num++)
		{
			if(Chunk_num == 3)
			{
				chunk_size = file_size - (chunk_size*3) ;
			}

			char *buffer = (char *)calloc(1,chunk_size);
			if(buffer != NULL)
			{
				count = 0;
				while (count < 2)
				{	

					if(count == 0)
					{
						fread(buffer,1,chunk_size,fp_original_file);

						packets = 0;
						// if file size is exactly in multiples of BUFFERSIZE
						if((chunk_size % BUFFER ) == 0)
						{
							packets = (chunk_size/BUFFER);
						}
						// if file size is not exactly a multiple of BUFFERSIZE
						else
						{
							packets = (chunk_size/BUFFER) + 1;
						}

						printf("Total packets = %d\n",packets);

						FILE *chunk;
						chunk = fopen("chunk_file","w");
						if(chunk==NULL)
						{
							printf("Error on chunk_file\n");
							return ERROR;
						}

						fwrite(buffer,1,chunk_size,chunk);
						fclose(chunk);
						//printf("Buffer %s\n",buffer);

						//***************Chunk file name*********************//
						memset(chunk_filename,0,sizeof(chunk_filename));
						sprintf(chunk_filename,"%s.%d",filename_command,(Chunk_num+1));
					}

					

					if(validity[chunk_layout[x][Chunk_num][count]] == 1)
					{
						FILE *chunk_send;
						chunk_send = fopen("chunk_file","r");	
						if(chunk_send==NULL)
						{
							printf("Error on chunk_file\n");
							return ERROR;
						}

						//send chunk filename
						send(socket_server[chunk_layout[x][Chunk_num][count]], chunk_filename, SIZE_STR , 0); //1
					

						send(socket_server[chunk_layout[x][Chunk_num][count]], &chunk_size, sizeof(int) , 0); //2

						send(socket_server[chunk_layout[x][Chunk_num][count]], &packets, sizeof(int) , 0); //3

						for(int frame_num =0; frame_num <packets;frame_num++)
						{
					
							memset(&frame,0,sizeof(frame));
							frame.length  = fread(frame.file_data,1,BUFFER,chunk_send);

							send(socket_server[chunk_layout[x][Chunk_num][count]],&frame ,sizeof(frame), 0); //4
							
						}

						printf("Chunk %d size %d\n",Chunk_num,chunk_size);
						printf("Sending chunk %d to server %d\n",Chunk_num, chunk_layout[x][Chunk_num][count]);
						//printf("Buffer content:\n%s\n\n",buffer);
						fclose(chunk_send);

					}

					count++;

				}
				free(buffer);
				buffer = NULL;
			}
			else
			{
				printf("Malloc failed for buffer \n");
				return ERROR;
			}

		}
	
	return SUCCESS;

}	

// Calculate the MD5sum of every file and calculate md5sum%4 and return the remainder
int MD5HASH()
{
  char md5_command[80];
  sprintf(md5_command,"md5sum %s > tmp.txt",filename_command);
  system(md5_command);
  char md5_value[40];

  FILE *tmp;
  tmp = fopen("tmp.txt","r");
  if(tmp == NULL)
  {
    printf("no file tmp.txt");
    return 0;
  }

  int file_size = 0;
    struct stat file;

    stat("tmp.txt", &file);
    file_size = file.st_size;

  char *tmp_buffer = (char *) malloc(file_size);

  if(tmp_buffer == NULL)
  {
    return 0;
  }
  
  fread(tmp_buffer,1,file_size,tmp);
  sscanf(tmp_buffer,"%s",md5_value);
  system("rm tmp.txt");

  int last_byte;

  #ifdef DEBUG
  printf("md5 value %s\n",md5_value);

  printf("md5 last_byte %c\n",md5_value[31]);

  #endif

  if((md5_value[31] == 'a') || (md5_value[31] == 'b') || (md5_value[31] == 'c') || (md5_value[31] == 'd') || (md5_value[31] == 'e') || (md5_value[31] == 'f'))
  {
    if((md5_value[31] == 'a'))  
      last_byte = 10;
    else if(md5_value[31] == 'b')
      last_byte = 11;
    else if(md5_value[31] == 'c')
      last_byte = 12;
    else if(md5_value[31] == 'd')
      last_byte = 13;
    else if(md5_value[31] == 'e')
      last_byte = 14;
    else if(md5_value[31] == 'f')
      last_byte = 15;
  }
  else
  {
    last_byte = atoi(&md5_value[31]);
  }

  #ifdef DEBUG
  printf("last byte %d\n",last_byte);
  #endif

  return (last_byte%4);

}

status login_to_server()
{
	printf("Verifying credentials...\n");
	//-------------- Login Authentication----------------------
	for(int server =0; server < 4; server++)
	{
		//sending username and password
		send(socket_server[server], &user, sizeof(user) , 0);

		recv(socket_server[server],&validity[server] ,sizeof(int), 0);

		if(validity[server] == 0)
			printf("Logged in failed for server %d\n",(server+1));

	}

	return SUCCESS;
}

status LIST()
{
	int file_size[4] = {0};
	FILE *list;

	list = fopen("list.txt","w");
	if(list == NULL)
	{
		printf("Could not open list.txt\n");
		return ERROR;
	}


	for(int server = 0; server < 4; server++)
	{
		if(validity[server] == 1)
		{
			recv(socket_server[server],&file_size[server] ,sizeof(int), 0);

			char *buffer = (char *)malloc(file_size[server]);
			if(buffer == NULL)
			{
				printf("error on malloc\n");
				return ERROR;
			}
			recv(socket_server[server],buffer ,file_size[server], 0);

			fwrite(buffer,1,file_size[server],list);
			free(buffer);
			buffer = NULL;
		}

	}
	fclose(list);

	FILE *read_list_database;
	read_list_database = fopen("list.txt","r");
	if(read_list_database == NULL)
	{
		printf("error on reading list.txt\n");
		return ERROR;
	}

	FILE *search_in_file;
	search_in_file = fopen("list.txt","r");
	if(search_in_file == NULL)
	{
		printf("error on reading list.txt\n");
		return ERROR;
	}
	
	int32_t file_size_list = 0;
	struct stat file;

	stat("list.txt", &file);
	file_size_list = file.st_size;

	char *buffer = (char *) malloc (file_size_list);
	if(buffer == NULL)
	{
		printf("Error on malloc of buffer\n");
		return ERROR;
	}

	fread(buffer,1,file_size_list,search_in_file);

	#ifdef DEBUG
	printf("buffer\n\n%s\n",buffer);
	#endif

	fclose(search_in_file);

	char line_from_list[100];
	char *temp = NULL;
	char *found;
	int FOUND;
	char display_buffer[400];


	while (fgets(line_from_list, sizeof(line_from_list), read_list_database))
	{
        char *temp = strrchr(line_from_list,'.');
        if(temp == NULL)
        {
        	printf("No files present in server\n");
        	return EMPTY;
        }

        *temp = '\0';
        #ifdef DEBUG
        printf("%s\n",line_from_list);
        #endif

        char chunk_name[50];
        FOUND = 0;

        for(int file_chunk_num = 0; file_chunk_num < 4 ; file_chunk_num++)
        {
        	memset(chunk_name,0,sizeof(chunk_name));
        	sprintf(chunk_name,"%s.%d",line_from_list,(file_chunk_num+1));
        	
        	#ifdef DEBUG
        	printf("--file_chunk name search %s\n",chunk_name);
        	#endif

        	found = strstr(buffer,chunk_name);
        	if(found != NULL)
        	{
        		FOUND++;
        	}
        	else
        	{
        		#ifdef DEBUG
        		printf("could not find chunk %s\n",chunk_name);
        		#endif
        	}

        }
        char format_to_display[100]; 
        if(FOUND == 4)
        {
        	#ifdef DEBUG
        	printf("File completely recoverable\n");
        	#endif

        	sprintf(format_to_display,"%s - Complete\n",line_from_list);
        	if(strstr(display_buffer,line_from_list) == NULL)
        	{
        		strcat(display_buffer,format_to_display);
        	}
        }
        else
        {
        	#ifdef DEBUG
        	printf("file not completely recoverable\n");
        	#endif

        	sprintf(format_to_display,"%s - InComplete\n",line_from_list);
        	if(strstr(display_buffer,line_from_list) == NULL)
        	{
        		strcat(display_buffer,format_to_display);
        	}
        }

    }
    printf("------------------------------------\n");
    printf("%s\n",display_buffer);
	free(read_list_database);
	return SUCCESS;
}

status GET()
{
	//--------------------- Status of files from server--------------------//

	int list_size[4] = {0};
	char *buffer[4];

	for(int server = 0; server < 4; server++)
	{
		if(validity[server] == 1)
		{

			recv(socket_server[server],&list_size[server] ,sizeof(int), 0); //1

			buffer[server] = (char *)malloc(list_size[server]);
			if(buffer[server] == NULL)
			{
				printf("error on malloc\n");
				return ERROR;
			}
			recv(socket_server[server],buffer[server] ,list_size[server], 0);//2
			printf("buffer[%d]\n---------\n%s\n",server,buffer[server]);
		}
	}

	//----------------- fetching file ------------------------------//
	int FOUND_CHUNK[4] = {0};
	char chunk_name[SIZE_STR];
	int request = 0	;
	int chunk_size = 0;
	int packets;

	FILE *get_data;
	get_data = fopen(filename_command,"w");
	if(get_data == NULL)
	{
		printf("Error on creating file %s\n",filename_command);
		return ERROR;
	}

	for(int chunk_num = 0; chunk_num < 4; chunk_num++)
	{
		sprintf(chunk_name,"%s.%d",filename_command,(chunk_num+1));
		printf("\n\n\nChunk_name %s\n-------------\n",chunk_name);

		for(int server = 0; server < 4; server++)
		{
			if(validity[server] == 1)
			{
				printf("FOUND_CHUNK[%d] %d\n",server,FOUND_CHUNK[chunk_num]);
				printf("searching %s in buffer[%d]\n",chunk_name,server);

				char *search = NULL;
				search = strstr(buffer[server],chunk_name);
				if((search != NULL) && (FOUND_CHUNK[chunk_num] != 1))
				{
					printf("Found %s in server %d\n",chunk_name,server);

					request = 1;
					printf("request = %d sent to server %d\n",request,server);
					send(socket_server[server], &request, sizeof(int) , 0); //1.1

					send(socket_server[server], chunk_name, SIZE_STR , 0); //1.2

					recv(socket_server[server],&chunk_size ,sizeof(chunk_size), 0); //1.3

					packets = 0;
					recv(socket_server[server],&packets ,sizeof(packets), 0); //1.4

					for(int frame_num = 0; frame_num < packets; frame_num++)
					{
						// receiving data from server					
						memset(&frame,0,sizeof(frame));
						recv(socket_server[server],&frame ,sizeof(frame), 0); //4

						fwrite(frame.file_data,1,frame.length,get_data);

					}
				
					/*#ifdef DEBUG
					printf("Received chunk %s\n\n",file_data);
					#endif */
					
					printf("Chunk_%d size = %d\n",chunk_num,chunk_size);

					FOUND_CHUNK[chunk_num] = 1;

				}
				else
				{
					request = 0;
					printf("request = %d sent to server %d\n",request,server);
					send(socket_server[server], &request, sizeof(request) , 0); //1.1
				}
			}

		}
		
	}
	fclose(get_data);

	//Clearing the buffer
	for(int server = 0; server < 4; server++)
	{
		if(buffer[server] != NULL)
			free(buffer[server]);
	}
	return SUCCESS;
}
