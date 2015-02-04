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

using namespace std;

#define START 0x3400
#define END   0x9fff
#define DATA  "table.dat"
string dict[0x9fff][10] = {};

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
int main()
{
    init();

//    string testcase = "b追a适c好";
//    string testcase = "这是一个test文档.doc";
//    string testcase = "这.长.长.重.档.doc";
    //string testcase = "这长test.doc";
    string testcase;
    cout << "Input a string: (for example: 新建文档test.txt)" << endl; 
    cin >> testcase;

    list <string> result;
    list <string>::iterator it;
    result = parse(testcase.data(), testcase.length());

    cout << "testcase: " << testcase << endl;

    list <string> fin;
    for (it = result.begin(); it != result.end(); it++) {
//        cout << "*it: "<< *it << " " << endl;
        combin(fin, getCandPinYin(*it));
    }

    //print result
//    list <string>::iterator it;
    for (it = fin.begin(); it != fin.end(); it++) {
        cout << "fin : " << *it << endl;
    }

    cout << "total: "<< fin.size() << endl;

    return 0;
}
