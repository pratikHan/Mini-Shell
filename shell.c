#include<stdlib.h>
#include<stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <time.h>
#include <fcntl.h>
#define B_SIZE 1000
#define I_BUF 100
#define A_Max 10


struct instruction
{
    char * argval[I_BUF];
    int instr_count;
};
typedef struct instruction instruction;

char *input,*input1;
//char input[1000];
int exit_f = 0;
int pid_f,fd[2];
char cwd[B_SIZE];
char* argval[A_Max]; // our local argc, argv
int instr_count = 0,bg_flag = 0;
int ex_in=0,ex_out=0;
char ip_file[I_BUF],op_file[I_BUF];

void getinput_cmds();
void getpwd(char*, int);
void getcd(char*);
void nameFile(struct dirent*, char*);
void executable();
void dup_files(int, instruction*);
void run_process(int, int, instruction*);


void Signal_handler()
{


}

int main(int argc, char* argv[])
{
    signal(SIGINT,Signal_handler);
    int i;
    int pipe1 = pipe(fd);

  //  screenfetch();
    getpwd(cwd,0);

    while(exit_f==0)
    {
      //get input
        ex_in = 0; ex_out = 0;bg_flag = 0;
        printf("%s minish> ",cwd );

        getinput_cmds();


        if(strcmp(argval[0],"exit")==0)
        {
          exit_f = 1;
          return 0;
        }


        else if(strcmp(argval[0],"pwd")==0 && !bg_flag)
        {
            getpwd(cwd,1);
        }
        else if(strcmp(argval[0],"cd")==0 && !bg_flag)
        {
            char* path = argval[1];
            getcd(path);
        }

        else if(strcmp(argval[0],"ls")==0 && !bg_flag)
        {
            char* opt = argval[1];

             //ls implementation

             int i=0;
             struct dirent **listr;
             int ln = scandir(".", &listr, 0, alphasort);
             if (ln >= 0)
             {
                 printf("Total %d items in this directory\n",ln-2);
                 for(i = 0; i < ln; i++ )
                 {
                     if(strcmp(listr[i]->d_name,".")==0 || strcmp(listr[i]->d_name,"..")==0)
                     {
                         continue;
                     }
                     else nameFile(listr[i],"    ");
                     if(i%8==0) printf("\n");
                 }
                 printf("\n");
             }
             else
             {
                 perror ("Error in ls ");
             }

        }

        else
        {
          //  executable();
            instruction cmd[I_BUF];
            int i=0,j=1,status;
            char* current = strsep(&input1," \t\n");

            cmd[0].argval[0] = current;

            while(current!=NULL)
            {
                current = strsep(&input1, " \t\n");
                if(current==NULL)
                {
                    cmd[i].argval[j++] = current;
                }

                else if(strcmp(current,"<")==0)
                {
                    ex_in = 1;
                    current = strsep(&input1, " \t\n");
                    strcpy(ip_file, current);
                }
                else if(strcmp(current,">")==0)
                {
                    ex_out = 1;
                    current = strsep(&input1, " \t\n");
                    strcpy(op_file, current);
                }
                else if(strcmp(current, "&")==0)
                {
                    bg_flag = 1;
                }
                else
                {
                    cmd[i].argval[j++] = current;
                }
            }
            cmd[i].argval[j++] = NULL; // handle last cmd separately
            cmd[i].instr_count = j;
            i++;

            // parent process waits for execution and then reads from terminl
            pid_f = fork();
            if(pid_f == 0)
            {
                dup_files(i, cmd);
            }
            else
            {
                if(bg_flag==0)
                {
                    waitpid(pid_f,&status, 0);
                }
                else
                {
                    printf(" Process running in Background. PID:%d\n",pid_f);
                }
            }
            pid_f = 0;
            free(input1);
        }

    }

}



void getinput_cmds(){
  fflush(stdout);
  input = NULL;
  ssize_t buf = 0;
  getline(&input,&buf,stdin);

  input1 = (char *)malloc(strlen(input) * sizeof(char));
  strncpy(input1,input,strlen(input));
  instr_count = 0;bg_flag = 0;
  while((argval[instr_count] = strsep(&input, " \t\n")) != NULL && instr_count < A_Max-1)
  {
      instr_count++;
      if(strcmp(argval[instr_count-1],"&")==0)
      {
          bg_flag = 1;
          return ;
      }
  }
  free(input);

}


void runprocess(char * client, char* args[], int count)
{

    int ret = execvp(client, args);
    char* pathx;
    pathx = getenv("PATH");// check for all paths , executing non./
    char path[1000];
    strcpy(path, pathx);
    strcat(path,":");
    strcat(path,cwd);
    char * cmd = strtok(path, ":\r\n");
    while(cmd!=NULL)
    {
       char l_sort[1000];
        strcpy(l_sort, cmd);
        strcat(l_sort, "/");
        strcat(l_sort, client);
        printf("execvp : %s\n",l_sort );

        if(execvp(l_sort, args)==-1)
        {
            perror("Error in running executable ");
            exit(0);
        }
        cmd = strtok(NULL, ":\r\n");
    }
}



void dup_files(int n, instruction* cmd)
{
    int in = 0,fd[2], i;
    int pid, status,pipest;

    if(ex_in)
    {
        in = open(ip_file, O_RDONLY); // open the file
        if(in < 0)
        {
            perror("Error in executable : input file ");
        }
    }


    for (i = 1; i < n; i++)
    {
      pipe (fd);
        int id = fork();
        if (id==0)
        {

            if (in!=0)
            {
                dup2(in, 0);
                close(in);
            }
            if (fd[1]!=1)
            {
                dup2(fd[1], 1);
                close(fd[1]);
            }

            //printf("dup_files send %s to runprocess\n",cmd[i].argval[0]);
            runprocess(cmd[i-1].argval[0], cmd[i-1].argval,cmd[i-1].instr_count);
            exit(0);

        }
        else wait(&pipest);
        close(fd[1]);
        in = fd[0];
    }
    i--;

    if(in != 0)
    {
        dup2(in, 0);
    }
    if(ex_out)
    {
        int ofd = open(op_file, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        dup2(ofd, 1);
    }

    runprocess(cmd[i].argval[0], cmd[i].argval, cmd[i].instr_count);
}

void nameFile(struct dirent* name,char* fp)
{
  printf("%s/%s", name->d_name, fp);

}

/*change directory functionality*/
void getcd(char* path)
{
  if(chdir(path)==0) // path could be changed if cd successful
    {
        getpwd(cwd,0);
    }
    else perror(" Error in cd ");
}


/*Implement basic exit*/


/* Implement pwd function in shell - 1 prints, 0 stores*/
void getpwd(char* cwdstr,int cmd)
{
    char t[B_SIZE];
    char* path=getcwd(t, sizeof(t));
    if(path != NULL)
    {
        strcpy(cwdstr,t);
        if(cmd==1)  // check if pwd is to be printed
        {
            printf("%s\n",cwdstr);
        }
    }
    else perror("Error in getcwd() : ");

}
