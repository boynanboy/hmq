/**
 * $RCSfile$  log.h
 * 
 * @author  Hyper <boynanboy@gmail.com>
 * @date    一  6/15 16:47:02 2020
 * @version $Id$ 
*/

#ifndef HMQ_UTIL_LOG_H
#define HMQ_UTIL_LOG_H

#include <glog/logging.h>

namespace HmqUtil{

	#define HMQ_INFO_LOG 	LOG(INFO)
	#define HMQ_WARNING_LOG LOG(WARNING)
	#define HMQ_ERROR_LOG 	LOG(ERROR)

	bool InitLog(char* log_name){
		std::cout << log_name << std::endl;
		google::InitGoogleLogging(log_name);
		FLAGS_log_dir = "./log";
		return true;
	}

};

#endif
