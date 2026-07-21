/**
 * @file    TaskController.h
 * @brief   Lớp quản lý vòng đời của một POSIX thread gắn với một đối tượng C++.
 *
 * @details TaskController là một template wrapper bao quanh POSIX pthread API,
 *          cho phép gắn một member function bất kỳ của lớp T làm entry point
 *          cho một thread. Lớp cung cấp các thao tác cơ bản:
 *            - Tạo và đặt tên thread (Start)
 *            - Chờ thread kết thúc có/không có timeout (Join)
 *            - Truy vấn tên thread đang chạy (GetTaskControllerName)
 *
 *          **Thread safety**: Mỗi instance chỉ được phép gọi Start() một lần.
 *          Copy constructor và copy assignment bị xóa để ngăn sao chép nhầm.
 *
 * @note    Phụ thuộc vào GNU extension:
 *            - `pthread_setname_np` — đặt tên thread (Linux-specific).
 *            - `pthread_timedjoin_np` — join có timeout (Linux-specific).
 *          Compile với `-lpthread`.
 *
 * @tparam  T   Kiểu của đối tượng chứa hàm entry point.
 *
 * @version 1.0
 * @date    2026-07-15
 */

#ifndef TASK_CONTROLLER_H
#define TASK_CONTROLLER_H

#include <string>    ///< std::string — dùng cho tên thread
#include <cerrno>    ///< errno — mã lỗi POSIX
#include <cstdint>   ///< uint8_t, uint32_t — kiểu nguyên cố định độ rộng
#include <cstdio>    ///< (dự phòng) printf/snprintf nếu cần log
#include <cstring>   ///< strncpy — copy tên thread an toàn
#include <ctime>     ///< clock_gettime, timespec — tính thời gian timeout
#include <pthread.h> ///< POSIX Threads API

/**
 * @class   TaskController
 * @brief   Wrapper POSIX thread gắn với một member function của lớp T.
 *
 * @details Mỗi instance TaskController<T> quản lý đúng **một** thread.
 *          Entry point của thread là một non-static member function có chữ ký
 *          `void T::func(void)`, được truyền vào constructor dưới dạng
 *          con trỏ đến member function (pointer-to-member).
 *
 *          Sơ đồ vòng đời:
 *          @code
 *          [Constructed] --Start()--> [Running] --Join()--> [Stopped]
 *          @endcode
 *
 * @tparam  T   Kiểu lớp chứa hàm entry; T phải tồn tại trong suốt vòng đời
 *              của thread (không được destroy trước khi Join() trả về).
 *
 * @example
 * @code
 * class Worker {
 * public:
 *     void Run() { while (!done) { ... } }
 *     bool done = false;
 * };
 *
 * Worker w;
 * TaskController<Worker> tc(&w, &Worker::Run);
 * tc.Start("worker-thread");
 * // ... làm việc khác ...
 * w.done = true;
 * tc.Join();
 * @endcode
 */
template <typename T>
class TaskController
{
public:
    /**
     * @typedef ENTRY_FUNC_t
     * @brief   Kiểu con trỏ đến member function không nhận tham số, không trả về.
     *
     * @details Con trỏ này chỉ hợp lệ khi gọi thông qua một instance cụ thể
     *          của T. Bên trong lớp, nó được gọi bằng cú pháp:
     *          `(obj->*entry)()`.
     */
    using ENTRY_FUNC_t = void (T::*)(void);

    /**
     * @brief   Khởi tạo TaskController với đối tượng và hàm entry point.
     *
     * @param   obj     Con trỏ đến đối tượng T mà entry function thuộc về.
     *                  Không được là nullptr; phải còn hiệu lực cho đến khi
     *                  Join() hoàn thành.
     * @param   entry   Con trỏ đến member function của T sẽ chạy trong thread.
     *
     * @note    Constructor không khởi động thread. Gọi Start() để bắt đầu.
     */
    TaskController(T *obj, ENTRY_FUNC_t entry);

    /**
     * @brief   Destructor — giải phóng tài nguyên nội bộ, **không** join thread.
     *
     * @warning Nếu thread vẫn đang chạy khi destructor được gọi, handle sẽ bị
     *          reset nhưng thread OS-level vẫn tiếp tục hoạt động (resource leak).
     *          Luôn gọi Join() trước khi để đối tượng bị hủy.
     */
    virtual ~TaskController();

    /**
     * @brief   Khởi động thread với tên mặc định (rỗng — kernel tự đặt).
     *
     * @return  `true`  nếu thread được tạo thành công.
     * @return  `false` nếu thread đã đang chạy hoặc `pthread_create` thất bại.
     *
     * @see     Start(const std::string&)
     */
    bool Start(void);

    /**
     * @brief   Khởi động thread và đặt tên cho thread.
     *
     * @details Tên thread bị giới hạn 15 ký tự hữu ích (MAX_NAME_LEN - 1) do
     *          giới hạn của kernel Linux (`TASK_COMM_LEN = 16` kể cả null-terminator).
     *          Nếu tên rỗng, bước đặt tên bị bỏ qua.
     *
     * @param   name    Tên muốn đặt cho thread (hiển thị trong `/proc/<pid>/comm`
     *                  và công cụ như `ps`, `top`, `htop`).
     *
     * @return  `true`  nếu thread được tạo thành công (dù đặt tên có thể thất bại).
     * @return  `false` nếu thread đã đang chạy hoặc `pthread_create` thất bại.
     *
     * @note    Thất bại khi đặt tên không làm Start() trả về false — thread vẫn
     *          chạy với tên mặc định của kernel.
     */
    bool Start(const std::string &name);

    /**
     * @brief   Chờ thread kết thúc (không có timeout).
     *
     * @details Gọi `pthread_join` để block cho đến khi thread hoàn thành.
     *          Nếu thread chưa bắt đầu (m_is_run == false), hàm trả về `true`
     *          ngay lập tức (không phải lỗi).
     *
     * @return  `true`  nếu thread kết thúc thành công hoặc chưa từng chạy.
     * @return  `false` nếu `pthread_join` thất bại (ví dụ: thread đã được join
     *                  bởi luồng khác, hoặc handle không hợp lệ).
     *
     * @note    Sau khi Join() thành công, m_is_run được reset về false, cho phép
     *          gọi lại Start() nếu cần.
     */
    bool Join(void);

    /**
     * @brief   Chờ thread kết thúc với giới hạn thời gian (timeout).
     *
     * @details Sử dụng `pthread_timedjoin_np` (GNU extension) với thời gian
     *          tuyệt đối được tính bằng cách lấy `CLOCK_REALTIME` hiện tại
     *          cộng thêm `timeout` mili-giây.
     *
     *          Công thức tính deadline:
     *          @code
     *          deadline = clock_gettime(CLOCK_REALTIME) + timeout_ms
     *          @endcode
     *
     * @param   timeout Thời gian chờ tối đa tính bằng **mili-giây**.
     *
     * @return  `true`  nếu thread kết thúc trong khoảng thời gian cho phép.
     * @return  `false` nếu:
     *            - Không lấy được thời gian thực (clock_gettime lỗi).
     *            - Thread không kết thúc trong thời gian timeout (ETIMEDOUT).
     *            - pthread_timedjoin_np trả về lỗi khác.
     *
     * @warning `pthread_timedjoin_np` là GNU extension, chỉ khả dụng trên Linux.
     *          Không portable sang macOS hay BSD.
     */
    bool Join(uint32_t timeout);

    /**
     * @brief   Lấy tên hiện tại của thread đang chạy.
     *
     * @details Gọi `pthread_getname_np` để đọc tên thread từ kernel.
     *          Hàm chỉ hoạt động khi thread đang ở trạng thái running.
     *
     * @param[out]  name    Con trỏ đến std::string để nhận tên thread.
     *                      Không được là nullptr.
     *
     * @return  `true`  nếu tên được lấy thành công và ghi vào *name.
     * @return  `false` nếu:
     *            - `name` là nullptr.
     *            - Thread chưa chạy (m_is_run == false).
     *            - `pthread_getname_np` thất bại.
     */
    bool GetTaskControllerName(std::string *const name);

    /**
     * @brief   Copy constructor bị xóa — TaskController không được phép sao chép.
     *
     * @details Sao chép một thread controller sẽ dẫn đến hai instance cùng
     *          quản lý một pthread handle, gây double-join hoặc race condition.
     */
    TaskController(const TaskController &rhs) = delete;

    /**
     * @brief   Copy assignment bị xóa — TaskController không được phép gán sao chép.
     *
     * @see     TaskController(const TaskController&)
     */
    TaskController &operator=(const TaskController &rhs) = delete;

private:
    /**
     * @brief   Độ dài tối đa của tên thread (bao gồm null-terminator).
     *
     * @details Linux kernel giới hạn `TASK_COMM_LEN = 16` byte cho tên task/thread.
     *          Tên hiệu dụng tối đa là 15 ký tự có nghĩa.
     */
    static constexpr uint8_t MAX_NAME_LEN = 16;

    /**
     * @brief   Hàm trung gian (thunk) được truyền vào `pthread_create`.
     *
     * @details `pthread_create` yêu cầu entry point có chữ ký `void*(void*)`.
     *          Hàm này nhận `param` là con trỏ `this` (TaskController<T>*),
     *          ép kiểu và gọi member function `m_entry` trên đối tượng `m_obj`.
     *
     *          Cơ chế gọi member function qua pointer:
     *          @code
     *          ((self->m_obj)->*(self->m_entry))();
     *          @endcode
     *
     * @param   param   Con trỏ `void*` trỏ đến instance TaskController<T>.
     *                  Phải khác nullptr; nếu nullptr hàm trả về ngay.
     *
     * @return  Luôn trả về `nullptr` (không có giá trị thread trả về).
     */
    static void *EntryImpl(void *param);

    bool          m_is_run; ///< Trạng thái: `true` nếu thread đang chạy, `false` nếu chưa/đã dừng.
    T            *m_obj;    ///< Con trỏ đến đối tượng sở hữu entry function.
    ENTRY_FUNC_t  m_entry;  ///< Con trỏ đến member function sẽ chạy trong thread.
    pthread_t     m_handle; ///< Handle của POSIX thread, khởi tạo là 0.
};

/* =========================================================================
 * Phần triển khai template (phải nằm trong header do quy tắc của C++)
 * ========================================================================= */

/**
 * @brief   Triển khai TaskController::TaskController(T*, ENTRY_FUNC_t).
 */
template <typename T>
TaskController<T>::TaskController(T *obj, ENTRY_FUNC_t entry)
    : m_is_run(false), m_obj(obj), m_entry(entry), m_handle(0) {}

/**
 * @brief   Triển khai TaskController::~TaskController().
 */
template <typename T>
TaskController<T>::~TaskController()
{
    /* Reset handle và con trỏ để tránh dangling reference,
     * nhưng KHÔNG join thread — caller có trách nhiệm gọi Join() trước. */
    m_handle = 0;
    m_entry  = nullptr;
    m_obj    = nullptr;
}

/**
 * @brief   Triển khai TaskController::Start(void).
 */
template <typename T>
bool TaskController<T>::Start(void)
{
    /* Delegate sang overload có tham số tên, với tên rỗng (dùng tên mặc định). */
    return Start("");
}

/**
 * @brief   Triển khai TaskController::Start(const std::string&).
 */
template <typename T>
bool TaskController<T>::Start(const std::string &name)
{
    /* Ngăn start thread hai lần từ cùng một instance. */
    if (true == m_is_run)
    {
        return false;
    }

    /* Tạo thread mới; EntryImpl là hàm thunk, `this` là tham số ngữ cảnh. */
    int ret = pthread_create(&m_handle, nullptr, EntryImpl, this);
    if (0 > ret)
    {
        //_STD_ERR(ret);   // Ghi lỗi nếu có logger
        return false;
    }

    /* Đặt tên thread nếu caller cung cấp tên khác rỗng. */
    if ("" != name)
    {
        char buf[MAX_NAME_LEN] = {0};
        /* strncpy đảm bảo không tràn bộ đệm; byte cuối vẫn là '\0' vì buf đã zero-init. */
        strncpy(buf, name.c_str(), MAX_NAME_LEN - 1);
        ret = pthread_setname_np(m_handle, buf);
        if (0 != ret)
        {
            //_STD_ERR(ret);
            //_LOGW("Set thread name failed. thread name is default.");
            /* Không return false — thread đã chạy dù đặt tên thất bại. */
        }
    }

    m_is_run = true;
    return true;
}

/**
 * @brief   Triển khai TaskController::Join(void).
 */
template <typename T>
bool TaskController<T>::Join(void)
{
    /* Nếu thread chưa bao giờ chạy, coi như join thành công. */
    if (false == m_is_run)
    {
        return true;
    }

    /* Block cho đến khi thread hoàn thành; bỏ qua giá trị trả về của thread. */
    int ret = pthread_join(m_handle, nullptr);
    if (0 != ret)
    {
        //_STD_ERR(ret);
        return false;
    }

    /* Reset trạng thái để cho phép tái sử dụng instance (Start lại nếu cần). */
    m_handle = 0;
    m_is_run = false;

    return true;
}

/**
 * @brief   Triển khai TaskController::Join(uint32_t).
 */
template <typename T>
bool TaskController<T>::Join(uint32_t timeout)
{
    /* Nếu thread chưa bao giờ chạy, coi như join thành công. */
    if (false == m_is_run)
    {
        return true;
    }

    /* Lấy thời gian hiện tại làm điểm xuất phát tính deadline. */
    struct timespec tmspec;
    if (0 > clock_gettime(CLOCK_REALTIME, &tmspec))
    {
        //_STD_ERR(errno);
        return false;
    }

    /* Chuyển đổi timeout từ mili-giây sang giây + nano-giây. */
    time_t sec  = static_cast<time_t>(timeout) / 1000;           // Phần nguyên giây
    long   nsec = static_cast<long>((timeout % 1000) * 1000 * 1000); // Phần dư (ns)

    tmspec.tv_sec  += sec;
    tmspec.tv_nsec += nsec;

    /* pthread_timedjoin_np trả về ETIMEDOUT nếu thread không kết thúc kịp. */
    int ret = pthread_timedjoin_np(m_handle, nullptr, &tmspec);
    if (0 != ret)
    {
        //_STD_ERR(ret);
        return false;
    }

    /* Reset trạng thái sau khi join thành công. */
    m_handle = 0;
    m_is_run = false;

    return true;
}

/**
 * @brief   Triển khai TaskController::GetTaskControllerName(std::string*).
 */
template <typename T>
bool TaskController<T>::GetTaskControllerName(std::string *const name)
{
    /* Kiểm tra con trỏ đầu ra hợp lệ. */
    if (nullptr == name)
    {
        return false;
    }

    /* Chỉ có thể lấy tên khi thread đang chạy (handle hợp lệ). */
    if (false == m_is_run)
    {
        return false;
    }

    char buf[MAX_NAME_LEN] = {0};
    int  ret = pthread_getname_np(m_handle, buf, MAX_NAME_LEN);
    if (0 != ret)
    {
        //_STD_ERR(ret);
        return false;
    }

    *name = buf; // Ghi tên vào output parameter
    return true;
}

/**
 * @brief   Triển khai TaskController::EntryImpl(void*).
 */
template <typename T>
void *TaskController<T>::EntryImpl(void *param)
{
    /* Bảo vệ khỏi null pointer — không nên xảy ra nếu Start() đúng. */
    if (nullptr == param)
    {
        return nullptr;
    }

    /* Ép kiểu `void*` trở lại con trỏ đúng kiểu để truy cập thành viên. */
    TaskController *self = static_cast<TaskController<T> *>(param);

    /* Gọi member function entry point trên đối tượng gốc.
     * Cú pháp pointer-to-member: (obj->*func)() */
    if ((nullptr != self->m_obj) && (nullptr != self->m_entry))
    {
        ((self->m_obj)->*(self->m_entry))();
    }

    return nullptr;
}

#endif /* TASK_CONTROLLER_H */
