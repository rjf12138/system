#ifndef __SYSTEM_UTILS__
#define __SYSTEM_UTILS__

#include "basic/basic_head.h"
#include "basic/logger.h"
#include "basic/byte_buffer.h"
#include "data_structure/queue.h"

namespace os {

using namespace basic;

// 设置所有系统函数的回调输出
extern basic::msg_to_stream_callback g_msg_to_stream_trace;
extern basic::msg_to_stream_callback g_msg_to_stream_debug;
extern basic::msg_to_stream_callback g_msg_to_stream_info;
extern basic::msg_to_stream_callback g_msg_to_stream_warn;
extern basic::msg_to_stream_callback g_msg_to_stream_error;
extern basic::msg_to_stream_callback g_msg_to_stream_fatal;

extern void set_systemcall_message_output_callback(basic::InfoLevel level, basic::msg_to_stream_callback func);

// 执行shell命令行
extern int exe_shell_cmd(std::string &result, const char *format, ...);
extern int exe_shell_cmd_to_stdin(const char *format, ...);

/*
* 所有的类成员函数成功了返回0， 失败返回非0值
*/
//////////////////////////// 时间 //////////////////////////////////////////////////
typedef uint64_t mtime_t; 
typedef struct stime {
    uint32_t year;
    uint32_t month;
    uint32_t days;
    uint32_t hours;
    uint32_t mins;
    uint32_t secs;
    uint32_t wday;

    stime(void)
    :year(0),
    month(0),
    days(0),
    hours(0),
    mins(0),
    secs(0),
    wday(0) {}
} stime_t;

// 时间转换：默认格式： YY-MM-DD hh:mm:ss:ms
#define DEFAULT_TIME_FMT "%Y-%m-%d %H:%M:%S"

class Time : public Logger{
public:
    Time(void);
    ~Time(void);


    // 获取当前时间
    static mtime_t now(void);
    // 获取当前时间
    static stime_t snow(void);
    // 格式化当前时间
    static std::string format(bool mills_enable = true, const char *fmt = DEFAULT_TIME_FMT);

    // 设置时间
    void set_time(mtime_t tm);
    // 当前时间加上一段时间
    void add(const stime_t &t);
    // 当前时间减去一段时间
    void sub(const stime_t &t);

public:
    // 线程休眠时间
    static int sleep(uint32_t mills);
    // 字符串转成毫秒数
    static mtime_t convert_to(const std::string &ti, bool mills_enable = true, const char *fmt = DEFAULT_TIME_FMT);
    // 毫秒数转成字符串
    static std::string convert_to(const mtime_t &ti, bool mills_enable = true, const char *fmt = DEFAULT_TIME_FMT);
    // 将秒数转成结构体
    static stime_t convert_to(const time_t &ti);

private:
    Time(const Time&) = delete;
    Time& operator=(const Time&) = delete;

private:
    mtime_t time_;
};

/////////////////////////////// 终端界面 ///////////////////////////////////////////
#define PrintChars  "~`!@#$%^&*()_-+={}[]\\|:;\"',<.>/? *-+"
#define ProjWin_RetEmpty    ""
#define ProjWin_InputPath   "NULL"
// 定义key
#define Keyboard_Tab        9
#define Keyboard_Enter      10
#define Keyboard_Esc        27

#define Form_MaxInput       69

class TerminalWindow {
public:
    TerminalWindow(void);
    virtual ~TerminalWindow(void);

    /// 失败返回"", 主动输入路径返回 "-Input"
    std::pair<std::string, std::string> display_menu(std::vector<std::string> proj_name, std::vector<std::string> proj_path);
    /// 获取输入
    int get_input(std::string &ret, std::string title, std::string default_value = "");
    // 消息提示
    bool message(std::string title);

private:
    void print_in_middle(void *win, int starty, int startx, int width, const char *string, uint32_t color);
};
/////////////////////////////// 获取系统信息 ////////////////////////////////////////
class SystemInfo {
public:
    SystemInfo(void);
    ~SystemInfo(void);

    // 获取当前用户可以使用的 cpu 核心数 
    static int get_nprocs(void);
    // 获取 cpu 总的核心数
    static int get_nprocs_conf(void);
};

/////////////////////////////// 互斥量 /////////////////////////////////////////////
class Mutex : public basic::Logger {
public:
    Mutex(bool is_show_lock_info = false);
    virtual ~Mutex(void);

    virtual int lock(const std::string& func_pos = __FILE__, int pos = __LINE__);
    virtual int trylock(const std::string& func_pos = __FILE__, int pos = __LINE__);
    virtual int unlock(const std::string& func_pos = __FILE__, int pos = __LINE__);
    virtual int get_errno(void) { return errno_;}

#ifdef __RJF_LINUX__
    pthread_mutex_t* get_mutex(void) {return mutex_ptr_;}
#endif

private:
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

private:
    int errno_;
    bool is_show_lock_info_;
    bool is_locked_;
    mtime_t lock_id_;
#ifdef __RJF_LINUX__
    pthread_mutex_t *mutex_ptr_;
#endif
};

#define MUTEX_LOCK(mutex) (mutex.lock(__FILE__, __LINE__))
#define MUTEX_TRY_LOCK(mutex) (mutex.try_lock(__FILE__, __LINE__))
#define MUTEX_UNLOCK(mutex) (mutex.unlock(__FILE__, __LINE__))

//////////////////////////////////////////////////////////////////////////////////////
template<class T>
class Atomic {
public:
    Atomic(void);
    ~Atomic(void);

    static bool compare_and_swap(T *reg, T old_value, const T &new_value)
    {
    #ifdef __RJF_LINUX__
        T ret = 0;
        __asm__ volatile ("lock; cmpxchg %2, %3"
                            : "=a" (ret), "=m" (*reg)
                            : "r" (new_value), "m" (*reg), "0" (old_value)
                            : "cc");
        return old_value == ret;
    #elif __RJF_WINDOWS__
    #endif
    }
    
    static uint32_t fetch_and_add(volatile uint32_t *val, uint32_t incr)
    {
    #ifdef __RJF_LINUX__
        unsigned int result = 0;
        __asm__ volatile ("lock; xadd %0, %1"
            : "=r" (result), "=m" (*val)
            : "0" (incr), "m" (*val)
            : "memory");
        return result;
    #elif __RJF_WINDOWS__
    #endif
    }

    static unsigned int fetch_and_sub(volatile unsigned int* p, unsigned int decr)
    {
    #ifdef __RJF_LINUX__
        unsigned int result;
        __asm__ __volatile__ ("lock; xadd %0, %1"
                :"=r"(result), "=m"(*p)
                :"0"(-decr), "m"(*p)
                :"memory");
        return result;
    #elif __RJF_WINDOWS__
    #endif
    }
private:
};

/////////////////////////////// 线程、线程池 ///////////////////////////////////////////
// 线程回调函数
#ifdef __RJF_LINUX__
    typedef void *(*thread_callback)(void*);
    typedef pthread_t thread_id_t;
#endif


// 通用线程类，直接继承它，设置需要重载的函数
class Thread : public basic::Logger {
public:
    Thread(void);
    virtual ~Thread(void);

    // 线程初始化
    virtual int init(void);

    // 以下函数需要重载
    // 线程调用的主体
    virtual int run_handler(void);
    // 设置结束标志，调用完后线程结束运行
    virtual int stop_handler(void);
    // 设置开始标识，设置完后线程可以运行
    virtual int start_handler(void);

    // 如何处理结构体的线程id
    // 获取当前线程类id
    thread_id_t get_thread_id(void) const {return thread_id_;}
    // 获取当前运行的线程id
    static thread_id_t current_thread_id(void);

private:
    static void* create_func(void *arg);

private:
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

private:
    bool is_init_;
#ifdef __RJF_LINUX__
    pthread_attr_t attr_;
    thread_id_t thread_id_;
#endif
};

///////////////////////////////////////// Thread Pool /////////////////////////////////
enum TaskWorkState {
    THREAD_TASK_WAIT,
    THREAD_TASK_HANDLE,
    THREAD_TASK_COMPLETE,
    THREAD_TASK_ERROR,
};

struct Task {
    int state;
    void *thread_arg;
    void *exit_arg;
    thread_callback work_func;
    thread_callback exit_task;

    Task()
    :state(THREAD_TASK_WAIT),
    thread_arg(nullptr),
    exit_arg(nullptr),
    work_func(nullptr),
    exit_task(nullptr) {}
};

// thread_pool 的工作线程。
// 从 threadpool 工作队列中领任务，任务完成后暂停
// 有新任务分配时，启动继续执行任务
enum ThreadState {
    WorkThread_RUNNING,
    WorkThread_WAITING,
    WorkThread_WAITINGEXIT,
    WorkThread_EXIT,
};

class ThreadPool;
class WorkThread : public Thread {
public:
    WorkThread(ThreadPool *thread_pool, int idle_life = 30);
    virtual ~WorkThread(void);

    // 以下函数需要重载
    // 线程调用的主体
    virtual int run_handler(void);
    virtual int stop_handler(void);
    virtual int start_handler(void);

    // 暂停线程
    virtual int pause(void);
    // 继续执行线程
    virtual int resume(void);

    virtual ThreadState get_current_state(void) const {return state_;}

private:
    void thread_cond_wait(void);
    void thread_cond_signal(void);

private:
    WorkThread(const WorkThread&) = delete;
    WorkThread& operator=(const WorkThread&) = delete;

private:
    time_t idle_life_; // 单位：秒
    time_t start_idle_life_;
    ThreadState state_;

    Task task_;
    Mutex mutex_;
    ThreadPool *thread_pool_;
    
#ifdef __RJF_LINUX__
    pthread_cond_t thread_cond_;
#endif
};

// 添加时间轮来防止空闲的线程过多
#define MAX_THREADS_NUM         500     // 最多 500 个线程
#define MAX_THREAD_IDLE_LIFE    1800    // 最长半小时
enum ThreadPoolExitAction {
    UNKNOWN_ACTION,                     // 未知行为
    WAIT_FOR_ALL_TASKS_FINISHED,        // 等待所有任务都完成
    SHUTDOWN_ALL_THREAD_IMMEDIATELY,    // 立即关闭所有线程
};

struct ThreadPoolConfig {
    std::size_t threads_num;             // 线程数
    std::size_t max_waiting_task;        // 缓存中保存的最大任务数
    ThreadPoolExitAction threadpool_exit_action; // 默认时强制关闭所有线程
};

struct ThreadPoolRunningInfo {
    int running_threads_num;
    int idle_threads_num;
    int waiting_tasks;
    int waiting_prio_tasks;
    std::string curr_status;
    std::string exit_action;
    ThreadPoolConfig config;
};

class ThreadPool : public Thread {
    friend WorkThread;
public:
    ThreadPool(void);
    virtual ~ThreadPool(void);

    // 以下函数需要重载
    // 线程调用的主体
    virtual int run_handler(void);
    virtual int stop_handler(void);
    virtual int start_handler(void);

    // 添加普通任务
    int add_task(Task &task);
    // 任务将被优先执行
    int add_priority_task(Task &task);

    // 设置最小的线程数量，当线程数量等于它时，线程即使超出它寿命依旧不杀死
    int set_threadpool_config(const ThreadPoolConfig &config);
    // 获取线程池配置
    ThreadPoolConfig get_threadpool_config(void) const {return thread_pool_config_;}
    
    // 获取线程运行状态
    ThreadPoolRunningInfo get_running_info(void);
    // 实时打印当前线程池信息
    void show_threadpool_info(void);

private:
    // 关闭线程池中的所有线程
    int shutdown_all_threads(void);

    // 获取任务，优先队列中的任务先被取出,有任务返回大于0，否则返回等于0
    // 除了工作线程之外，其他任何代码都不要去调用该函数，否则会导致的任务丢失
    int get_task(Task &task);

    // 随机唤醒一个线程
    void wakeup_random_thread(void);
    // 调整线程数量
    int ajust_threads_num(void);
    
    // 打印线程池信息
    static void *print_threadpool_info(void *arg);

private:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    bool print_info_;
    bool exit_;
    ThreadPoolConfig thread_pool_config_;// 多余的空闲线程会被杀死，直到数量达到最小允许的线程数为止。单位：秒

    Mutex thread_mutex_;
    Mutex task_mutex_;

    ds::Queue<Task> tasks_;   // 普通任务队列
    ds::Queue<Task> priority_tasks_; // 优先任务队列
    std::map<thread_id_t, WorkThread*> threads_; // 运行中的线程列表
};
//////////////////////////////// 目录操作 /////////////////////////////////////////////////
enum EFileType {
	EFileType_Unknown = -1,
	EFileType_File,
	EFileType_Dir,
	EFileType_Link,
};

struct SFileType {
	EFileType type;
	std::string name;
	std::string abs_path_;
};

#define DEFAULT_DIR_RIGHT  (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
// 存在问题，要不统一使用绝对路径
class Directory : public basic::Logger {
public:
    Directory(void);
    ~Directory(void);

    // 获取程序当前运行路径
    static std::string get_program_running_dir(void);
    // 修改程序当前的运行路径
	static int change_program_running_dir(std::string &new_path);
    // 获取可执行文件所在目录
    static std::string get_cur_executable_path(bool exec_file_path = true);

    // 获取当前打开的目录路径
	std::string get_curr_dir_path(void);
    // 获取当前打开目录名称
	std::string get_curr_dir_name(void);

    // 打开目录
	int open_dir(const std::string &path);
	std::map<std::string, SFileType> file_list(bool ret_default_dir = false); // 是否返回 . 和 .. 目录

	std::string get_abs_path(const std::string &file_path);// 获取当前打开目录下文件的绝对路径

	// is_abs == true 表示使用绝对路径, is_abs == false 表示使用当前打开目录下。
    // 指定路径下文件/目录是否存在
	bool exist(const std::string &file_path, bool is_abs = false);
    // 指定路径文件的类型
	EFileType file_type(const std::string  &file_path, bool is_abs = false);
    // 创建文件或者是目录
	int create(const std::string &path, EFileType type, bool is_abs = false, uint mode = DEFAULT_DIR_RIGHT);
    // 删除文件或目录
	int remove(const std::string &path, bool is_abs = false);
    // 重命名文件或目录
	int rename(const std::string &old_name, const std::string &new_name, bool is_abs = false);

	// 拷贝、移动当前目录
	int copy(const std::string &des_path);
	int move(const std::string &des_path);

private:
    Directory(const Directory&) = delete;   // 禁止拷贝
    Directory& operator=(const Directory&) = delete;

private:
    DIR* dir_;
	std::string dir_path_;
	bool is_open_dir_;
};

/////////////////////////////// 文件流 ////////////////////////////////////////////////////
// 修改一下成功返回0，失败返回非0
#define DEFAULT_OPEN_FLAG   O_RDWR
#define DEFAULT_FILE_RIGHT  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

class File : public basic::Logger {
public:
    File(void);
    virtual ~File(void);

    // 根据文件路径打开文件
    int open(const std::string file_path, int flag = DEFAULT_OPEN_FLAG, int file_right = DEFAULT_FILE_RIGHT);
    // 根据文件描述符打开文件
    // 默认退出时不关闭文件描述符
    int set_fd(int fd, bool open_when_exit = true);
    int get_fd(void) const {return fd_;};

    // 返回文件信息
    int fileinfo(struct stat &file_info);
    // 返回文件的大小
    off_t file_size() {return file_info_.st_size;}
    // 关闭文件
    int close_file(void);
    // 检查文件描述符
    int check_fd(int fd);
    // 清空文件
    int clear_file(void);
    
    // 设置文件偏移
    off_t seek(off_t offset, int whence);
    // 返回当前文件偏移
    off_t curr_pos(void);

    // 从当前位置读取任意数据
    ssize_t read(basic::ByteBuffer &buff, size_t buf_size);
    // 从某一位置读取数据
    ssize_t read_from_pos(basic::ByteBuffer &buff, size_t buf_size, off_t pos, int whence);

    // 写数据
    ssize_t write(basic::ByteBuffer &buff, size_t buf_size);
    // 从某一位置写数据
    ssize_t write_to_pos(basic::ByteBuffer &buff, size_t buf_size ,off_t pos, int whence);

    // 格式化读/写到缓存中
    static ssize_t read_buf_fmt(basic::ByteBuffer &buff, const char *fmt, ...);
    static ssize_t write_buf_fmt(basic::ByteBuffer &buff, const char *fmt, ...);

    // 格式化写到文件中
    ssize_t write_file_fmt(const char *fmt, ...);

    // 从file中拷贝文件数据到当前文件中
    ssize_t copy(File &file);
private:
    File(const File&) = delete;
    File& operator=(const File&) = delete;

private:
    int fd_;
    bool open_on_exit_;
    bool file_open_flag_;
    unsigned max_lines_;
    struct stat file_info_;
};


/////////////////////////////// 网络套接字 /////////////////////////////////////////////////
// IPv4: 只支持TCP
class SocketTCP : public basic::Logger {
public:
    SocketTCP(void);
    SocketTCP(std::string ip, uint16_t port);
    virtual ~SocketTCP();

    // 设置不是由本类所创建的套接字
    int set_socket(int clisock, struct sockaddr_in *cliaddr = nullptr, socklen_t *addrlen = nullptr);
    // ip,port可以是本地的用于创建服务端程序，也可以是客户端连接到服务端IP和端口
    int create_socket(std::string ip, uint16_t port);

    // 这里listen()合并了 bind()和listen
    int listen(void);
    // 客户端连接到服务端
    int connect(void);
    // 由TCP服务器调用
    // clisock: 返回的客户端socket
    // cliaddr: 返回的客户端IP信息
    // addrlen: 设置cliaddr 结构的大小
    int accept(int &clisock, struct sockaddr *cliaddr = nullptr, socklen_t *addrlen = nullptr);
    ssize_t recv(basic::ByteBuffer &buff, int flags = 0);
    ssize_t send(basic::ByteBuffer &buff, int flags = 0);

    // 配置socket
    // 设置成非阻塞
    int setnonblocking(void);
    // 设置成地址复用
    int set_reuse_addr(void);
    
    // 获取socket信息
    int get_socket(void);
    int get_ip_info(std::string &ip, uint16_t &port);
    std::string get_ip_info(void);
    bool get_socket_state(void) const;
    int get_addr_by_hostname(std::string hostname, std::string &addr);
    bool check_ip_addr(std::string addr);

    // 关闭套接字
    int close(void);
    // 禁用套接字I/O
    // how:的选项
    // SHUT_WR(关闭写端)
    // SHUT_RD(关闭读端)
    // SHUT_RDWR(关闭读和写)
    int shutdown(int how);

public: // 调试代码
    void print_family(struct addrinfo *aip);
    void print_type(struct addrinfo *aip);
    void print_protocol(struct addrinfo *aip);
    void print_flags(struct addrinfo *aip);
    void test_getaddr(std::string str);

private:
    SocketTCP(const SocketTCP&) = delete;
    SocketTCP& operator=(const SocketTCP&) = delete;

private:
    bool is_enable_;
    int socket_;

    uint16_t port_;
    std::string ip_;

    struct sockaddr_in addr_;
};

}

#endif