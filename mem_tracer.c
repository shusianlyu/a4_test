/**
 * file: mem_tracer.c
 * author: Shu Sian (Jessie) Lyu
 * description: program that store the
 * command lines in a dynamically allocated array of type char **
 * and trace the memory usage.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * TRACE_NODE_STRUCT is a linked list of
 * pointers to function identifiers
 * TRACE_TOP is the head of the list is the top of the stack
**/
struct TRACE_NODE_STRUCT {
    char* functionid;                // ptr to function identifier (a function name)
    struct TRACE_NODE_STRUCT* next;  // ptr to next frama
};
typedef struct TRACE_NODE_STRUCT TRACE_NODE;
static TRACE_NODE* TRACE_TOP = NULL;       // ptr to the top of the stack

/**
 * CMD_NODE_STRUCT is a linked list of char array
 * CMD_TOP is the head of the list.
**/
struct CMD_NODE_STRUCT {
    char cmd[100];                       // char array to store each command
    int line;                            // lineIndex of the command
    struct CMD_NODE_STRUCT* next;        // ptr to next cmd
};
typedef struct CMD_NODE_STRUCT CMD_NODE;
static CMD_NODE* CMD_TOP = NULL;         // ptr to the top of the list

/* function PUSH_TRACE */
/*
 * The purpose of this stack is to trace the sequence of function calls,
 * just like the stack in your computer would do.
 * The "global" string denotes the start of the function call trace.
 * The char *p parameter is the name of the new function that is added to the call
trace.
 * See the examples of calling PUSH_TRACE and POP_TRACE below
 * in the main, make_extend_array, add_column functions.
**/
void PUSH_TRACE(char* p)          // push p on the stack
{
    TRACE_NODE* tnode;
    static char glob[] = "global";
    if (TRACE_TOP == NULL) {
        // initialize the stack with "global" identifier
        TRACE_TOP = (TRACE_NODE*) malloc(sizeof(TRACE_NODE));
        // no recovery needed if allocation failed, this is only
        // used in debugging, not in production
        if (TRACE_TOP==NULL) {
            printf("PUSH_TRACE: memory allocation error\n");
            exit(1);
        }
        TRACE_TOP->functionid = glob;
        TRACE_TOP->next=NULL;
    } // end of first if

    // create the node for p
    tnode = (TRACE_NODE*) malloc(sizeof(TRACE_NODE));
    // no recovery needed if allocation failed, this is only
    // used in debugging, not in production
    if (tnode == NULL) {
        printf("PUSH_TRACE: memory allocation error\n");
        exit(1);
    } // end of second if
    tnode->functionid = p;
    tnode->next = TRACE_TOP;  // insert tnode as the first in the list
    TRACE_TOP = tnode;          // point TRACE_TOP to the first node
} /* end PUSH_TRACE */

/* function POP_TRACE */
/* Pop a function call from the stack */
void POP_TRACE()    // remove the op of the stack
{
    TRACE_NODE* tnode;
    tnode = TRACE_TOP;
    TRACE_TOP = tnode->next;
    free(tnode);
} /* end POP_TRACE */

/* function PRINT_TRACE prints out the sequence of function
 * calls that are on the stack at this instance */
/* For example: funcC:funcB:funcA:global */
char* PRINT_TRACE()
{
    int depth = 50;              // A max of 50 levels in the stack will be combined in a string for printing out.
    int i, length, j;
    TRACE_NODE* tnode;
    static char buf[100];
    if (TRACE_TOP == NULL) {     // stack not initialized yet, so we are
        strcpy(buf,"global");    // still in the `global' area
        return buf;
    }
    /* peek at the depth(50) top entries on the stack, but do not
       go over 100 chars and do not go over the bottom of the
       stack */
    sprintf(buf,"%s",TRACE_TOP->functionid);
    length = strlen(buf);                     // length of the string so far
    for(i = 1, tnode = TRACE_TOP->next; tnode!=NULL && i < depth; i++, tnode = tnode->next) {
        j = strlen(tnode->functionid);        // length of what we want to add
        if (length + j + 1 < 100) {              // total length is ok
            sprintf(buf+length, ":%s", tnode->functionid);
            length += j + 1;
        }else                                    // it would be too long
            break;
    }
    return buf;
} /*end PRINT_TRACE*/

/** function REALLOC calls realloc and
 * it trace the memory consumption.
 */
void* REALLOC(void* p, int t, char* file, int line)
{
    p = realloc(p,t);
    printf("File %s, line %d, function=%s reallocated the memory segment at address %p to a new size %d\n", file, line, PRINT_TRACE(), p, t);
    return p;
} /* end REALLOC */

/** function MALLOC calls malloc and
 * it trace the memory consumption.
 */
void* MALLOC(int t, char* file, int line)
{
    void* p;
    p = malloc(t);
    printf("File %s, line %d, function=%s allocated new memory segment at address %p to size %d\n", file, line, PRINT_TRACE(), p, t);
    return p;
} /* end MALLOC */

/** function FREE calls free and
 * it trace the memory consumption.
 */
void FREE(void* p, char* file, int line)
{
    printf("File %s, line %d, function=%s deallocated the memory segment at address %p\n", file, line, PRINT_TRACE(), p);
    free(p);
} /* end FREE */

#define realloc(a,b) REALLOC(a,b,__FILE__,__LINE__)
#define malloc(a) MALLOC(a,__FILE__,__LINE__)
#define free(a) FREE(a,__FILE__,__LINE__)

/** function insert_node inserts each command line
 * with their command index into a linked list
 */
void insert_node(char* buf, int cmdIndex)      // insert command to the list with line count
{
    PUSH_TRACE("insert_node");
    if (CMD_TOP == NULL) {
        // initialize the stack with cmd
        CMD_TOP = (CMD_NODE*) malloc(sizeof(CMD_NODE));
        // no recovery needed if allocation failed, this is only
        // used in debugging, not in production
        if (CMD_TOP == NULL) {
            printf("insert_node: memory allocation error\n");
            exit(1);
        }
        strcpy(CMD_TOP->cmd, buf);
        CMD_TOP->line = cmdIndex;
        CMD_TOP->next = NULL;
    } // end of first if
    else{
        // create the node for buf
        CMD_NODE *tnode;
        tnode = (CMD_NODE*) malloc(sizeof(CMD_NODE));
        CMD_NODE *curNode = CMD_TOP;

        // no recovery needed if allocation failed, this is only
        // used in debugging, not in production
        if (tnode == NULL) {
            printf("insert_node: memory allocation error\n");
            exit(1);
        } // end of second if

        while(curNode->next != NULL){
            curNode = curNode->next;
        }
        strcpy(tnode->cmd, buf);
        tnode->line = cmdIndex;
        tnode->next = NULL;  // insert tnode at the tail
        curNode->next = tnode;
    }
    POP_TRACE();
    return;
} /* end insert_node */

/** function print_nodes display each command line
 * with their command index in the linked list
 */
void print_nodes(){

    PUSH_TRACE("print_nodes");
    CMD_NODE *curNode;
    if (CMD_TOP == NULL){
        printf("List is empty.\n");
        return;
    }
    printf("Elements of list are:\n");
    curNode = CMD_TOP;
    while(curNode != NULL){
        printf("LinkedList[%d] Command: %s\n", curNode->line, curNode->cmd);
        curNode = curNode->next;
    }
    POP_TRACE();
    return;
} /* end print_nodes */

/** function free_list call free
 * to free each node in the linked list
 */
void free_list(CMD_NODE *head){
    PUSH_TRACE("free_list");
    CMD_NODE *cur;
    while (head != NULL){
        cur = head;
        head = head->next;
        free(cur);
    }
    POP_TRACE();
    return;
} /* end free_list */

int main() {
    PUSH_TRACE("main");
    char *cmd;                // char array
    char **cmdsPtr;           // dynamically allocated array of type char **
    int numOfCmds = 10;       // Start with 10 commands
    size_t nbytes = 100;
    int cmdIndex = 0;         // holds line index

    int outFile = open("memtrace.out",  O_RDWR | O_CREAT | O_APPEND, 0777);
    if (outFile < 0){
        printf("Error opening file\n");
    }
    dup2(outFile, 1);         // redirecting stdout to memtrace.out file

    cmd = (char *) malloc(nbytes + 1);
    cmdsPtr = malloc(sizeof(char*) * numOfCmds);    // an initial size of 10 char * pointers.
    if (cmdsPtr == NULL){
        perror("Unable to allocate buffer");
        exit(1);
    }

    while (getline(&cmd, &nbytes,stdin) != -1){
        if (cmd[strlen(cmd) - 1] == '\n'){
            cmd[strlen(cmd) - 1] = 0;      // replace newline with null
        }

        if(cmdIndex == numOfCmds){          // check if the initial size turns out not to be big enough
            numOfCmds *= 2;                 // expand to double the current size
            cmdsPtr = realloc(cmdsPtr, sizeof(char*) * numOfCmds); // reallocate memory.
        }
        cmdsPtr[cmdIndex++] = cmd;          // store into dynamically allocated array
        insert_node(cmd, cmdIndex);    // insert each command into linked list
    }

    print_nodes();                     // display nodes in the list

    free_list(CMD_TOP);          // free linked list memory
    free(cmd);                         // free cmd memory
    free(cmdsPtr);                     // free cmdsPtr memory

    POP_TRACE();

    return(0);
}