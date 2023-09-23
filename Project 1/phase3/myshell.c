/* $begin shellmain */
#include "myshell.h"
#include <errno.h>
#define MAXARGS 128
#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

void exec_pipe(char ** argv,int bg);
int check = 0;
int pipenum;
int pid_cnt=1;
int fpid_cnt = 1;
int pid_arr[MAXARGS];
int fpid_arr[MAXARGS];
char fpid_com[MAXARGS][MAXARGS];
char pid_com[MAXARGS][MAXARGS];
int pid_flag[MAXARGS] = {0, };
volatile sig_atomic_t sig_main = 0;

void sigset_setting(sigset_t *mask_a, sigset_t *mask_b, sigset_t *mask_f);
void handle_sigint(int sig);
void handle_sigtstp(int sig);
void handle_sigchld(int sig);

int main()
{  
    signal(SIGTSTP,handle_sigtstp);
    signal(SIGINT, handle_sigint);
    signal(SIGCHLD,handle_sigchld);

    char cmdline[MAXLINE]; /* Command line */
    while (1)
    {
        sig_main=0;
        printf("> ");
        check = 0;
        /* Read */
        fgets(cmdline, MAXLINE, stdin);
        if (feof(stdin))
            exit(0);

        sig_main=1;
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
                else if (cmdline[i + 1] >= '0' && cmdline[i + 1] <='9')
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

    if (strcmp(temp, cmdline) && !check && strcmp("", cmdline) && strcmp("\n", cmdline))
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
        exec_pipe(argv,bg);
    }
    else
    {
        if (!builtin_command(argv))
        { // quit -> exit(0), & -> ignore, other -> run
            if((pid=fork())==0)
            {
                if (execvp(argv[0], argv) < 0)
                    {
                        printf("%s: Command not found.\n", argv[0]);
                    }
                    exit(0);
                
            }
            else /* Parent waits for foreground job to terminate */
            {
                if(!bg)
                {
                    fpid_arr[fpid_cnt]= pid;
                    int i;
                    for (i = 0; argv[i] != NULL; i++) {
                        strcat(fpid_com[fpid_cnt], argv[i]);
                        if(argv[i+1]!= NULL)
                            strcat(fpid_com[fpid_cnt], " ");
                    }
                    strcat(fpid_com[fpid_cnt],"\n");
                    fpid_cnt++;

                    int status;
                    waitpid(pid, &status, WUNTRACED);
                    for(int i=1;i<fpid_cnt;i++)
                    {
                        if(fpid_arr[i]==pid)
                        {
                            fpid_arr[i]=-1;
                            break;
                        }
                    }
                }
                else
                {
                    
                    pid_arr[pid_cnt]= pid;
                    int i;
                    for (i = 0; argv[i] != NULL; i++) {
                        strcat(pid_com[pid_cnt], argv[i]);
                        if(argv[i+1]!= NULL)
                            strcat(pid_com[pid_cnt], " ");
                    }
                    strcat(pid_com[pid_cnt],"\n");
                    pid_cnt++;
                    
                }
            }
        }

        return;
    }

}
void exec_pipe(char **argv, int bg)
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

        if (!builtin_command(cmdline[j]))
        { // quit -> exit(0), & -> ignore, other -> run
            int pid;
            if((pid=fork())==0)
            {
                if (execvp(cmdline[j][0], cmdline[j]) < 0)
                    {
                        printf("%s: Command not found.\n", argv[0]);
                    }
                    exit(0);
                
            }
            else /* Parent waits for foreground job to terminate */
            {
                if(!bg)
                {
                    fpid_arr[fpid_cnt]= pid;
                    int i;
                    for (i = 0; argv[i] != NULL; i++) {
                        strcat(fpid_com[fpid_cnt], cmdline[i]);
                        if(cmdline[i+1]!= NULL)
                            strcat(fpid_com[fpid_cnt], " ");
                    }
                    strcat(fpid_com[fpid_cnt],"\n");
                    fpid_cnt++;

                    int status;
                    waitpid(pid, &status, WUNTRACED);
                    for(int i=1;i<fpid_cnt;i++)
                    {
                        if(fpid_arr[i]==pid)
                        {
                            fpid_arr[i]=-1;
                            break;
                        }
                    }
                }
                else
                {
                    
                    pid_arr[pid_cnt]= pid;
                    int i;
                    for (i = 0; cmdline[i] != NULL; i++) {
                        strcat(pid_com[pid_cnt], cmdline[i]);
                        if(argv[i+1]!= NULL)
                            strcat(pid_com[pid_cnt], " ");
                    }
                    strcat(pid_com[pid_cnt],"\n");
                    pid_cnt++;
                    
                }
            }
        }

            exit(0);
            } 
        else {
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
    sigset_t mask_one, prev_one;
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
    sigemptyset(&mask_one);
	sigaddset(&mask_one, SIGCHLD);			
	sigprocmask(SIG_BLOCK, &mask_one, &prev_one);

    if (!strcmp(argv[0], "kill"))
    {
        if(argv[1][0]== '%')
        {
            char num[MAXARGS];
            memset(num,0,sizeof num);
            strcpy(num,argv[1]+1);
            int curr = atoi(num);

            if(pid_flag[curr]==2 || pid_arr[curr]==0)
            {
                printf("No such Job\n");
            }
            else
            {
                kill(pid_arr[curr],SIGKILL);
                pid_flag[curr]=2;
            }
        }
        return 1;
    }
    if (!strcmp(argv[0], "bg"))
    {
        char num[MAXARGS];
        memset(num,0,sizeof num);
        strcpy(num,argv[1]+1);
        int curr = atoi(num);

        if(pid_flag[curr]==2 || pid_arr[curr]==0)
        {
            printf("No such Job\n");
        }
        else
        {
            kill(pid_arr[curr],SIGCONT);
            pid_flag[curr]=0;
        }
        return 1;
    }
    if (!strcmp(argv[0], "fg"))
    {
        char num[MAXARGS];
        memset(num,0,sizeof num);
        strcpy(num,argv[1]+1);
        int curr = atoi(num);
        
        if(pid_flag[curr]==2 || pid_arr[curr]==0)
        {
            printf("No such Job\n");
        }
        else
        {
            printf("[%d] running %s",curr,pid_com[curr]);
            eval(pid_com[curr]);
            pid_flag[curr]=2;
        }
        return 1;
    }

    sigprocmask(SIG_SETMASK,&prev_one,NULL);

    if (!strcmp(argv[0], "jobs"))
    {
        for(int i=1; i<pid_cnt;i++)
        {
            if(pid_flag[i]!=2)
            {
                if(pid_flag[i]==1)
                printf("[%d] suspended %s",i,pid_com[i]);
                if(pid_flag[i]==0)
                printf("[%d] running %s",i,pid_com[i]);
            }
        }
        return 1;
    }


    return 0; /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build ay */
int parseline(char *buf, char **argv)
{
    char *delim; /* Points to first space delim*/
    int argc;    /* Number of args */
    int bg;      /* Background job? */

    buf[strlen(buf) - 1] = ' ';   /* Replace tr ith space */
    while (*buf && (*buf == ' ')) /* Ignore lea */
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
        else if (buf[i-1]!=' ' && buf[i]=='&')
        {
            temp[cnt++] = ' ';
            temp[cnt++] = '&';
        }
        else if (buf[i] == '\'' || buf[i] == '\"') 
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
        while (*buf && (*buf == ' ')) /* Ignore*/
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

void handle_sigint(int sig) {
    int temp_errno = errno;
    
    sigset_t mask, prev;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK,&mask,&prev);

    if(!sig_main)
        sio_puts("\n> ");
    else
        sio_puts("\n");
    for(int i=1;i<fpid_cnt;i++)
    {
        if(fpid_arr[i]!=-1)
        {    
            kill(fpid_arr[i],SIGKILL);
            fpid_arr[i]=-1;
        }
    }
    sigprocmask(SIG_SETMASK, &prev, NULL);
    errno = temp_errno;
}

void handle_sigtstp(int sig){
    int temp_errno = errno;
    
    sigset_t mask, prev;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK,&mask,&prev);
    sio_puts("\n");
    for(int i=1;i<fpid_cnt;i++)
    {
        if(fpid_arr[i]!=-1)
        {    
            kill(fpid_arr[i],SIGSTOP);
            pid_arr[pid_cnt] = fpid_arr[i];
            strcpy(pid_com[pid_cnt],fpid_com[i]);
            pid_flag[pid_cnt] = 1;
            fpid_arr[i]=-1;
            pid_cnt++;
        }
    }
    sigprocmask(SIG_SETMASK, &prev, NULL);
    errno = temp_errno;
}

void handle_sigchld(int sig)
{	
	int temp_errno = errno;
	sigset_t mask, prev;
    pid_t pid;
    sigfillset(&mask);

    while((pid=waitpid(-1,NULL, WNOHANG))>0)
    {
        sigprocmask(0,&mask,&prev);
        for(int i=1;i<pid_cnt;i++)
        {
            if(pid_arr[i] == pid)
            {
                pid_arr[i]=-1;
                pid_flag[i]=2;
                break;	
            }
        }
        sigprocmask(SIG_BLOCK,&prev,NULL);
        
    }
	errno = temp_errno;
}
