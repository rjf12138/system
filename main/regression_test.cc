#include "system.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/stat.h>

#define CHECK(cond) do { \
    if (!(cond)) { \
        std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        return 1; \
    } \
} while (0)

static bool write_file(const std::string &path, const std::string &data)
{
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << data;
    return out.good();
}

static std::string read_file(const std::string &path)
{
    std::ifstream in(path.c_str(), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

static long file_size(const std::string &path)
{
    struct stat info;
    if (::stat(path.c_str(), &info) != 0) {
        return -1;
    }
    return static_cast<long>(info.st_size);
}

static bool exists(const std::string &path)
{
    return ::access(path.c_str(), F_OK) == 0;
}

int main(void)
{
    char root_template[] = "/tmp/system_regression_XXXXXX";
    char *root_ptr = ::mkdtemp(root_template);
    if (root_ptr == nullptr) {
        std::printf("mkdtemp failed\n");
        return 1;
    }
    std::string root(root_ptr);

    // File: write 必须遵守 buf_size
    std::string file_path = root + "/file.bin";
    os::File file;
    CHECK(file.open(file_path, O_RDWR | O_CREAT | O_TRUNC, 0644) == 0);
    basic::ByteBuffer buffer(std::string("abcdef"));
    CHECK(file.write(buffer, 3) == 3);
    CHECK(file_size(file_path) == 3);
    CHECK(file.seek(0, SEEK_SET) >= 0);
    basic::ByteBuffer read_buffer;
    CHECK(file.read(read_buffer, 10) == 3);

    // File: 长格式化写不能越界/泄漏
    std::string long_text(9000, 'A');
    CHECK(file.seek(0, SEEK_SET) >= 0);
    CHECK(file.write_file_fmt("%s", long_text.c_str()) == 9000);
    CHECK(file_size(file_path) == 9000);

    // File: 空文件读取和 procfs 读取
    std::string empty_path = root + "/empty";
    os::File empty_file;
    CHECK(empty_file.open(empty_path, O_RDWR | O_CREAT | O_TRUNC, 0644) == 0);
    basic::ByteBuffer empty_buffer;
    CHECK(empty_file.read(empty_buffer, 4096) == 0);

    os::File proc_file;
    if (proc_file.open("/proc/self/status", O_RDONLY) == 0) {
        basic::ByteBuffer proc_buffer;
        CHECK(proc_file.read(proc_buffer, 4096) > 0);
    }

    // File: set_fd 后必须可读写
    std::string setfd_path = root + "/setfd";
    int fd = ::open(setfd_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    CHECK(fd >= 0);
    {
        os::File fd_file;
        CHECK(fd_file.set_fd(fd, true) == 0);
        basic::ByteBuffer fd_buffer(std::string("x"));
        CHECK(fd_file.write(fd_buffer, 1) == 1);
    }
    CHECK(file_size(setfd_path) == 1);

    // Directory: 文件类型、重复 file_list、create/rename/remove
    std::string work = root + "/work";
    CHECK(::mkdir(work.c_str(), 0755) == 0);
    CHECK(write_file(work + "/regular", "x"));
    CHECK(::mkdir((work + "/dir").c_str(), 0755) == 0);
    CHECK(::symlink("regular", (work + "/link").c_str()) == 0);

    os::Directory directory;
    CHECK(directory.open_dir(work) == 0);
    CHECK(directory.file_type("regular") == os::EFileType_File);
    CHECK(directory.file_type("dir") == os::EFileType_Dir);
    CHECK(directory.file_type("link") == os::EFileType_Link);
    CHECK(directory.file_list().size() == directory.file_list().size());
    CHECK(directory.create("created", os::EFileType_File) == 0);
    CHECK(exists(work + "/created"));
    CHECK(directory.rename("created", "renamed") == 0);
    CHECK(exists(work + "/renamed"));
    CHECK(directory.remove("renamed") == 0);
    CHECK(!exists(work + "/renamed"));

    // Directory: copy 必须保留源文件并完整复制
    std::string src = work + "/src";
    CHECK(::mkdir(src.c_str(), 0755) == 0);
    CHECK(write_file(src + "/a.txt", "hello"));
    CHECK(::mkdir((src + "/sub").c_str(), 0755) == 0);
    CHECK(write_file(src + "/sub/b.txt", "world"));

    std::string dest = root + "/dest";
    CHECK(::mkdir(dest.c_str(), 0755) == 0);
    os::Directory src_dir;
    CHECK(src_dir.open_dir(src) == 0);
    CHECK(src_dir.copy(dest) == 0);
    CHECK(read_file(src + "/a.txt") == "hello");
    CHECK(read_file(dest + "/src/a.txt") == "hello");
    CHECK(read_file(dest + "/src/sub/b.txt") == "world");
    CHECK(directory.remove("src") == 0);
    CHECK(!exists(src));

    // Directory: move
    std::string move_src = work + "/movesrc";
    CHECK(::mkdir(move_src.c_str(), 0755) == 0);
    CHECK(write_file(move_src + "/m.txt", "m"));
    os::Directory move_dir;
    CHECK(move_dir.open_dir(move_src) == 0);
    CHECK(move_dir.move(dest) == 0);
    CHECK(read_file(dest + "/movesrc/m.txt") == "m");
    CHECK(!exists(move_src));

    // Time: 非法输入不能崩溃，合法输入可往返
    CHECK(os::Time::convert_to(std::string("not-a-time"), false) == 0);
    os::mtime_t timestamp = os::Time::convert_to("2026-01-02 03:04:05", true);
    CHECK(timestamp > 0);
    std::string time_text = os::Time::convert_to(timestamp, true);
    CHECK(time_text.find(":000") != std::string::npos);

    // Socket: localhost 解析和创建
    os::SocketTCP socket;
    std::string addr;
    CHECK(socket.get_addr_by_hostname("localhost", addr) == 0);
    CHECK(!addr.empty());
    CHECK(socket.create_socket("localhost", 0) == 0);
    CHECK(socket.get_socket_state());
    socket.close();

    // Mutex
    os::Mutex mutex;
    CHECK(mutex.trylock() == 0);
    CHECK(mutex.unlock() == 0);
    CHECK(mutex.get_errno() == 0);

    // ThreadPool 生命周期（ASan 下验证无 UAF）
    {
        os::ThreadPool pool;
        os::Time::sleep(50);
    }

    std::printf("system regression passed\n");
    return 0;
}
