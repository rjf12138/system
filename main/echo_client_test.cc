#include "system.h"

using namespace basic;
using namespace util;
using namespace std;

void *echo_handler(void *arg);
void *echo_exit(void *arg);

ThreadPool pool;
File write_file;
bool exit_state = false;

void sig_int(int sig);

int main(void)
{
    if (signal(SIGINT, sig_int) == SIG_ERR) {
        LOG_GLOBAL_DEBUG("signal error!");
        return 0;
    }

    write_file.open("./file.txt");
    ByteBuffer buff;
    string str;

    std::size_t min_thread = 45;
    std::size_t max_thread = 45;
    ThreadPoolConfig config = {min_thread, max_thread, 30, SHUTDOWN_ALL_THREAD_IMMEDIATELY};
    pool.init();
    pool.set_threadpool_config(config);

    for (std::size_t j = 1;j <= min_thread;++j) {
        for (std::size_t i = 0; i < j; ++i) {
            SocketTCP *cli = new SocketTCP("127.0.0.1", 12138);
            Task task;
            task.exit_arg = cli;
            task.thread_arg = cli;
            task.work_func = echo_handler;
            task.exit_task = echo_exit;

            if (cli->get_socket_state() == false) {
                delete cli;
                cli = nullptr;
                continue;
            }
            write_file.write_file_fmt("create: %d\n", cli->get_socket());
            pool.add_task(task);
        }
        if (j == min_thread) {
            j = 1;
        }
        if (exit_state == true)
            break;
        Time::sleep(3000);
    }
    pool.wait_thread();

    return 0;
}

void sig_int(int sig)
{
    LOG_GLOBAL_DEBUG("Exit client!");
    pool.stop_handler();
    exit_state = true;
}

void *echo_handler(void *arg)
{
    if (arg == nullptr) {
        return nullptr;
    }

    string data = "{\"data\": \"client\", \"num\":12138}";
    ByteBuffer buff;

    SocketTCP *cli_info = (SocketTCP*)arg;
    string cli_ip;
    uint16_t cli_port;
    cli_info->get_ip_info(cli_ip, cli_port);
    write_file.write_file_fmt("connect: %d\n", cli_info->get_socket());
    if (cli_info->connect() != 0) {
        LOG_GLOBAL_ERROR("Client: %s:%d sockeid: %d connect failed!", cli_ip.c_str(), cli_port, cli_info->get_socket());
        goto end;
    }
    LOG_GLOBAL_DEBUG("Client: %s:%d sockeid: %d connect successed!", cli_ip.c_str(), cli_port, cli_info->get_socket());
    cli_info->set_reuse_addr();    

    for (int i = 0; i <= 10 && cli_info->get_socket_state(); ++i) {
        buff.clear();
        if (i == 10) {
            buff.write_string("quit");
        } else {
            buff.write_string(data);
        }

        cli_info->send(buff, 0);
        cli_info->recv(buff, 0);
        string recv_data;
        buff.read_string(recv_data);
        if (recv_data != data && recv_data != "quit") {
            LOG_GLOBAL_DEBUG("Recv from server: %s", recv_data.c_str());
            LOG_GLOBAL_WARN("Client: %s:%d exit error!", cli_ip.c_str(), cli_port);
            break;
        } else {
            LOG_GLOBAL_DEBUG("Recv from server: %s", recv_data.c_str());
            if (recv_data == "quit") {
                break;
            }
        }

         Time::sleep(100);
    }
end:
    LOG_GLOBAL_DEBUG("Client: %s:%d exit successed!", cli_ip.c_str(), cli_port);
    delete cli_info;
    cli_info = nullptr;

    return nullptr;
}

void *echo_exit(void *arg)
{
    if (arg == nullptr) {
        return nullptr;
    }

    SocketTCP *cli_info = (SocketTCP*)arg;
    cli_info->close();

    return nullptr;
}