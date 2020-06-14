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

	const string TRANSDI_INDEXFILE_SFIX = ".idx";
	const string TRANSDI_METAFILE_SFIX = ".meta";
	const string TRANSDI_DATAFILE_SFIX = ".data";

	const uint32_t TRANSDI_MAGICNUM = 0x81097f6c;

	enum class TRANSDI_OPEN_TYPE{
		READ = 1,
		WRITE
	};

	enum class TRANSDI_FLAG{
		NOTHING = 0x00,
		CHECKSUM
	};

	const int TRANSDI_FLAG_DEFAULT = TRANSDI_FLAG::NOTHING;

#pragma pack(4)

	// meta文件信息，40Bytes
	typedef struct _transdi_meta_t{
		uint32_t magic_num;
		uint32_t version;
		uint32_t flags;
		uint32_t block_size;
		uint32_t idxfile_perdir;
		uint32_t idxnum_perfile;
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
