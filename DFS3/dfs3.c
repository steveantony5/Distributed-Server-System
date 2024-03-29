/*-------------------------------------------------------------------------------
 *File name : dfc.c
 *Author    : Steve Antony Xavier Kennedy

 *Description: This file implements distributed filesystem client

-------------------------------------------------------------------------------*/

/**************************************************************************************
*                                   Includes
**************************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <errno.h>
#include <netdb.h>
#include <dirent.h>
#include <sys/wait.h>
/**************************************************************************************
*                                   Macros
**************************************************************************************/

/*For debug purpose*/
#define DEBUG

#define LISTEN_MAX (10)

#define TIME_OUT_SOCKET (10)

#define SIZE_STR (40)

#define BUFFER (2048) 

#define KEY (255)

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
	char username[50];
	char password[50];
}credential;

credential user;

/*creating the socket*/
int server_socket, new_socket;


/*For setting socket timeout in Pipelining*/
struct timeval tv;

socklen_t clilen;
struct sockaddr_in server_address, to_address;

int validity = 0;

struct FRAME
{
	char file_data[BUFFER];
	int length;
};

struct FRAME frame;
/*****************************************************************
*						Local function prototypes
*****************************************************************/
status socket_creation(int);
status receive_file();
status folder_creation_user();
status Login();
status list_files();
status get_file();
void create_folder();

/***************************************************************
*                      Main Function
**************************************************************/

int main(int argc, char *argv[])
{
	//for knowing the status of each called function : error handling
	status status_returned;
	char command[20];


	if(argc<2)// passing port number as command line argument
	{
	    perror("Please provide port number");
	    exit(EXIT_FAILURE);
    }
        
    //Storing the port number for proxy server from command line vaiable
	int port = atoi(argv[1]);

	//Creating socket for proxy server
    status_returned = socket_creation(port);
	if(status_returned == ERROR)
	{
		printf("Error on socket creation\n");
	    exit(EXIT_FAILURE);
	}
	
	int child_id;

	while(1)
	{
		new_socket = 0;
		clilen = sizeof(to_address);

		/*Accepting Client connection*/
		new_socket = accept(server_socket,(struct sockaddr*) &to_address, &clilen);
		if(new_socket<0)
		{
			perror("Error on accepting client");
			exit(1);
		}
		else
			printf("established connection\n");

		


		child_id = 0;
		/*Creating child processes*/
		/*Returns zero to child process if there is successful child creation*/
		child_id = fork();

		// error on child process
		if(child_id < 0)
		{
			perror("error on creating child\n");
			exit(1);
		}

		//closing the parent
		if (child_id > 0)
		{
			close(new_socket);
			waitpid(0, NULL, WNOHANG);	//Wait for state change of the child process
		}


		if(child_id == 0)
		{
			close(server_socket);
			memset(command,0,sizeof(command));

			//receiving the command from client

			while(recv(new_socket,command ,20, 0)>0)
			{

				printf("-----------------------------\n");
				validity = 0;

				//PUT command
				if(strcmp(command,"PUT")==0)
				{
					Login();
					if(validity == 1)
					{
						folder_creation_user();
						receive_file();
					}
				}
				else if(strcmp(command,"LIST")==0)
				{
					Login();
					if(validity == 1)
					{
						list_files();
					}
				}

				else if(strcmp(command,"GET")==0)
				{
					Login();
					if(validity == 1)
					{
						get_file();
					}
				}
				else if(strcmp(command,"MKDIR")==0)
				{
					Login();
					if(validity == 1)
					{
						create_folder();
					}
				}
				else if(strcmp(command,"EXIT")==0)
				{
					exit(1);
				}
				else
				{
					printf("Invalid command\n");
				}

				memset(command,0,sizeof(command));

			}
			close(new_socket);
			exit(0);
		}
	}
		
}

/********************************************************************************************
*									Local Functions
********************************************************************************************/
//*****************************************************************************
// Name        : socket_creation
//
// Description : Funtion to create socket
//
// Arguments   : port - Port number
//
// return      : Not used
//
//****************************************************************************/

status socket_creation(int port)
{
	server_socket = 0;
	server_socket = socket(AF_INET,SOCK_STREAM,0);// setting the server socket
	if(server_socket < 0)
	{
		perror("error on server socket creation");
		return ERROR;
	}
	memset(&server_address,0,sizeof(server_address));

	int option = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr	= INADDR_ANY;
	server_address.sin_port = htons(port);

	/*bind the server socket with the remote client socket*/
	if(bind(server_socket,(struct sockaddr*)&server_address,sizeof(server_address))<0)
	{
		perror("Binding failed in the server");
		return ERROR;
	}


	/*Listening for clients*/
	if(listen(server_socket,LISTEN_MAX) < 0)
	{
		perror("Error on Listening ");
		return ERROR;
	}
	else
	{
		printf("\nlistening.....\n");
	}
	return SUCCESS;


}

status folder_creation_user()
{

	DIR *dir = opendir(user.username);
	if(dir)
		{
			printf("User's folder exists in the server\n");
		}
		else
		{
			char cmd[100];

			// creating new folder for the user
			sprintf(cmd,"mkdir -p %s",user.username);

			system(cmd);
			
			printf("Created Folder for the user: %s\n",user.username);
		}
	return SUCCESS;
}

status receive_file()
{


	int chunk_size;
	int count = 0;
	char chunk_filename[SIZE_STR];
	char Chunk_file_path[100];
	char chunk_subfolder[SIZE_STR];
	char folder_exists[200];

	int packets;

	while(count<2)
	{
		FILE *file_recv;
		chunk_size = 0;
		packets = 0;

		//receive chunk filename
		memset(chunk_filename,0,SIZE_STR);
		memset(folder_exists,0,SIZE_STR);
		memset(Chunk_file_path,0,sizeof(Chunk_file_path));
		memset(chunk_subfolder,0,sizeof(chunk_subfolder));

		recv(new_socket,chunk_filename ,SIZE_STR, 0); //1
		printf("Chunk filename %s\n",chunk_filename);

		recv(new_socket,chunk_subfolder ,SIZE_STR, 0); //1.1
		printf("Chunk subfolder %s\n",chunk_subfolder);

		sprintf(folder_exists,"%s/%s",user.username,chunk_subfolder);


		if(strlen(chunk_subfolder) > 0)
		{
			DIR *dir = opendir(folder_exists);
			if(dir)
			{
				printf("User's Sub folder exists in the server\n");
			}
			else
			{
				printf("User's Sub folder do not exists in the server\nCreating sub folder");
				char cmd[100];

				// creating new folder for the user
				sprintf(cmd,"mkdir -p %s",folder_exists);

				system(cmd);
			}
			sprintf(Chunk_file_path,"%s/%s/%s",user.username,chunk_subfolder,chunk_filename);
		}
		else
			sprintf(Chunk_file_path,"%s/%s",user.username,chunk_filename);

		recv(new_socket,&chunk_size ,sizeof(int), 0); //2
		printf("Chunk size : %d\n",chunk_size);

		recv(new_socket,&packets ,sizeof(int), 0); //3
		printf("packets : %d\n",packets);

		file_recv = NULL;

		file_recv = fopen(Chunk_file_path,"w");
		if(file_recv != NULL)
		{
			for(int frame_num = 0; frame_num < packets; frame_num++)
			{
				// receiving data from server					
				memset(&frame,0,sizeof(frame));
				
				recv(new_socket,&frame ,sizeof(frame), 0); //4

				for(int elt =0; elt<frame.length ; elt++)
				{	
					frame.file_data[elt] ^= KEY;
				}

				fwrite(frame.file_data,1,frame.length,file_recv);

			}
				
			/*#ifdef DEBUG
			printf("Received chunk %s\n\n",file_data);
			#endif */

			fclose(file_recv);
			file_recv = NULL;
		}
		else
		{
			printf("Error on malloc");
			return ERROR;
		
		}
		count++;
	}
	return SUCCESS;
}	


status Login()
{
	/***************** Checking user authenticity*********************/

		FILE *fp_DFS3_conf ;
		char username_verify[20], password_verify[20];

		fp_DFS3_conf = fopen("dfs3.conf","r");
		if(fp_DFS3_conf == NULL)
		{
			printf("Could not open dfs3.conf");
			return ERROR;
		}

		recv(new_socket,&user ,sizeof(user), 0);

		printf("Username of client %s\n",user.username);
		printf("Password of client %s\n",user.password);

		char line_from_config[100];

		validity = 0;

		while (fgets(line_from_config, sizeof(line_from_config), fp_DFS3_conf))
		{
			sscanf(line_from_config,"%s %s",username_verify,password_verify);

			if((strcmp(user.username,username_verify)==0) && (strcmp(user.password,password_verify)==0))
			{
				validity = 1;
				printf("Authenticated by DFS3\n");
			}
		}

		if(validity == 0)
			printf("Authentication error on DFS3\n");

		send(new_socket, &validity, sizeof(int) , 0);
		fclose(fp_DFS3_conf);
		return SUCCESS;

}

status list_files()
{
	char list_command[100];
	char sub_folder[40];
	memset(sub_folder,0,sizeof(sub_folder));
	
	recv(new_socket, sub_folder ,40, 0);

		printf("Subfolder name %s\n",sub_folder);


	if(strlen(sub_folder) > 0)
		sprintf(list_command,"ls -r %s/%s > list.log",user.username,sub_folder);
	else
		sprintf(list_command,"find %s -type f -exec basename {} \\; > list.log",user.username);

	system(list_command);
	FILE *list;
	list = fopen("list.log","r");
	if(list == NULL)
	{
		printf("Couldn't open list.log\n");
		return ERROR;
	}

	int32_t file_size = 0;
	struct stat file;

	stat("list.log", &file);
	file_size = file.st_size;


	char *list_data = (char *) malloc(file_size);
	if(list_data == NULL)
	{
		printf("Error on malloc in list files\n");
		return ERROR;
	}

	fread(list_data,1,file_size,list);
	send(new_socket, &file_size, sizeof(int) , 0);
	send(new_socket, list_data, file_size , 0);
	system("rm list.log");
	return SUCCESS;
}

status get_file()
{
	char list_command[100];
	int sub_folder_existance = 0;

	recv(new_socket,&sub_folder_existance ,sizeof(int), 0);

	if(sub_folder_existance)
		sprintf(list_command,"ls -LR %s > list.log",user.username);
	else
		sprintf(list_command,"ls %s > list.log",user.username);


	system(list_command);
	FILE *list;
	list = fopen("list.log","r");
	if(list == NULL)
	{
		printf("Couldn't open list.log\n");
		return ERROR;
	}

	int32_t file_size = 0;
	struct stat file;

	stat("list.log", &file);
	file_size = file.st_size;


	char *list_data = (char *) malloc(file_size);
	if(list_data == NULL)
	{
		printf("Error on malloc in list files\n");
		return ERROR;
	}

	fread(list_data,1,file_size,list);

	send(new_socket, &file_size, sizeof(int) , 0);
	send(new_socket, list_data, file_size , 0);

	system("rm list.log");

    //------------------sending the file-----------------------//
	int request;
	char chunk_name[SIZE_STR];
	char chunk_file_path[100];
	char chunk_sub_folder[40];
	
	int packets = 0;

	for(int count =0; count < 4;count ++ )
	{
		request =0;
		recv(new_socket,&request ,sizeof(int), 0); //1.1
		printf("received request %d\n",request);

		if(request == 1)
		{
			memset(chunk_file_path,0,sizeof(chunk_file_path));
			recv(new_socket,chunk_name ,SIZE_STR, 0); //1.2

			memset(chunk_sub_folder,0,sizeof(chunk_sub_folder));
			recv(new_socket,chunk_sub_folder ,sizeof(chunk_sub_folder), 0); //1.2

			if(strlen(chunk_sub_folder) > 0)
			{
				sprintf(chunk_file_path,"%s/%s/%s",user.username,chunk_sub_folder,chunk_name);
				printf("Requested chunk_name %s\n",chunk_file_path);	
			}
			else
			{
				sprintf(chunk_file_path,"%s/%s",user.username,chunk_name);
				printf("Requested chunk_name %s\n",chunk_file_path);
			}
			

			int32_t chunk_size = 0;
			struct stat file;

			stat(chunk_file_path, &file);
			chunk_size = file.st_size;

			printf("%s %d\n",chunk_name,chunk_size);

			send(new_socket, &chunk_size, sizeof(chunk_size) , 0); //1.3

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
			send(new_socket, &packets, sizeof(packets) , 0); //1.4

			FILE *data_chunk;
			data_chunk = fopen(chunk_file_path,"r");
			if(chunk_file_path ==  NULL)
			{
				printf("Error on opening chunk data\n");
				return ERROR;
			}

			for(int frame_num =0; frame_num <packets;frame_num++)
			{
					
				memset(&frame,0,sizeof(frame));

				frame.length  = fread(frame.file_data,1,BUFFER,data_chunk);

				for(int elt =0; elt<frame.length ; elt++)
				{	
					frame.file_data[elt] ^= KEY;
				}


				send(new_socket,&frame ,sizeof(frame), 0); //1.5
							
			}

		}

	}


	return SUCCESS;
}
void create_folder()
{
	char sub_folder_name[SIZE_STR];
	char folder_path[100];

	recv(new_socket,sub_folder_name ,SIZE_STR, 0);
	
	sprintf(folder_path,"%s/%s",user.username,sub_folder_name);
	
	DIR *dir = opendir(folder_path);
	if(dir)
	{
		printf("User's folder exists in the server\n");
	}
	else
	{
		char cmd[100];

		// creating new folder for the user
		sprintf(cmd,"mkdir -p %s",folder_path);

		system(cmd);
		
		#ifdef DEBUG
		printf("Created Folder for the user: %s\n",folder_path);
		#endif
	}
}