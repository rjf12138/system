#include "system.h"

using namespace util;
using namespace basic;

int main(void)
{
    File stream;
    stream.open("./file");

    stream.write_file_fmt("Hello, World!\n");

    return 0;
}