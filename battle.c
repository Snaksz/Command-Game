
/*
 * socket demonstrations:
 * This is the server side of an "internet domain" socket connection, for
 * communicating over the network.
 *
 * In this case we are willing to wait for chatter from the client
 * _or_ for a new connection.
*/

#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>


// Bonus adding in the timeout module if we want to timeout
#include <time.h>

// simpleselect.c helper functions
static struct client *addclient(struct client *top, int fd, struct in_addr addr);
static struct client *removeclient(struct client *top, int fd);
static void broadcast(struct client *top, char *s, int size);
int handleclient(struct client *p, struct client *top);
int bindandlisten(void);


// our function signatures to avoid implicit declaration warnings
int GetNewOpponent(struct client* idle_client, struct client* clientlinkedlist);



#ifndef PORT
    #define PORT 94728
#endif

struct client {
    int fd;
    struct in_addr ipaddr;
    struct client *next;
    int special_moves; // how many speial moves they got
    int hp; // how many health points they have
    int activity;//-1:waiting, 1:attacking, 0:defending
    char *name; // Client's Name Needs to be stored here
    char name_size[256];
    struct client *last_played;
    struct client *current_opponent;
};


int main(void) {
    // Helper to debug makefile
	// printf("Port number: %d\n", PORT);
    // 	return 0;

    // See the response of the server
    int response = bindandlisten();
    return 0;

}

// ***************************************
// Helper functions given to us in simpleselect.c
// ***************************************

int handleclient(struct client *p, struct client *top) {
    char buf[256];
    char outbuf[512];
    int len = read(p->fd, buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        printf("Received %d bytes: %s", len, buf);
        sprintf(outbuf, "%s says: %s", inet_ntoa(p->ipaddr), buf);
        broadcast(top, outbuf, strlen(outbuf));
        return 0;
    } else if (len <= 0) {
        // socket is closed
        printf("Disconnect from %s\n", inet_ntoa(p->ipaddr));
        sprintf(outbuf, "Goodbye %s\r\n", inet_ntoa(p->ipaddr));
        broadcast(top, outbuf, strlen(outbuf));
        return -1;
    }

    // should never reach here
    return -1;
}

 /* bind and listen, abort on error
  * returns FD of listening socket
  */
int bindandlisten(void) {
    struct sockaddr_in r;
    int listenfd;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    int yes = 1;
    if ((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == -1) {
        perror("setsockopt");
    }
    memset(&r, '\0', sizeof(r));
    r.sin_family = AF_INET;
    r.sin_addr.s_addr = INADDR_ANY;
    r.sin_port = htons(PORT);

    if (bind(listenfd, (struct sockaddr *)&r, sizeof r)) {
        perror("bind");
        exit(1);
    }

    if (listen(listenfd, 5)) {
        perror("listen");
        exit(1);
    }
    return listenfd;
}

static struct client *addclient(struct client *top, int fd, struct in_addr addr) {
    struct client *p = malloc(sizeof(struct client));
    if (!p) {
        perror("malloc");
        exit(1);
    }

    printf("Adding client %s\n", inet_ntoa(addr));

    p->fd = fd;
    p->ipaddr = addr;
    p->next = top;
    top = p;
    return top;
}

static struct client *removeclient(struct client *top, int fd) {
    struct client **p;

    for (p = &top; *p && (*p)->fd != fd; p = &(*p)->next)
        ;
    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        struct client *t = (*p)->next;
        printf("Removing client %d %s\n", fd, inet_ntoa((*p)->ipaddr));
        free(*p);
        *p = t;
    } else {
        fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n",
                 fd);
    }
    return top;
}


static void broadcast(struct client *top, char *s, int size) {
    struct client *p;
    for (p = top; p; p = p->next) {
        write(p->fd, s, size);
    }
    /* should probably check write() return value and perhaps remove client */
}

// lab 10 helper
/*
 * Search the first inbuf characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
// we shouldn't treat as string as it stops when we have null terminator. In communications it
// is okay to have null terminaotr inbetween strings, so we could loose data if we use strchr.
// ex. "message" = _____|\0|message|____
int find_network_newline(const char *buf, int inbuf) {
    for (int i=0; i < inbuf - 1; i++){
        if (buf[i] == '\r' && buf[i+1] == '\n'){
            return i + 2;
        }
    }
    return -1;}


// Login Linked List Implmentation Stuff
/* 
When a new client arrives, add them to the end of the active clients list
- Adds them to the end of the Linked List of all clients
- Ask for and store their name
- Tell the new client that they are waiting on a opponent
- Tell everyone else that someone new has entered the arena
- Scan the connected clients; if a suitable client is available, 
start a match with the newly-connected client.
*/
void Login(struct client * new_client, struct client *clientlinkedlist){
    // Check if the new_client has a name
    if (new_client->name == NULL) {
        int length;
        // If they don't have a name, prompt it and save it
        // Read the name into a buffer
        int capacity = sizeof(new_client->name_size);
        // Need to use find network newline to be able to process the name
        int position;
        int size = 0;
        char *current_char;
        current_char = new_client->name_size;
        // Read from client 
        while ((length = read(new_client->fd, current_char, sizeof(new_client->name_size) - 1)) > 0) {
            size += length;
            position = find_network_newline(new_client->name_size, size);
            if (position >= 0) {
                new_client->name_size[position] = '\0';
                new_client->name_size[position + 1] = '\0';
                size -= position + 2;
                break;
            }
            capacity = sizeof(new_client->name_size) - size;
            current_char = new_client->name_size + size;
        }
    } new_client->name = new_client->name_size;

    // Traverse the ClientLinkedList and Broadcast that the newname has entered the arena!
    // When you hit the new person; Say Welcome ____ Awaiting Opponent!

    // Assume that the client linked list does not yet have the new_client in it!
    struct client * current_client = clientlinkedlist;

    if (current_client == NULL) {
        clientlinkedlist = new_client;
        // Initialize head of Linked list
    }
    else {
        // Loop over every client until we reach the end of the linked list
        while (current_client->next != NULL) {
            // Write to the client that a new person has joined!

            // How to do that? 
            // Use client's fd??? Maybe ask Taras.

            current_client = current_client->next;
            // To iterate over the linked list.
        }
    
        // Now that you've made it to here, you've got to insert the new_client
        // You do need to initialize the client linked list somewhere! and store Null if its the first thing
        // Also need to check this case!
        current_client->next = new_client;
    
        // Now write that second specific message to this client only, i.e. write that Welcome (name) Awaiting Opponent!


        // Not sure if we should just keep this in a while loop, since it might block the server
        // from doing other things like finishing matches or adding new clients

        int found_match;
        // Could possibly call this whenever a match finishes or new client joins
        while ((found_match = GetNewOpponent(current_client, clientlinkedlist)) != 1) {
            continue;
        }


    }
}


int GetNewOpponent(struct client* idle_client, struct client* clientlinkedlist) {

    // Returns 1 if paired with a new opponent and 0 if not
    
    // If this function is called, idle_client should not be in a match

    // Not sure if we will set current to NULL after a match finishes
    // Can be changed if necessary

    if (idle_client->activity == -1) {
        
        struct client* curr_client = clientlinkedlist;

        // clientlinkedlist is not empty, since we must have someone
        // connceted to the server to begin pairing them


        // Iterate through the linked list
        while (curr_client != NULL) {
            // Check for a client that is:
            //      waiting
            //      someone we did not just play
            //      and who did not just play us

            //      if we fought them last, but they fought someone else last, then the match is valid

            if (curr_client->activity == -1 && idle_client->last_played != curr_client && curr_client->last_played != idle_client) {
                
                // Pair the clients together
                idle_client->current_opponent = curr_client;
                curr_client->current_opponent = idle_client;

    
                // Need to refresh their skills/attacks and hp either here or in the start of the game loop

                // Could start match here, depending on code structure

                return 1;

            }

            else {
                curr_client = curr_client->next;
            }

        }
    }

    // Could not match with anyone available
    // or because we are in a match
    return 0;

}