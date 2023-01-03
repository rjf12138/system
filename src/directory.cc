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
		printf("Not a directory: %s\n", path.c_str());
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
		printf("realpath: %s\n", strerror(errno));
		return -1;
	}

	dir_path_ = dir_path_ptr;
	dir_ = opendir(dir_path_.c_str());
	if (dir_ == nullptr) {
		printf("opendir: %s\n", strerror(errno));
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
Directory::create(const std::string &path, EFileType type, bool is_abs)
{
	std::string abs_path = path;
	if (is_abs == false && dir_ == nullptr) {
		printf("Current not open any dir!\n");
		return -1;
	} else if (is_abs == false && dir_ != nullptr) {
		abs_path = get_abs_path(path);
	}

	switch (type)
	{
	case EFileType_Dir: {
		int ret = mkdir(abs_path.c_str(), 0755);
		if (ret == -1) {
			printf("mkdir: %s\n", strerror(errno));
			return -1;
		} 
	} break;
	case EFileType_File: {
		//
	} break;
	default:
		printf("Creating this type of file is not supported\n");
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

}

// 拷贝、移动当前目录
int 
Directory::copy(const std::string &des_path)
{

}

int 
Directory::move(const std::string &des_path)
{

}

}