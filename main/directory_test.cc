#include "system.h"
#include "basic/process_control.h"

int main(void)
{
    os::Directory dir_1;
    dir_1.open_dir("./");
    os::Directory dir;
    if (dir.exist("./test", true) == true) {
        dir.remove("./test", true);
    }

    dir.create("./test", os::EFileType_Dir, true, DEFAULT_DIR_RIGHT);
    dir.open_dir("./test");

    dir.create("file1.txt", os::EFileType_File);
    dir.create("file2.txt", os::EFileType_File);
    dir.create("file3.txt", os::EFileType_File);
    dir.create("sub_test", os::EFileType_Dir);

    auto file_list = dir.file_list();
    for (auto &item : file_list) {
        const os::SFileType &file_info = item.second;
        if (file_info.type == os::EFileType_File) {
            os::File file;
            CONTINUE_FUNC_EQ(file.open(file_info.abs_path_), -1);

            basic::ByteBuffer buffer(std::string("Hello, world!!! Directory Test!!!\n"));
            file.write(buffer, buffer.data_size());
        } else {
            os::Directory sub_dir;
            CONTINUE_FUNC_EQ(sub_dir.open_dir(file_info.abs_path_), -1);

            sub_dir.create("file1.txt", os::EFileType_File);
            sub_dir.create("file2.txt", os::EFileType_File);
            sub_dir.create("file3.txt", os::EFileType_File);

            auto sub_file_list = sub_dir.file_list();
            for (auto &sub_item : sub_file_list) {
                const os::SFileType &sub_file = sub_item.second;
                if (sub_file.type == os::EFileType_File) {
                    os::File file;
                    CONTINUE_FUNC_EQ(file.open(sub_file.abs_path_), -1);

                    basic::ByteBuffer buffer(std::string("Hello, world!!! Sub Directory Test!!!\n"));
                    file.write(buffer, buffer.data_size());
                }
            }
        }
    }

    return 0;
}