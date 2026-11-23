#include "system.h"

using namespace os;
using namespace basic;

int main(void)
{
    File stream;
    stream.open("./file");

    stream.write_file_fmt("Hello, World!\n");

    // 回归检查：空文件读取不能崩溃，且从文件开头读取应成功
    File empty_stream;
    if (empty_stream.open("./empty_file", O_RDWR | O_CREAT | O_TRUNC, 0644) != 0) {
        std::cout << "open empty file failed" << std::endl;
        return 1;
    }

    ByteBuffer empty_buffer;
    if (empty_stream.read(empty_buffer, 4096) != 0) {
        std::cout << "read empty file failed" << std::endl;
        return 1;
    }

    if (empty_stream.read_from_pos(empty_buffer, 4096, 0, SEEK_SET) != 0) {
        std::cout << "read_from_pos empty file failed" << std::endl;
        return 1;
    }

    return 0;
}