/*
 * =====================================================================================
 *
 *       Filename:  chinese2pinyin.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/04/2015 12:04:59 AM
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */


#ifndef _CHINESE2fPINYIN_H_
#define _CHINESE2fPINYIN_H_

#include <string>
using namespace std;

int init();
int indexFile(string indexPath);
int updateDB();
string getHomePath();
void my_handler(int s);

#endif
