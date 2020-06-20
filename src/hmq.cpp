/**
 * $RCSfile$  src/hmq.cpp
 * 
 * @author  Hyper <boynanboy@gmail.com>
 * @date    二  6/16 00:06:07 2020
 * @version $Id$ 
*/

#include <iostream>
#include <util/log.h>
#include <lib/transdi/transdi.h>
#include <lib/transdi/transdifile.h>

using namespace std;

int main(){
	HmqUtil::InitLog("hmq");
	cout << "hello world, i am hmq" << endl;
	LOG(INFO) << "hello world, i am hmq";
	HmqTransdi::transdi_file_t* di_file = new HmqTransdi::transdi_file_t;
	cout << HmqTransdi::transdi_open_w(di_file, "/home/hyper/Codes/hmq/build/bin", "hmq") <<endl;
	return 0;
}
