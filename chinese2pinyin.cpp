/*
 * =====================================================================================
 *
 *       Filename:  chinese2pinyin.cpp
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
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <list>
#include <cstring>

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>

#include <sqlite3.h> 
#include "chinese2pinyin.h"

#define PIDFILE "/dev/shm/pinyin.pid"
#define DBDIR "/.pinyinsearch"
#define DBFILE "/.pinyinsearch/pinyin.db"

using namespace std;

#define START 0x3400
#define END   0x9fff
#define DATA  "/usr/share/chinese2pinyin/data/table.dat"
string dict[0x9fff][10] = {};

//vedis *pStore;          /* Datastore handle */
sqlite3 *db;
int rc;
bool bHasChinese = false;

bool fileExists(string filename); 

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
    cout << "dst size : " << dst.size() << "     arg size : " << arg.size() << endl;
#endif
    if(dst.size()>500) {
        return 0;
    }

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

    if(infile.fail()){
        return -2;
    }


    if (!fileExists(getHomePath() + DBDIR))
    {
        const int dir_err = mkdir((getHomePath() + DBDIR).data(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (-1 == dir_err)
        {
            printf("Error creating directory");
            exit(1);
        }
    }

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

static int cnt = 0;
static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
      cnt = atoi(argv[0]);
//      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
//   printf("\n");
   return 0;
}

static int updateDBCallback(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   stringstream s;
   char *zErrMsg = 0;

   for(i=0; i<argc; i++){
      //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
       string file(argv[i]);
       if (!fileExists(argv[i])){
           s << "delete from dashpinyin where chinese = '" <<  argv[i] <<  "';";
           rc = sqlite3_exec(db, s.str().c_str(), 0, 0, &zErrMsg);
           if( rc != SQLITE_OK ){
               fprintf(stderr, "SQL error: %s\n", zErrMsg);
               sqlite3_free(zErrMsg);
           }
       }
   }
   //printf("\n");
   return 0;
}

bool is_dir(const char* path) {
    struct stat buf;
    stat(path, &buf);
    return S_ISDIR(buf.st_mode);
}

string getHomePath()
{
    const char* home = getenv("HOME");
    string ret;

    if (home) {
        return string(home);
    }
    else
        return ret;
}

bool fileExists(string filename) {
    struct stat fileInfo;
    return stat(filename.c_str(), &fileInfo) == 0;
}

bool creatTable()
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int  rc;
    const char *sql;

    /* Open database */
    rc = sqlite3_open((getHomePath()+DBFILE).data(), &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return false;
    }else{
        /* Create SQL statement */
        sql = "CREATE TABLE dashpinyin (" \
                    "pinyin TEXT NOT NULL PRIMARY KEY," \
                    "chinese TEXT NOT NULL );";

        /* Execute SQL statement */
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
        if( rc != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }else{
            //fprintf(stdout, "Table created successfully\n");
        }
        sqlite3_close(db);

        return true;
    }

}

int openDB()
{
	/* Open our datastore */
    rc = sqlite3_open((getHomePath() + DBFILE).data(), &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(0);
    }else{
        return rc;
    }
}

int indexFile(string indexPath)
{
  FILE *fp;
  char filePath[2048];
  string sql;
  char *zErrMsg = 0;

  string cmd = "updatedb --require-visibility 0 -o " + getHomePath() + DBDIR + "/locate.db";
  system(cmd.data());
  /* Open the command for reading. */
  cmd = "locate  " + indexPath + " --database=" + getHomePath() + DBDIR + "/locate.db";
  fp = popen(cmd.data(), "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  if (fileExists((getHomePath()+DBFILE).data())){
      openDB();
  }
  else{
      if (!creatTable())
          exit(-2);
      else{
          openDB();
      }
  }

  /* Read the output a line at a time - output it. */
    while (fgets(filePath, sizeof(filePath), fp) != NULL) {

#if 1
        list <string> result;
        list <string>::iterator it;
        bHasChinese = false;
        //FIXME: length -1
cout << filePath;
        stringstream s;
        string pathstr(filePath);
        if(is_dir(pathstr.substr(0, pathstr.length() - 1).data())) {
            continue;
        }

        s << "select count(*) from dashpinyin where chinese='" << pathstr.substr(0, pathstr.length() - 1).data() << "';";
        rc = sqlite3_exec(db, s.str().c_str(), callback, 0, &zErrMsg);
        if (cnt > 0){
            continue;
        }

      result = parse(filePath, strlen(filePath)-1);

      if (!bHasChinese)
          continue;

      list <string> fin;
      for (it = result.begin(); it != result.end(); it++) {
 //         cout << "*it: "<< *it << " " << endl;
          combin(fin, getCandPinYin(*it));

      }
      //too many result
      if(fin.size()>100) {
          continue;
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

int updateDB()
{
    openDB();
    stringstream s;
    char *zErrMsg = 0;
    s << "select DISTINCT chinese from dashpinyin;";
    rc = sqlite3_exec(db, s.str().c_str(), updateDBCallback, 0, &zErrMsg);

    sqlite3_close(db);
    return 0;
}

void my_handler(int s){
    unlink(PIDFILE);
    exit(1);
}
#if 0
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

    string dst;
    if (argc > 1)
        dst = argv[1];
    else 
        dst = getHomePath() + "/Document";
    
    indexFile(dst);
    updateDB();

    return 0;
}
#endif
