#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX 256

#define SPACE ' '
#define TAB '\t'
#define NEWLINE '\n'

int wait_for_child(pid_t pid)
{
    int returnStatus;
    waitpid(pid, &returnStatus, 0);

    if (returnStatus == 1)
        printf("Error(child process terminated unexpectedly)\n");
}

void  execute(char **argv)
{
    pid_t pid;

    if ((pid = fork()) < 0)
    {
        printf("Error(frok failed)\n");
        exit(1);
    }
    else if (pid == 0)
    {
        if (execvp(*argv, argv) < 0)
        {
            printf("Error(exec failed)\n");
            exit(1);
        }
    }
    else
    {
        wait_for_child(pid);
    }
}

void execute_pipe(char **argv, char **argvv)
{
    pid_t pid;

    int fds[2];
    pipe(fds);

    if ((pid = fork()) < 0)
    {
        printf("Error(frok failed)\n");
        exit(1);
    }

    else if (pid == 0)
    {
        dup2(fds[1], 1); //STDOUT

        if (execvp(*argv, argv) < 0)
        {
            printf("Error(exec failed)\n");
            exit(1);
        }
    }

    else
    {
        wait_for_child(pid);
        close(fds[1]);

        pid_t pid2;

        if ((pid2 = fork()) < 0)
        {
            printf("Error(frok failed)\n");
            exit(1);
        }
        else if (pid2 == 0)
        {
            dup2(fds[0], 0); // STDIN

            if (execvp(*argvv, argvv) < 0)
            {
                printf("Error(exec failed)\n");
                return;
            }
        }
        else
        {
            wait_for_child(pid2);
            close(fds[0]);
        }
    }
}

void execute_io(char **argv, char **argvv, int cmd)
{
    pid_t pid;
    if ((pid = fork()) < 0)
        printf("Error(frok failed)\n");
    else if (pid == 0)
    {
        int fd;

        if (cmd==0) //INPUT
        {
            fd = open(*argvv, O_RDONLY);
            dup2(fd, 0); // STDIN
        }

        else if (cmd==1) // OUTPUT
        {
            fd = creat(*argvv , 0644) ;
            dup2(fd, 1); //STDOUT
        }

        close(fd);
        execvp(*argv, argv);
        printf("Error(exec failed)\n");
        exit(1);
    }
    else
    {
        wait_for_child(pid);
    }
}

void cd(char *argv, int home)
{
    char path[MAX];
    strcpy(path,argv);

    char cwd[MAX];

    if(home==1)
    {
        //printf("%s\n",getenv("HOME"));
        chdir(getenv("HOME"));
    }

    else
    {
        if(argv[0] != '/') // the dir in cwd
        {
            getcwd(cwd,sizeof(cwd));
            if(strcmp(cwd,"/")!=0) strcat(cwd,"/");
            strcat(cwd,path);
            chdir(cwd);
        }
        else if(argv[0] == '/') // new dir
        {
            chdir(argv);
        }
    }

}

void help()
{
    printf("SHEILLA Help Menu\nThese shell commands are defined internally. ");
    printf("Type `help' to see this list.\nUse `man -k' or `info' to find out more about commands not in this list.\n");
    printf("1. cd\n2. echo\n3. help\n4. exit\n");
    printf("\n***Warning***\n");
    printf("Multiple piping is not supported yet.\nBoth input & output redirection in a single command is not supported yet.\n");
    printf("Compound command having piping and IO Redirection is not supported yet.\n\n\n");
}

void echo(char *stream)
{
    execute(stream);
}

void parse(char *line, char **argv)
{
    while (*line != NULL)
    {
        while (*line == SPACE || *line == TAB || *line == NEWLINE)
            *line++ = NULL;
        *argv++ = line;
        //printf("%s\n",line);
        while (*line != NULL && *line != SPACE && *line != TAB && *line != NEWLINE)
            line++;
    }
    *argv = NULL;
}

void  custom_parse(char *line, char **argv, char **argvv, char ch)
{
    while (*line != ch)
    {
        int flag= 0;
        while (*line == SPACE || *line == TAB || *line == NEWLINE || *line == ch )
        {
            if(*line==ch) flag=1;
            *line++ = NULL;
        }

        if(flag) break;
        *argv++ = line;
        while (*line != NULL && *line != SPACE && *line != TAB && *line != NEWLINE && *line != ch)
            line++;
    }
    *argv = NULL;

    while (*line != NULL)
    {
        while (*line == SPACE || *line == TAB || *line == NEWLINE)
            *line++ = NULL;
        *argvv++ = line;
        while (*line != NULL && *line != SPACE && *line != TAB && *line != NEWLINE)
            line++;
    }
    *argvv = NULL;
}

//-----------------------------------------MAIN------------------------------------------//
void  main(void)
{
    char line[MAX];
    char *argv[MAX];
    char *argvv[MAX];

    // clear scrn and welcome
    if(fork() == 0)
    {
        execl("/usr/bin/clear", "clear",0);
        exit(1);
    }
    else
        wait(NULL);
    printf("SHEILLA FROM MEHA, VERSION 1.2\n***Please use `help' first***\n\n");

    while (1)
    {
        //prompt message
        char cwd[MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            printf("%s$ ", cwd);
        else
            perror("Error(cwd not found)\n");

        fgets(line, MAX, stdin);
        int len = strlen(line);
        if (len > 0 && line[len-1] == '\n')
            line[len-1] = NULL;

        if(strchr(line, '|')!=NULL)
        {
            custom_parse(line,argv,argvv,'|');
            execute_pipe(argv, argvv);
            continue;
        }

        else if(strchr(line, '>')!=NULL)
        {
            custom_parse(line,argv,argvv,'>');
            execute_io(argv, argvv, 1); //1==out
            continue;
        }

        else if(strchr(line, '<')!=NULL)
        {
            custom_parse(line,argv,argvv,'<');
            execute_io(argv, argvv, 0); //0==in
            continue;
        }

        else
            parse(line, argv);

        if (strcmp(argv[0], "exit") == 0)
            exit(0);
        else if (strcmp(argv[0], "help") == 0)
            help();
        else if (strcmp(argv[0], "echo") == 0)
            echo(argv);
        else if (strcmp(argv[0], "cd") == 0)
        {
            if(argv[1]==NULL) cd(argv[0],1);
            else cd(argv[1],0);
        }
        else
            execute(argv);
    }
}

