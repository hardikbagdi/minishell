#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#define INP_SIZE_MAX    1024
#define ARGS_MAX 100

//bash commands
char *exit_cmd  ="exit";
char *jobs_cmd  ="jobs";
char *cd_cmd  ="cd";
char *fg_cmd  ="fg";
//handling termination
pid_t last_started_process = 0;
char *inp_backup;
static char *background_operator = "&";
int is_background = 0;

char inp[INP_SIZE_MAX];
pid_t pid = 0; // pid of the current foreground child process

//DS to hold jobs, implemented as LinkedList
typedef struct Job{
    pid_t pid;
    char* cmd;
    struct Job* next;
}Job;
//jobs linked list
Job *jobs;
int no_of_jobs = 0;
Job* createJob(char* cmd, pid_t pid){
    Job* job = calloc(sizeof(Job),1);
    char* pos;
    char* cmd_copy = calloc(strlen(cmd),1);
    if(job == NULL || cmd_copy == NULL){
        printf("Out of Memory\n");
        fflush(stdout);
        exit(0);
    }
    memcpy(cmd_copy,cmd,strlen(cmd));
    if ((pos=strchr(cmd_copy, '\n')) != NULL)
        *pos = '\0';

    job->pid = pid;
    job->cmd = cmd_copy;
    job->next = NULL;
    return job;
}
int addjob(Job* job){
    no_of_jobs++;
    if(jobs== NULL){
        jobs=job;
        return 0;
    }
    Job* curr = jobs;
    while(curr->next != NULL){
        curr = curr->next;
    }
    curr->next = job;
    return 0;
}
//wrapper to hold background jobs
void put_into_background(char* inp_backup, pid_t pid){
    Job *job = createJob(inp_backup,pid);
    addjob(job);
}
//search by pid if exist. NULL if doesn't exists
Job* search(pid_t pid_to_search){
    Job* curr = jobs;
    while(curr!= NULL && curr->pid!=pid_to_search){
        curr = curr->next;
    }
    return curr;
}
//get background at an index
Job* search_by_index(int index){
    Job* job = jobs;
    while(index-- > 1)
        job = job->next;
    return job;

}
int removejob(pid_t pid_to_remove){
    if(search(pid_to_remove) == NULL)
        return 0;
    Job* curr = jobs;
    if(curr->pid == pid_to_remove){
        Job* temp = jobs;
        jobs = jobs->next;
        --no_of_jobs;
        free(temp);
        return 0;
    }
    Job* prev = jobs;
    curr = jobs->next;
    while(curr!= NULL && curr->pid != pid_to_remove){
        curr = curr->next;
        prev = prev->next;
    }
    if(curr== NULL){
        return 0;
    }
    prev->next = curr->next;
    --no_of_jobs;
    free(curr);
    return 0;

}
//removes zombie process from background jobs
void remove_dead_jobs(){
    Job* curr = jobs;
    pid_t pid;
    Job* temp= NULL;
    while(curr!=NULL){
        temp = curr->next;
        pid = waitpid(curr->pid,NULL, WNOHANG);
        if(pid == curr->pid){
            removejob(curr->pid);
        }
        curr = temp;
    }
}
//print used for 'jobs' command
void print_jobs(){
    remove_dead_jobs();
    if(jobs==NULL){
        printf("No background jobs\n");
        fflush(stdout);
        return;
    }
    int i=1;
    Job* curr = jobs;
    while(curr != NULL){
        printf("[%d] Process: %d\t%s\n",i++,curr->pid,curr->cmd );
        curr = curr->next;
    }
    fflush(stdout);
    return;
}

//find length of a command
int length(char** argv){
	int len = 0;
	while(argv[len] != NULL){
		len++;
	}
	return len;
}

int make_process_background(char* cmd, pid_t pid){
    put_into_background(cmd,pid);
    printf("Background process: [%d] %s\n",pid,cmd);
    return 0;
}
int remove_background_operator(char** argv){
    int len  = length(argv);
    argv[len-1] =NULL;
    return 0;
}
int check_if_background(char* inp){
    char *argv[ARGS_MAX];
    char *inp_copy = calloc(strlen(inp),1);
    memcpy(inp_copy,inp,strlen(inp));
    split_str( inp_copy, INP_SIZE_MAX, argv, ARGS_MAX); 
    int len = length(argv);
    if(strcmp(argv[len-1],background_operator)==0){
        // printf("background requested yes\n");fflush(stdout);
       return 1;
   }
   return 0;
}
//end hardik

void wait_for_child(int pid){
    int child_status;
    pid_t pid_returned;
        tcsetpgrp (STDIN_FILENO,pid);
       //  printf("parent  wait\n");
         pid_returned = waitpid(pid, &child_status, WUNTRACED );
         //printf("waitpid: %d\n",pid_returned);
         if(pid == pid_returned && WIFSTOPPED(child_status)){
                // printf("Current process stopped \n");fflush(stdout);
            if(search(pid)==NULL)
                put_into_background(inp_backup,pid_returned);
         }
         else if(pid == pid_returned && WIFEXITED(child_status)){
            if(search(pid_returned))
                removejob(pid_returned);
                // printf("Current process terminated normally \n");fflush(stdout);
         }
         else if(pid == pid_returned && WIFSIGNALED(child_status)){
                // printf("Current process terminated on signal \n");fflush(stdout);
                // tcsetpgrp (STDIN_FILENO, getpid());
            if(search(pid_returned))
                removejob(pid_returned);

         }
}
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
        // printf("CMD: %s\n",tok_list[i] );
        tok_list[++i] = strtok(NULL, " ");
    }
    return i;        
}                



int run_child( char *cmd, int len)
{
    char *argv[ARGS_MAX];
    int i=0;
    memset(argv, 0, sizeof(argv));
    i =  split_str( cmd, len, argv, ARGS_MAX); 
    if(is_background){
        remove_background_operator(argv);
    }

    if( execvp(argv[0], argv) < 0 )  {
    //if( execv("/bin/date", argv) < 0 )  {
        printf("Error %s for cmd %s\n", strerror(errno), argv[0]);
    }
    exit(0); //added to safely terminate the process when unknown command is given 
    return 0;
}
void execute_cd(char* directory){
    char* pos;
    if(directory== NULL){
        printf("Please specify a directory.\n");
    }
    else{
        if(chdir(directory)!=0){
            //printf("Error: %d\n",errno);
            printf("%s\n", strerror( errno ));
        }
    }
    fflush(stdout);
}

void execute_fg(char* job_id){
    if(job_id == NULL || job_id[0]!='%'){
        printf("Syntax: fg %%N\n");
        return;
    }
    int index = atoi(++job_id);
    // printf("%d job id requested\n",index );
    if(index > no_of_jobs){
      printf("Job doesn't exist\n");
      return;
    }
    Job* job_to_resume = search_by_index(index);
    
    kill(job_to_resume->pid, SIGCONT);
    wait_for_child(job_to_resume->pid);
    tcsetpgrp (STDIN_FILENO, getpid());
    printf("wait returned\n");

}
//checks for exit, jobs, fg and cd command
int check_and_handle_bash_cmd(char* inp){
    //tokenize input
    char *argv[ARGS_MAX];
    char *inp_copy = calloc(strlen(inp),1);
    memcpy(inp_copy,inp,strlen(inp));
    split_str( inp_copy, INP_SIZE_MAX, argv, ARGS_MAX);    
    if(argv[0] == NULL)
        return 0;
    else{
        if((strcmp(argv[0],exit_cmd)==0)){
            printf("Shell will exit now\n");fflush(stdout);
            exit(0);
        }
        else if((strcmp(argv[0],cd_cmd)==0)){
            execute_cd(argv[1]);
            return 1;
        }
        else if((strcmp(argv[0],jobs_cmd)==0)){
            printf("Background Jobs:\n");
            print_jobs();
            fflush(stdout);
            return 1;
        }
        else if((strcmp(argv[0],fg_cmd)==0)){
            execute_fg(argv[1]);
            return 1;
        }
        else{
            return 0;
        }
    }
    return 0;
}
/*
avoid using this, race conditions and a shit load of problems
void handle_SIGCHILD(int sig)
{
    pid_t pid;
    int child_status;
    printf("parent waiting \n");fflush(stdout);
    pid = waitpid(-1, &child_status, WNOHANG | WUNTRACED | WCONTINUED);
    if(errno == ECHILD){
     printf(" ECHILD errno\n" );
     printf("%d pid is returned\n",pid );
     return;
 }
 printf("%d pid is returned\n",pid );
 if(pid == last_started_process && WIFSTOPPED(child_status)){
    printf("Current process stopped \n");fflush(stdout);
    tcsetpgrp (STDIN_FILENO, getpid());
}
else if(pid == last_started_process && WIFEXITED(child_status)){
    printf("Current process terminated normally \n");fflush(stdout);
    tcsetpgrp (STDIN_FILENO, getpid());

}
else if(pid == last_started_process && WIFSIGNALED(child_status)){
    printf("Current process terminated on signal \n");fflush(stdout);
    tcsetpgrp (STDIN_FILENO, getpid());
} 
    //cases- 
    // 1.current foreground process has received SIGTSTP - put into jobs
    // 2.current foreground process has been terminated - do nothing
    // 3.a background process has terminated - remove from jobs
    // 4.a background process has stopped (previously running) - update jobs
{
    printf("something else \n");fflush(stdout);
} 
}*/

void init_shell(){
    setpgid(getpid(), getpid());
    signal (SIGINT, SIG_IGN);
    signal (SIGQUIT, SIG_IGN);
    signal (SIGTSTP, SIG_IGN);
    signal (SIGTTIN, SIG_IGN);
    signal (SIGTTOU, SIG_IGN);
}
void init_child_process(){
    setpgid(getpid(), getpid());
    tcsetpgrp (STDIN_FILENO, getpid());
    signal (SIGINT, SIG_DFL);
    signal (SIGQUIT, SIG_DFL);
    signal (SIGTSTP, SIG_DFL);
    signal (SIGTTIN, SIG_DFL);
    signal (SIGTTOU, SIG_DFL);
    signal (SIGCHLD, SIG_DFL);
}

int main(void)
{
    //give shell it's own process-group and ignore all signals
    init_shell();
    // signal (SIGCHLD, SIG_IGN);
    //signal handler for SIGCHILD
    // sigset_t mask;
    // sigemptyset (&mask);
    // sigaddset(&mask, SIGCHLD);
    // struct sigaction sa;
    // sa.sa_handler = &handle_SIGCHILD;
    // sa.sa_flags = SA_RESTART;
    // sa.sa_mask = mask;
    // sigaction(SIGCHLD, &sa, NULL);
    // 
    int child_status = 0;
    char* argv[ARGS_MAX];
    memset(inp,0,sizeof(inp)); 
    printf("SHELL PROCESS ID: %u\n", getpid());   
    while(1)    {
        pid=0; // to handle signals (Ctrl+c,z) properly
        printf("msh>");
        fflush(stdin);
        fgets(inp, INP_SIZE_MAX, stdin);
        inp_backup = calloc(strlen(inp),1); //backing up input- for background and ctrl+z
        memcpy(inp_backup,inp,strlen(inp));
        //handle bash commands
        if(check_and_handle_bash_cmd(inp)){
            continue;
        }
       is_background = check_if_background(inp);//hardik
       pid = fork();
       if( pid < 0 )   {
        printf(" ERROR creating child\n");
        exit(0);
    }
    if( pid == 0 )   {

        // printf("NEW PROCESS ID: %u\n", getpid());   
        //setup child process
        init_child_process();

        run_child(inp, INP_SIZE_MAX);
    }
        else    {  
         last_started_process=pid;   
         if(is_background){
          make_process_background(inp, pid);
      }else{
         wait_for_child(pid);
     }
     tcsetpgrp (STDIN_FILENO, getpid());
 }
}
return 0;
}