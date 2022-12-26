#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

// Maximum length for a command
#define MAXLEN 1024
// Maximum number of arguments allowed for a command
#define ARGLEN 256

// Command handlers declaration
void lsHandler(char *args[], int countArgs);
void touchHandler(char *args[], int countArgs);
void cdHandler(char *args[], int countArgs);
void cpHandler(char *args[], int countArgs);
void catHandler(char *args[], int countArgs);
void lsL(char *dir, struct dirent *dir_entr);
void lsPipe(char *dir, char *args[], int scIndex);
void anyCmdHandler(char *args[], int countArgs);
int isSubstring(char *str, char *substr);
void catPipe(char *args[], int countArgs);

// static string to store present working directory
static char *curDir;
static int pipeFlag;

// Command strings
const char *LS = "ls";
const char *EXIT = "exit";
const char *PWD = "pwd";
const char *CLR = "clear";
const char *CD = "cd";
const char *TOUCH = "touch";
const char *CAT = "cat";
const char *CP = "cp";

// ERROR MESSAGES
const char *INSUFFARGS = "Insuffiecient arguments!!";
const char *CMDNOTFOUND = "Command not found!!";
const char *FILENOTFOUND = "File not found!!";
const char *INVALIDCMDFORMAT = "Invalid command format!";

void printError(int errorCode)
{
    switch (errorCode)
    {
    case 0:
        printf("%s", INSUFFARGS);
        break;
    case 1:
        printf("%s", CMDNOTFOUND);
        break;
    case 2:
        printf("%s", FILENOTFOUND);
        break;
    case 3:
        printf("%s", INVALIDCMDFORMAT);
    default:
        break;
    }
}

void initialPrompt()
{
    getcwd(curDir, MAXLEN);
    printf("\nCShell: %s>", curDir);
}

// Handler function to execute user input commands
void commandHandler(char *commandArgs[], int countArgs)
{
    if (strcmp(commandArgs[0], LS) == 0)
        lsHandler(commandArgs, countArgs);
    else if (strcmp(commandArgs[0], PWD) == 0)
        printf("%s", curDir);
    else if (strcmp(commandArgs[0], CLR) == 0)
        system(CLR);
    else if (strcmp(commandArgs[0], CD) == 0)
        cdHandler(commandArgs, countArgs);
    else if (strcmp(commandArgs[0], TOUCH) == 0)
        touchHandler(commandArgs, countArgs);
    else if (strcmp(commandArgs[0], CAT) == 0)
        catHandler(commandArgs, countArgs);
    else if (strcmp(commandArgs[0], CP) == 0)
        cpHandler(commandArgs, countArgs);
    else if (strcmp(commandArgs[0], EXIT) == 0)
        exit(0);
    else
        anyCmdHandler(commandArgs, countArgs);
}

int main(int argc, char *argv[])
{
    // String to store current user input command
    char *command;
    char *commandArgs[ARGLEN];
    // Number of arguments in command
    int countArgs = 0;
    // Memory allocation for static string curDir
    curDir = (char *)malloc(MAXLEN * sizeof(char));

    while (1)
    {
        initialPrompt();
        command = (char *)malloc(MAXLEN * sizeof(char));

        countArgs = 0;
        // get command from user
        fgets(command, MAXLEN, stdin);

        if (isSubstring(command, "|") == 1)
            pipeFlag = 1;
        else
            pipeFlag = 0;

        // Tokenizing the command
        commandArgs[countArgs] = strtok(command, " \n");
        // If Empty string passed
        if (commandArgs[countArgs] == NULL)
            continue;

        while (commandArgs[countArgs] != NULL)
        {
            ++countArgs;
            commandArgs[countArgs] = strtok(NULL, " \n");
        }

        // Implement history feature
        /*

        */

        commandHandler(commandArgs, countArgs);
    }
    return 0;
}

void anyCmdHandler(char *args[], int countArgs)
{
    int pid = fork();
    if (pid == -1)
    {
        printf("Child process could not be created.");
        return;
    }
    else if (pid == 0)
    {
        if(pipeFlag==1)
        {
            int i=0;
            while(strcmp(args[i],"|")!=0) ++i;
            char *args1[i];
            char *args2[countArgs-i-1];
            for(int j=0;j<i;++j)
                args1[j] = args[j];
            for(int j=i+1;j<countArgs;++j)
                args2[countArgs-j-1]=args[j];
            
            // File descriptor for piped process
            int fd[2];
            // p1,p2: Store process ids of child processes
            pid_t p1, p2;
            // Create a pipe between fd[0] and fd[1]
            pipe(fd);
            p1 = fork();
            if (p1 == 0)
            {
                close(fd[0]);
                dup2(fd[1], 1);
                // execlp(args1[0],args1[0],args1[1],NULL);
                execvp(args1[0],args1);
            }

            else
            {
                waitpid(p1, NULL, 0);
                close(fd[1]);
                dup2(fd[0], 0);
                // execlp(args2[0],args2[0],args2[1],NULL);
                execvp(args2[0],args2);
            }
        }
        else if(execvp(args[0], args) == -1)
        {
            printError(1);
        }
        kill(getpid(), SIGTERM);
    }
    else
    {
        waitpid(pid, NULL, 0);
    }
}

void lsHandler(char *args[], int countArgs)
{
    struct dirent **dirlist;
    int dirCount;
    dirCount = scandir(curDir, &dirlist, NULL, alphasort);
    if (dirCount < 0)
    {
        printf("No directories/files found!\n");
        return;
    }
    if (pipeFlag == 1)
    {
        int i = 0;
        while (strcmp(args[i], "|") != 0)
            ++i;
        if (fork() == 0)
            lsPipe(curDir, args, i + 1);
        else
            wait(NULL);
    }
    else
    {
        if (countArgs == 2 && strcmp(args[1], "-l") == 0)
        {
            while (dirCount--)
            {
                lsL(curDir, dirlist[dirCount]);
                free(dirlist[dirCount]);
            }
        }
        else
        {
            while (dirCount--)
            {
                printf("%s\t\t", dirlist[dirCount]->d_name);
                free(dirlist[dirCount]);
            }
            free(dirlist);
        }
    }
}

void lsPipe(char *dir, char *args[], int scIndex)
{
    struct dirent **dirlist;
    int dirCount = scandir(dir, &dirlist, NULL, alphasort);
    if (dirCount < 0)
    {
        printf("No directories/files found!\n");
        return;
    }
    // File descriptor for piped process
    int fd[2];
    // p1,p2: Store process ids of child processes
    pid_t p1, p2;
    // Create a pipe between fd[0] and fd[1]
    pipe(fd);

    // Fork to run ls command and pass its output to pipe
    // By redirecting the STDOUT of process in pipe using dup2()
    p1 = fork();
    if (p1 == 0)
    {
        close(fd[0]);
        dup2(fd[1], 1);
        if (strcmp(args[1], "-l") == 0)
        {
            while (dirCount--)
            {
                lsL(curDir, dirlist[dirCount]);
                free(dirlist[dirCount]);
            }
        }
        else
        {
            // execlp("ls","ls",NULL);
            while (dirCount--)
            {
                printf("%s\n", dirlist[dirCount]->d_name);
                // puts(dirlist[dirCount]->d_name);
                free(dirlist[dirCount]);
            }
            free(dirlist);
        }
        kill(getpid(), SIGTERM);
    }

    // Fork to run second command(more/sort) command which will get input from pipe
    // By redirecting the STDIN of process in pipe using dup2()
    else
    {
        wait(NULL);
        close(fd[1]);
        dup2(fd[0], 0);
        execlp(args[scIndex], args[scIndex], NULL);
    }
}

void lsL(char *dir, struct dirent *dir_entr)
{
    struct stat statbuf;
    char fp[PATH_MAX];
    sprintf(fp, "%s/%s", dir, dir_entr->d_name);
    if (stat(fp, &statbuf) == -1)
    {
        perror("stat");
        return;
    }

    // permission data/nlink
    printf((S_ISDIR(statbuf.st_mode)) ? "d" : "-");
    printf((statbuf.st_mode & S_IRUSR) ? "r" : "-");
    printf((statbuf.st_mode & S_IWUSR) ? "w" : "-");
    printf((statbuf.st_mode & S_IXUSR) ? "x" : "-");
    printf((statbuf.st_mode & S_IRGRP) ? "r" : "-");
    printf((statbuf.st_mode & S_IWGRP) ? "w" : "-");
    printf((statbuf.st_mode & S_IXGRP) ? "x" : "-");
    printf((statbuf.st_mode & S_IROTH) ? "r" : "-");
    printf((statbuf.st_mode & S_IWOTH) ? "w" : "-");
    printf((statbuf.st_mode & S_IXOTH) ? "x " : "- ");
    printf("%li ", statbuf.st_nlink);

    // group and user data
    struct passwd *pw;
    struct group *gid;
    pw = getpwuid(statbuf.st_uid);
    if (pw == NULL)
    {
        perror("getpwuid");
        printf("%d ", statbuf.st_uid);
    }
    else
    {
        printf("%s ", pw->pw_name);
    }
    gid = getgrgid(statbuf.st_gid);
    if (gid == NULL)
    {
        perror("getpwuid");
        printf("%d ", statbuf.st_gid);
    }
    else
    {
        printf("%s ", gid->gr_name);
    }

    // file size
    printf("%5ld ", statbuf.st_size);

    // timestamp
    struct tm *tmp;
    char outstr[200];
    time_t t = statbuf.st_mtime;
    tmp = localtime(&t);
    if (tmp == NULL)
    {
        perror("localtime");
        exit(EXIT_FAILURE);
    }
    strftime(outstr, sizeof(outstr), "%b %d %R", tmp);
    printf("%s ", outstr);

    // file name
    printf("%s\n", dir_entr->d_name);
}

void cdHandler(char *arg[], int countArgs)
{
    if (countArgs == 1 || arg[1] == NULL)
    {
        chdir(getenv("HOME"));
    }
    else
    {
        if (chdir(arg[1]) == -1)
        {
            printf(" %s: no such directory\n", arg[1]);
        }
    }
}

void touchHandler(char *args[], int countArgs)
{
    if (countArgs == 1 || args[1] == NULL)
    {
        printError(0);
    }
    else
    {
        FILE *fptr;
        printf("%s", args[1]);
        fptr = fopen(args[1], "w");
        if (fptr == NULL)
        {
            printf("File creation failed! Try again.");
            free(fptr);
            return;
        }
        fclose(fptr);
    }
}

void catHandler(char *args[], int countArgs)
{
    if (countArgs == 1 || args[1] == NULL)
    {
        printf("%s", INSUFFARGS);
    }
    else if (countArgs == 2 && strcmp(args[1], ">") == 0)
    {
        printf("%s", INSUFFARGS);
    }
    else if (countArgs == 2)
    {
        FILE *fptr;
        fptr = fopen(args[1], "r");
        if (fptr == NULL)
        {
            printError(2);
        }
        else
        {
            char buf[MAXLEN];
            while (fgets(buf, MAXLEN, fptr) != NULL)
                puts(buf);
            fclose(fptr);
        }
    }
    else if (countArgs == 3 && strcmp(args[1], ">") == 0)
    {
        FILE *fptr;
        fptr = fopen(args[2], "w");
        if (fptr == NULL)
        {
            printError(2);
        }
        else
        {
            char buf[MAXLEN];
            memset(buf, '\0', MAXLEN);
            int i = 0;
            char c = getchar();
            while (c != '\1')
            {
                buf[i++] = c;
                if (i >= MAXLEN - 1)
                {
                    fputs(buf, fptr);
                    memset(buf, '\0', MAXLEN);
                    i = 0;
                }
                c = getchar();
            }
            fputs(buf, fptr);
            fclose(fptr);
        }
    }
    else if (pipeFlag == 1)
    {
        if (fork() == 0)
            catPipe(args, countArgs);
        else
            wait(NULL);
    }
    else
    {
        printError(3);
    }
}

void catPipe(char *args[], int countArgs)
{
    // File descriptor for piped process
    int fd[2];
    // p1,p2: Store process ids of child processes
    pid_t p1, p2;
    // Create a pipe between fd[0] and fd[1]
    pipe(fd);

    // Fork to run cat command and pass its output to pipe
    // By redirecting the STDOUT of process in pipe using dup2()
    p1 = fork();
    if (p1 == 0)
    {
        close(fd[0]);
        dup2(fd[1], 1);
        FILE *fptr;
        fptr = fopen(args[1], "r");
        if (fptr == NULL)
        {
            printError(2);
        }
        else
        {
            char buf[MAXLEN];
            while (fgets(buf, MAXLEN, fptr) != NULL)
                puts(buf);
            fclose(fptr);
        }
        kill(getpid(),SIGTERM);
    }

    // Fork to run grep command which will get input from pipe
    // By redirecting the STDIN of process in pipe using dup2()
    else
    {
        wait(NULL);
        close(fd[1]);
        dup2(fd[0], 0);
        execlp(args[3], args[3], args[4], NULL);
    }
}

void cpHandler(char *args[], int countArgs)
{
    if (countArgs < 3 || args[1] == NULL || args[2] == NULL)
    {
        printf("%s", INSUFFARGS);
    }
    else
    {
        char buf[MAXLEN];
        FILE *inptr, *optr;
        inptr = fopen(args[1], "r");
        optr = fopen(args[2], "w");
        if (inptr == NULL || optr == NULL)
            printError(2);
        else
        {
            fgets(buf, MAXLEN, inptr);
            fputs(buf, optr);
            fclose(inptr);
            fclose(optr);
            printf("Content copied!");
        }
    }
}

int isSubstring(char *str, char *substr)
{
    int i = 0, j = 0;
    int l1 = strlen(str);
    int l2 = strlen(substr);
    if (l2 > l1)
        return 0;
    while (i < l1 && j < l2)
    {
        if (*(substr + j) == *(str + i))
        {
            ++i;
            ++j;
        }
        else
        {
            j = 0;
            ++i;
        }
    }
    if (j == l2)
        return 1;
    return 0;
}