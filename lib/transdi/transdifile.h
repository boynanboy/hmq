/**
 * $RCSfile$  transdifile.h
 * 
 * @author  Hyper <boynanboy@gmail.com>
 * @date    日  6/14 11:23:34 2020
 * @version $Id$ 
*/

#ifndef HMQ_LIB_TRANSDI_TRANSDIFILE_H
#define HMQ_LIB_TRANSDI_TRANSDIFILE_H

#include <string>

namespace HmqTransdi{

	const std::string TRANSDI_INDEXFILE_SFIX = ".idx.";
	const std::string TRANSDI_METAFILE_SFIX = ".meta";
	const std::string TRANSDI_DATAFILE_SFIX = ".data.";
	const std::string TRANSDI_LASTWRITE_SFIX = ".last_write.transid";
	const std::string TRANSDI_LASTSYNC_SFIX = ".last_sync.transid";

	const uint32_t TRANSDI_MAGICNUM = 0x81097f6c;

	const uint32_t TRANSDI_OPEN_TYPE_READ = 1;
	const uint32_t TRANSDI_OPEN_TYPE_WRITE = 2;

	typedef enum _transdi_falgs_t{
		TRANSDI_FLAG_NOTHING = 0x00,
		TRANSDI_CHECKSUM = 0x01,
	}transdi_flags_t;

	const uint32_t TRANSDI_FLAG_DEFAULT = TRANSDI_FLAG_NOTHING;
	//#define TRANSDI_FLAG_DEFAULT TRANSDI_FLAG_NOTHING

	// di数据文件最大长度 2G
	const uint32_t TRANSDI_MAX_DATA_FILESIZE = 1024*1024*1024*2U;

	// 一个目录里面最多的index文件个数
	const uint32_t TRANSDI_MAX_IDXFILE_PERDIR = 10000;
	


#pragma pack(4)

	// meta文件信息，36Bytes
	typedef struct _transdi_meta_t{
		uint32_t magic_num;
		uint32_t flags;
		uint32_t block_size;
		uint32_t idx_file_perdir;
		uint32_t idx_num_perfile;
		uint32_t reserved[4];
	}transdi_meta_t;

	// di数据头，后续跟每段长度，然后是每段数据
	typedef struct _transdi_data_head_t{
		unsigned long long di_id;
		unsigned long long log_id;

		uint32_t data_length;
		uint32_t magic_num;
		uint32_t check_sum;
		uint32_t write_time_s;
		uint32_t write_time_us;
		unsigned short segment_num;
		char reserved[18];
		uint32_t segment_lengths[0];
	}transdi_data_head_t;

#pragma pack()
};

#endif
