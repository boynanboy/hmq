/**
 * $RCSfile$  transdifile.cpp
 * 
 * @author  Hyper <boynanboy@gmail.com>
 * @date    二  6/16 11:29:34 2020
 * @version $Id$ 
*/

#include "transdi.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/uio.h>
#include <dirent.h>
#include <cstdio>
#include <errno.h>
#include <dirent.h>
#include <new>
#include <sys/stat.h>
#include <sys/file.h>
#include <cstring>
#include <iostream>


namespace HmqTransdi{
	static int _create_parent_dir(const char* _path){
		static const size_t _max_path_size = 1024;
		char parent[_max_path_size];

		int32_t length = snprintf(parent, sizeof(parent), "%s", _path);

		//the tracing '/'s
		int32_t last = length - 1;
		while (last >= 0 && '/' == parent[last]){
			--last;
		}

		//the letters
		while (last >=0 && '/' != parent[last]){
			--last;
		}

		//now last is indexing to an '/' or is -1
		if(last <= 0 ) {
			return 0;
		}
		parent[last+1] = '\0';
		struct stat dirstat;
		if( 0 == stat(parent, &dirstat) && S_ISDIR(dirstat.st_mode) && 0 == access(parent, R_OK|W_OK|X_OK)) {
			return 0;
		} else {
			if(0 == access(parent, F_OK)) {
				return -1;
			} else if (0 == _create_parent_dir(parent) && 0 == mkdir(parent, 0775)) {
				return 0;
			} else {
				return -1;
			}
		}
	}

	static int _create_dir(const char* _path){
		struct stat dirstat;
		if (0 == stat(_path, &dirstat) && S_ISDIR(dirstat.st_mode) && 0 == access(_path, R_OK|W_OK|X_OK)){
			return 0;
		} else if (0 == access(_path, F_OK)) {
			return -1;
		} else if (0 == _create_parent_dir(_path) && 0 == mkdir(_path, 0755)){
			return 0;
		} else{
			return -1;
		}
	}

	static int _openfile_w(char* file, bool is_sync){
		// 打开文件，如果没有则创建{{{
		int flag = O_CREAT|O_RDWR;
		if (is_sync){
			flag |= O_SYNC; // 以同步的方式打开文件，因为文件打开后，系统不会自动更新文件写的位置，对于每个文件状态标志和当前文件偏移量 不会更新
		}
		return open(file, flag, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);/*}}}*/
	}

	static int _transdi_load_cur(transdi_file_t* di_file){
	/*{{{*/
		char buf[TRANSDI_MAX_FILENAME_LEN];
		off_t fileoff = 0;
		struct stat st;
		int error;

		snprintf(buf, sizeof(buf), "%s/%u/%s%s%u", di_file->data_dir, di_file->cur_dir_no, di_file->basename, TRANSDI_INDEXFILE_SFIX.c_str(), di_file->cur_idx_no);

		// 读不需要打开
		if (di_file->open_type == TRANSDI_OPEN_TYPE_WRITE){
			di_file->index_fd = _openfile_w(buf, di_file->sync_strategy == TRANSDI_SYNC_EVERY_WRITE);
			if (di_file->index_fd < 0){
				goto label_exit;
			}
		}else{
			di_file->index_fd = -1;
		}

		if ((error = stat(buf, &st)) < 0){
			if ( errno == ENOENT){
				fileoff = 0;
			}else{
				goto label_exit;
			}
		} else {
			// 文件大小，单位是字节
			fileoff = st.st_size;
		}

		// 判断索引文件大小是索引的整数倍
		if (fileoff%sizeof(transdi_index_t) != 0){
			goto label_exit;
		}

		/*
		 * 获取下一个transid
		 * 文件号从0开始，索引文件也是从0开始
		 * transid = 前面所有目录的transid总数 加上 当前目录索引文件总数
		 * */
		di_file->next_transid = 
			(unsigned long long)di_file->cur_dir_no * (di_file->idx_file_perdir * di_file->idx_num_perfile)
			+ (unsigned long long)di_file->cur_idx_no * di_file->idx_num_perfile 
			+ (fileoff/sizeof(transdi_index_t))
			+1;


		snprintf(buf, sizeof(buf), "%s/%u/%s%s%u", di_file->data_dir, di_file->cur_dir_no, di_file->basename, TRANSDI_DATAFILE_SFIX.c_str(), di_file->cur_data_no);
		// 读不需要打开
		if (di_file->open_type == TRANSDI_OPEN_TYPE_WRITE){
			di_file->data_fd = _openfile_w(buf, di_file->sync_strategy == TRANSDI_SYNC_EVERY_WRITE);
			if (di_file->data_fd < 0){
				goto label_exit;
			}

			fileoff = lseek(di_file->data_fd, 0, SEEK_END);
			if (fileoff < 0){
				goto label_exit;
			}

			di_file->cur_block_no = (fileoff + di_file->block_size -1)/di_file->block_size;
		}else{
			di_file->data_fd = -1;
		}

		return 0;

		label_exit:
		if (di_file->index_fd > 0){
			close(di_file->index_fd);
			di_file->index_fd = -1;
		}
		if (di_file->data_fd > 0){
			close(di_file->data_fd);
			di_file->data_fd = -1;
		}
		return -1;/*}}}*/
	}

	static int _transdi_open_last_file(transdi_file_t* di_file){
		/*{{{*/
		int ret = 0;
		char buf[TRANSDI_MAX_FILENAME_LEN];
		char idxfilepre[TRANSDI_MAX_FILENAME_LEN];
		char datafilepre[TRANSDI_MAX_FILENAME_LEN];
		DIR *cur_data_dir = NULL;
		struct dirent cur_dirent;
		struct dirent *result = NULL;
		int idxfilepre_len = 0;
		int datafilepre_len = 0;

		snprintf(buf,sizeof(buf),"%s/%u",di_file->data_dir,di_file->cur_dir_no);
		snprintf(idxfilepre,sizeof(idxfilepre),"%s%s",di_file->basename,TRANSDI_INDEXFILE_SFIX.c_str());
		snprintf(datafilepre,sizeof(datafilepre),"%s%s",di_file->basename,TRANSDI_DATAFILE_SFIX.c_str());
		idxfilepre_len = strlen(idxfilepre);
		datafilepre_len = strlen(datafilepre);

		int largest_datafile = -1;
		int largest_idxfile = -1;

		cur_data_dir = opendir(buf);
		if (cur_data_dir == NULL){
			//UB_LOG_WARNING("%s opendir error[path:%s]", __FUNCTION__, buf);
			ret = -1;
			goto label_exit;
		}

		if (0 != readdir_r(cur_data_dir, &cur_dirent, &result)){
			//UB_LOG_WARNING("%s readdir error ", __FUNCTION__);
			ret = -1;
			goto label_exit;
		}

		while (result != NULL){
			int tmp = 0;
			char *ctmp = 0;
			if (strcmp(result->d_name, "..") == 0||
					strcmp(result->d_name, ".") == 0){
				readdir_r(cur_data_dir, &cur_dirent, &result);
				continue;
			}

			if (memcmp(result->d_name, idxfilepre, idxfilepre_len) == 0){
				tmp = strtol(result->d_name + idxfilepre_len ,&ctmp,10);
				if (ctmp != NULL && *ctmp == '\0' && tmp > largest_idxfile)
					largest_idxfile = tmp;

			} else if (memcmp(result->d_name, datafilepre, datafilepre_len) == 0){
				tmp = strtol(result->d_name + datafilepre_len, &ctmp, 10);
				if (ctmp != NULL && *ctmp == '\0' && tmp > largest_datafile)
					largest_datafile = tmp;
			}
			readdir_r(cur_data_dir, &cur_dirent, &result);
		}

		//找到最大文件号
		di_file->cur_idx_no = (largest_idxfile < 0)? 0 : largest_idxfile;
		di_file->cur_data_no = (largest_datafile < 0)? 0 : largest_datafile;

		if (_transdi_load_cur(di_file) < 0)
			ret = -1;

	label_exit:
		if (cur_data_dir != NULL){
			closedir(cur_data_dir);
			cur_data_dir = NULL;
		}
		return ret;/*}}}*/
	}

	static unsigned int _transdi_find_first_last_dir(transdi_file_t* di_file, unsigned int &first_dir,
				unsigned int &last_dir, bool write_mode){
		/*{{{*/
		int ret = 0;
		char *directory = di_file->data_dir;

		DIR *datadir = NULL;
		long int largest_dir = -1;
		long int smallest_dir = -1;
		long int cur_dir = -1;
		char *endptr = NULL;
		struct dirent cur_dirent;
		struct dirent *result = NULL;
		//struct dirent *subdir = NULL;
		if (access(directory,(write_mode ? R_OK|W_OK|X_OK : R_OK|X_OK)) < 0)
		{
			//UB_LOG_WARNING("%s can't access %s",__FUNCTION__, directory);
			ret = -1;
			goto label_exit;
		}

		if ((datadir = opendir(directory)) == NULL)
		{
			//UB_LOG_WARNING("%s opendir %s error", __FUNCTION__, directory);
			ret = -1;
			goto label_exit;
		}

		if (0 != readdir_r(datadir, &cur_dirent, &result))
		{
			//UB_LOG_WARNING("%s readdir %s error", __FUNCTION__, directory);
			ret = -1;
			goto label_exit;
		}

		while (result != NULL)
		{
			if (strcmp(result->d_name, ".") == 0
					|| strcmp(result->d_name, "..") == 0
					|| strncmp(result->d_name, di_file->basename, strlen(di_file->basename)) == 0)//过滤掉
			{
				readdir_r(datadir, &cur_dirent, &result);
				continue;
			}

			cur_dir = strtol(result->d_name,&endptr,10);
			if (endptr == NULL)
			{
				//UB_LOG_WARNING("%s strtol %s endptr null",__FUNCTION__, result->d_name);
				readdir_r(datadir, &cur_dirent, &result);
				continue;
			}
			else if ((*endptr) != '\0')
			{
				//UB_LOG_WARNING("%s strtol filename %s error", __FUNCTION__, result->d_name);
				readdir_r(datadir, &cur_dirent, &result);
				continue;
			}
			else
			{
				if (cur_dir > largest_dir)
				{
					largest_dir = cur_dir;
				}
				if (smallest_dir < 0 || cur_dir < smallest_dir)
				{
					smallest_dir = cur_dir;
				}
			}
			readdir_r(datadir, &cur_dirent, &result);
		}

		closedir(datadir);
		datadir = NULL;

		if (largest_dir < 0)
		{
			if (di_file->open_type == TRANSDI_OPEN_TYPE_WRITE)
			{
				//没有目录，创建
				char buf[TRANSDI_MAX_FILENAME_LEN];
				snprintf(buf,sizeof(buf),"%s/0",directory);
				if (_create_dir(buf) != 0)
				{
					//UB_LOG_WARNING("%s create dir %s error. maybe permission denied. [%m]", __FUNCTION__, buf);
					ret = -1;
					goto label_exit;
				}
				largest_dir = 0; 
				smallest_dir = 0;
			}
			else
			{
				//UB_LOG_WARNING("open for read, but there is no data dir");
				ret = -1;
				goto label_exit;
			}
		}

		first_dir = smallest_dir;
		last_dir = largest_dir;
		return ret;
	label_exit:
		if (datadir != NULL)
		{
			closedir(datadir);
			datadir = NULL;
		}
		return ret;/*}}}*/
	}

	static int _transdi_find_last_dir(transdi_file_t* di_file, bool write_mode = true){
		/*{{{*/
		int ret = 0;
		unsigned int first_dir = 0;
		unsigned int last_dir = 0;
		if ((ret = _transdi_find_first_last_dir(di_file, first_dir, last_dir, write_mode)) != 0){
			return -1;
		}else{
			return (int)last_dir;
		}/*}}}*/
	}

	/*
	 * 存在meta文件则检查是否匹配，如果不存在则创建新的元数据文件
	 * */
	static int _transdi_check_meta(transdi_file_t* di_file){
		// meta文件的绝对路径和文件名{{{
		char buf[TRANSDI_MAX_FILENAME_LEN];
		snprintf(buf, sizeof(buf), "%s/%s%s", di_file->data_dir, di_file->basename, TRANSDI_METAFILE_SFIX.c_str());
		
		int fd = -1;
		int ret = 0;
		transdi_meta_t* head = new transdi_meta_t;

		if (access(buf, F_OK) == 0){
			fd = open(buf, O_RDONLY);
			if (fd < 0){
				return -1;
			}

			if (lseek(fd, 0, SEEK_SET) < 0){
				return -1;
			}

			if (read(fd, head, sizeof(*head)) != sizeof(*head)){
				return -1;
			} else if (
						head->magic_num != TRANSDI_MAGICNUM ||
						head->block_size != di_file->block_size ||
						head->idx_file_perdir != di_file->idx_file_perdir ||
						head->idx_num_perfile != di_file->idx_num_perfile ||
						head->flags != di_file->flags
					){
				return -1;		
			}
	
		} else if (di_file->open_type == TRANSDI_OPEN_TYPE_WRITE){
			head->magic_num = TRANSDI_MAGICNUM;
			head->flags = (di_file->flags) & (TRANSDI_CHECKSUM);
			head->block_size = di_file->block_size;
			head->idx_file_perdir = di_file->idx_file_perdir;
			head->idx_num_perfile = di_file->idx_num_perfile;
			
			fd = _openfile_w(buf, true);
			if (fd < 0){
				// TODO 
				return -1; 
			}

			if (lseek(fd, 0, SEEK_SET) < 0){
				return -1;
			}

			if (write(fd, head, sizeof(*head)) != sizeof(*head)){
				return -1;
			}
		} else {
			return -1;
		}
		return ret;/*}}}*/
	}


	int _transdi_open_w(transdi_file_t* di_file, const char* directory, const char* basename, 
			bool enable_sync, uint32_t sync_strategy, uint32_t sync_step, uint32_t sync_interval_s,
			uint32_t block_size, uint32_t idx_file_perdir, uint32_t idx_num_perfile, uint32_t flags
			){
		//TODO 校验{{{
	
		flags = flags & (~TRANSDI_FLAG_NOTHING); // TODO
		
		memset(di_file, 0, sizeof(transdi_file_t));
		di_file->index_fd = -1;
		di_file->data_fd = -1;
		
		strncpy(di_file->data_dir, directory, sizeof(di_file->data_dir)-1);
		strncpy(di_file->basename, basename, sizeof(di_file->basename)-1);

		di_file->flags = flags;
		di_file->block_size = block_size;
		di_file->idx_file_perdir = idx_file_perdir;
		di_file->idx_num_perfile = idx_num_perfile;
		di_file->idx_num_perdir = (unsigned long long)idx_file_perdir * (unsigned long long)idx_num_perfile;
		
		di_file->last_write_fd = -1;
		di_file->last_sync_fd = -1;

		di_file->read_data_no = -1;
		di_file->read_idx_no = -1;
		di_file->read_idx_dir_no = -1;
		di_file->read_data_dir_no = -1;

		di_file->open_type = TRANSDI_OPEN_TYPE_WRITE;
		di_file->enable_sync = enable_sync;
		di_file->sync_strategy = sync_strategy;
		di_file->sync_step = sync_step;
		di_file->sync_interval_s = sync_interval_s;

		if( sync_strategy == TRANSDI_SYNC_EVERY_WRITE && (sync_step != 0 || sync_interval_s != 0)){
			std::cout << "bad sync strategy" << std::endl;
			//HmqUtil::HMQ_WARNING_LOG << __FUNCTION__ << "bad sync strategy";
		}

		// 检查元数据
		if (_transdi_check_meta(di_file) < 0){
			std::cout << "check meta file failed" << std::endl;
			return -1;
		}

		// 确定last_dir
		int cur_dir = _transdi_find_last_dir(di_file);
		if (cur_dir < 0){
			return -1;
		}

		di_file->cur_dir_no = (uint32_t)cur_dir;
	
		// 找到最大的di文件号，含idx 和 data
		if (_transdi_open_last_file(di_file) < 0){
			return -1;
		}

		/*
		// 加载进度文件
		if (_transdi_load_write_progress(di_file) < 0){
			return -1;
		}
		
		if (_transdi_load_sync_progress(di_file) < 0){
			return -1;
		}
		*/
		di_file->is_opened = 1;
		return 0;/*}}}*/
	}

	/*
	 * 初始化DI文件写句柄
	 * */
	int transdi_open_w(transdi_file_t* di_file, const char* directory, const char* basename,
			 bool enable_sync, uint32_t sync_strategy, uint32_t sync_step, uint32_t sync_interval_s,
			uint32_t block_size, uint32_t idx_file_perdir, uint32_t idx_num_perfile, bool checksum){
		
		uint32_t flags = TRANSDI_FLAG_DEFAULT;
		if (checksum){
			flags |= TRANSDI_CHECKSUM;
		}

		return _transdi_open_w(di_file, directory, basename, 
				enable_sync, sync_strategy, sync_step, sync_interval_s,
				block_size, idx_file_perdir, idx_num_perfile, flags);
	}

};


