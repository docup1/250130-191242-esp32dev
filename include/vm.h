#ifndef VM_H
#define VM_H

#include <Arduino.h>
#include <LittleFS.h>

#define MEM_SIZE     4096   // Размер оперативной памяти
#define NUM_REGS     8      // Количество регистров
#define STACK_SIZE   256    // Размер стека

// Определения опкодов для инструкций
enum Opcode : uint8_t {
    // Управляющие инструкции
    OP_HALT       = 0x01,
    OP_LOAD       = 0x10,
    OP_STORE      = 0x11,
    OP_ADD        = 0x20,
    OP_SUB        = 0x21,
    OP_MUL        = 0x22,
    OP_DIV        = 0x23,
    OP_PUSH       = 0x30,
    OP_POP        = 0x31,
    OP_SYSCALL    = 0xFF,
};

class VirtualMachine {
private:
    // Вложенная структура для работы с памятью и хранением состояния на файловой системе
    struct Storage {
        uint8_t ram[MEM_SIZE] = {0}; // Оперативная память (RAM)
        const char* storageFile = "/system/systemdata.dat";

        // Инициализация файловой системы и файла для хранения состояния
        void init() {
            if (!LittleFS.begin()) {
                Serial.println("Failed to mount LittleFS");
                return;
            }
            if (!LittleFS.exists(storageFile)) {
                File f = LittleFS.open(storageFile, "w");
                if (f) f.close();
                else Serial.println("Failed to create system data file");
            }
        }

        // Чтение байта по адресу (без дополнительных проверок, т.к. проверка происходит на уровне VM)
        uint8_t read(uint32_t address) const {
            return (address < MEM_SIZE) ? ram[address] : 0;
        }

        // Запись байта по адресу (без дополнительных проверок)
        void write(uint32_t address, uint8_t value) {
            if (address < MEM_SIZE) {
                ram[address] = value;
            }
        }

        // Сохранение состояния памяти в файл
        void persist() {
            File f = LittleFS.open(storageFile, "w");
            if (f) {
                f.write(ram, MEM_SIZE);
                f.close();
            } else {
                Serial.println("Failed to persist state");
            }
        }

        // Восстановление состояния памяти из файла
        void restore() {
            File f = LittleFS.open(storageFile, "r");
            if (f) {
                size_t readBytes = f.read(ram, MEM_SIZE);
                if (readBytes != MEM_SIZE) {
                    Serial.printf("Warning: Expected %d bytes, but read %d bytes\n", MEM_SIZE, readBytes);
                }
                f.close();
            } else {
                Serial.println("Failed to restore state");
            }
        }
    };

    Storage storage;

    uint32_t reg[NUM_REGS] = {0};   // Регистры общего назначения
    uint32_t pc = 0;                // Счётчик команд (program counter)
    uint32_t sp = STACK_SIZE - 1;   // Указатель стека
    uint32_t stack[STACK_SIZE] = {0}; // Стек
    bool running = false;           // Флаг работы ВМ

    // Вспомогательные функции для чтения/записи 32-битных значений
    uint32_t read32(uint32_t address);
    void write32(uint32_t address, uint32_t value);

    // Обработчик системных вызовов
    void handleSystemCall(uint8_t code);

    // Методы для работы со стеком
    bool push(uint32_t value);
    bool pop(uint32_t &value);

public:
    VirtualMachine();
    void reset();
    void loadProgram(const uint8_t* program, size_t size);
    void run();
    void persistState();
    void printState();
};

#endif // VM_H
