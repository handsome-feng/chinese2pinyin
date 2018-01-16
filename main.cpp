/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/04/2015 12:03:25 AM
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/file.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <iostream>
#include <list>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/inotify.h>

#include "chinese2pinyin.h"

#define PIDFILE "/dev/shm/pinyin.pid"

const char *socket_path = "\0hidden";
int watch_fd;

list <string> gPath;
list <string> gDefault;

void *thread_index(void*)
{
    signal (SIGINT,my_handler);
    signal (SIGTERM,my_handler);
    signal (SIGSEGV,my_handler);

    int pid_file = open(PIDFILE, O_CREAT | O_RDWR, 0666);
    int rc = flock(pid_file, LOCK_EX | LOCK_NB);
    if(rc) {
        if(EWOULDBLOCK == errno)
        fprintf(stderr, "Another instance is running!\n");
        exit(1);
    }

    if (init()) {
        perror("table.dat");
        exit(-2);
    }

    gDefault.push_back (getHomePath() + "/Document");
    gDefault.push_back (getHomePath() + "/文档");
    list <string>::iterator it;

    char buffer[1024];
    int len, wd;
    watch_fd = inotify_init ();
    if (watch_fd < 0) {
        perror ("Fail to initialize inotify.\n");
        exit (-2);
    }

    for (it = gDefault.begin(); it != gDefault.end(); it++) {
        wd = inotify_add_watch(watch_fd, (*it).c_str(), IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);
        if (wd < 0) {
            printf ("Can't add watch for %s.\n", (*it).c_str());
            it = gDefault.erase(it);
            continue;
        }
    }

    do
    {
#ifdef PERMANENT_ADD
        if (gPath.size()) {
            for (it = gPath.begin(); it != gPath.end(); it++) {
                indexFile(*it);
            }
            gPath.clear();
        } else {
#endif
            for (it = gDefault.begin(); it != gDefault.end(); it++) {
                indexFile(*it);
            }
            updateDB();
#ifdef PERMANENT_ADD
        }
#endif
    } while (len = read (watch_fd, buffer, 1024));
}

void* srv(void*)
{
  struct sockaddr_un addr;
  char buf[40960];
  int fd,cl,rc;

  if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);

  unlink(socket_path);

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind error");
    exit(-1);
  }

  if (listen(fd, 5) == -1) {
    perror("listen error");
    exit(-1);
  }

  while (1) {
    if ( (cl = accept(fd, NULL, NULL)) == -1) {
      perror("accept error");
      continue;
    }

    while ( (rc=read(cl,buf,sizeof(buf))) > 0) {
        string dir(buf);
        if (dir.size()){
            //dir.erase(dir.end()-1);
#ifdef PERMANENT_ADD
            int wd = inotify_add_watch(watch_fd, dir.c_str(), IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);
            if (wd < 0) {
                printf ("Can't add watch for %s.\n", dir.c_str());
            }
            gPath.push_back(dir);
#endif
            indexFile(dir);
        }

        memset(buf,0,sizeof(buf));
    }
    if (rc == -1) {
      perror("read");
      exit(-1);
    }
    else if (rc == 0) {
//      printf("EOF\n");
      close(cl);
    }
  }
}

void *cli(const char* argv)
{
    struct sockaddr_un addr;
    int fd;


    if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        cout << "please run daemon first" << endl;
        exit(-1);
    }

    write(fd, argv, strlen(argv));
    return 0;
}

int main(int argc, char* argv[])
{
    pthread_t thread1, thread2;
    int  iret1, iret2, i;

    if (argc >= 2 && (strcmp(argv[1], "-d")== 0 || (strcmp(argv[1], "-f")== 0)) ) {

        if (strcmp(argv[1], "-d")== 0 ){
                pid_t pid;
                int i;

                pid = fork ( );  
                if (pid == -1)  
                return -1;  
                else if (pid != 0)  
                exit (EXIT_SUCCESS);  

                if (setsid ( ) == -1)  
                return -1;  

                if (chdir ("/") == -1)  
                return -1;  

                for (i = 0; i < 1024; i++)  
                close (i);  

                open ("/dev/null", O_RDWR);  
                dup (0);  
                dup (0);  
        }

        iret1 = pthread_create( &thread1, NULL, thread_index, NULL);
        iret2 = pthread_create( &thread2, NULL, srv, NULL);
        pthread_join( thread1, NULL);
        pthread_join( thread2, NULL);
    }
    else if (argc >= 2 && (strcmp(argv[1], "-i") == 0)){
        if (argc >= 3){
            for (i = 2; i < argc; i++)
                cli(argv[i]);
        }
        else
            cout << "Please check your arguments" << endl;
    }
    else {
        cout << "chinese2pinyin version 1.0" << endl;
        cout << "    -d \tRun as deamon" << endl;
        cout << "    -f \tRun in foreground" << endl;
        cout << "    -i \tIndex a directory to database (using full path)" << endl;
        cout << "    -h \tPrint this help" << endl;
        cout << endl;

        return 1;
    }

    return 0;
}

