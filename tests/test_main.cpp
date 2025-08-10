#include <iostream>
#include <vector>
#include <functional>
#include <string>
#include <cmath>

// Simple test framework
class TestRunner {
public:
    struct TestCase {
        std::string name;
        std::function<bool()> test;
    };
    
    static TestRunner& getInstance() {
        static TestRunner instance;
        return instance;
    }
    
    void addTest(const std::string& name, std::function<bool()> test) {
        tests.push_back({name, test});
    }
    
    int runAll() {
        int passed = 0;
        int failed = 0;
        
        std::cout << "Running " << tests.size() << " tests...\n\n";
        
        for (const auto& test : tests) {
            std::cout << "Running: " << test.name << " ... ";
            
            try {
                if (test.test()) {
                    std::cout << "PASSED\n";
                    passed++;
                } else {
                    std::cout << "FAILED\n";
                    failed++;
                }
            } catch (const std::exception& e) {
                std::cout << "FAILED (Exception: " << e.what() << ")\n";
                failed++;
            }
        }
        
        std::cout << "\n========================================\n";
        std::cout << "Test Results: " << passed << " passed, " << failed << " failed\n";
        std::cout << "========================================\n";
        
        return failed;
    }
    
private:
    std::vector<TestCase> tests;
};

// Test macros
#define TEST(TestName) \
    bool Test_##TestName(); \
    static struct TestRegistrar_##TestName { \
        TestRegistrar_##TestName() { \
            TestRunner::getInstance().addTest(#TestName, Test_##TestName); \
        } \
    } testRegistrar_##TestName; \
    bool Test_##TestName()

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        std::cerr << "\nAssertion failed: " #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    }

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        std::cerr << "\nAssertion failed: expected " << (expected) << " but got " << (actual) \
              << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    }

// Include test files
#include "trading/test_OrderManager.cpp"
#include "trading/test_Portfolio.cpp"
#include "trading/test_MarketData.cpp"
#include "trading/test_MatchingEngine.cpp"

int main(int argc, char** argv) {
    return TestRunner::getInstance().runAll();
}