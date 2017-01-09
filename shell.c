#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include "parse.h"

#define _GNU_SOURCE


extern char **environ;
char **tempenv;

int pc=0,pe=0,tp=0;
int f[2],f1[2],isPipe=0;


static void prCmd(Cmd c)
{
  int i;
  char curDir[100];
  char str[1000];
  const char *s1;
  char *s,*tkn,*stemp;
  int fd,fdin,in;
  int nice_val,err=0;
  int old_stdout,old_stderr,old_stdin,output_redirect=5,error_redirect=5,input_redirect=5;
  char *bin[8]={"nice","echo","cd","logout","pwd","setenv","unsetenv","where"};

  pid_t curr_pid,pid,pid1,pid2,ppid1,ppid2;
  int status,status1;

  if ( c ) {
    //printf("%s%s ", c->exec == Tamp ? "BG " : "", c->args[0]);
    pc;pe=0;


    if ( c->in == Tin )
    {
      //printf("<(%s) ", c->infile);
      old_stdin = dup(0);
      fdin = open(c->infile,O_CREAT | O_RDWR,S_IRWXU);
      input_redirect=dup2(fdin,0);
    }
    if ( c->out != Tnil )
    switch ( c->out ) {
      case Tout:
      //printf(">(%s) ", c->outfile);
      old_stdout = dup(1);
      fd = open(c->outfile,O_CREAT | O_RDWR,S_IRWXU);
      output_redirect=dup2(fd,1);
      break;
      case Tapp:
      //printf(">>(%s) ", c->outfile);
      old_stdout = dup(1);
      fd = open(c->outfile,O_CREAT | O_RDWR | O_APPEND,S_IRWXU);
      output_redirect=dup2(fd,1);
      break;
      case ToutErr:
      //printf(">&(%s) ", c->outfile);
      old_stdout = dup(1);
      old_stderr = dup(2);
      fd = open(c->outfile,O_CREAT | O_RDWR,S_IRWXU);
      output_redirect=dup2(fd,1);
      error_redirect=dup2(fd,2);
      break;
      case TappErr:
      //printf(">>&(%s) ", c->outfile);
      old_stdout = dup(1);
      old_stderr = dup(2);
      fd = open(c->outfile,O_CREAT | O_RDWR | O_APPEND,S_IRWXU);
      output_redirect=dup2(fd,1);
      error_redirect=dup2(fd,2);
      break;
      case Tpipe:
      //printf("| (%d)%s ",++pc,c->args[0]);
      pc++;

      break;
      case TpipeErr:
      pe++;
      break;
      default:
      fprintf(stderr, "Shouldn't get here\n");
      exit(-1);
    }

    //if(pc)
    //;//execlp(c->args[0],c->args[1]);
    getcwd(curDir, 100);

    //if command is nice
    if(!strcmp(c->args[0], "nice"))
    {
      if(c->nargs>1)
      {
        if(c->nargs>2)
        {
          curr_pid=getpid();
          nice_val=getpriority(PRIO_PROCESS,curr_pid);
          nice_val = atoi( c->args[1] );
          setpriority(PRIO_PROCESS,curr_pid,nice_val);
          if(nice_val!=0 || strcmp(c->args[1],"0")==0)
          {
            setpriority(PRIO_PROCESS,curr_pid,nice_val);
            for ( i = 0; c->args[i+2] != NULL; i++ )
            strcpy(c->args[i],c->args[i+2]);
            c->args[i]='\0';
          }
          else
          {
            nice_val = 4;
            setpriority(PRIO_PROCESS,curr_pid,nice_val);
            for ( i = 0; c->args[i+1] != NULL; i++ )
            strcpy(c->args[i],c->args[i+1]);
            c->args[i]='\0';
          }
        }

        else
        {
          curr_pid=getpid();
          nice_val=getpriority(PRIO_PROCESS,curr_pid);
          nice_val = atoi( c->args[1] );
          if(nice_val!=0 || c->args[1]!="0")
          setpriority(PRIO_PROCESS,curr_pid,nice_val);
          else
          {
            nice_val = 4;
            setpriority(PRIO_PROCESS,curr_pid,nice_val);
          }
          for ( i = 0; c->args[i+1] != NULL; i++ )
          strcpy(c->args[i],c->args[i+1]);
          c->args[i]='\0';
        }
      }
      //putchar('\n');
    }

    //if command is cd
    else if(!strcmp(c->args[0], "cd"))
    {
      if(c->nargs>1)
      {
        chdir(c->args[1]);
        getcwd(curDir, 100);
        setenv("PWD",curDir,1);
      }
      else
      {
        chdir(getenv("HOME"));
        getcwd(curDir, 100);
        setenv("PWD",curDir,1);
      }
      //putchar('\n');
    }

    //if command is setenv
    else if(!strcmp(c->args[0], "setenv"))
    {
      if(c->nargs>1)
      {
        if(c->nargs>2)
        {
          setenv(c->args[1],c->args[2],1);
        }
        else
        {
          setenv(c->args[1],NULL,1);
        }
      }
      else
      {
        tempenv=environ;
        while(*tempenv!=NULL)
        {
          printf("\n%s",*tempenv);
          *tempenv++;
        }
      }
      putchar('\n');
    }

    //if command is unsetenv
    else if(!strcmp(c->args[0], "unsetenv"))
    {
      unsetenv(c->args[1]);
      putchar('\n');
    }

    //if command is echo
    else if(!strcmp(c->args[0], "echo"))
    {
      if(c->args[1]==NULL)
      {;}
      else
      {
        for ( i = 1; c->args[i] != NULL; i++)
        {
          if(i==1)
          strcpy(str,"");
          else
          strcat(str," ");
          strcat(str,c->args[i]);
        }
        printf("%s",str);
        putchar('\n');
      }
    }

    //get the present working directory
    else if(!strcmp(c->args[0], "pwd"))
    {
      getcwd(curDir, 100);
      printf("%s",curDir  );
      putchar('\n');
    }

    //if command is where
    else if(!strcmp(c->args[0], "where"))
    {
      for(i=0;i<8;i++)
      {
        if(!strcmp(bin[i],c->args[1]))
        printf("/bin/%s\n",bin[i]);
      }
      s1 = (char*)malloc(1000 * sizeof(char));
      s1=getenv("PATH");
      s = (char*)malloc(1000 * sizeof(char));
      strcpy(s,s1);
      tkn = (char*)malloc(1000 * sizeof(char));
      stemp = (char*)malloc(1000 * sizeof(char));

      tkn = strtok(s,":");

      while( tkn != NULL )
      {
        strcpy(stemp,tkn);
        strcat(stemp,"/");
        strcat(stemp,c->args[1]);
        if(access(stemp,F_OK)==0)
        printf( "%s\n", stemp);

        tkn = strtok(NULL,":");
      }

      //putchar('\n');
    }

    // exit command
    else if ( !strcmp(c->args[0], "logout") || !strcmp(c->args[0], "end")){
      exit(0);}

      else
      {
        if(pc)
        pipe(f);
        tp=pc;

        pid = fork();

        if( pid < 0)
        {
          printf("Error occured");
          exit(-1);
        }

        else if(pid == 0)
        {
          if(pc)
          {close(f[0]);		//close input of pipe
            dup2(f[1], fileno(stdout));
            close(f[1]);
          }
          execvp(c->args[0],c->args);

        }
        else
        {

          {
            waitpid(pid, &status1, 0);		//wait for process 1 to finish
            pid2 = fork();
            if(pid2 < 0)
            {
              printf("error in forking");
              exit(-1);
            }
            else if(pid2 == 0)
            {
              close(f[1]);		//close output to pipe
              dup2(f[0], fileno(stdin));
              close(f[0]);
              execvp(c->args[0],c->args);
            }
            else
            {
              ;//wait(NULL);
              //waitpid(pid, &status1, 0);
              //waitpid(pid2, &status2, 0);
              close(f[0]);
              close(f[1]);
            }
          }

        }
      }

      /*if ( c->nargs > 1 ) {
      printf("[");
      for ( i = 1; c->args[i] != NULL; i++ )
      printf("%d:%s,", i, c->args[i]);
      printf("\b]");
    }*/



  }


  if(output_redirect==1)
  {
    close(fd);
    dup2(old_stdout,1);
    close(old_stdout);
    if(error_redirect==2)
    {
      dup2(old_stderr,2);
      close(old_stderr);
    }
  }

  if(input_redirect==0)
  {
    close(fdin);
    dup2(old_stdin,0);
    close(old_stdin);
    input_redirect=5;
  }
}

static void prPipe(Pipe p)
{
  int i = 0;
  Cmd c;
  int status;
  pid_t pid2;

  if ( p == NULL )
  return;

  //printf("Begin pipe%s\n", p->type == Pout ? "" : " Error");
  for ( c = p->head; c != NULL; c = c->next ) {
    //printf("  Cmd #%d: ", ++i);
    prCmd(c);
    isPipe=1;
  }

  /*do{
  pid2=waitpid(-1,&status,0);
}while(pid2!=-1);*/

isPipe=0;

// printf("End pipe\n");
prPipe(p->next);
}

int main(int argc, char *argv[])
{
  Pipe p;
  char hostname[1024];
  int os,fd;

  os = dup(0);
  fd = open("~/.ushrc",O_RDONLY,S_IRWXU);
  dup2(fd,0);
  p = parse();
  prPipe(p);
  freePipe(p);
  close(fd);
  dup2(os,0);
  close(os);


  while ( 1 ) {
    //getcwd(curDir, 100);

    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    printf("%s%%", hostname);
    fflush(stdout);

    //printf("%s%%",getlogin());
    p = parse();
    prPipe(p);
    freePipe(p);
  }
}

/*........................ end of main.c ....................................*/
