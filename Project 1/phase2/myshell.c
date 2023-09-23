/* $begin shellmain */
#include "myshell.h"
#include <errno.h>
#define MAXARGS 128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

void exec_pipe(char ** argv);
int check = 0;
int pipenum;

int main()
{
    char cmdline[MAXLINE]; /* Command line */

    while (1)
    {
        check = 0;
        /* Read */
        printf("> ");
        fgets(cmdline, MAXLINE, stdin);
        if (feof(stdin))
            exit(0);

        /* Evaluate */
        eval(cmdline);
    }
}
/* $end shellmain */

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline)
{
    FILE *fp = fopen("./command.log", "a+");
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    char temp[MAXLINE];
    memset(argv,0,sizeof argv);
    memset(temp,0,sizeof temp);
    memset(buf,0,sizeof buf);

    int cnt =0;
    int k =0 ;
    pipenum =0;
    for(int i=0 ; i<strlen(cmdline);i++)
    {
        if (cmdline[i] == '!')
            {
                if (cmdline[i + 1] == '!')
                {
                    char temp_str[MAXLINE];
                    fseek(fp, 0, SEEK_END);
                    if (ftell(fp) == 0)
                    {
                        check = 1;
                    }
                    else
                    {
                        rewind(fp);
                        while (!feof(fp))
                        {
                            fgets(temp_str, MAXLINE, fp);
                        }
                        temp_str[strcspn(temp_str, "\n")] = '\0';
                        printf("%s\n", temp_str);

                        for (int j = 0; j < strlen(temp_str); j++)
                        {
                            temp[cnt++] = temp_str[j];
                        }
                        i++;
                        rewind(fp);
                    }
                }
                else if (cmdline[i + 1] > '0' && cmdline[i + 1] <= '9')
                {
                    if(cmdline[i + 1] == '0')
                        check = 2;
                    char num[MAXLINE];
                    memset(num,0,sizeof num);
                    k = 0;
                    while (cmdline[i + 1] >= '0' && cmdline[i + 1] <= '9')
                    {
                        num[k++] = cmdline[++i];
                    }

                    k = atoi(num);
                    char temp_str[MAXLINE];
                    fseek(fp, 0, SEEK_END);
                    if (ftell(fp) == 0)
                    {
                        check = 2;
                    }
                    else
                    {
                        rewind(fp);
                        int t =k;
                        while (t--)
                        {   
                            if(fgets(temp_str, MAXLINE, fp)==NULL)
                            {
                                check=2;
                                break;
                            }
                        }
                        temp_str[strcspn(temp_str, "\n")] = '\0';
                        if(check!=2)
                            printf("%s\n", temp_str);

                        for (int j = 0; j < strlen(temp_str); j++)
                        {
                            temp[cnt++] = temp_str[j];
                        }
                        rewind(fp);
                    }
                }
                else
                {
                    temp[cnt++] = cmdline[i];
                }
            }
            else
                {
                    temp[cnt++] = cmdline[i];
                }
    }
    temp[strlen(temp)]='\0';
    strcpy(cmdline,"");
    strcat(cmdline,temp);

    if (check == 1)
    {
        printf("bash: !!: event not found\n");
        return;
    }
    else if (check == 2)
    {
        printf("bash: !%d: event not found\n", k);
        return;
    }

    memset(temp,0,sizeof temp);
    while (!feof(fp))
    {
        fgets(temp, MAXLINE, fp);
    }

    if (strcmp(temp, cmdline) && !check)
    {
        fprintf(fp, "%s", cmdline);
    }

    fclose(fp);
    memset(buf,0,sizeof buf);
    strcat(buf, cmdline);
    bg = parseline(buf, argv);

    if (argv[0] == NULL)
        return; /* Ignore empty lines */


    if(pipenum>0)
    {
        exec_pipe(argv);
    }
    else
    {
        if (!builtin_command(argv))
        { // quit -> exit(0), & -> ignore, other -> run
            
            if(fork()==0)
            {
                if (execvp(argv[0], argv) < 0)
            {
                printf("%s: Command not found.\n", argv[0]);
            }
            exit(0);
            }
            else
            {
                wait(NULL);
                return;
            }

            /* Parent waits for foreground job to terminate */
            if (!bg)
            {
                int status;
            }
            else // when there is backgrount process!
                printf("%d %s", pid, cmdline);
        }
    }
    return;
}

void exec_pipe(char **argv)
{
    int i, j = 0, k = 0;
    char ***cmdline = (char ***)malloc(sizeof(char **) * (pipenum + 1));
    for (i = 0; i <= pipenum; i++) {
        cmdline[i] = (char **)malloc(sizeof(char *) * MAXARGS);
        for (j = 0; j < MAXARGS; j++) {
            cmdline[i][j] = (char *)malloc(sizeof(char) * MAXLINE);
        }
    }

    int **fd = (int **)malloc(sizeof(int *) * pipenum);
    for (i = 0; i < pipenum; i++) {
        fd[i] = (int *)malloc(sizeof(int) * 2);
    }

    for (i = 0; i < pipenum; i++) {
        pipe(fd[i]);
    }

    j = 0;
    for (i = 0; argv[i] != NULL; i++) {
        if (!strcmp(argv[i], "|")) {
            cmdline[j][k] = NULL;   // 구분할 수 있도록 null 삽입
            j++;
            k = 0;
        } else {
            strcpy(cmdline[j][k], argv[i]); // 명령어 파싱
            k++;
        }
    }
    cmdline[j][k] = NULL; // 마지막 명령어 끝에 null 삽입

    for (j = 0; j <= pipenum; j++) {
        if (fork() == 0) {
            if (j > 0) {    // 첫번째는 쓰기만.
                dup2(fd[j - 1][0],0);
                close(fd[j - 1][0]);
                close(fd[j - 1][1]);
            }
            if (j < pipenum) { // 마지막은 읽기만.
                dup2(fd[j][1], 1);
                close(fd[j][0]);
                close(fd[j][1]);
            }

            if (!builtin_command(cmdline[j])) {
                if (execvp(cmdline[j][0], cmdline[j]) < 0) {
                    printf("%s: Command not found.\n", cmdline[j][0]);
                    exit(0);
                }
            }
            exit(0);
        } else {
            if (j > 0) {
                close(fd[j - 1][0]);
            }
            if (j < pipenum) {
                close(fd[j][1]);
            }
            wait(NULL);
        }
    }

    for (i = 0; i < pipenum; i++) {
        close(fd[i][0]);
        close(fd[i][1]);
    }
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv)
{

    if (!strcmp(argv[0], "quit")) /* quit command */
        exit(0);
    if (!strcmp(argv[0], "exit")) /* quit command */
        exit(0);
    if (!strcmp(argv[0], "&")) /* Ignore singleton & */
        return 1;

    if (!strcmp(argv[0], "cd"))
    {
        char *str = (char *)malloc(sizeof(char) * (strlen(argv[1]) + 3));
        strcat(str, "./");
        strcat(str, argv[1]);
        chdir(str);
        free(str);
        return 1;
    }
    if (!strcmp(argv[0], "history"))
    {
        FILE *fp = fopen("./command.log", "r");
        char cmdline[MAXLINE];
        int i = 1;
        while (fgets(cmdline, MAXLINE, fp) != NULL)
        {
            printf("%5d  %s", i++, cmdline);
        }
        fclose(fp);
        return 1;
    }

    return 0; /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv)
{
    char *delim; /* Points to first space delimiter */
    int argc;    /* Number of args */
    int bg;      /* Background job? */

    buf[strlen(buf) - 1] = ' ';   /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
        buf++;

    char temp[MAXLINE];
    memset(temp,0,sizeof temp);
    int cnt = 0;
    for (int i = 0; i < strlen(buf); i++)
    {
        if (buf[i] == '|')
        {
            pipenum++;
            temp[cnt++] = ' ';
            temp[cnt++] = '|';
            temp[cnt++] = ' ';
        }
        else if (buf[i] == '\'' || buf[i] == '"')
        {
            char standard = buf[i];
            while (buf[++i] != standard)
            {
                temp[cnt++] = buf[i];
            }
        }
        else
        {
            temp[cnt++] = buf[i];
        }
    }
    strcpy(buf,"");
    strcat(buf,temp);

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' ')))
    {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;

    if (argc == 0) /* Ignore blank line */
        return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0)
        argv[--argc] = NULL;


    return bg;
}
/* $end parseline */
