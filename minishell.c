#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define INP_SIZE_MAX    1024
#define ARGS_MAX 100
char inp[INP_SIZE_MAX];

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


int main(void)
{
    int child_status = 0;
    pid_t pid;
    memset(inp,0,sizeof(inp));    
    while(1)    {
        printf("msh>");
        fgets(inp, INP_SIZE_MAX, stdin);
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
