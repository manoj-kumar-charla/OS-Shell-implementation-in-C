#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()

#define BUFSIZE 100
#define CWD_SIZE 100
#define PATH_MAX 100



void changeDirectory(char **tokens) {

    if(chdir(tokens[1])!=0){
	    printf("Shell: Incorrect command\n");
    }

}

char** parseInput(char *input){
    int position = 0;
    int buf_size = BUFSIZE;

    char **tokens = malloc(buf_size*sizeof(char*));
    char *token;
    while((token = strsep(&input," ")) != NULL) {
        if(strlen(token) == 0) {
            continue;
        }
        tokens[position++] = token;
    }

    tokens[position] = NULL;
    return tokens;

}
    

void executeCommand(char **tokens)
{
    if(strcmp(tokens[0],"cd") == 0) {
        changeDirectory(tokens);
    }
    else {
        int rc = fork();

        if(rc < 0) {
            exit(0);
        }
        else if(rc == 0) {
            signal(SIGINT, SIG_DFL);        // for ctrl + c    
		    signal(SIGTSTP, SIG_DFL);       // for ctrl + z

            if(execvp(tokens[0], tokens) == -1) {
                printf("Shell: Incorrect command\n");
                exit(1);
            }   
        }
        else {
            int rc_wait = wait(NULL);
        } 
    }

    
    
}


void executeSequentialCommands(char **tokens)
{	
    int start_of_cmd = 0;
    int i=0;
    while(tokens[i]){
        
        while(tokens[i] && strcmp(tokens[i],"##")!=0) {
            i++;
        }
        tokens[i] = NULL;
        executeCommand(&tokens[start_of_cmd]);
        i++;
        start_of_cmd = i;
        
    }
}


void executeParallelCommands(char **tokens)
{
    int rc;
    int i = 0;
    int start_of_cmd = 0;
    while(tokens[i]){

        while(tokens[i] && strcmp(tokens[i],"&&")!=0) {
            i++;
        }
        tokens[i] = NULL;
        if(strcmp(tokens[start_of_cmd],"cd") == 0) {
            changeDirectory(&tokens[start_of_cmd]);
        }
        else {
            rc = fork();
            if(rc < 0) {
                exit(0);
            }
            else if(rc == 0) {
                signal(SIGINT, SIG_DFL);
		        signal(SIGTSTP, SIG_DFL);

                if(execvp(tokens[start_of_cmd],&tokens[start_of_cmd]) == -1) {
                    printf("Shell: Incorrect command\n");
                    exit(1);
                }
            }
            else {
               int rc_wait = wait(NULL);
            }
        }
        i++;
        start_of_cmd = i;
    }

     
   
}


void executeCommandRedirection(char **tokens)
{
    int total_tokens = 0;   
    
    for(int i=0;tokens[i]!=NULL;i++) {
        total_tokens++;
    }

    int rc = fork();

    if(rc < 0) {
        exit(0);
    }
    else if(rc == 0) {
        signal(SIGINT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
        close(STDOUT_FILENO);
        int f = open(tokens[total_tokens-1],O_CREAT | O_WRONLY | O_APPEND);
        char **execCommand;
        execCommand = (char**)malloc((total_tokens-1)*sizeof(char*));

        for(int i=0;i<total_tokens-2;i++) {
            execCommand[i] = tokens[i];
        }
        execCommand[total_tokens-2] = NULL;
        if(execvp(execCommand[0],execCommand) == -1) {
            printf("Shell: Incorrect command\n");
            exit(1);
        }

        fflush(stdout); 
        close(f);
    }
    else {
        int rc_wait = wait(NULL);
    }

}
char cwd[PATH_MAX];     


void signalHandler(int sig) {
    printf("\n");
	printf("%s$", cwd);
	fflush(stdout);
    return;

}

int main()
{

    signal(SIGTSTP,&signalHandler);              // for ctrl + z
    signal(SIGINT,&signalHandler);              // for ctrl + c
	

	
	while(1)	
	{
        if (getcwd(cwd, sizeof(cwd)) != NULL) 
	   	{
	       printf("%s$", cwd);
	   	}

		
		
        char *input = NULL;
        size_t size = 0;
        
        int byte_read  = getline(&input,&size,stdin);
        int len = strlen(input);
        input[len-1] = '\0';



		char **args = parseInput(input); 		
		
		if(strcmp(args[0],"exit") == 0 || strcmp(args[0], "EXIT") == 0)	
		{
			printf("Exiting shell...\n");
			break;
		}

        int type = 0;
        for(int i=0; args[i]!= NULL ; i++) {

            if(strcmp(args[i],"&&") == 0) {
                type = 1;
                break;
            }
            else if(strcmp(args[i],"##") == 0) {
                type = 2;
                
                break;
            }
            else if(strcmp(args[i],">") == 0) {
                type=3;
                break;
            }
        }

        if(type == 1) {
            executeParallelCommands(args);
        }
        else if(type == 2) {
            executeSequentialCommands(args);
        }
        else if(type == 3) {
            executeCommandRedirection(args);
        }
        else {
            executeCommand(args);
        }
				
	}
	
	return 0;
}
