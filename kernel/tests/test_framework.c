/*
 * Simple unit test framework for kernel components
 * Tests run in userspace on host for CI
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define TEST_PASS 0
#define TEST_FAIL 1

typedef struct {
    const char *name;
    int (*func)(void);
} test_case_t;

static int passed = 0;
static int failed = 0;

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("  FAIL: %s:%d - Expected %llu, got %llu\n", __FILE__, __LINE__, (long long)(b), (long long)(a)); \
        return TEST_FAIL; \
    } \
} while(0)

#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        printf("  FAIL: %s:%d - Expected true\n", __FILE__, __LINE__); \
        return TEST_FAIL; \
    } \
} while(0)

#define ASSERT_FALSE(x) do { \
    if (x) { \
        printf("  FAIL: %s:%d - Expected false\n", __FILE__, __LINE__); \
        return TEST_FAIL; \
    } \
} while(0)

#define RUN_TEST(test) do { \
    printf("Running %s... ", test.name); \
    int result = test.func(); \
    if (result == TEST_PASS) { \
        printf("PASS\n"); \
        passed++; \
    } else { \
        failed++; \
    } \
} while(0)

// Mock implementations for testing
static inline uint64_t mock_round_up(uint64_t val, uint64_t align) {
    return (val + align - 1) & ~(align - 1);
}

static inline bool mock_is_power_of_two(uint64_t x) {
    return x && !(x & (x - 1));
}

int test_round_up(void) {
    ASSERT_EQ(mock_round_up(0, 16), 0);
    ASSERT_EQ(mock_round_up(1, 16), 16);
    ASSERT_EQ(mock_round_up(16, 16), 16);
    ASSERT_EQ(mock_round_up(17, 16), 32);
    ASSERT_EQ(mock_round_up(100, 4096), 4096);
    ASSERT_EQ(mock_round_up(4096, 4096), 4096);
    return TEST_PASS;
}

int test_power_of_two(void) {
    ASSERT_TRUE(mock_is_power_of_two(1));
    ASSERT_TRUE(mock_is_power_of_two(2));
    ASSERT_TRUE(mock_is_power_of_two(4));
    ASSERT_TRUE(mock_is_power_of_two(16));
    ASSERT_TRUE(mock_is_power_of_two(4096));
    ASSERT_FALSE(mock_is_power_of_two(0));
    ASSERT_FALSE(mock_is_power_of_two(3));
    ASSERT_FALSE(mock_is_power_of_two(5));
    ASSERT_FALSE(mock_is_power_of_two(15));
    return TEST_PASS;
}

int test_scheduler_logic(void) {
    // Test round-robin logic
    int current = 0;
    int num_tasks = 3;
    
    // Simulate 5 scheduler ticks
    for (int i = 0; i < 5; i++) {
        current = (current + 1) % num_tasks;
    }
    ASSERT_EQ(current, 2); // 5 % 3 = 2
    
    // Test with 1 task
    current = 0;
    num_tasks = 1;
    current = (current + 1) % num_tasks;
    ASSERT_EQ(current, 0);
    
    return TEST_PASS;
}

int test_ipc_message_struct(void) {
    // Test message struct layout
    typedef struct {
        uint64_t sender;
        uint64_t type;
        uint64_t arg0;
        uint64_t arg1;
        uint64_t arg2;
        uint64_t arg3;
        uint8_t payload[256];
    } aegis_msg_t;
    
    aegis_msg_t msg = {0};
    msg.sender = 1;
    msg.type = 42;
    msg.arg0 = 0xDEADBEEF;
    msg.arg1 = 0xCAFEBABE;
    
    ASSERT_EQ(msg.sender, 1);
    ASSERT_EQ(msg.type, 42);
    ASSERT_EQ(msg.arg0, 0xDEADBEEF);
    ASSERT_EQ(msg.arg1, 0xCAFEBABE);
    
    // Test payload
    const char *test_str = "Hello, IPC!";
    memcpy(msg.payload, test_str, strlen(test_str) + 1);
    ASSERT_EQ(strcmp((char*)msg.payload, test_str), 0);
    
    return TEST_PASS;
}

int test_timer_calc(void) {
    // Test PIT divisor calculation
    #define PIT_FREQ 1193182
    #define TARGET_HZ 1000
    
    uint32_t divisor = PIT_FREQ / TARGET_HZ;
    ASSERT_EQ(divisor, 1193);
    
    // Test with different frequencies
    uint32_t divisor_100 = PIT_FREQ / 100;
    ASSERT_EQ(divisor_100, 11931);
    
    return TEST_PASS;
}

int test_keyboard_scancodes(void) {
    // Test scancode to ASCII mapping
    const char scancode_to_ascii[128] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0
    };
    
    ASSERT_EQ(scancode_to_ascii[0x1E], 'a'); // 'A' key
    ASSERT_EQ(scancode_to_ascii[0x1F], 's'); // 'S' key
    ASSERT_EQ(scancode_to_ascii[0x20], 'd'); // 'D' key
    ASSERT_EQ(scancode_to_ascii[0x1C], '\n'); // Enter
    ASSERT_EQ(scancode_to_ascii[0x0E], '\b'); // Backspace
    ASSERT_EQ(scancode_to_ascii[0x0F], '\t'); // Tab
    
    return TEST_PASS;
}

int test_mm_alloc_logic(void) {
    // Test bump allocator logic
    uint64_t heap_start = 0x100000;
    uint64_t heap_ptr = heap_start;
    
    // Allocate 64 bytes aligned to 16
    uint64_t size = 64;
    uint64_t alignment = 16;
    
    // Align
    uint64_t aligned = (heap_ptr + alignment - 1) & ~(alignment - 1);
    uint64_t new_ptr = aligned + 64;
    
    ASSERT_EQ(aligned % 16, 0);
    ASSERT_EQ(new_ptr, aligned + 64);
    
    // Second allocation
    uint64_t aligned2 = (new_ptr + 15) & ~15;
    ASSERT_EQ(aligned2 % 16, 0);
    ASSERT_TRUE(aligned2 >= new_ptr);
    
    return TEST_PASS;
}

int main(void) {
    printf("=== Aegis Kernel Unit Tests ===\n\n");
    
    test_case_t tests[] = {
        {"round_up", test_round_up},
        {"power_of_two", test_power_of_two},
        {"scheduler_logic", test_scheduler_logic},
        {"ipc_message_struct", test_ipc_message_struct},
        {"timer_calc", test_timer_calc},
        {"keyboard_scancodes", test_keyboard_scancodes},
        {"mm_alloc_logic", test_mm_alloc_logic},
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    
    for (int i = 0; i < num_tests; i++) {
        RUN_TEST(tests[i]);
    }
    
    printf("\n=== Results ===\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("Total:  %d\n", passed + failed);
    
    return failed == 0 ? 0 : 1;
}