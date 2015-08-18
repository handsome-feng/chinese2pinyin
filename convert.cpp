/*
 * =====================================================================================
 *
 *       Filename:  convert.cpp
 *
 *    Description:  convert Chinese to pinyin
 *
 *        Version:  1.0
 *        Created:  11/20/2014 12:38:55 AM
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  FJKong
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <list>
#include <cstring>

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/file.h>
#include <errno.h>

#include <sqlite3.h> 

#define PIDFILE "/dev/shm/pinyin.pid"

using namespace std;

#define START 0x3400
#define END   0x9fff
#define DATA  "table.dat"
string dict[0x9fff][10] = {};

//vedis *pStore;          /* Datastore handle */
sqlite3 *db;
int rc;
bool bHasChinese = false;

std::list <string> parse(const char* c, int len)
{
    list <string> ret;
    if (len < 1)
        return ret;

    char ch[0xf]= "";

    for(int i = 0; i < len; i++) {
        if(c[i] & 0x80) {
            memset(ch, 0, 0xf);
            memcpy(ch, &c[i], 3);
            string tmp(ch);
            ret.push_back(tmp);
            i += 2;
            bHasChinese = true;
        }
        else {
            memset(ch, 0, 0xf);
            memcpy(ch, &c[i], 1);
            string tmp(ch);
            ret.push_back(tmp);
        }
    }

    return ret;
}

list<string> getCandPinYin(string target)
{
    list <string> pinyin;
    string cand;
    for(int i = START; i < END; i++) {
        if (target == dict[i][0]) {
//           cout << "Chinese is : " << dict[i][0] << endl;
            for(int j = 1; j < 10; j++) {
                if (dict[i][j] != "") {
                    cand = dict[i][j];
//                    cout << "cand  is : " << cand << endl;
                    pinyin.push_back(cand);
                }
            }
        }
    }

    if(pinyin.empty()){
//        cout << "no need convert for en : " << target << endl;
        pinyin.push_back(target);
    }

    return pinyin;
}

int combin(list<string>& dst, list<string> arg)
{
    string tmp;
    list <string> result;
    list <string>::iterator itDst, itArg;

    if(!dst.size()){
        dst = arg;
        return 0;
    }
#if 0
    //print
    for (itDst = dst.begin(); itDst != dst.end(); itDst++) {
        cout << "dst : " << *itDst << endl;
    }
#endif
    for (itArg = arg.begin(); itArg != arg.end(); itArg++) {
 //       cout << "arg : " << *itArg<<endl;
        for (itDst = dst.begin(); itDst != dst.end(); itDst++) {
            tmp = *itDst + *itArg;
            result.push_back(tmp);
//            cout << "dst : " << *itDst<<endl;
        }
    }
    dst.clear();
    dst = result;
    return 0;
}

int init()
{
    int index;
    int cur;
    string value;
    ifstream infile(DATA);
    string line;

    while (getline(infile, line))
    {
        std::stringstream strm(line);
        //get index
        strm >> hex >> index;
        //get Chinese character
        strm >> value;
        dict[index][0] = value;
        cur = 0;
#if 0
        if(index == 0x3437 || index == 0x9002) {
            cout << hex << index << "value: "<< value << endl;
        }
#endif
        //found
        while (!strm.eof()) {
            cur++;
            strm >> value;
            dict[index][cur] = value;
#if 0
            if(index == 0x3437 || index == 0x9002) {
                cout << hex << index << "value: "<< value << endl;
            }
#endif

        }
    }
    return 0;
}
static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

int indexFile(string indexPath)
{

  FILE *fp;
  char filePath[2048];
  string sql;
  char *zErrMsg = 0;

  /* Open the command for reading. */
  fp = popen(("locate "+ indexPath).data(), "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }
	/* Open our datastore */
    rc = sqlite3_open("v.db", &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(0);
    }else{
//        fprintf(stderr, "Opened database successfully\n");
    }

  /* Read the output a line at a time - output it. */
  while (fgets(filePath, sizeof(filePath), fp) != NULL) {

#if 1
      list <string> result;
      list <string>::iterator it;
      bHasChinese = false;
      //FIXME: length -1
      result = parse(filePath, strlen(filePath)-1);

      if (!bHasChinese)
          continue;

      list <string> fin;
      for (it = result.begin(); it != result.end(); it++) {
 //         cout << "*it: "<< *it << " " << endl;
          combin(fin, getCandPinYin(*it));

      }

      //save to database 
      //print result
      for (it = fin.begin(); it != fin.end(); it++) {
          //cout << "fin:" << *it  << " length is : " << (*it).length() << "  data   " << strlen((*it).data())<< endl;
//          cout << "*it " << (*it).data() << "filePath " << filePath;

          //sql = "INSERT INTO dashpinyin (pinyin,chinese) VALUES('insert','done');";
          
          stringstream str;
          string path(filePath);
          str << "INSERT INTO dashpinyin (pinyin,chinese) VALUES('" << (*it).data() << "','" <<path.substr(0, path.length() - 1).data() << "');";
          //cout << str.str().c_str()<< endl;
#if 1
          /* Execute SQL statement */
          rc = sqlite3_exec(db, str.str().c_str(), callback, 0, &zErrMsg);
          if( rc != SQLITE_OK ){
              fprintf(stderr, "SQL error: %s\n", zErrMsg);
              sqlite3_free(zErrMsg);
          }else{
              //fprintf(stdout, "Records created successfully\n");
          }
#endif

      }


  #endif
  }//end of while

  pclose(fp);

  sqlite3_close(db);
  return 0;
}

void my_handler(int s){
    unlink(PIDFILE);
    exit(1);
}

int main(int argc, char* argv[])
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

    init();

    if (argc > 1)
        string testcase = argv[1];
    else 
        string testcase = "/home";
    
    indexFile(argv[1]);
    return 0;
}
