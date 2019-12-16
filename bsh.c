 
/**
 * @file shell.c
 * Main entrypoint
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <setjmp.h>



/* Function declarations */
/* Return type - void */
void error(const char *);
void sigint_handler(int);

/* Return type - char */
char **get_input(char *);

/* Return type - int */
int cd(char *);



/* static type initialisations */
static sigjmp_buf env;
/*  
    jump_active is used to test whether set point is set by sigsetjmp
    before calling jump.
*/
static volatile sig_atomic_t jump_active = 0;



/*
 * Function main
 * --------------------
 *  Entrypoint of the program.
 *
 *  argc: Argument count; contains the number of argument passed to
 *        the program
 *  argv: Pointer to argument values passed to the program
 * 
 *  returns: an integer
 */
int main(int argc, char **argv) {
    
    /* Declaring and initialising variables */
    /* Variable declaration type - char */
    char **command;
    char *input;

    /* Datatype to store process ids - pid_t */
    pid_t child_pid;
    int stat_loc;

    /* Setup SIGINT */
    struct sigaction s;
    s.sa_handler = sigint_handler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = SA_RESTART;
    sigaction(SIGINT, &s, NULL);

    /* Infinite loop to keep the shell running */
    while (1) {
        /*
            Want to restart shell ? 
            Non-local jump here to restart the shell.
        */
        /* 
            Function sigsetjmp
                Sets a jmp point for non-local jump and accepts a sigbmp_buf buffer.
                The value returned by sigsetjmp is set by siglongjmp while calling, so 42 is 
                selected as a random number here. So whenever we get SIGINT interrupt or
                (ctrl+C) is pressed, the program jumps here to restart the shell.
                ** Unblocks the interrupt block set by sigaction.
         */
        if (sigsetjmp(env, 1) == 42) {
            printf("\n");
            continue;
        }

        /* Set jump_active to indicate that jump set point is set */
        jump_active = 1;

        /* Read input from shell */
        input = readline("bsh> ");

        /* Exit on Ctrl-D */
        if (input == NULL) {
            printf("\n");
            exit(0);
        }

        /* Call get_input to parse the input command*/
        command = get_input(input);

        /* Handle empty commands */
        if (!command[0]) {
            free(input);
            free(command);
            continue;
        }

        /* Check if the command is "change directory - cd" */
        if (strcmp(command[0], "cd") == 0) {
            if (cd(command[1]) < 0) {
                perror(command[1]);
            }

            /* Skip the fork */
            continue;
        }

        /* 
            Forking the parent process - child
            For a valid command, create a child process to run the command.
            The command can be run in the parent process itself but if the command
            ran into any errors, it is pretty hard to recover.
            After forking, both parent and the child process will run the codes after
            fork(). The child_pid of child is 0 and that of the parent is non-zero which is 
            same as the "pid" of child(NOTE: pid not child_pid).
        */
        child_pid = fork();
        if (child_pid < 0) {
            error("Fork failed");
        }

        /* Child process enter */
        if (child_pid == 0) {

            struct sigaction s_child;
            s_child.sa_handler = sigint_handler;
            sigemptyset(&s_child.sa_mask);
            s_child.sa_flags = SA_RESTART;

            /*
                Function sigaction
        
                    Handles interrupt in a custom way.
                    ** Blocks the interrupt given in the first param to make sure that
                    it does not messes with already executing incomplete interrupt
                    handler function. So, next time when this interrupt is invoked, 
                    without unblocking the interrupt does not get delivered.
                    This interrupt is unblocked after invoking sigsetjmp.
                    
                    param1 - int; Signal number; denoting the interrupt to be handled
                    param2 - sigaction struct; contains new configurations to be set
                    param3 - sigaction struct; current configs to be saved before
                             overwriting with new one.
            */
            sigaction(SIGINT, &s_child, NULL);

            /* Never returns if the call is successful */
            if (execvp(command[0], command) < 0) {
                error(command[0]);
            }
        } else {
            /* Parent process wait till the child process command to complete */
            waitpid(child_pid, &stat_loc, WUNTRACED);
        }

        /* Free memory to avoid memory leak */
        if (!input)
            free(input);
        if (!command)
            free(command);
    }

    return 0;
}



/*
 * Function error
 * --------------------
 *  Prints error message using perror and exit the program
 *
 *  msg: a pointer containing the message to be printed
 * 
 *  returns: void
 */
void error(const char *msg) {
	perror(msg);
	exit(1);
}


/*
 * Function sigint_handler
 * --------------------
 *  Handles interrupt for interrupt SIGINT (can be produced by ctrl+C) by
 *  restarting the shell.
 * 
 *      siglongjmp non-local jumps to the set point set by sigsetjmp.
 *      42 is passed so that, sigsetjmp returns 42 after its call.
 *
 *  signo: int
 * 
 *  returns: void
 */
void sigint_handler(int signo) {
    if (!jump_active) {
        return;
    }
    siglongjmp(env, 42);
}


/*
 * Function get_input
 * --------------------
 *  Parses the command given and splits the command string into substring 
 *  with " " as its delimiter and pushes them into an array.
 *
 *  input: input command given to the shell
 * 
 *  returns: a pointer to an array
 */
char **get_input(char *input) {
    char **command = malloc(8 * sizeof(char *));
    if (command == NULL) {
        error("malloc failed");
    }

    char *delim = " ";
    char *parsed;
    int index = 0;

    /* strtok divides input into tokens separated by characters in delim.  */
    parsed = strtok(input, delim);
    while (parsed != NULL) {
        command[index] = parsed;
        index++;

        parsed = strtok(NULL, delim);
    }

    /* Last element of the array should be NULL to identify the end of command */
    command[index] = NULL;
    return command;
}


/*
 * Function cd
 * --------------------
 *  Changes the present working directory to the path given
 *
 *  path: pointer to the path given
 * 
 *  returns: int
 */
int cd(char *path) {
    return chdir(path);
}
