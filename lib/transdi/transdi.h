/**
 * $RCSfile$  transdi.h
 * 
 * @author  Hyper <boynanboy@gmail.com>
 * @date    日  6/14 11:22:35 2020
 * @version $Id$ 
*/

#ifndef HMQ_LIB_TRANSDI_TRANSDI_H
#define HMQ_LIB_TRANSDI_TRANSDI_H

namespace HmqTransdi{

	const int TRANSDI_MAX_PRODUCT_LEN = 8; // 产品名称最大长度
	const int TRANSDI_MAX_TOPIC_LEN = 8; // 产品子方向最大长度
	const int TRANSDI_MAX_CMD_LEN = 16; // 命令号最大长度
	const int TRANSDI_MAX_FLAG_LEN =8; // 其他标记分类最大长度，例如机房，保留8个字符，可以多扩展

	const int TRANSDI_MAX_FILENAME_LEN = 256;

	/*
	 *  结构体定义
	 * */	
#pragma pack(4)

	// di index结构 76Bytes
	typedef struct _transdi_index_t{
		unsigned long long di_id; 				// 数据顺序号
		uint32_t file_no;		  				// 数据文件编号，最大 65535
		uint32_t block_no;        				// 数据记录在data文件的块号
		uint32_t data_length;     				// 数据记录的长度，单位 Bytes
		char product[TRANSDI_MAX_PRODUCT_LEN]; 	// 产品名称
		char topic[TRANSDI_MAX_TOPIC_LEN]; 		// 产品子方向名称
		char cmd[TRANSDI_MAX_CMD_LEN];     		// 命令号
		char flag[TRANSDI_MAX_FLAG_LEN];   		// flag 
		char reserved[16];
	} transdi_index_t;  // 76Bytes

	// 写入di时的数据信息
	typedef struct _transdi_write_item_t{
		unsigned long long log_id;
		char* product;
		char* topic;
		char* cmd;
		char* flag;

		char* data; //传入的数据指针 和 数据长度
		uint32_t data_length;   

		uint32_t write_time_s; //数据传入时间，秒 微秒
		uint32_t write_time_us;
	}transdi_write_item_t;

	// 读di时的数据
	typedef struct _transdi_read_item_t{
		unsigned long long id_id;
		unsigned long long log_id;

		char* product;
		char* topic;
		char* cmd;
		char* flag;

		char* data;
		uint32_t data_length;

		uint32_t write_time_s;
		uint32_t write_time_us;
	}transdi_write_item_t;

	// transdi句柄
	typedef struct _transdi_file_t{

		char data_dir[TRANSDI_MAX_FILENAME_LEN];

		// 写入用
		uint32_t index_fd;      // 索引文件fd
		uint32_t data_fd;       // 数据文件fd

		// 读取用
		FILE* index_file;
		FILE* data_file;

	}transdi_file_t;

#pragma pack()

};


#endif
