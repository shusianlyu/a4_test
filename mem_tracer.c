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

struct CMD_NODE_STRUCT {
    char cmd[100];                       // pointer to each line
    int line;                        // lineCount
    struct CMD_NODE_STRUCT* next;    // ptr to next cmd
};
typedef struct CMD_NODE_STRUCT CMD_NODE;
static CMD_NODE* CMD_TOP = NULL;       // ptr to the top of the stack

/**
 *
 * @param p
 */
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
} /*end PUSH_TRACE*/

void POP_TRACE()    // remove the op of the stack
{
    TRACE_NODE* tnode;
    tnode = TRACE_TOP;
    TRACE_TOP = tnode->next;
    free(tnode);
} /*end POP_TRACE*/

char* PRINT_TRACE()
{
    int depth = 50; // A max of 50 levels in the stack will be combined in a string for printing out.
    int i, length, j;
    TRACE_NODE* tnode;
    static char buf[100];
    if (TRACE_TOP == NULL) {     // stack not initialized yet, so we are
        strcpy(buf,"global");   // still in the `global' area
        return buf;
    }
    /* peek at the depth(50) top entries on the stack, but do not
       go over 100 chars and do not go over the bottom of the
       stack */
    sprintf(buf,"%s",TRACE_TOP->functionid);
    length = strlen(buf);                  // length of the string so far
    for(i = 1, tnode = TRACE_TOP->next; tnode!=NULL && i < depth; i++, tnode = tnode->next) {
        j = strlen(tnode->functionid);             // length of what we want to add
        if (length + j + 1 < 100) {              // total length is ok
            sprintf(buf+length, ":%s", tnode->functionid);
            length += j + 1;
        }else                                // it would be too long
            break;
    }
    return buf;
} /*end PRINT_TRACE*/

// For instance, example of print out:
// "File tracemem.c, line X, function F reallocated the memory segment at address A to a new size S"
// Information about the function F should be printed by printing the stack (use PRINT_TRACE)
void* REALLOC(void* p, int t, char* file, int line)
{
    p = realloc(p,t);
    printf("File %s, line %d, function=%s reallocated the memory segment at address %p to a new size %d\n", file, line, PRINT_TRACE(), p, t);
    return p;
}

// For instance, example of print out:
// "File tracemem.c, line X, function F allocated new memory segment at address A to size S"
// Information about the function F should be printed by printing the stack (use PRINT_TRACE)
void* MALLOC(int t, char* file, int line)
{
    void* p;
    p = malloc(t);
    printf("File %s, line %d, function=%s allocated new memory segment at address %p to size %d\n", file, line, PRINT_TRACE(), p, t);
    return p;
}

// For instance, example of print out:
// "File tracemem.c, line X, function F deallocated the memory segment at address A"
// Information about the function F should be printed by printing the stack (use PRINT_TRACE)
void FREE(void* p, char* file, int line)
{
    printf("File %s, line %d, function=%s deallocated the memory segment at address %p\n", file, line, PRINT_TRACE(), p);
    free(p);
}

#define realloc(a,b) REALLOC(a,b,__FILE__,__LINE__)
#define malloc(a) MALLOC(a,__FILE__,__LINE__)
#define free(a) FREE(a,__FILE__,__LINE__)

// -----------------------------------------
// function add_column will add an extra column to a 2d array of ints.
// This function is intended to demonstrate how memory usage tracing of realloc is done
// Returns the number of new columns (updated)
int add_column(int** array,int rows,int columns)
{
    PUSH_TRACE("add_column");
    int i;
    for(i=0; i<rows; i++) {
        array[i]=(int*) realloc(array[i],sizeof(int)*(columns+1));
        array[i][columns]=10*i+columns;
    }//for
    POP_TRACE();
    return (columns+1);
} // end add_column

// ------------------------------------------
// function make_extend_array
// Example of how the memory trace is done
// This function is intended to demonstrate how memory usage tracing of malloc and free is done
void make_extend_array()
{
    PUSH_TRACE("make_extend_array");
    int i, j;
    int **array;
    int ROW = 4;
    int COL = 3;
    //make array
    array = (int**) malloc(sizeof(int*) * 4);  // 4 rows
    for(i = 0; i < ROW; i++) {
        array[i] = (int*) malloc(sizeof(int) * 3);  // 3 columns
        for(j = 0; j < COL; j++)
            array[i][j] = 10*i+j;
    } // end of for

    //display array
    for(i = 0; i < ROW; i++)
        for(j = 0; j < COL; j++)
            printf("array[%d][%d]=%d\n", i, j, array[i][j]);
    // and a new column
    int NEWCOL = add_column(array,ROW,COL);
    // now display the array again
    for(i = 0; i < ROW; i++)
        for(j = 0; j < NEWCOL; j++)
            printf("array[%d][%d]=%d\n", i, j, array[i][j]);
    //now deallocate it
    for(i = 0; i < ROW; i++)
        free((void*)array[i]);
    free((void*)array);
    POP_TRACE();
    return;
} //end make_extend_array

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
} /*end insert_node */

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
        printf("%s at line %d\n", curNode->cmd, curNode->line);
        curNode = curNode->next;
    }
    POP_TRACE();
    return;
}

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
}

int main() {

    char *cmd;              // ?
    char **cmdsPtr;         // dynamically allocated array of type char **
    int numOfCmds = 10;           // Start with 10 commands
    size_t nbytes = 100;
    int cmdIndex = 0;       // holds line index

    int outFile = open("memtrace.out",  O_RDWR | O_CREAT | O_APPEND, 0777);
    if (outFile < 0){
        printf("Error opening file\n");
    }
    dup2(outFile, 1);       // redirecting stdout to memtrace.out file

    cmd = (char *) malloc(nbytes + 1);
    cmdsPtr = malloc(sizeof(char*) * numOfCmds);   // an initial size of 10 char * pointers.
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
            cmdsPtr = realloc(cmdsPtr, sizeof(char*) * numOfCmds); // re allocate memory.
        }
        cmdsPtr[cmdIndex++] = cmd;
        insert_node(cmd, cmdIndex);
    }

    print_nodes();
    //make_extend_array();
    free_list(CMD_TOP);

    free(cmd);
    free(cmdsPtr);


    return(0);

}