#include "system.h"
#include "basic/process_control.h"

namespace os {
Directory::Directory(void)
:dir_(nullptr)
{}

Directory::~Directory(void)
{
	if (dir_ != nullptr) {
		closedir(dir_);
	}
	dir_ = nullptr;
}

std::string 
Directory::get_program_running_dir(void)
{
	char buffer[PATH_MAX];
	char *path_ptr = getcwd(buffer, PATH_MAX);
	if (path_ptr == nullptr) {
		return "";
	}
	return path_ptr;
}

int 
Directory::change_program_running_dir(std::string &new_path)
{
	int ret = chdir(new_path.c_str());
	if (ret == -1) {
		LOG_GLOBAL_ERROR("chdir: %s", strerror(errno));
		return -1;
	}
	return 0;
}

std::string 
Directory::get_cur_executable_path(bool exec_file_path)
{
    char arr_tmp[2048] = {0};

    ssize_t ret = ::readlink("/proc/self/exe", arr_tmp, sizeof(arr_tmp) - 1);
    if (ret < 0) {
        LOG_GLOBAL_ERROR("readlink: %s", strerror(errno));
        return "";
    }
    arr_tmp[ret] = '\0';

    if (exec_file_path == false) { // 返回可执行文件所在目录路径
        char *dir_path = strrchr(arr_tmp, '/'); // strrchr判断'/'最后一次出现位置，表示返回可执行文件所在目录
        if (dir_path == nullptr) {
            LOG_GLOBAL_ERROR("exec_dir_path: %s", arr_tmp);
            return "";
        }
        *(dir_path + 1) = '\0';
    }
    return std::string(arr_tmp);
}

std::string 
Directory::get_curr_dir_path(void)
{
	if (dir_ == nullptr) {
		LOG_ERROR("Current not open any dir!\n");
		return "";
	}
	return dir_path_;
}

std::string 
Directory::get_curr_dir_name(void)
{
	if (dir_ == nullptr) {
		LOG_ERROR("Current not open any dir!\n");
		return "";
	}
	if (dir_path_.empty() || dir_path_ == "/") {
		return "/";
	}

	std::string::size_type pos = dir_path_.find_last_of('/');
	if (pos == std::string::npos) {
		return dir_path_;
	}
	return dir_path_.substr(pos + 1);
}

int 
Directory::open_dir(const std::string &path)
{
    // 允许打开指向目录的符号链接，因此这里用 stat 跟随链接
    struct stat path_info;
    if (::stat(path.c_str(), &path_info) == -1 || !S_ISDIR(path_info.st_mode)) {
        LOG_ERROR("Not a directory: %s", path.c_str());
        return -1;
    }

    if (dir_ != nullptr) {
        ::closedir(dir_);
        dir_ = nullptr;
    }

    // 获取目录绝对路径
    char resolved_path[PATH_MAX] = {0};
    char *dir_path_ptr = ::realpath(path.c_str(), resolved_path);
    if (dir_path_ptr == nullptr) {
        LOG_ERROR("realpath: %s\n", strerror(errno));
        dir_path_.clear();
        return -1;
    }

    dir_path_ = dir_path_ptr;
    dir_ = ::opendir(dir_path_.c_str());
    if (dir_ == nullptr) {
        LOG_ERROR("opendir: %s\n", strerror(errno));
        dir_path_.clear();
        return -1;
    }
    return 0;
}

std::map<std::string, SFileType>
Directory::file_list(bool ret_default_dir)
{
	std::map<std::string, SFileType> files;
	if (dir_ == nullptr) {
		LOG_ERROR("Current not open any dir!\n");
		return files;
	}

	// 每次调用都从目录头开始，避免第二次调用直接返回空列表
	::rewinddir(dir_);

	struct dirent *dir_ptr = nullptr;
	while ((dir_ptr = ::readdir(dir_)) != nullptr) {
		std::string name = dir_ptr->d_name;
		if (ret_default_dir == false && (name == "." || name == "..")) {
			continue;
		}

		EFileType type = EFileType_Unknown;
		if (dir_ptr->d_type == DT_REG) {
			type = EFileType_File;
		} else if (dir_ptr->d_type == DT_LNK) {
			type = EFileType_Link;
		} else if (dir_ptr->d_type == DT_DIR) {
			type = EFileType_Dir;
		} else if (dir_ptr->d_type == DT_UNKNOWN) {
			// 部分文件系统不返回 d_type，需要 lstat 兜底
			struct stat file_info;
			std::string abs_path = get_abs_path(name);
			if (::lstat(abs_path.c_str(), &file_info) == 0) {
				if (S_ISREG(file_info.st_mode)) {
					type = EFileType_File;
				} else if (S_ISLNK(file_info.st_mode)) {
					type = EFileType_Link;
				} else if (S_ISDIR(file_info.st_mode)) {
					type = EFileType_Dir;
				}
			}
		}

		SFileType file;
		file.name = name;
		file.type = type;
		file.abs_path_ = get_abs_path(name);
		files[name] = file;
	}

	return files;
}

std::string 
Directory::get_abs_path(const std::string &file_name)
{
	if (dir_ == nullptr) {
		LOG_ERROR("Current not open any dir!");
		return "";
	}
	return dir_path_ + "/" + file_name;
}

bool 
Directory::exist(const std::string &file_path, bool is_abs)
{
	std::string abs_path = file_path;
	if (is_abs == false && dir_ == nullptr) {
		return false;
	} else if (is_abs == false && dir_ != nullptr) {
		abs_path = get_abs_path(file_path);
	}
	
	return access(abs_path.c_str(), F_OK) == 0 ? true : false;
}

EFileType 
Directory::file_type(const std::string  &file_path, bool is_abs)
{
	std::string abs_path = file_path;
	if (is_abs == false && dir_ == nullptr) {
		return EFileType_Unknown;
	} else if (is_abs == false && dir_ != nullptr) {
		abs_path = get_abs_path(file_path);
	}

	struct stat file_info;
    // 使用 lstat，避免把符号链接误判成它指向的目标类型
    int ret = ::lstat(abs_path.c_str(), &file_info);
    if (ret == -1) {
		LOG_ERROR("lstat: %s", strerror(errno));
		return EFileType_Unknown;
	}

	if (S_ISDIR(file_info.st_mode)) {
		return EFileType_Dir;
	} else if (S_ISLNK(file_info.st_mode)) {
		return EFileType_Link;
	} else if (S_ISREG(file_info.st_mode)) {
		return EFileType_File;
	}
	return EFileType_Unknown;
}

int 
Directory::create(const std::string &path, EFileType type, bool is_abs, uint mode)
{
	std::string abs_path = path;
	if (is_abs == false && dir_ == nullptr) {
		LOG_ERROR("Current not open any dir![abs: %s]", abs_path.c_str());
		return -1;
	} else if (is_abs == false && dir_ != nullptr) {
		abs_path = get_abs_path(path);
	}

	switch (type)
	{
		case EFileType_Dir: {
			if (::mkdir(abs_path.c_str(), mode) == -1) {
				LOG_ERROR("mkdir[%s]: %s\n", abs_path.c_str(), strerror(errno));
				return -1;
			}
		} break;
		case EFileType_File: {
			File file;
			// 第二个参数是 open flag，不是权限；权限通过第三个参数传入
			if (file.open(abs_path, O_RDWR | O_CREAT, mode) != 0) {
				LOG_ERROR("create file[%s] failed\n", abs_path.c_str());
				return -1;
			}
		} break;
		default:
			LOG_ERROR("Creating this type of file is not supported[type: %d]\n", type);
			return -1;
	}

	return 0;
}

int 
Directory::remove(const std::string &path, bool is_abs)
{
	std::string abs_path = path;
	if (is_abs == false) {
		if (dir_ == nullptr) {
			LOG_ERROR("Current not open any dir!\n");
			return -1;
		}
		abs_path = get_abs_path(path);
	}

	if (abs_path == "/") {
		LOG_ERROR("root dir can't remove!\n");
		return 0;
	}

	struct stat file_info;
	if (::lstat(abs_path.c_str(), &file_info) == -1) {
		if (errno == ENOENT) {
			return 0;
		}
		LOG_ERROR("lstat[%s]: %s", abs_path.c_str(), strerror(errno));
		return -1;
	}

	if (S_ISREG(file_info.st_mode) || S_ISLNK(file_info.st_mode)) {
		if (::remove(abs_path.c_str()) < 0) { // 系统调用删除文件/符号链接
			LOG_ERROR("remove[%s]: %s", abs_path.c_str(), strerror(errno));
			return -1;
		}
	} else if (S_ISDIR(file_info.st_mode)) {
		Directory dir;
		if (dir.open_dir(abs_path) < 0) {
			return -1;
		}

		// file_list 默认不返回 . 和 ..
		std::map<std::string, SFileType> files_list = dir.file_list();
		for (auto iter = files_list.begin(); iter != files_list.end(); ++iter) {
			if (dir.remove(iter->first) < 0) {
				return -1;
			}
		}

		if (::rmdir(abs_path.c_str()) < 0) {
			LOG_ERROR("rmdir[%s]: %s", abs_path.c_str(), strerror(errno));
			return -1;
		}
	} else {
		LOG_ERROR("remove[%s]: unsupported file type", abs_path.c_str());
		return -1;
	}

	return 0;
}

int 
Directory::rename(const std::string &old_name, const std::string &new_name, bool is_abs)
{
	std::string old_path = old_name;
	std::string new_path = new_name;

	if (is_abs == false) {
		if (dir_ == nullptr) {
			LOG_ERROR("Current not open any dir!\n");
			return -1;
		}
		old_path = get_abs_path(old_name);
		new_path = get_abs_path(new_name);
	}

	if (::rename(old_path.c_str(), new_path.c_str()) < 0) {
		LOG_ERROR("rename[%s -> %s]: %s", old_path.c_str(), new_path.c_str(), strerror(errno));
		return -1;
	}
	return 0;
}

// 拷贝当前目录到 des_path 下，目标目录名为当前目录名
int 
Directory::copy(const std::string &des_path)
{
	if (dir_ == nullptr) {
		LOG_ERROR("Current not open any dir, can't copy to %s", des_path.c_str());
		return -1;
	}
	if (des_path.empty()) {
		LOG_ERROR("des_path is empty");
		return -1;
	}

	std::string dir_name = get_curr_dir_name();
	if (dir_name.empty() || dir_name == "/") {
		LOG_ERROR("Get dir name failed[%s]", dir_path_.c_str());
		return -1;
	}

	std::string dest_parent = des_path;
	while (dest_parent.size() > 1 && dest_parent.back() == '/') {
		dest_parent.pop_back();
	}
	std::string dest_dir_path = dest_parent + "/" + dir_name;

	struct stat dest_info;
	if (::lstat(dest_dir_path.c_str(), &dest_info) == 0) {
		LOG_ERROR("destination already exists[%s]", dest_dir_path.c_str());
		return -1;
	} else if (errno != ENOENT) {
		LOG_ERROR("lstat[%s]: %s", dest_dir_path.c_str(), strerror(errno));
		return -1;
	}

	if (::mkdir(dest_dir_path.c_str(), DEFAULT_DIR_RIGHT) < 0) {
		LOG_ERROR("mkdir[%s]: %s", dest_dir_path.c_str(), strerror(errno));
		return -1;
	}

	::rewinddir(dir_);
	struct dirent *dir_ptr = nullptr;
	while ((dir_ptr = ::readdir(dir_)) != nullptr) {
		std::string name = dir_ptr->d_name;
		if (name == "." || name == "..") {
			continue;
		}

		std::string src_path = dir_path_ + "/" + name;
		std::string dst_path = dest_dir_path + "/" + name;
		EFileType type = file_type(src_path, true);

		if (type == EFileType_File) {
			File src, dest;
			if (src.open(src_path, O_RDONLY) != 0) {
				return -1;
			}
			if (dest.open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, DEFAULT_FILE_RIGHT) != 0) {
				return -1;
			}
			if (dest.copy(src) < 0) {
				return -1;
			}
		} else if (type == EFileType_Link) {
			char link_target[PATH_MAX] = {0};
			ssize_t link_len = ::readlink(src_path.c_str(), link_target, sizeof(link_target) - 1);
			if (link_len < 0) {
				LOG_ERROR("readlink[%s]: %s", src_path.c_str(), strerror(errno));
				return -1;
			}
			link_target[link_len] = '\0';
			if (::symlink(link_target, dst_path.c_str()) < 0) {
				LOG_ERROR("symlink[%s]: %s", dst_path.c_str(), strerror(errno));
				return -1;
			}
		} else if (type == EFileType_Dir) {
			Directory sub_dir;
			if (sub_dir.open_dir(src_path) != 0) {
				return -1;
			}
			if (sub_dir.copy(dest_dir_path) != 0) {
				return -1;
			}
		} else {
			LOG_ERROR("copy[%s]: unsupported file type", src_path.c_str());
			return -1;
		}
	}

	return 0;
}

int
Directory::move(const std::string &des_path)
{
	if (dir_ == nullptr) {
		LOG_ERROR("Current not open any dir, can't move to %s", des_path.c_str());
		return -1;
	}
	if (des_path.empty()) {
		return -1;
	}

	std::string dir_name = get_curr_dir_name();
	if (dir_name.empty() || dir_name == "/") {
		return -1;
	}

	std::string dest_parent = des_path;
	while (dest_parent.size() > 1 && dest_parent.back() == '/') {
		dest_parent.pop_back();
	}
	std::string dest_dir_path = dest_parent + "/" + dir_name;

	struct stat dest_info;
	if (::lstat(dest_dir_path.c_str(), &dest_info) == 0) {
		LOG_ERROR("destination already exists[%s]", dest_dir_path.c_str());
		return -1;
	}

	std::string src_path = dir_path_;
	if (::rename(src_path.c_str(), dest_dir_path.c_str()) == 0) {
		if (dir_ != nullptr) {
			::closedir(dir_);
			dir_ = nullptr;
		}
		return 0;
	}

	// 跨文件系统时 rename 会失败，退化为 copy + remove
	if (errno != EXDEV) {
		LOG_ERROR("rename[%s -> %s]: %s", src_path.c_str(), dest_dir_path.c_str(), strerror(errno));
		return -1;
	}
	if (this->copy(des_path) != 0) {
		return -1;
	}

	if (dir_ != nullptr) {
		::closedir(dir_);
		dir_ = nullptr;
	}
	return this->remove(src_path, true);
}

}