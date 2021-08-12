#include "system_utils.h"
#include "msg_record.h"

using namespace my_utils;
using namespace system_utils;

int main(void)
{
    Stream stream;
    stream.open("./file");

    stream.write_file_fmt("Hello, World!\n");

    return 0;
}