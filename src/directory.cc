#include "system.h"

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
		printf("chdir: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

std::string 
Directory::get_curr_dir_path(void)
{
	if (dir_ == nullptr) {
		printf("Current not open any dir!\n");
		return "";
	}
	return dir_path_;
}

std::string 
Directory::get_curr_dir_name(void)
{
	if (dir_ == nullptr) {
		printf("Current not open any dir!\n");
		return "";
	}

	return basename(dir_path_.c_str());
}

int 
Directory::open_dir(const std::string &path)
{
	if (file_type(path) != EFileType_Dir) {
		LOG_ERROR("Not a directory: %s", path.c_str());
		return -1;
	}

	if (dir_ != nullptr) {
		closedir(dir_);
		dir_ = nullptr;
	}

	// 获取目录绝对路径
	char resolved_path[PATH_MAX] = {0};
	char *dir_path_ptr = realpath(path.c_str(), resolved_path);
	if (dir_path_ptr == nullptr) {
		LOG_ERROR("realpath: %s\n", strerror(errno));
		return -1;
	}

	dir_path_ = dir_path_ptr;
	dir_ = opendir(dir_path_.c_str());
	if (dir_ == nullptr) {
		LOG_ERROR("opendir: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

std::vector<SFileType> 
Directory::file_list(void)
{
	SFileType file;
	std::vector<SFileType> files;
	if (dir_ == nullptr) {
		printf("Current not open any dir!\n");
		return files;
	}

	struct dirent *dir_ptr = nullptr; 
	while ((dir_ptr = readdir(dir_)) != nullptr) {
		if (dir_ptr->d_type == 8) { // 普通文件
			file.name = dir_ptr->d_name;
			file.type = EFileType_File;
		} else if (dir_ptr->d_type == 10) { // 符号链接
			file.name = dir_ptr->d_name;
			file.type = EFileType_Link;
		} else if (dir_ptr->d_type == 4) { // 目录文件
			file.name = dir_ptr->d_name;
			file.type = EFileType_Dir;
		}
		file.abs_path_ = get_abs_path(dir_ptr->d_name);
		files.push_back(file);
	}

	return files;
}

std::string 
Directory::get_abs_path(const std::string &file_name)
{
	if (dir_ == nullptr) {
		printf("Current not open any dir!\n");
		return "";
	}
	return dir_path_ + "/" + file_name;
}

bool 
Directory::exist(const std::string &file_path, bool is_abs)
{
	return access(file_path.c_str(), F_OK) == 0 ? true : false;
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
    int ret = stat(abs_path.c_str(), &file_info);
    if (ret == -1) {
		printf("stat: %s\n", strerror(errno));
		return EFileType_Unknown;
	}

	if (file_info.st_mode & S_IFDIR) {
		return EFileType_Dir;
	} else if (file_info.st_mode & S_IFLNK) {
		return EFileType_Link;
	} else if (file_info.st_mode & S_IFREG) {
		return EFileType_File;
	}
	return EFileType_Unknown;
}

int 
Directory::create(const std::string &path, EFileType type, uint mode, bool is_abs)
{
	std::string abs_path = path;
	if (is_abs == false && dir_ == nullptr) {
		LOG_ERROR("Current not open any dir!\n");
		return -1;
	} else if (is_abs == false && dir_ != nullptr) {
		abs_path = get_abs_path(path);
	}

	switch (type)
	{
		case EFileType_Dir: {
			int ret = mkdir(abs_path.c_str(), mode);
			if (ret == -1) {
				LOG_ERROR("mkdir: %s\n", strerror(errno));
				return -1;
			} 
		} break;
		case EFileType_File: {
			File file;
			file.open(abs_path, mode);
		} break;
		default:
			LOG_ERROR("Creating this type of file is not supported\n");
			return -1;
	}

	return 0;
}

int 
Directory::remove(const std::string &path, bool is_abs)
{
	std::string abs_path = path;
	if (is_abs == false && dir_ == nullptr) {
		printf("Current not open any dir!\n");
		return -1;
	} else if (is_abs == false && dir_ != nullptr) {
		abs_path = get_abs_path(path);
	}

	if (exist(abs_path) == false) {
		return 0;
	}

	if (file_type(abs_path.c_str()) == EFileType_File || file_type(abs_path.c_str()) == EFileType_Link) {
		if (::remove(abs_path.c_str()) < 0) { // 系统调用删除文件
			return -1;
		}
	} else if (file_type(abs_path.c_str()) == EFileType_Dir) {
		Directory dir;
		int ret = dir.open_dir(abs_path);
		if (ret < 0) {
			return -1;
		}

		std::vector<SFileType> files_list = dir.file_list();
		for (std::size_t i = 0; i < files_list.size(); ++i) {
			if (files_list[i].name == "." || files_list[i].name == "..") {
				continue;
			}

			if (dir.remove(files_list[i].name) < 0) {
				return -1;
			}
		}

		if (::rmdir(abs_path.c_str()) < 0) {
			return -1;
		}
	} else {
		return -1;
	}

	return 0;
}

int 
Directory::rename(const std::string &old_name, const std::string &new_name, bool is_abs)
{
	int ret = ::rename(old_name.c_str(), new_name.c_str());
	if (ret < 0) {
		LOG_ERROR("rename: %s", strerror(errno));
		return -1;
	}
	return 0;
}

// 拷贝、移动当前目录
int 
Directory::copy(const std::string &des_path)
{
	if (this->exist(des_path, true) == false) {
		LOG_ERROR("des_path is not exist[%s]", des_path.c_str());
		return -1;
	}

	if (dir_ == nullptr) {
		LOG_ERROR("Current not open any dir, can't copy to %s", des_path.c_str());
		return -1;
	}
	
	std::string dir_name = get_curr_dir_name();
	if (dir_name == "") {
		LOG_ERROR("Get dir name failed[%s]", dir_path_.c_str());
		return -1;
	}
	std::string des_abs_path = des_path + "/" + dir_name + "/"; // 要拷贝到的目标目录
	if (this->exist(des_abs_path, true) == true) {
		LOG_ERROR("des_abs_path is exist[%s]", des_abs_path.c_str());
		return -1;
	}

	// 创建并打开目标目录
	RETURN_FUNC_EQ(this->create(des_abs_path, EFileType_Dir), -1, -1);
	Directory dest_dir;
	RETURN_FUNC_EQ(dest_dir.open_dir(des_abs_path), -1, -1);

	struct dirent *dir_ptr = nullptr; 
	while ((dir_ptr = readdir(dir_)) != nullptr) {
		std::string sub_dir_dest_path = des_abs_path + dir_ptr->d_name;
		if (dir_ptr->d_type == 8 || dir_ptr->d_type == 10) { // 普通文件 和 符号链接
			
		} else if (dir_ptr->d_type == 4) { // 目录文件
			Directory sub_dir;
			CONTINUE_FUNC_EQ(sub_dir.copy(sub_dir_dest_path), -1);
		}
	}
}

int 
Directory::move(const std::string &des_path)
{
	return 0;
}

}