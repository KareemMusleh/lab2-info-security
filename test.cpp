#include "Allocator.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>

// Тестовый класс, который будет использовать наш аллокатор
class TestClass {
    public:
        // Конструктор
        TestClass(int id, const std::string& name) : m_id(id), m_name(name) {
            std::cout << "Constructing TestClass " << m_id << " (" << m_name << ")" << std::endl;
        }
        
        // Деструктор
        ~TestClass() {
            std::cout << "Destroying TestClass " << m_id << " (" << m_name << ")" << std::endl;
        }
        
        // Метод для демонстрации работы объекта
        void print() const {
            std::cout << "TestClass instance - ID: " << m_id << ", Name: " << m_name << std::endl;
        }
        
        // Переопределим операторы new и delete для использования нашего аллокатора
        static void* operator new(size_t size, Allocator& allocator) {
            return allocator.Allocate(size);
        }
        
        static void operator delete(void* p, Allocator& allocator) {
            allocator.Deallocate(p);
        }
        
    private:
        int m_id;
        std::string m_name;
};

void testHeapBlocksMode() {
    std::cout << "=== Testing HEAP_BLOCKS mode ===\n";
    
    // Create allocator in HEAP_BLOCKS mode (objects = 0)
    Allocator alloc(sizeof(int), 0, nullptr, "Heap Blocks Allocator");
    
    // Allocate some blocks
    std::vector<int*> blocks;
    try {
        for (int i = 0; i < 10; ++i) {
            int* p = (int*)alloc.Allocate(sizeof(int));
            *p = i;
            blocks.push_back(p);
            std::cout << "Allocated block " << i << " at " << p << " with value " << *p << "\n";
        }
        
        // Deallocate
        for (void* p : blocks) {
            alloc.Deallocate(p);
        }
        blocks.clear();
        
        // Test allocation after deallocation
        int* p = (int*)alloc.Allocate(sizeof(int));
        *p = 100;
        std::cout << "New allocated block after deallocation: " << *p << " at " << p;
        alloc.Deallocate(p);
        
    } catch (const std::bad_alloc& e) {
        std::cerr << "Allocation failed: " << e.what() << "\n";
    }
    
    std::cout << "\n";
}

void testHeapPoolMode() {
    std::cout << "=== Testing HEAP_POOL mode ===\n";
    
    // Create allocator with fixed pool size
    const int POOL_SIZE = 5;
    Allocator alloc(sizeof(double), POOL_SIZE, nullptr, "Heap Pool Allocator");
    
    std::vector<void*> blocks;
    try {
        // Allocate within pool limits
        for (int i = 0; i < POOL_SIZE; ++i) {
            double* p = (double*)alloc.Allocate(sizeof(double));
            *p = 3.14 * i;
            blocks.push_back(p);
            std::cout << "Allocated block " << i << " at " << p << " with value " << *p << "\n";
        }
        
        // Try to allocate beyond pool limits - should throw
        try {
            double* p = (double*)alloc.Allocate(sizeof(double));
            std::cerr << "ERROR: Should not be able to allocate beyond pool size!\n";
            blocks.push_back(p);
        } catch (const std::bad_alloc& e) {
            std::cout << "Successfully caught bad_alloc when exceeding pool size: " << e.what() << "\n";
        }
        
        // Deallocate some and allocate again
        alloc.Deallocate(blocks.back());
        blocks.pop_back();
        
        double* p = (double*)alloc.Allocate(sizeof(double));
        *p = 99.99;
        std::cout << "Allocated new block after deallocation: " << *p << "\n";
        alloc.Deallocate(p);
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    
    std::cout << "\n";
}

void testStaticPoolMode() {
    std::cout << "=== Testing STATIC_POOL mode ===\n";
    
    // Create static memory pool
    const int POOL_SIZE = 3;
    char staticPool[POOL_SIZE * sizeof(long double)];
    
    // Create allocator using our static pool
    Allocator alloc(sizeof(long double), POOL_SIZE, staticPool, "Static Pool Allocator");
    
    std::vector<void*> blocks;
    try {
        // Allocate within pool limits
        for (int i = 0; i < POOL_SIZE; ++i) {
            long double* p = (long double*)alloc.Allocate(sizeof(long double));
            *p = 123.456L * i;
            blocks.push_back(p);
            std::cout << "Allocated block " << i << " at " << p << " with value " << *p << "\n";
        }
        
        // Try to allocate beyond pool limits - should throw
        try {
            long double* p = (long double*)alloc.Allocate(sizeof(long double));
            std::cerr << "ERROR: Should not be able to allocate beyond static pool size!\n";
        } catch (const std::bad_alloc& e) {
            std::cout << "Successfully caught bad_alloc when exceeding static pool size: " << e.what() << "\n";
        }
        
        // Deallocate and reuse
        alloc.Deallocate(blocks[0]);
        long double* p = (long double*)alloc.Allocate(sizeof(long double));
        *p = 789.012L;
        std::cout << "Allocated reused block: " << *p << "\n";
        alloc.Deallocate(p);
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    
    std::cout << "\n";
}
void testAllocatorWithClass() {
    constexpr UINT POOL_SIZE = 5;
    constexpr size_t OBJECT_SIZE = sizeof(TestClass);
    
    // Создаем аллокатор с пулом фиксированного размера
    Allocator allocator(OBJECT_SIZE, POOL_SIZE, nullptr, "TestClass Allocator");
    
    std::cout << "=== Testing allocator with class ===" << std::endl;
    
    // Создаем несколько объектов с использованием аллокатора
    TestClass* obj1 = new(allocator) TestClass(1, "First");
    TestClass* obj2 = new(allocator) TestClass(2, "Second");
    TestClass* obj3 = new(allocator) TestClass(3, "Third");
    
    // Используем объекты
    obj1->print();
    obj2->print();
    obj3->print();
    
    // Удаляем объекты (возвращаем память в аллокатор)
    obj1->~TestClass();
    allocator.Deallocate(obj1);
    obj2->~TestClass();
    allocator.Deallocate(obj2);
    obj3->~TestClass();
    allocator.Deallocate(obj3);
    
    // Проверяем работу аллокатора после освобождения
    std::cout << "\nCreating more objects after deallocations..." << std::endl;
    
    // Снова создаем объекты - должны переиспользовать освобожденную память
    TestClass* obj4 = new(allocator) TestClass(4, "Reused First");
    TestClass* obj5 = new(allocator) TestClass(5, "Reused Second");
    
    obj4->print();
    obj5->print();
    
    // Удаляем новые объекты
    obj4->~TestClass();
    allocator.Deallocate(obj4);
    obj5->~TestClass();
    allocator.Deallocate(obj5);
    
    // Тестирование переполнения пула
    std::cout << "\nTesting pool overflow..." << std::endl;
    try {
        TestClass* objects[POOL_SIZE + 2];
        for (UINT i = 0; i < POOL_SIZE + 2; ++i) {
            objects[i] = new(allocator) TestClass(i + 10, "Overflow Test");
            std::cout << "Created object " << i << std::endl;
        }
        
        // Очистка (этот код не выполнится из-за исключения)
        for (UINT i = 0; i < POOL_SIZE + 2; ++i) {
            objects[i]->~TestClass();
            allocator.Deallocate(objects[i]);
        }
    } catch (const std::bad_alloc& e) {
        std::cout << "Caught bad_alloc as expected: " << e.what() << std::endl;
    }
    
    std::cout << "=== Test completed ===" << std::endl;
}

void testHugeAllocation() {
    std::cout << "=== Testing huge allocation ===\n";
    
    try {
        // Try to allocate an unrealistically large block
        Allocator alloc(sizeof(char), 0, nullptr, "Huge Allocation Test");
        
        try {
            // Attempt to allocate 10TB of memory - should fail
            void* p = alloc.Allocate(10ULL * 1024 * 1024 * 1024 * 1024);
            std::cerr << "ERROR: Should not be able to allocate 10TB!\n";
            alloc.Deallocate(p);
        } catch (const std::bad_alloc& e) {
            std::cout << "Successfully caught bad_alloc for huge allocation: " << e.what() << "\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    
    std::cout << "\n";
}

int main() {
    testHeapBlocksMode();
    testHeapPoolMode();
    testStaticPoolMode();
    testAllocatorWithClass();
    testHugeAllocation();
    
    return 0;
}