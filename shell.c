#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define MAX_COMMAND_LINE_LEN 1024
#define MAX_COMMAND_LINE_ARGS 128

char prompt[] = "> ";
char delimiters[] = " \t\r\n";
extern char **environ;

static volatile pid_t fg_pid = 0;

void handle_sigint(int sig){
  (void)sig;
  write(STDOUT_FILENO, "\n", 1);
}

void handle_alarm(int sig){
  (void)sig;
  if(fg_pid > 0) kill(fg_pid, SIGKILL);
}


int main() {
    // Stores the string typed into the command line.
    char command_line[MAX_COMMAND_LINE_LEN];
    char cmd_bak[MAX_COMMAND_LINE_LEN];
  
    // Stores the tokenized command line input.
    char *arguments[MAX_COMMAND_LINE_ARGS];

    //Installs signal handlers.
    signal(SIGINT, handle_sigint);
    siginterrupt(SIGINT, 1);
    signal(SIGALRM, handle_alarm);
    	
    while (true) {
      
        do{ 

            //Prints current working directory
            char cwd[MAX_COMMAND_LINE_LEN];
            if(getcwd(cwd, sizeof(cwd)) != NULL){
              printf("%s%s", cwd, prompt);
            } else{
              printf("%s", prompt);
            }
            fflush(stdout);

            // Read input from stdin and store it in command_line. If there's an
            // error, exit immediately. (If you want to learn more about this line,
            // you can Google "man fgets")
        
            if ((fgets(command_line, MAX_COMMAND_LINE_LEN, stdin) == NULL) && ferror(stdin)) {
                if (errno == EINTR){
                  clearerr(stdin);
                  command_line[0] = 0x0A;
                  continue;
                }
                fprintf(stderr, "fgets error");
                exit(0);
            }
 
        }while(command_line[0] == 0x0A);  // while just ENTER pressed
        command_line[strlen(command_line) - 1] = '\0';
      
        // If the user input was EOF (ctrl+d), exit the shell.
        if (feof(stdin)) {
            printf("\n");
            fflush(stdout);
            fflush(stderr);
            return 0;
        }

        // TODO:
        // 
        
			  // 0. Modify the prompt to print the current working directory
			  
			
        // 1. Tokenize the command line input (split it on whitespace)
        int argc = 0;
        char *token = strtok(command_line, delimiters);
        while (token != NULL && argc < MAX_COMMAND_LINE_ARGS - 1){
          if (token[0] == '$'){
            char * val = getenv(token + 1);
            arguments[argc++] = val ? val: ""; 
          } else {
            arguments[argc++] = token;
          }
          token = strtok(NULL, delimiters);
        }
        arguments[argc] = NULL;
        if (argc == 0) continue;

      
        // 2. Implement Built-In Commands
        if (strcmp(arguments[0], "exit") == 0){
          exit (0);
        } else if (strcmp(arguments[0], "cd") == 0){
          const char *dir = arguments[1] ? arguments [1]: getenv("HOME");
          if (dir == NULL || chdir(dir) != 0) perror("cd failed");
          continue; 
        } else if (strcmp(arguments[0], "pwd") == 0) {
          char cwd[MAX_COMMAND_LINE_LEN];
          if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
          else perror("getcwd failed");
          continue;
        }else if (strcmp(arguments[0], "echo") == 0) {
          for (int i =1; arguments[i] != NULL; i++){
            printf("%s%s", arguments[i], arguments[i + 1] ? " ": "");
          }
          printf("\n");
          continue;
        }else if (strcmp(arguments[0], "env") == 0){
          if (arguments[1] !=NULL) {
            char *val = getenv(arguments[1]);
            if (val) printf("%s\n", val);
          } else{
            for (char **e = environ; *e != NULL; e++) printf("%s\n", *e);
          }
          continue;
        } else if (strcmp(arguments[0], "setenv") == 0){
          if (arguments[1] == NULL){
            fprintf(stderr, "usage: setenv NAME-VALUE pr setenv NAME VALUE\n");
          } else if (strchr(arguments[1], '=') != NULL){
            char *eq = strchr(arguments[1], '=');
            *eq = '\0';
            setenv(arguments[1], eq + 1, 1);
          } else {
            setenv(arguments[1], arguments[2] ? arguments[2] : "", 1);
          }
          continue;
        }

        //run in background

        bool background = false;
        if (strcmp(arguments[argc - 1], "&") == 0){
          background = true;
          arguments[--argc] = NULL;
          if (argc == 0) continue;
        }
      
    
        // 3. Create a child process which will execute the command line input
        pid_t pid = fork();
        if (pid < 0){
          perror("fork failed");
          continue;
        }

        // Output redirection
        if (pid == 0){
          for (int i = 0; arguments[i] != NULL; i++){
            if (strcmp(arguments[i], ">") == 0){
              if (arguments[i + 1] == NULL){
                fprintf(stderr, "syntax error: expected filename after >\n");
                exit(1);
              }
              int fd = open(arguments[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
              if (fd < 0){
                perror("open failed");
                exit(1);
              }
              dup2(fd, STDOUT_FILENO);
              close(fd);
              arguments[i] =NULL;

              break;
            }
          }
          execvp(arguments[0], arguments);
          perror("execvp() failed");
          exit(127);
        }

  
        // 4. The parent process should wait for the child to complete unless its a background process
        if (!background){
          int status;
          fg_pid = pid;
          alarm(10);

          while(waitpid(pid, &status, 0) < 0 && errno == EINTR){}
          alarm(0);
          fg_pid = 0;
          if(WIFEXITED(status) && WEXITSTATUS(status) == 127){
            printf("An error occurred.\n");
          }
        }
      
      
        // Hints (put these into Google):
        // man fork
        // man execvp
        // man wait
        // man strtok
        // man environ
        // man signals
        
        // Extra Credit
        // man dup2
        // man open
        // man pipes
    }
    // This should never be reached.
    return -1;
}