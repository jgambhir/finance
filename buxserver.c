#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "lists.h"
#include "wrapsock.h"

#define INPUT_BUFFER_SIZE 256
#define INPUT_ARG_MAX_NUM 5
#define DELIM "\r\n"
#define DELIM2 " "
#define LISTENQ 30
#define MAXCLIENTS 30

#ifndef PORT
#define PORT 56179
#endif

typedef struct {
	int soc;
	char buf[INPUT_BUFFER_SIZE];
	int curpos;
	char name[INPUT_BUFFER_SIZE];
} Client;

/* 
 * Read and process buxfer commands
 */
int process_cmd(int cmd_argc, char **cmd_argv, Group **group_list_addr, Client *user, Client *clients) {
    Group *group_list = *group_list_addr; 
    Group *g;
	char *buf;
	User *u;
	int length;
	char *message;

    if (cmd_argc <= 0) {
        return 0;
    } else if (strcmp(cmd_argv[0], "quit") == 0 && cmd_argc == 1) {
        return -1;
    
    } else if (strcmp(cmd_argv[0], "add_group") == 0 && cmd_argc == 2) {
		buf = add_group(group_list_addr, cmd_argv[1]);
		if (write(user->soc, buf, strlen(buf) + 1) == -1) {
			perror("write");
		}
		
		//add current user to the group
		char *temp = buf;
		g = find_group(*group_list_addr, cmd_argv[1]);
		// check if user is already in the group, before calling add_user.
		u = find_prev_user(g, user->name);
		//attempt to add the user to group g, and send them the result string.
		buf = add_user(g, user->name);
		if (write(user->soc, buf, strlen(buf) + 1) == -1) {
			perror("write");
		}
		free(temp);
		
		//if the user wasn't already in the group, tell the other users a new user was added.
		if (u == NULL) {
			//build the "new user added" message to broadcast to every other member in the group
			length = snprintf(NULL, 0, "%s has been added to group %s.\r\n", user->name, cmd_argv[1]) + 1;
			message = malloc(length + 1);
			if (message == NULL) {
				perror("Malloc error.");
				exit(1);
			}
			snprintf(message, length, "%s has been added to group %s.\r\n", user->name, cmd_argv[1]);
			int i;
			//broadcast the "new user added" message to every other member in the group
			for (i = 0; i < MAXCLIENTS; i++) {
				if (clients[i].soc != -1 && find_prev_user(g, clients[i].name) != NULL && &clients[i] != user) {
					if (write(clients[i].soc, message, strlen(message) + 1) == -1) {
						perror("write");
					}
				}
			}
		}
        
    } else if (strcmp(cmd_argv[0], "list_groups") == 0 && cmd_argc == 1) {
        buf = list_groups(group_list);
		if (write(user->soc, buf, strlen(buf) + 1) == -1) {
			perror("write");
		}
    
    } else if (strcmp(cmd_argv[0], "list_users") == 0 && cmd_argc == 2) {
        if ((g = find_group(group_list, cmd_argv[1])) == NULL) {
			buf = "Group does not exist.\r\n";
        } else {
            buf = list_users(g);
        }
		if (write(user->soc, buf, strlen(buf) + 1) == -1) {
			perror("write");
		}
		
    } else if (strcmp(cmd_argv[0], "user_balance") == 0 && cmd_argc == 2) {
        if ((g = find_group(group_list, cmd_argv[1])) == NULL) {
			buf = "Group does not exist.\r\n";
        } else {
			buf = user_balance(g, user->name);
        }
		if (write(user->soc, buf, strlen(buf) + 1) == -1) {
			perror("write");
		}
		
    } else if (strcmp(cmd_argv[0], "add_xct") == 0 && cmd_argc == 3) {
        if ((g = find_group(group_list, cmd_argv[1])) == NULL) {
			buf = "Group does not exist.\r\n";
        } else {
            char *end;
            double amount = strtod(cmd_argv[2], &end);
            if (end == cmd_argv[2]) {
				buf = "Incorrect number format\r\n";
            } else {
				buf = add_xct(g, user->name, amount);
            }
        }
		if (write(user->soc, buf, strlen(buf) + 1) == -1) {
			perror("write");
		}

    } else {
		buf = "Incorrect syntax\r\n";
		if (write(user->soc, buf, strlen(buf) + 1) == -1) {
			perror("write");
		}
    }
    return 0;
}

/*  read from the client
* 	returns -1 if the socket needs to be closed and 0 otherwise */
// code is based on readfromclient from selectbuffer.c
int readfromclient(Client *c, Group **group_list_addr, Client *clients) {
	
	char *startptr = &c->buf[c->curpos];
	int len = read(c->soc, startptr, INPUT_BUFFER_SIZE - c->curpos);
	char *cmd_argv[INPUT_ARG_MAX_NUM];
    int cmd_argc;
	int i;
	i = 0;
	char *commands[INPUT_BUFFER_SIZE];
	int n_commands;

	if(len <= 0) {
		if(len == -1) {
			perror("read on socket");
		}
		return -1;
		/* connection closed by client */

	} else {
		c->curpos += len;
		c->buf[c->curpos] = '\0';
		
		/* Did we get a whole line?*/
		if (strchr(c->buf, '\n') || strchr(c->buf, '\r')) {
			//delimit the buffer by newlines to possibly split multiple commands.
			char *next_token = strtok(c->buf, DELIM);
			n_commands = 0;
			while (next_token != NULL) {
				commands[n_commands] = next_token;
				n_commands++;
				next_token = strtok(NULL, DELIM);
			}
			commands[n_commands] = NULL;
			
			//execute each command in commands
			int n = 0;
			while (commands[n] != NULL) {
				//tokenize the command (delimit by spaces, since \r and \n have already been removed
				// by the previous strtok.
				char *cmd_token = strtok(commands[n], DELIM2);
				cmd_argc = 0;
				while (cmd_token != NULL) {
					if (cmd_argc >= INPUT_ARG_MAX_NUM - 1) {
						char *msg = "Too many arguments!\r\n";
						if(write(c->soc, msg, strlen(msg) + 1) == -1) {
						perror("write");
						}
						cmd_argc = 0;
						break;
					}
					cmd_argv[cmd_argc] = cmd_token;
					cmd_argc++;
					cmd_token = strtok(NULL, DELIM2);
				}
				cmd_argv[cmd_argc] = NULL;
				
				//if the user has no name, the buffer contains their name.
				if (c->name[0] == '\0') {
				char *message;
				int length;
				strncpy(c->name, cmd_argv[0], strlen(cmd_argv[0]));
				c->name[strlen(cmd_argv[0])] = '\0';
				
				length = snprintf(NULL, 0, "Welcome, %s! Please enter Buxfer commands.\r\n", c->name) + 1;
				message = malloc(length);
				if (message == NULL) {
					perror("Malloc error.");
					exit(1);
				}
				snprintf(message, length, "Welcome, %s! Please enter Buxfer commands.\r\n", c->name);
				if(write(c->soc, message, strlen(message) + 1) == -1) {
					perror("write");
				}
				free(message);
				}
				//The user has a name. The buffer contains a buxfer command.
				else {
					i = process_cmd(cmd_argc, cmd_argv, group_list_addr, c, clients);
				}
				n++; //move on to the next command (if any)
			}
			
			// Need to shift anything still in the buffer over to beginning.
			char *leftover = &c->buf[c->curpos];
			memmove(c->buf, leftover, c->curpos);
			c->curpos = 0;
		}
		return i;
	}
}

int main(int argc, char* argv[]) {
	//code based on selectbuffer
	int i, maxi, maxfd, listenfd, connfd;
	int nready; 
	Client client[MAXCLIENTS];
	fd_set  rset, allset;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	/* Initialize the list head */
    Group *group_list = NULL;

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(PORT);
	
	//release port when server process terminates
	int on = 1;
	if((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on))) == -1) {
		perror("setsockopt");
	}

	Bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	Listen(listenfd, LISTENQ);

	maxfd = listenfd;   /* initialize */
	maxi = -1;      /* index into client[] array */
	for (i = 0; i < MAXCLIENTS; i++) {
		client[i].soc = -1; /* -1 indicates available entry */
		client[i].curpos = 0;
		client[i].name[0] = '\0';
	}

	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	
	char *first_prompt = "What is your name?\r\n";
	while(1) {
		rset = allset; //structure assignment
		nready = Select(maxfd+1, &rset, NULL, NULL, NULL);
		
		if (FD_ISSET(listenfd, &rset)) {    /* new client connection */
			clilen = sizeof(cliaddr);
			connfd = Accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);

			for (i = 0; i < MAXCLIENTS; i++) {
				if (client[i].soc < 0) {
					client[i].soc = connfd; /* save descriptor */
					//Send the first message to the user (ask what their name is)
					if (write(client[i].soc, first_prompt, strlen(first_prompt) + 1) == -1) {
						perror("write");
					}
					break;
				}
			}

			FD_SET(connfd, &allset);    /* add new descriptor to set */
			if (connfd > maxfd)
				maxfd = connfd; /* for select */
			if (i > maxi)
				maxi = i;   /* max index in client[] array */

			if (--nready <= 0)
				continue;   /* no more readable descriptors */
		}
		
		for (i = 0; i <= maxi; i++) {   /* check all clients for data */
			if ( client[i].soc < 0)
				continue;
			if (FD_ISSET(client[i].soc, &rset)) {
				int result = readfromclient(&client[i], &group_list, client);
				
				if(result == -1)  {
					Close(client[i].soc);
					FD_CLR(client[i].soc, &allset);
					client[i].soc = -1;
					client[i].name[0] = '\0';
				}
				if (--nready <= 0)
					break;  /* no more readable descriptors */
			}
		}
	}
	return 0;
}
