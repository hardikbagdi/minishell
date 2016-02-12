#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define INP_SIZE_MAX    1024 //max chars in a cmd
#define ARGS_MAX 10	// max args per cmd
#define CHILD_PER_CMD_MAX 100	//max number of cmds in a pipe

char inp[INP_SIZE_MAX]; //cmd line
char *inp_tokens[CHILD_PER_CMD_MAX * ARGS_MAX]; // token list of entire command line
struct child_process {
	char cmd_start_index; // index of the first token for this cmd in cmd line
	char cmd_len;	//number of command line arguments for this process( including cmd name)
	pid_t pid;	//pid of this child
} child_list[CHILD_PER_CMD_MAX];	// a cmd line can have at most
int num_childs;	// total number of child processes in this command 

int split_str( char *line, unsigned int line_len, char **tok_list, unsigned int tok_list_len)
{
    int i = 0;
    char *pos;
    if( (line == NULL) || (line_len == 0) || (tok_list == NULL) || (tok_list_len == 0)) {
        printf(" error parsing tokens\n");
        return -1;
    }
    if ((pos=strchr(line, '\n')) != NULL)
        *pos = '\0';
    tok_list[i] = strtok(line," ");

    while( (i < tok_list_len) &&  (tok_list[i] != NULL))  {
            tok_list[++i] = strtok(NULL, " ");
    }
            
}                

int run_child( char *cmd, int len)
{
    char *argv[ARGS_MAX];
    memset(argv, 0, sizeof(argv));
    split_str( cmd, len, argv, ARGS_MAX); 
    //printf("running :%s:\n", argv[0]);
    if( execvp(argv[0], argv) < 0 )  {
    //if( execv("/bin/date", argv) < 0 )  {
        printf("Error %s for cmd %s\n", strerror(errno), argv[0]);
    }
    return 0;
}

int fill_child_list( void )
{
    int i = 0;
    int j = 0;
    child_list[j].cmd_start_index = 0;
    child_list[j].cmd_len = 1;
    for( i = 1; inp_tokens[i] != NULL; i++) {
        if( *inp_tokens[i] == '|')  {
            j++;
            child_list[j].cmd_start_index = i + 1;
        }else   {
            child_list[j].cmd_len++;
        }
    }
    num_childs = j+1;
    return 0;	
}

int main(void)
{
    int child_status = 0;
    pid_t pid;
    memset(inp,0,sizeof(inp));    
    while(1)    {
        printf("msh>");
        fgets(inp, INP_SIZE_MAX, stdin);
       	//tokanise the cmd line input
    	split_str( inp, sizeof(inp), inp_tokens, sizeof(inp_tokens)); 
	    //fillout child process array
	    fill_child_list();	
	    pid = fork();
        if( pid < 0 )   {
            printf(" ERROR creating child\n");
            exit(0);
        }
        if( pid == 0 )   {
            run_child(inp, INP_SIZE_MAX);
        }
        else    {   //this is parent
            //printf("parent waiting\n");
            waitpid(pid, &child_status, WUNTRACED | WCONTINUED);
            //printf("child done\n");
        }
        //printf("%s",inp);
    }
	return 0;
}
