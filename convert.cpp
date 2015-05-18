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

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "cdb.h"


using namespace std;

#define START 0x3400
#define END   0x9fff
#define DATA  "table.dat"
string dict[0x9fff][10] = {};

struct cdb_make cdbm;
int fd;

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

int find()
{
    struct cdb cdb;
    char key[40] = "/tmp/test";
    char *val;
    unsigned int vlen, vpos, len = 0;

#if 0
    fd = open("demo.db", O_RDONLY);
    if(fd < 0)
        return -1;

    fd = open("demo.db", O_RDONLY);
    if (cdb_seek(fd, key, strlen(key), &vlen) > 0) {
        /* if key was found, file will be positioned to the
         * start of data value and it's length will be placed to vlen */
        val = (char*)malloc(vlen);
        cdb_bread(fd, val, len); /* read the value;
                                  * plain read() will do as well. */
        /* handle the value */
        printf("value is %s\n", val);
    }

    struct cdb_find cdbf; /* structure to hold current find position */
    cdb_findinit(&cdbf, &cdb, key, strlen(key)); /* initialize search of key */
    while(cdb_findnext(&cdbf) > 0) {
        vpos = cdb_datapos(&cdb);
        vlen = cdb_datalen(&cdb);
        val = (char*)malloc(vlen);
        cdb_read(&cdb, val, vlen, vpos);
        printf("value is %s\n", val);
        /* handle the value */
        free(val);
    }
#endif
#if 1
    cdb_init(&cdb, fd); /* initialize internal structure */
    if (cdb_find(&cdb, key, strlen(key)) > 0) { /* if search successeful */
        vpos = cdb_datapos(&cdb); /* position of data in a file */
        vlen = cdb_datalen(&cdb); /* length of data */
        val = (char*)malloc(vlen+1); /* allocate memory */
        memset(val,0, vlen);
        cdb_read(&cdb, val, vlen, vpos); /* read the value into buffer */
        printf("value is %s\n", val);
    }
    else{

        printf("not found key is %s\n", key);
        printf("keylen is %ld\n", strlen(key));
    }
#endif
    return 0;
}

int indexFile(string indexPath)
{

  FILE *fp;
  char filePath[2048];

  /* Open the command for reading. */
  fp = popen(("locate "+ indexPath).data(), "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  fd = open("tmpfile", O_RDWR|O_CREAT, 0666);
  cdb_make_start(&cdbm, fd);
  /* Read the output a line at a time - output it. */
  while (fgets(filePath, sizeof(filePath), fp) != NULL) {

      printf("file:%s", filePath);
#if 1
      list <string> result;
      list <string>::iterator it;
      //FIXME: length -1
      result = parse(filePath, strlen(filePath));

      cout << "filePath len: "<< strlen(filePath)<< endl;
      list <string> fin;
      for (it = result.begin(); it != result.end(); it++) {
          cout << "*it: "<< *it << " " << endl;
          combin(fin, getCandPinYin(*it));

      }

      //save to database 

      //print result
      for (it = fin.begin(); it != fin.end(); it++) {
          cout << "fin:" << *it  << " length is : " << (*it).length() << "data" << strlen((*it).data())<< endl;
          cdb_make_add(&cdbm, (*it).data(), (*it).length(), filePath, strlen(filePath));
      }

      //usleep(200);
  #endif
  }
  /* final stage - write indexes to CDB file */
  cdb_make_finish(&cdbm);
  rename("tmpfile", "demo.db");
  /* atomically replace CDB file with newly built one */

  pclose(fp);

  return 0;
}

int main(int argc, char* argv[])
{
    init();

    if (argc > 1)
        string testcase = argv[1];
    else 
        string testcase = "/tmp";
    
    indexFile(argv[1]);

    find();
    return 0;
}
