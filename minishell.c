#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "minishell.h"

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

pid_t shell_pid;    //Pid of shell

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
        printf("found token :%s:\n", inp_tokens[i]);
        if( *inp_tokens[i] == '|')  {
            j++;
            child_list[j].cmd_start_index = i + 1;
           // child_list[j].cmd_len = 1;
        }else   {
            child_list[j].cmd_len++;
        }
    }
    num_childs = j+1;
    printf("total number of childs = %d\n",num_childs);
    return 0;	
}

int current_child = -1;  //index to the child_list[]

int run_exec(void)
{
    char *argv[ARGS_MAX];
    int i = 0, j = 0;
    int in_fd;
    int out_fd;
    char* in_file = NULL;
    char* out_file = NULL;
   printf(" exec for child %d\n", current_child); 
    //printf(" exec for child --"); 
    memset(argv, 0 , sizeof(argv)); //check this 
    printf("exec for child %d, cmd_start_index = %d, cmd len = %d\n", current_child, child_list[current_child].cmd_start_index, child_list[current_child].cmd_len);
    for( i = child_list[current_child].cmd_start_index; i < ( child_list[current_child].cmd_start_index + child_list[current_child].cmd_len); i++)  {
    //for( i = child_list[current_child].cmd_start_index; inp_tokens[i] != NULL;  i++)  {
        if(*inp_tokens[i] == '>')    {
            out_file = inp_tokens[i + 1];
            i++;    //skip the next token because it is a fine name
        }   
        else if(*inp_tokens[i] == '<')   { 
            in_file = inp_tokens[i + 1];   
            i++;    //skip the next token because it is a fine name
        }
        else    {
            argv[j++] = inp_tokens[i];
            printf("inp_tokens[%d] = %s \n",i,  inp_tokens[i]);
        }
        printf("1HERE \n");
    }
    printf("HERE \n");
    if( in_file != NULL )   {
       if( open(in_file, O_RDONLY ) < 0 )   {
            printf("Error %s for cmd %s, file %s\n", strerror(errno), argv[0], in_file);
            return 1;   
       }
       if( dup2(in_fd, STDIN_FILENO) < 0)   {
            printf("Error in dup %s for cmd %s, file %s\n", strerror(errno), argv[0], in_file);
            return 1;   
       }
       close(in_fd);
    }

    if( out_file != NULL )   {
       if( open(out_file, O_CREAT|O_WRONLY|O_TRUNC , 0600) < 0 )   {
            printf("Error %s for cmd %s, file %s\n", strerror(errno), argv[0], out_file);
            return 1;   
       }
       
       fsync(1);
        if( dup2(out_fd, 1) < 0)   {
            printf("Error in dup %s for cmd %s, file %s\n", strerror(errno), argv[0], in_file);
            return 1;   
       }
       fsync(out_fd);
       close(out_fd);
    }

    printf("Running:\n");
    int k = 0;
    while( argv[k] != NULL ){
        printf("%s ", argv[k]);
        k++;
    }
    printf("\n");

    if( execvp(argv[0], argv) < 0 )  {
        printf("Error %s for cmd %s\n", strerror(errno), argv[0]);
    }
    return 0;
}
int spawn_child( void )
{
    pid_t pid;

//    current_child++;
    printf("current child = %d\n", current_child);
    pid = fork();
    if( pid < 0 )   {
        printf("Error1 %s \n", strerror(errno));
        exit(1);
    }
    if( pid == 0 )  {   //child
        current_child++;
        if( current_child < (num_childs - 1))   { // there are more commands in pipe, spawn recursively
            printf("spawning recursive child. cur child = %d\n", current_child);
          // current_child++;
            spawn_child();
        } else{             // last cmd in pipe
            printf("last command, %d\n", current_child);
            run_exec();
        }
    }
    else    {   //parent
        if( this_is_shell())    {
            wait(NULL); //shell does not exec itself
        }
        else    {
           // sleep(1);
            printf("runnig exec for child  %d\n", current_child);
            run_exec(); // this is a child
        }
    }
    return 0;
}

int this_is_shell( void )
{
    pid_t this_pid = 0;
    this_pid = getpid();
    if( this_pid == shell_pid )
        return TRUE;
    else
        return FALSE;
}
int main(void)
{
    int child_status = 0;
    pid_t pid;
    memset(inp,0,sizeof(inp));    
    //record the pid of shell before spawining children
    shell_pid = getpid();
    while(1)    {
        printf("msh>");
        fgets(inp, INP_SIZE_MAX, stdin);
       	//tokanise the cmd line input
    	split_str( inp, sizeof(inp), inp_tokens, sizeof(inp_tokens)); 
	    //fillout child process array
	    fill_child_list();
        spawn_child();   
        memset( inp_tokens, 0, sizeof(inp_tokens));
        memset( inp, 0, sizeof(inp));

        memset( child_list, 0, sizeof(child_list));
        num_childs = 0;

        current_child = -1;  //index to the child_list[]
/*
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
*/
        //printf("%s",inp);
    }
	return 0;
}
