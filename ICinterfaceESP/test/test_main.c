#include "unity.h"

// Test runner function prototypes
void run_app_controller_tests();
void run_can_bus_tests();
void run_sensor_reading_tests();

// Main entry point of the test runner
int main() {
    UNITY_BEGIN();

    run_app_controller_tests();
    run_can_bus_tests();
    run_sensor_reading_tests();

    return UNITY_END();
}