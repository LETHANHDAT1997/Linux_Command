/**
 * @file    SharedMemory.h
 * @brief   Template wrapper quản lý POSIX Shared Memory an toàn kiểu dữ liệu.
 *
 * @details SharedMemory<TYPE> đóng gói toàn bộ vòng đời của một POSIX shared
 *          memory segment:
 *            - Tạo mới hoặc mở lại segment theo tên (qua `shm_open`).
 *            - Map vào không gian địa chỉ tiến trình (qua `mmap`).
 *            - Đồng bộ hóa đa tiến trình thông qua kế thừa từ lớp `Sync`.
 *            - Unmap và đóng file descriptor khi đối tượng bị hủy.
 *
 *          **Mô hình Master/Slave**:
 *            - Tiến trình đầu tiên gọi constructor thành công sẽ trở thành
 *              **master**: có quyền set kích thước (`ftruncate`) và zero-clear
 *              vùng nhớ ngay sau khi tạo.
 *            - Các tiến trình sau mở lại segment đã tồn tại sẽ là **slave**:
 *              kích thước được suy ra từ metadata của file (`fstat`).
 *
 *          **Bố cục vùng nhớ (Memory Layout)**:
 *          @verbatim
 *          +-------------+---------------+-----------------+-----------------------+
 *          |  mSize(4B)  |  mData*(ptr)  | mDataHead*(ptr) |  TYPE[0]...TYPE[N-1]  |
 *          +-------------+---------------+-----------------+-----------------------+
 *          |<---------------- SharedData header ---------->|<----- Data area ----->|
 *          +-----------------------------------------------+-----------------------+
 *          @endverbatim
 *          `mDataHead` đóng vai trò padding để `zeroclear()` không vô tình
 *          xóa con trỏ `mData` (vì memset chỉ bắt đầu từ `mData`).
 *
 * @note    Phụ thuộc POSIX:
 *            - `shm_open`, `shm_unlink` — POSIX.1-2001.
 *            - `mmap`, `munmap`         — POSIX.1-2001.
 *            - `ftruncate`, `fstat`     — POSIX.1-2001.
 *          Compile với `-lrt` (trên một số hệ thống) và `-lpthread`.
 *
 * @tparam  TYPE    Kiểu dữ liệu lưu trong shared memory. Nên là kiểu POD
 *                  (Plain Old Data) để tránh vấn đề con trỏ cross-process.
 *
 * @version 1.0
 * @date    2026-07-15
 */

#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <cerrno>    ///< errno — mã lỗi POSIX
#include <cstdio>    ///< (dự phòng) printf/snprintf nếu cần log
#include <cstdlib>   ///< (dự phòng) abort, exit
#include <cstring>   ///< memset, strcmp, strlen
#include <string>    ///< std::string — xây dựng tên segment
#include <fcntl.h>   ///< O_RDWR, O_CREAT, O_EXCL — flags cho shm_open
#include <sys/mman.h>  ///< mmap, munmap, PROT_*, MAP_SHARED
#include <sys/shm.h>   ///< (tham chiếu SysV shm — không dùng trực tiếp ở đây)
#include <sys/stat.h>  ///< fstat, struct stat, S_IRUSR, ...
#include <sys/types.h> ///< size_t, off_t, ...
#include <unistd.h>    ///< ftruncate, close
#include "Sync.h"      ///< Lớp đồng bộ hóa POSIX semaphore (inter-process)

/**
 * @def     DEFAULT_ELEMENT_NUM
 * @brief   Số phần tử mặc định khi khởi tạo shared memory với constructor 1 tham số.
 */
#define DEFAULT_ELEMENT_NUM 1

/**
 * @class   SharedMemory
 * @brief   Template wrapper POSIX shared memory an toàn kiểu, có đồng bộ hóa.
 *
 * @details Lớp kế thừa `Sync` để tự động khóa semaphore liên tiến trình khi
 *          truy cập vùng nhớ dùng chung. Caller nên dùng `SyncAuto` (RAII
 *          lock guard) hoặc gọi `Lock()`/`Unlock()` thủ công trước khi đọc/ghi.
 *
 *          Sơ đồ vòng đời:
 *          @code
 *          Constructor --> [Mapped] --> get()/size()/zeroclear() --> Destructor
 *          @endcode
 *
 * @tparam  TYPE    Kiểu phần tử lưu trong shared memory.
 *
 * @example
 * @code
 * // Tiến trình A (master):
 * SharedMemory<SensorData> shm("sensor_data", 10);
 * SensorData *buf = shm.get();
 * buf[0].value = 42.0f;
 *
 * // Tiến trình B (slave):
 * SharedMemory<SensorData> shm("sensor_data");
 * const SensorData *buf = shm.get();
 * printf("value = %f\n", buf[0].value);
 * @endcode
 */
template <typename TYPE>
class SharedMemory : public Sync
{
public:
    /**
     * @brief   Độ dài tối đa của tên shared memory segment (không kể ký tự '/').
     *
     * @details POSIX yêu cầu tên shm bắt đầu bằng '/'. Lớp tự thêm tiền tố '/'.
     *          Hệ thống Linux giới hạn tên shm tối đa 255 byte (`NAME_MAX`),
     *          trừ đi 1 byte cho '/', còn 254 byte — ở đây ta giới hạn thêm
     *          xuống 248 để có margin an toàn.
     */
    const unsigned int MAX_SHARED_NAME_SIZE = 248;

    /**
     * @brief   Constructor tạo/mở shared memory với 1 phần tử duy nhất.
     *
     * @details Delegate sang constructor chính với `elementNum = DEFAULT_ELEMENT_NUM`.
     *
     * @param   npName  Tên định danh shared memory (không có tiền tố '/').
     *                  Phải khác NULL và không rỗng.
     */
    SharedMemory(const char *npName)
        : SharedMemory(npName, DEFAULT_ELEMENT_NUM) {};

    /**
     * @brief   Constructor chính — tạo hoặc mở lại một POSIX shared memory segment.
     *
     * @details Quá trình khởi tạo:
     *            1. Kiểm tra tên hợp lệ qua `ObjNameVaridate`.
     *            2. Thử tạo mới bằng `shm_open(..., O_EXCL)`:
     *               - Thành công → trở thành **master**, gọi `ftruncate` để set kích thước.
     *               - Lỗi `EEXIST` → trở thành **slave**, mở lại và đọc kích thước qua `fstat`.
     *            3. Map segment vào không gian địa chỉ bằng `mmap`.
     *            4. Nếu là master: gọi `zeroclear()` để khởi tạo vùng nhớ về 0.
     *
     * @param   npName      Tên định danh shared memory. Không được NULL hay rỗng.
     * @param   elementNum  Số phần tử TYPE muốn cấp phát. Phải > 0.
     *                      Với slave, tham số này bị bỏ qua — kích thước lấy từ segment.
     *
     * @note    Nếu khởi tạo thất bại, `mpMapView` được giữ ở `(void*)-1`
     *          (sentinel value cho "chưa map"). Các hàm `get()`, `size()` sẽ
     *          trả về 0/null an toàn trong trạng thái này.
     */
    SharedMemory(const char *npName, const int elementNum);

    /**
     * @brief   Destructor — unmap vùng nhớ và đóng file descriptor.
     *
     * @details Thực hiện theo thứ tự:
     *            1. `munmap` nếu `mpMapView` hợp lệ (khác `(void*)-1`).
     *            2. `close(shmFd_)` nếu fd đang mở (khác -1).
     *
     * @note    Destructor **không** gọi `shm_unlink`. Segment tồn tại trên
     *          hệ thống cho đến khi có tiến trình nào đó gọi `shm_unlink`
     *          hoặc hệ thống reboot. Đây là hành vi POSIX chuẩn.
     */
    virtual ~SharedMemory();

    /**
     * @brief   Trả về con trỏ có thể ghi đến mảng dữ liệu trong shared memory.
     *
     * @details Tính lại địa chỉ `mData` mỗi lần gọi để đảm bảo offset chính xác
     *          (tránh phụ thuộc vào giá trị con trỏ lưu trong shared memory —
     *          con trỏ cross-process không có nghĩa).
     *
     * @return  Con trỏ `TYPE*` đến phần tử đầu tiên của mảng dữ liệu.
     * @return  `0` (null) nếu shared memory chưa được map thành công.
     *
     * @warning Caller phải tự đảm bảo đồng bộ hóa (dùng `SyncAuto` hoặc
     *          `Lock()`/`Unlock()`) trước khi đọc/ghi dữ liệu.
     */
    TYPE *get();

    /**
     * @brief   Trả về con trỏ chỉ đọc đến mảng dữ liệu trong shared memory.
     *
     * @return  Con trỏ `const TYPE*` đến phần tử đầu tiên.
     * @return  `0` (null) nếu shared memory chưa được map thành công.
     *
     * @see     get()
     */
    const TYPE *get() const;

    /**
     * @brief   Trả về kích thước tổng của vùng dữ liệu (byte), không phải số phần tử.
     *
     * @details Giá trị trả về = `sizeof(TYPE) * elementNum` đã được thiết lập
     *          lúc khởi tạo và lưu trong `mpMapView->mSize`.
     *
     * @return  Số byte của vùng dữ liệu.
     * @return  `0` nếu shared memory chưa được map.
     *
     * @note    Để lấy số phần tử: `size() / sizeof(TYPE)`.
     */
    unsigned int size() const;

    /**
     * @brief   Zero-fill toàn bộ vùng dữ liệu trong shared memory.
     *
     * @details Gọi `memset(mData, 0, mSize)` trong vùng được bảo vệ bởi
     *          `SyncAuto` (tự động Lock/Unlock semaphore liên tiến trình).
     *          Chỉ tác động đến phần data, không xóa header (`mSize`, `mData`).
     *
     * @note    Được gọi tự động bởi master trong constructor sau khi map thành công.
     *          Caller cũng có thể gọi thủ công để reset dữ liệu.
     */
    void zeroclear();

    /**
     * @brief   Copy constructor bị xóa — không cho phép sao chép SharedMemory.
     *
     * @details Sao chép sẽ dẫn đến hai instance cùng quản lý một fd và
     *          map view, gây double-close và double-munmap.
     */
    SharedMemory(const SharedMemory<TYPE> &nObj) = delete;

    /**
     * @brief   Copy assignment bị xóa — không cho phép gán sao chép.
     */
    SharedMemory<TYPE> &operator=(const SharedMemory<TYPE> &nObj) = delete;

private:
    /**
     * @struct  SharedData
     * @brief   Header layout được map tại đầu vùng nhớ dùng chung.
     *
     * @details Cấu trúc này được đặt tại địa chỉ đầu tiên của segment.
     *          Toàn bộ bố cục vùng nhớ:
     *          @verbatim
     *          Offset 0                   : mSize     (4 byte)
     *          Offset sizeof(unsigned int): mData     (pointer — padding only)
     *          Offset + sizeof(TYPE*)     : mDataHead (pointer — padding only)
     *          Offset + sizeof(TYPE*)     : TYPE[0], TYPE[1], ... TYPE[N-1]
     *          @endverbatim
     *
     * @warning `mData` và `mDataHead` là con trỏ lưu trong shared memory.
     *          Giá trị con trỏ chỉ có ý nghĩa trong không gian địa chỉ của
     *          tiến trình hiện tại — không dùng được cross-process. `get()`
     *          luôn tính lại địa chỉ từ offset để tránh vấn đề này.
     */
    struct SharedData
    {
        unsigned int mSize;     ///< Kích thước vùng data (byte): `sizeof(TYPE) * N`.
        TYPE        *mData;     ///< Con trỏ tới vùng data (tính lại mỗi lần dùng).
        TYPE        *mDataHead; ///< Padding: giữ chỗ để `zeroclear` không xóa nhầm `mData`.
    };

    /**
     * @brief   Kiểm tra tính hợp lệ của tên shared memory.
     *
     * @details Kiểm tra ba điều kiện:
     *            1. `npName` khác NULL.
     *            2. `npName` không phải chuỗi rỗng.
     *            3. `strlen(npName)` không vượt quá `MAX_SHARED_NAME_SIZE`.
     *
     * @param   npName  Tên cần kiểm tra.
     * @return  `true`  nếu tên hợp lệ.
     * @return  `false` nếu bất kỳ điều kiện nào ở trên bị vi phạm.
     */
    bool ObjNameVaridate(const char *npName);

    int         shmFd_;     ///< File descriptor của shared memory (`shm_open`), -1 nếu chưa mở.
    SharedData *mpMapView;  ///< Con trỏ đến vùng nhớ được map; `(void*)-1` nếu chưa map.
    bool        isMaster_;  ///< `true` nếu tiến trình này tạo segment (master), `false` nếu mở lại (slave).
};

/* =========================================================================
 * Phần triển khai template (phải nằm trong header do quy tắc của C++)
 * ========================================================================= */

/**
 * @brief   Triển khai SharedMemory::SharedMemory(const char*, int).
 */
template <typename TYPE>
SharedMemory<TYPE>::SharedMemory(const char *npName, const int elementNum)
    : Sync(npName, Sync::en_TYPE_INTER_PROCESS), shmFd_(-1),
      mpMapView(static_cast<SharedData *>((void *)-1)), isMaster_(false)
{
    /* Bước 1: Kiểm tra tên trước khi làm bất cứ thao tác hệ thống nào. */
    if (false == ObjNameVaridate(npName))
    {
        return;
    }

    struct stat sShmStat;
    size_t lElmSize = elementNum;

    if (0 >= lElmSize)
    {
        //_LOGE("SharedMemory Failed. Error. Num of shared element is invalid %d\n", lElmSize);
        return;
    }

    /* Bước 2: Khóa semaphore để đảm bảo chỉ một tiến trình thực hiện
     * shm_open + ftruncate tại một thời điểm (tránh race condition). */
    SyncAuto autolock(this);

    /* Quyền truy cập: rw-rw-r-- (owner, group read/write; others read-only). */
    const int mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

    /* Tên segment trên hệ thống phải bắt đầu bằng '/' (POSIX requirement). */
    std::string sharedName("/");
    sharedName += npName;

    /* Bước 3a: Thử tạo segment mới với O_EXCL (atomic create-or-fail). */
    this->shmFd_ = shm_open(sharedName.c_str(), O_RDWR | O_CREAT | O_EXCL, mode);

    if (-1 == this->shmFd_)
    {
        if (EEXIST == errno)
        {
            /* Segment đã tồn tại — đây là slave, mở lại không dùng O_EXCL. */
            this->shmFd_ = shm_open(sharedName.c_str(), O_RDWR | O_CREAT, mode);
            if (-1 == this->shmFd_)
            {
                //_STD_ERR(errno);
                return;
            }
        }
        else
        {
            /* Lỗi bất thường (quyền, hết tài nguyên, ...). */
            //_STD_ERR(errno);
            return;
        }
    }
    else
    {
        /* Tạo mới thành công — tiến trình này là master. */
        this->isMaster_ = true;
    }

    /* Bước 4: Set kích thước (master) hoặc đọc kích thước hiện có (slave). */
    if (true == this->isMaster_)
    {
        /* Tổng kích thước = header (mSize + mData* + mDataHead*) + data array. */
        if (-1 == ftruncate(this->shmFd_,
                            sizeof(unsigned int) + sizeof(TYPE *) + sizeof(TYPE) * lElmSize))
        {
            //_STD_ERR(errno);
            /* Không return — tiếp tục map với kích thước có thể 0. */
        }
    }
    else
    {
        /* Slave: suy ra số phần tử từ kích thước file hiện tại qua fstat.
         * TODO: Cần xử lý trường hợp slave yêu cầu kích thước lớn hơn master. */
        if (-1 == fstat(this->shmFd_, &sShmStat))
        {
            //_STD_ERR(errno);
            return;
        }
        lElmSize = static_cast<size_t>(sShmStat.st_size - sizeof(unsigned int) -
                                       sizeof(TYPE *)) / sizeof(TYPE);
    }

    /* Bước 5: Map segment vào không gian địa chỉ tiến trình.
     * MAP_SHARED đảm bảo thay đổi được nhìn thấy bởi các tiến trình khác. */
    this->mpMapView = (SharedData *)mmap(
        0,
        sizeof(unsigned int) + sizeof(TYPE *) + sizeof(TYPE) * lElmSize,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        this->shmFd_,
        0);

    if ((void *)-1 != this->mpMapView)
    {
        /* Bước 6: Khởi tạo header trong shared memory. */
        this->mpMapView->mSize = sizeof(TYPE) * lElmSize;
        this->mpMapView->mData = (TYPE *)((char *)this->mpMapView +
                                          sizeof(unsigned int) + sizeof(TYPE *));
        /*
         * Sơ đồ bố cục vùng nhớ:
         * +-------------+---------------+-----------------+-----------------------+
         * |  mSize(4B)  |  mData*(ptr)  | mDataHead*(ptr) | TYPE[0]...TYPE[N-1]   |
         * +-------------+---------------+-----------------+-----------------------+
         * |<---------------- SharedData header ---------->|<----- Data area ----->|
         * +-----------------------------------------------+-----------------------+
         * mDataHead là padding: zeroclear() dùng memset bắt đầu từ mData,
         * nên mData* không bị xóa. Nếu thiếu padding, zeroclear() sẽ ghi đè
         * lên chính con trỏ mData.
         */
        if (true == this->isMaster_)
        {
            /* Master zero-clear toàn bộ data ngay sau khi tạo. */
            this->zeroclear();
        }
    }
    else
    {
        //_STD_ERR(errno);
        return;
    }
}

/**
 * @brief   Triển khai SharedMemory::~SharedMemory().
 */
template <typename TYPE>
SharedMemory<TYPE>::~SharedMemory()
{
    /* Unmap vùng nhớ nếu đã được map thành công. */
    if ((void *)-1 != this->mpMapView)
    {
        munmap(this->mpMapView,
               sizeof(unsigned int) + sizeof(TYPE *) + this->mpMapView->mSize);
        /* Reset về sentinel value để tránh dangling pointer. */
        this->mpMapView = static_cast<SharedData *>((void *)-1);
    }

    /* Đóng file descriptor nếu đang mở. */
    if (-1 != this->shmFd_)
    {
        close(this->shmFd_);
        this->shmFd_ = -1;
    }
    /* Lưu ý: KHÔNG gọi shm_unlink — segment vẫn tồn tại cho tiến trình khác. */
}

/**
 * @brief   Triển khai SharedMemory::get() — non-const version.
 */
template <typename TYPE>
TYPE *SharedMemory<TYPE>::get()
{
    /* Trả về null nếu map thất bại. */
    if ((void *)-1 == mpMapView)
    {
        return 0;
    }
    /* Tính lại offset mỗi lần thay vì dùng con trỏ lưu sẵn trong shared memory,
     * vì địa chỉ map có thể khác nhau giữa các tiến trình. */
    this->mpMapView->mData =
        (TYPE *)((char *)this->mpMapView + sizeof(unsigned int) + sizeof(TYPE *));
    return mpMapView->mData;
}

/**
 * @brief   Triển khai SharedMemory::get() const — read-only version.
 */
template <typename TYPE>
const TYPE *SharedMemory<TYPE>::get() const
{
    if ((void *)-1 == mpMapView)
    {
        return 0;
    }
    this->mpMapView->mData =
        (TYPE *)((char *)this->mpMapView + sizeof(unsigned int) + sizeof(TYPE *));
    return mpMapView->mData;
}

/**
 * @brief   Triển khai SharedMemory::size().
 */
template <typename TYPE>
unsigned int SharedMemory<TYPE>::size() const
{
    if ((void *)-1 == mpMapView)
    {
        return 0;
    }
    /* mSize lưu tổng số byte của vùng data (sizeof(TYPE) * N). */
    return mpMapView->mSize;
}

/**
 * @brief   Triển khai SharedMemory::zeroclear().
 */
template <typename TYPE>
void SharedMemory<TYPE>::zeroclear()
{
    if ((void *)-1 != mpMapView)
    {
        /* RAII lock: tự động Lock() khi vào scope, Unlock() khi ra scope. */
        SyncAuto autolock(this);
        /* Chỉ clear phần data, không clear header SharedData. */
        memset(mpMapView->mData, 0, mpMapView->mSize);
    }
}

/**
 * @brief   Triển khai SharedMemory::ObjNameVaridate().
 */
template <typename TYPE>
bool SharedMemory<TYPE>::ObjNameVaridate(const char *npName)
{
    /* Điều kiện 1: Con trỏ không được NULL. */
    if (0 == npName)
    {
        //_LOGE("SharedMemory Failed. Error. Name is NULL.\n");
        return false;
    }

    /* Điều kiện 2: Tên không được là chuỗi rỗng. */
    if (strcmp("", npName) == 0)
    {
        //_LOGE("SharedMemory Failed. Error. Name is nothing.\n");
        return false;
    }

    /* Điều kiện 3: Tên không được vượt quá giới hạn độ dài. */
    if (MAX_SHARED_NAME_SIZE < strlen(npName))
    {
        //_LOGE("SharedMemory Failed. Error. Name is over the length.\n");
        return false;
    }

    return true;
}

#endif /* SHAREDMEMORY_H */
