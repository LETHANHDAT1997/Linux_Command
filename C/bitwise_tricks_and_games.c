/**
 * ============================================================================
 * File: bitwise_tricks_and_games.c
 * Description: Tổng hợp các kỹ thuật Bitwise từ cơ bản đến nâng cao,
 *              các "Bit Hacks" kinh điển (Branchless Math) và các trò chơi,
 *              câu đố lập trình áp dụng Bitwise (Nim Game, Bitboard, Lights Out...).
 *
 * Biên dịch: gcc -Wall -Wextra -std=c11 bitwise_tricks_and_games.c -o bitwise_demo
 * Chạy:      ./bitwise_demo
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/* ============================================================================
 * PHẦN 0: CÁC HÀM TIỆN ÍCH HIỂN THỊ DẠNG NHỊ PHÂN
 * ============================================================================ */

void print_binary_8(uint8_t num) 
{
    for (int i = 7; i >= 0; i--) 
    {
        printf("%d", (num >> i) & 1);
        if (i == 4) 
        {
            printf(" ");
        }
    }
}

void print_binary_32(uint32_t num) 
{
    for (int i = 31; i >= 0; i--) 
    {
        printf("%d", (num >> i) & 1);
        if (i > 0 && i % 4 == 0) 
        {
            printf(" ");
        }
    }
}

/* ============================================================================
 * PHẦN 1: CÁC THAO TÁC CƠ BẢN VÀ THAO TÁC THANH GHI MCU (REGISTER MANIPULATION)
 * ============================================================================ */

#define SET_BIT(reg, bit)       ((reg) |= (1U << (bit)))
#define CLEAR_BIT(reg, bit)     ((reg) &= ~(1U << (bit)))
#define TOGGLE_BIT(reg, bit)    ((reg) ^= (1U << (bit)))
#define CHECK_BIT(reg, bit)     (((reg) >> (bit)) & 1U)

/**
 * GIẢI THÍCH CHI TIẾT CÔNG THỨC MODIFY_FIELD(reg, pos, len, val):
 * 
 * Mục tiêu: Ghi đè một giá trị `val` có độ dài `len` bit vào thanh ghi `reg`
 *           bắt đầu từ vị trí bit `pos` mà KHÔNG LÀM THAY ĐỔI bất kỳ bit nào khác.
 *
 * Công thức gồm 2 vế ghép với nhau bằng phép OR (|):
 *   Vế 1 (XÓA VÙNG CŨ VỀ 0): ((reg) & ~(((1U << (len)) - 1U) << (pos)))
 *     - Bước 1: (1U << len) - 1U
 *               -> Tạo mặt nạ gồm `len` bit 1 ở LSB.
 *               Ví dụ: len = 3 -> (1 << 3) - 1 = 8 - 1 = 7 (0b00000111).
 *     - Bước 2: (((1U << len) - 1U) << pos)
 *               -> Dịch mặt nạ bit 1 đến vị trí `pos`.
 *               Ví dụ: pos = 2 -> 0b00000111 << 2 = 0b00011100 (Field Mask).
 *     - Bước 3: ~(((1U << len) - 1U) << pos)
 *               -> Đảo bit để tạo mặt nạ Clear: Các bit cần xóa là 0, các bit khác là 1.
 *               Ví dụ: ~0b00011100 = 0b11100011 (Clear Mask).
 *     - Bước 4: (reg) & Clear_Mask
 *               -> Giữ nguyên các bit khác, ép các bit tại vùng [pos + len - 1 : pos] về 0.
 *
 *   Vế 2 (CHUẨN BỊ GIÁ TRỊ MỚI): (((val) & ((1U << (len)) - 1U)) << (pos))
 *     - Bước 5: (val) & ((1U << len) - 1U)
 *               -> Cắt tỉa (Clamp/Mask) chỉ lấy đúng `len` bit thấp nhất của `val`.
 *               -> Phòng ngừa lỗi khi người dùng truyền `val` quá lớn làm tràn sang bit khác!
 *     - Bước 6: (val_đã_cắt) << pos
 *               -> Dịch giá trị mới đến đúng vị trí `pos`.
 *
 *   Gộp 2 vế: (reg đã clear vùng cần ghi) | (giá trị mới đã dịch đến vị trí pos)
 */
#define MODIFY_FIELD(reg, pos, len, val) \
    ((reg) = ((reg) & ~(((1U << (len)) - 1U) << (pos))) | (((val) & ((1U << (len)) - 1U)) << (pos)))

void demo_mcu_register_operations(void) 
{
    printf("\n==================================================\n");
    printf("1. THAO TÁC BIT CƠ BẢN & THANH GHI MCU\n");
    printf("==================================================\n");

    uint8_t reg = 0b00000000;
    printf("Ban đầu              : ");
    print_binary_8(reg);
    printf(" (0x%02X)\n", reg);

    // 1. Set bit 3 và bit 0
    SET_BIT(reg, 3);
    SET_BIT(reg, 0);
    printf("Sau khi SET bit 3, 0 : ");
    print_binary_8(reg);
    printf(" (0x%02X)\n", reg);

    // 2. Toggle bit 3
    TOGGLE_BIT(reg, 3);
    printf("Sau khi TOGGLE bit 3 : ");
    print_binary_8(reg);
    printf(" (0x%02X)\n", reg);

    // 3. Clear bit 0
    CLEAR_BIT(reg, 0);
    printf("Sau khi CLEAR bit 0  : ");
    print_binary_8(reg);
    printf(" (0x%02X)\n", reg);

    // 4. Modify bit-field (Ví dụ ghi giá trị 0b101 vào bit 2..4 - độ dài 3 bit)
    reg = 0b11111111;
    printf("Thanh ghi trước ghi  : ");
    print_binary_8(reg);
    printf("\n");

    MODIFY_FIELD(reg, 2, 3, 0b101);
    printf("Ghi field [4:2]=101  : ");
    print_binary_8(reg);
    printf(" (Giá trị mới tại bit 2..4 là 5)\n");

    // 5. Swap 2 biến không dùng biến tạm bằng XOR
    int a = 42, b = 99;
    printf("\nXOR Swap: Trước  a = %d, b = %d\n", a, b);
    a ^= b;
    b ^= a;
    a ^= b;
    printf("XOR Swap: Sau    a = %d, b = %d\n", a, b);
}

/* ============================================================================
 * PHẦN 2: TUYỂN TẬP BIT HACKS KINH ĐIỂN (BRANCHLESS MATH & TRICKS)
 * ============================================================================ */

/* 1. Kiểm tra chẵn lẻ */
bool is_odd(int n) 
{
    return (n & 1) != 0;
}

/* 2. Kiểm tra lũy thừa của 2 (Power of 2): 1, 2, 4, 8, 16... */
bool is_power_of_two(uint32_t n) 
{
    return (n > 0) && ((n & (n - 1)) == 0);
}

/* 3. Cô lập bit 1 thấp nhất (Lowest Set Bit / Isolate LSB) */
uint32_t isolate_lowest_set_bit(uint32_t n) 
{
    return n & (-n);
}

/* 4. Xóa bit 1 thấp nhất (Clear lowest set bit) */
uint32_t clear_lowest_set_bit(uint32_t n) 
{
    return n & (n - 1);
}

/* 5. Cô lập bit 0 thấp nhất (Isolate lowest zero bit) */
uint32_t isolate_lowest_zero_bit(uint32_t n) 
{
    return ~n & (n + 1);
}

/* 6. Đếm số bit 1 (Hamming Weight / Popcount) - Thuật toán Brian Kernighan O(số bit 1) */
int count_set_bits_kernighan(uint32_t n) 
{
    int count = 0;
    while (n > 0) 
    {
        n &= (n - 1); // Xóa bit 1 thấp nhất sau mỗi bước
        count++;
    }
    return count;
}

/* 7. Đếm số bit 1 song song trong O(1) (Parallel bit count) */
int count_set_bits_parallel(uint32_t n) 
{
    n = n - ((n >> 1) & 0x55555555);
    n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
    n = (n + (n >> 4)) & 0x0F0F0F0F;
    n = n + (n >> 8);
    n = n + (n >> 16);
    return n & 0x3F;
}

/* 8. Đảo ngược toàn bộ 32 bit (Bit reversal) */
uint32_t reverse_bits_32(uint32_t n) 
{
    n = ((n >> 1) & 0x55555555) | ((n & 0x55555555) << 1);
    n = ((n >> 2) & 0x33333333) | ((n & 0x33333333) << 2);
    n = ((n >> 4) & 0x0F0F0F0F) | ((n & 0x0F0F0F0F) << 4);
    n = ((n >> 8) & 0x00FF00FF) | ((n & 0x00FF00FF) << 8);
    n = (n >> 16) | (n << 16);
    return n;
}

/* 9. Làm tròn lên lũy thừa của 2 kế tiếp (Next Power of 2) */
uint32_t next_power_of_two(uint32_t n) 
{
    if (n == 0) 
    {
        return 1;
    }
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

/* 10. Giá trị tuyệt đối không cần rẽ nhánh (Branchless Absolute Value) */
int branchless_abs(int x) 
{
    int mask = x >> 31; // 0 nếu dương, -1 (0xFFFFFFFF) nếu âm
    return (x + mask) ^ mask;
}

/* 11. Tìm Max / Min không cần lệnh if (Branchless Min / Max) */
int branchless_min(int a, int b) 
{
    return b ^ ((a ^ b) & -(a < b));
}

int branchless_max(int a, int b) 
{
    return a ^ ((a ^ b) & -(a < b));
}

/* 12. Kiểm tra hai số có trái dấu không */
bool have_opposite_signs(int x, int y) 
{
    return (x ^ y) < 0;
}

/* 13. Thao tác chữ cái ASCII (Upper / Lower / Toggle) */
char to_upper_bitwise(char c) 
{
    return (char)(c & '_'); // '_' có mã ASCII là 0b01011111, xóa bit 5
}

char to_lower_bitwise(char c) 
{
    return (char)(c | ' '); // ' ' có mã ASCII là 32 (0b00100000), bật bit 5
}

char toggle_case_bitwise(char c) 
{
    return (char)(c ^ ' '); // Đảo bit 5
}

void demo_bit_hacks(void) 
{
    printf("\n==================================================\n");
    printf("2. CÁC BIT HACKS & BRANCHLESS MATH KINH ĐIỂN\n");
    printf("==================================================\n");

    // Test lũy thừa 2
    uint32_t test_val = 16;
    printf("Số %u có phải lũy thừa của 2? %s\n", test_val, is_power_of_two(test_val) ? "CÓ" : "KHÔNG");
    test_val = 18;
    printf("Số %u có phải lũy thừa của 2? %s\n", test_val, is_power_of_two(test_val) ? "CÓ" : "KHÔNG");

    // Test cô lập bit 1 thấp nhất (LSB)
    uint32_t x = 0b00101100; // 44 thập phân, LSB là bit 2 (giá trị 4)
    printf("\nSố x                 : ");
    print_binary_8((uint8_t)x);
    printf(" (%u)\n", x);

    uint32_t lsb = isolate_lowest_set_bit(x);
    printf("Cô lập LSB x & (-x)  : ");
    print_binary_8((uint8_t)lsb);
    printf(" (%u)\n", lsb);

    uint32_t clear_lsb = clear_lowest_set_bit(x);
    printf("Xóa LSB x & (x - 1)  : ");
    print_binary_8((uint8_t)clear_lsb);
    printf(" (%u)\n", clear_lsb);

    // Test đếm bit 1
    uint32_t p = 0b1011011100101;
    printf("\nĐếm bit 1 trong 0b1011011100101:\n");
    printf("- Kernighan : %d bit\n", count_set_bits_kernighan(p));
    printf("- Parallel  : %d bit\n", count_set_bits_parallel(p));

    // Test làm tròn lũy thừa 2 tiếp theo
    uint32_t need_buf = 37;
    printf("\nCần buffer size %u -> Làm tròn lên lũy thừa 2: %u\n", need_buf, next_power_of_two(need_buf));

    // Test Branchless Math
    int neg = -25;
    printf("\nTrị tuyệt đối branchless của %d: %d\n", neg, branchless_abs(neg));

    int val1 = 105, val2 = 42;
    printf("Min(%d, %d) [branchless] = %d\n", val1, val2, branchless_min(val1, val2));
    printf("Max(%d, %d) [branchless] = %d\n", val1, val2, branchless_max(val1, val2));

    printf("Kiểm tra trái dấu (%d và %d): %s\n", val1, neg, have_opposite_signs(val1, neg) ? "TRÁI DẤU" : "CÙNG DẤU");

    // Test ASCII
    printf("\nThao tác ký tự ASCII qua bitwise:\n");
    printf("'a' -> Upper  : %c\n", to_upper_bitwise('a'));
    printf("'G' -> Lower  : %c\n", to_lower_bitwise('G'));
    printf("'k' -> Toggle : %c, 'K' -> Toggle : %c\n", toggle_case_bitwise('k'), toggle_case_bitwise('K'));
}

/* ============================================================================
 * PHẦN 3: CÁC BÀI TOÁN & TRÒ CHƠI SỬ DỤNG BITWISE
 * ============================================================================ */

/* ----------------------------------------------------------------------------
 * GAME 1: NIM GAME (Trò chơi bốc sỏi & Định lý Bouton Nim-Sum XOR)
 * Luật chơi: Có N đống sỏi, mỗi lượt người chơi chọn 1 đống và bốc >= 1 viên.
 * Người bốc viên sỏi cuối cùng sẽ THẮNG.
 * Định lý: Nếu XOR tổng của số sỏi các đống (Nim-Sum) != 0, người đi đầu CÓ THỂ
 * luôn thắng nếu bốc để đưa Nim-Sum về 0!
 * ---------------------------------------------------------------------------- */
void play_nim_game_demo(void) 
{
    printf("\n==================================================\n");
    printf("3.1. TRÒ CHƠI NIM GAME (BỐC SỎI BẰNG XOR THEOREM)\n");
    printf("==================================================\n");

    int heaps[] = {3, 4, 5};
    int n = (int)(sizeof(heaps) / sizeof(heaps[0]));

    printf("Trạng thái các đống sỏi ban đầu: ");
    int nim_sum = 0;
    for (int i = 0; i < n; i++) 
    {
        printf("Đống %d: %d viên | ", i + 1, heaps[i]);
        nim_sum ^= heaps[i];
    }
    printf("\nNim-Sum (XOR tổng) = %d\n", nim_sum);

    if (nim_sum == 0) 
    {
        printf("-> Thế cờ THUA cho người đi trước (nếu đối thủ chơi chuẩn).\n");
    }
    else 
    {
        printf("-> Thế cờ THẮNG cho người đi trước!\n");
        printf("-> Thuật toán AI tính nước đi tối ưu:\n");

        for (int i = 0; i < n; i++) 
        {
            int target = heaps[i] ^ nim_sum;
            if (target < heaps[i]) 
            {
                int remove = heaps[i] - target;
                printf("   >>> Nước đi chuẩn: Bốc %d viên từ Đống %d (còn %d viên)\n", remove, i + 1, target);
                printf("   >>> Sau nước đi, Nim-Sum mới = %d (Đẩy đối thủ vào thế thua!)\n", 0);
                break;
            }
        }
    }
}

/* ----------------------------------------------------------------------------
 * GAME 2: TIC-TAC-TOE KIỂM TRA THẮNG THUA BẰNG BITBOARD TRONG O(1)
 * Thay vì dùng mảng 2 chiều 3x3, mỗi người chơi biểu diễn các ô đã đánh bằng 9 bit
 * của 1 số nguyên 16-bit.
 * Ô cờ:
 *  0 | 1 | 2       Bit 0 | Bit 1 | Bit 2
 * ---+---+---     -------+-------+-------
 *  3 | 4 | 5       Bit 3 | Bit 4 | Bit 5
 * ---+---+---     -------+-------+-------
 *  6 | 7 | 8       Bit 6 | Bit 7 | Bit 8
 * ---------------------------------------------------------------------------- */
static const uint16_t WIN_PATTERNS[8] = {
    0b000000111, // Hàng 0: ô 0, 1, 2
    0b000111000, // Hàng 1: ô 3, 4, 5
    0b111000000, // Hàng 2: ô 6, 7, 8
    0b001001001, // Cột 0 : ô 0, 3, 6
    0b010010010, // Cột 1 : ô 1, 4, 7
    0b100100100, // Cột 2 : ô 2, 5, 8
    0b100010001, // Chéo 1: ô 0, 4, 8
    0b001010100  // Chéo 2: ô 2, 4, 6
};

bool check_tic_tac_toe_win(uint16_t player_board) 
{
    for (int i = 0; i < 8; i++) 
    {
        if ((player_board & WIN_PATTERNS[i]) == WIN_PATTERNS[i]) 
        {
            return true;
        }
    }
    return false;
}

void demo_tic_tac_toe_bitboard(void) 
{
    printf("\n==================================================\n");
    printf("3.2. CỜ TIC-TAC-TOE CHECK THẮNG SIÊU TỐC (BITBOARD)\n");
    printf("==================================================\n");

    // Người chơi X đánh vào các ô: 0, 4, 8 (Đường chéo chính)
    uint16_t board_X = (1U << 0) | (1U << 4) | (1U << 8);
    // Người chơi O đánh vào các ô: 1, 2
    uint16_t board_O = (1U << 1) | (1U << 2);

    printf("Trạng thái Bitboard X: 0x%03X (nhị phân: ", board_X);
    for (int i = 8; i >= 0; i--) 
    {
        printf("%d", (board_X >> i) & 1);
    }
    printf(")\n");

    printf("Người chơi X có thắng không? %s\n", check_tic_tac_toe_win(board_X) ? "CHIẾN THẮNG!" : "CHƯA THẮNG");
    printf("Người chơi O có thắng không? %s\n", check_tic_tac_toe_win(board_O) ? "CHIẾN THẮNG!" : "CHƯA THẮNG");
}

/* ----------------------------------------------------------------------------
 * GAME 3: LIGHTS OUT 1D (TRÒ CHƠI BẬT TẮT ĐÈN BẰNG BITMASK)
 * Có 8 bóng đèn (8-bit). Khi nhấn công tắc tại vị trí `pos`, bóng đèn tại `pos`
 * và 2 bóng liền kề `pos-1`, `pos+1` đều bị đảo trạng thái (XOR).
 * ---------------------------------------------------------------------------- */
void toggle_light(uint8_t *lights, int pos) 
{
    if (pos < 0 || pos > 7) 
    {
        return;
    }

    uint8_t toggle_mask = (uint8_t)(1U << pos);
    if (pos > 0) 
    {
        toggle_mask |= (uint8_t)(1U << (pos - 1));
    }
    if (pos < 7) 
    {
        toggle_mask |= (uint8_t)(1U << (pos + 1));
    }

    *lights ^= toggle_mask;
}

void demo_lights_out_game(void) 
{
    printf("\n==================================================\n");
    printf("3.3. TRÒ CHƠI LIGHTS OUT 1D (BẬT TẮT ĐÈN VỚI XOR MASK)\n");
    printf("==================================================\n");

    uint8_t lights = 0b10101010; // Trạng thái đèn ban đầu (1: Bật, 0: Tắt)
    printf("Đèn ban đầu     : ");
    print_binary_8(lights);
    printf("\n");

    printf("-> Nhấn nút số 3 (đảo đèn 2, 3, 4)\n");
    toggle_light(&lights, 3);
    printf("Trạng thái mới  : ");
    print_binary_8(lights);
    printf("\n");

    printf("-> Nhấn nút số 0 (đảo đèn 0, 1)\n");
    toggle_light(&lights, 0);
    printf("Trạng thái mới  : ");
    print_binary_8(lights);
    printf("\n");
}

/* ----------------------------------------------------------------------------
 * PUZZLE 4: TÌM 2 SỐ ĐỘC NHẤT (SINGLE NUMBERS II)
 * Bài toán: Mảng có 2 phần tử xuất hiện đúng 1 lần, các phần tử còn lại
 * đều xuất hiện đúng 2 lần. Tìm 2 phần tử đó trong O(N) thời gian, O(1) bộ nhớ!
 * Giải thuật:
 *  1. XOR toàn bộ mảng -> thu được X = a ^ b.
 *  2. Lấy bit 1 phân biệt giữa a và b: diff_bit = X & (-X).
 *  3. Chia các số trong mảng thành 2 nhóm dựa trên diff_bit và XOR từng nhóm.
 * ---------------------------------------------------------------------------- */
void find_two_unique_numbers(const int *arr, int size, int *out_a, int *out_b) 
{
    int xor_all = 0;
    for (int i = 0; i < size; i++) 
    {
        xor_all ^= arr[i];
    }

    // Cô lập bit 1 thấp nhất (nơi a và b có giá trị bit khác nhau)
    int diff_bit = xor_all & (-xor_all);

    *out_a = 0;
    *out_b = 0;
    for (int i = 0; i < size; i++) 
    {
        if ((arr[i] & diff_bit) != 0) 
        {
            *out_a ^= arr[i];
        }
        else 
        {
            *out_b ^= arr[i];
        }
    }
}

void demo_single_number_puzzle(void) 
{
    printf("\n==================================================\n");
    printf("3.4. PUZZLE: TÌM 2 SỐ DUY NHẤT TRONG MẢNG (XOR BUCKETS)\n");
    printf("==================================================\n");

    int arr[] = {2, 4, 7, 9, 2, 4}; // 7 và 9 là 2 số xuất hiện 1 lần
    int size = (int)(sizeof(arr) / sizeof(arr[0]));

    printf("Mảng đầu vào: [ ");
    for (int i = 0; i < size; i++) 
    {
        printf("%d ", arr[i]);
    }
    printf("]\n");

    int num1 = 0;
    int num2 = 0;
    find_two_unique_numbers(arr, size, &num1, &num2);
    printf("=> Hai số xuất hiện đúng 1 lần tìm được là: %d và %d\n", num1, num2);
}

/* ----------------------------------------------------------------------------
 * PUZZLE 5: SINH TẬP CON (POWER SET GENERATION QUA BITMASK)
 * Một tập có N phần tử có 2^N tập con. Duyệt từ 0 đến (1<<N)-1.
 * ---------------------------------------------------------------------------- */
void generate_power_set(const char *elements[], int n) 
{
    printf("\n==================================================\n");
    printf("3.5. SINH TẤT CẢ TẬP CON (POWER SET BẰNG BITMASK)\n");
    printf("==================================================\n");

    int total_subsets = 1 << n; // 2^n
    printf("Tập gốc gồm %d phần tử -> có %d tập con:\n", n, total_subsets);

    for (int mask = 0; mask < total_subsets; mask++) 
    {
        printf("Mask ");
        print_binary_8((uint8_t)mask);
        printf(" -> { ");
        for (int i = 0; i < n; i++) 
        {
            if ((mask >> i) & 1) 
            {
                printf("%s ", elements[i]);
            }
        }
        printf("}\n");
    }
}

/* ============================================================================
 * HÀM MAIN
 * ============================================================================ */
int main(void) 
{
    printf("===============================================================\n");
    printf("     DEMO KỸ THUẬT BITWISE, BIT HACKS VÀ CÁC TRÒ CHƠI ĐỈNH CAO\n");
    printf("===============================================================\n");

    // 1. Thao tác thanh ghi MCU
    demo_mcu_register_operations();

    // 2. Các Bit Hacks & Branchless Math
    demo_bit_hacks();

    // 3. Các Trò chơi & Puzzles
    play_nim_game_demo();
    demo_tic_tac_toe_bitboard();
    demo_lights_out_game();
    demo_single_number_puzzle();

    const char *fruits[] = {"Apple", "Banana", "Cherry"};
    generate_power_set(fruits, 3);

    printf("\n===============================================================\n");
    printf("                     CHƯƠNG TRÌNH KẾT THÚC\n");
    printf("===============================================================\n");
    return 0;
}
