#include "system.h"

int main(void)
{
    os::Directory dir;
    if (dir.exist("./test", true) == true) {
        dir.remove("./test", true);
    }

    dir.create("./test", os::EFileType_Dir, DEFAULT_DIR_RIGHT, true);
    dir.open_dir("./test");

    dir.create("file1.txt", os::EFileType_File);
    dir.create("file2.txt", os::EFileType_File);
    dir.create("file3.txt", os::EFileType_File);
    dir.create("sub_test", os::EFileType_Dir);

    os::File file;
    std::vector<os::SFileType> file_list = dir.file_list();
    for (int i = 0; i < file_list.size(); ++i) {
        if (file_list[i].type == os::EFileType_File) {
            os::File file;
            CONTINUE_FUNC_EQ(file.open(file_list[i].abs_path_), -1);

            basic::ByteBuffer buffer(std::string("Hello, world!!! Directory Test!!!\n"));
            file.write(buffer, buffer.data_size());
        } else {
            os::Directory sub_dir;
            CONTINUE_FUNC_EQ(sub_dir.open_dir(file_list[i].abs_path_), -1);

            sub_dir.create("file1.txt", os::EFileType_File);
            sub_dir.create("file2.txt", os::EFileType_File);
            sub_dir.create("file3.txt", os::EFileType_File);

            std::vector<os::SFileType> sub_file_list = sub_dir.file_list();
            for (int i = 0; i < sub_file_list.size(); ++i) {
                if (sub_file_list[i].type == os::EFileType_File) {
                    os::File file;
                    CONTINUE_FUNC_EQ(file.open(sub_file_list[i].abs_path_), -1);

                    basic::ByteBuffer buffer(std::string("Hello, world!!! Sub Directory Test!!!\n"));
                    file.write(buffer, buffer.data_size());
                }
            }
        }
    }

    return 0;
}