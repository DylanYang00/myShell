/**
 * Author: Yang Yuchen
 * Student Number: 2018215065
 * Co-instructor: Li tun
 * 
 * Outline:
 * implement a shell.
 * 1. print prompt in format "mysh> "
 * 2. if there is |, split the string into tokens and using pipe()
 * 3. call exec() to process the token
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h> 
#include<sys/stat.h>
#include <fcntl.h>

#define PRMTSIZ 255
#define MAXARGS 63
#define DELIMS  " \t\n\'"
/**
 * A command consists of the executable and arguments
 * argv: store all arguments include the executable
 * tempToFree: malloced memory to deallocate
 */ 
struct Command
{
  const char **argv;
  void* tempToFree;// put into struct to store the pointer each time we process a command
};


void handleIORedirect(struct Command * cmd);
 char *strdup(const char *s);
char * strsep(char **stringp, const char *delim);
char *strtok_r(char *str, const char *delim, char **saveptr);
void readLine(char * line);
struct Command parseToken(const char * str, void ** tempToFree);
int spawn_proc (int in, int out, struct Command *cmd);
int fork_pipes (int n, struct Command *cmd);


int main(int argc, char const *argv[])
{
    while(1){
        // print prompt
        wait(NULL);
        printf("mysh> ");fflush(stdout);
        // read line
        char line[PRMTSIZ + 1] = { 0x0 };
        readLine(line);
        char * temp = strdup(line);// to avoid being changed by strtok
        // parse 
        struct Command commands[MAXARGS];
        
        int count = 0;
        char * rest = temp, * strCmd;
        
        while ((strCmd = strtok_r(rest, "|", &rest))) {
            void * tempToFree;
            commands[count] = parseToken(strCmd, &tempToFree);
            commands[count].tempToFree = tempToFree;
            count++;
        }
        // execute
        fork_pipes (count, commands);
        // free all heap memory
        for (size_t i = 0; i < count; i++){
           free(commands[i].argv);  
           free(commands[i].tempToFree);
        }
        free(temp);    
    }
    return 0;
}
/**
 * read a line from stdin
 */ 
void readLine(char * line){
  fgets(line, PRMTSIZ + 1, stdin);
  line[strcspn(line, "\n")] = '\0'; // remove trailing \n
}

/**
 * given a string representing a command, return a Command struct.
 */ 
struct Command parseToken(const char * str, void ** tempToFree){
  
  
  char ** argv =(char **) malloc((MAXARGS + 1) * sizeof(char*));

  
  char *temp;
  temp = strdup(str); *tempToFree = (void *)temp;
  char* token; 
  char* rest = temp; 
  int count = 0;
  //split into arguments and store them in argv
  while (token = strsep(&rest, DELIMS))
  {
    if (strcmp("",token) == 0)// token is ""
        continue;
    argv[count++] = token;  
  }
  argv[count] = NULL;
  struct Command result = {argv:(const char **)argv};

  return result;     
}
/**
 * produce a new process with given stdin and stdout fd and command to execute
 * in, out: file number if pipe used.
 * cmd: command to execute.
 */ 
int spawn_proc (int in, int out, struct Command *cmd)
{
  pid_t pid;

  if ((pid = fork ()) == 0){
      if (in != STDIN_FILENO){
          dup2 (in, STDIN_FILENO);
          close (in);
        }

      if (out != STDOUT_FILENO){
          dup2 (out, STDOUT_FILENO);
          close (out);
        }
      return execvp (cmd->argv [0], (char * const *)cmd->argv);
  }
  return pid;
}

/**
 * This is the command-execute procudure.
 * fork n processes and create n - 1 pipes to execute the command.
 */  
int fork_pipes (int n, struct Command *cmds)
{
  int i;
  pid_t pid;
  int in, fd [2];

  /* The first process should get its input from the original file descriptor 0.  */
  in = 0;

  /* Note the loop bound, we spawn here all, but the last stage of the pipeline.  */
  for (i = 0; i < n - 1; ++i)
    {
      pipe (fd);

      /* f [1] is the write end of the pipe, we carry `in` from the prev iteration.  */
      spawn_proc (in, fd [1], cmds + i);

      /* No need for the write end of the pipe, the child will write here.  */
      close (fd [1]);

      /* Keep the read end of the pipe, the next child will read from there.  */
      in = fd [0];
    }

 

  /* Execute the last stage with the current process. */
  if((pid = fork()) == 0 ) {
     /* Last stage of the pipeline - set stdin be the read end of the previous pipe
     and output to the original file descriptor 1. */  
    if (in != 0){
      dup2 (in, 0);
      close(in);
    }
    handleIORedirect(&(cmds[i]));
    execvp (cmds[i].argv [0], (char * const *)cmds[i].argv);
  }
  wait(NULL);
  return 0;
}

/**
 * if there exists argument like ">", make an I/O redirection.
 * and also delete ">" and the file from argv because the command to execute actually
 * don't need these 2 arguments.
 */ 
void handleIORedirect(struct Command * cmd){
  size_t i;
  for (i = 0; (cmd->argv[i]) != NULL; i++)
  {
    if (strcmp(">",(cmd->argv[i])) == 0)
      break;
  }
  
  if ((cmd->argv[i]) == NULL)
    return;
  
  int fd = open((cmd->argv[i + 1]), O_WRONLY|O_CREAT,0666);
  dup2 (fd, 1);
  close(fd);
  /* put args behind these two forward*/
  for (; i < 20; i++)
    (cmd->argv[i]) = (cmd->argv[i+2]);
  
  
}