/**
 * $RCSfile$  transdi.h
 * 
 * @author  Hyper <boynanboy@gmail.com>
 * @date    日  6/14 11:22:35 2020
 * @version $Id$ 
*/

#ifndef HMQ_LIB_TRANSDI_TRANSDI_H
#define HMQ_LIB_TRANSDI_TRANSDI_H

#include "transdifile.h"

namespace HmqTransdi{

	const int TRANSDI_MAX_PRODUCT_LEN = 8; // 产品名称最大长度
	const int TRANSDI_MAX_TOPIC_LEN = 8; // 产品子方向最大长度
	const int TRANSDI_MAX_CMD_LEN = 16; // 命令号最大长度
	const int TRANSDI_MAX_FLAG_LEN =8; // 其他标记分类最大长度，例如机房，保留8个字符，可以多扩展

	const int TRANSDI_MAX_FILENAME_LEN = 256;

	/*
	 * sync_threshold触发条件： 1. 写盘数超过sync_step 2. 与上次写盘时间间隔sync_interval_s  满足其一就会触发一次sync
	 * sync_every_write触发条件： 每次写盘都会触发sync
	 *
	 * */
	enum TRANSDI_SYNC_STRATEGY {
		TRANSDI_SYNC_THRESHOLD = 0,   
		TRANSDI_SYNC_EVERY_WRITE = 1,
	};
	const int TRANSDI_DEFAULT_SYNC_INTERVAL_S = 10;
	const int TRANSDI_DEFAULT_SYNC_STEP = 100;


	const uint32_t TRANSDI_DEFAULT_BLOCK_SIZE = 256;
	const uint32_t TRANSDI_DEFAULT_IDXFILE_PERDIR = 10;
	const uint32_t TRANSDI_DEFAULT_IDXNUM_PERFILE = 10*100*1000U;

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
	} transdi_read_item_t;

	// transdi句柄
	typedef struct _transdi_file_t{
		// TODO
		unsigned short version;
		uint32_t open_type;        // 打开类型
		uint32_t flags;            // 读写flags
		uint32_t block_size;       // 数据文件的基本单位，默认256Bytes
		uint32_t blocknum_perfile;
		uint32_t idx_file_perdir;  // 每个目录下的索引文件数
		uint32_t idx_num_perfile; // 每个文件的索引数
		unsigned long long idx_num_perdir;   // 每个目录的索引数，= idx_file_perdir * idex_num_perfile

		// 索引文件 base_name.idx.0 1 2....  数据文件：base_name.data.0 1 2...
		char data_dir[TRANSDI_MAX_FILENAME_LEN];
		char basename[TRANSDI_MAX_FILENAME_LEN];
		char data_filetag[TRANSDI_MAX_FILENAME_LEN]; // TODO
		char idx_filetag[TRANSDI_MAX_FILENAME_LEN];  // TODO

		// 写入用
		uint32_t index_fd;      // 索引文件fd
		uint32_t data_fd;       // 数据文件fd
		uint32_t last_write_fd; // write进度文件fd
		uint32_t last_sync_fd;  // sync进度文件fd

		// sync  TODO
		bool enable_sync;
		uint32_t sync_strategy;
		uint32_t sync_step;
		uint32_t sync_interval_s;
		uint32_t last_write_count;

		unsigned long long last_sync_time_us;
		unsigned long long last_sync_transid;

		// 读取用
		FILE* index_file;
		FILE* data_file;
	
		/*
		 * ??? TODO
		 * */
		uint32_t cur_dir_no;
		uint32_t cur_idx_no;   // 当前使用的index文件号
		uint32_t cur_data_no;  // 当前数据文件号
		uint32_t cur_block_no; // 当前数据文件块号

		unsigned long long next_transid;

		// TODO 读状态，基于访问的局部性避免重复打开文件
		uint32_t read_idx_dir_no;
		uint32_t read_data_dir_no;
		uint32_t read_idx_no;
		uint32_t read_data_no;

		int is_opened;
	}transdi_file_t;

#pragma pack()


	/*
	 * 初始化DI文件写句柄
	 * */
	int transdi_open_w(transdi_file_t* di_file,
			const char* directory,
			const char* basename,
			bool enable_sync = true,
			uint32_t sync_strategy = TRANSDI_SYNC_THRESHOLD,
			uint32_t sync_step = TRANSDI_DEFAULT_SYNC_STEP,
			uint32_t sync_interval_s = TRANSDI_DEFAULT_SYNC_INTERVAL_S,
			uint32_t block_size = TRANSDI_DEFAULT_BLOCK_SIZE,
			uint32_t idxfile_perdir = TRANSDI_DEFAULT_IDXFILE_PERDIR,
			uint32_t idxnum_perfile = TRANSDI_DEFAULT_IDXNUM_PERFILE,
			bool checksum = true);

};


#endif
