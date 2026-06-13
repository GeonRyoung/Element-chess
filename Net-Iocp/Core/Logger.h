#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#endif

class Logger
{
private:
    static std::mutex& GetMutex()
    {
        static std::mutex mtx;
        return mtx;
    }

    static std::string GetCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm parts;
        localtime_s(&parts, &now_c);
        
        std::ostringstream oss;
        oss << "[" << std::put_time(&parts, "%H:%M:%S") << "]";
        return oss.str();
    }

#ifdef _WIN32
    enum class Color
    {
        GREEN = 10,
        YELLOW = 14,
        RED = 12,
        WHITE = 15,
        GRAY = 8
    };

    static void SetColor(Color color)
    {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, static_cast<WORD>(color));
    }
#else
    static void SetColor(int) {}
#endif

public:
    static void Info(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(GetMutex());
#ifdef _WIN32
        SetColor(Color::GREEN);
        std::cout << "[INFO] " << GetCurrentTimestamp() << " ";
        SetColor(Color::WHITE);
        std::cout << message << std::endl;
#else
        std::cout << "[INFO] " << GetCurrentTimestamp() << " " << message << std::endl;
#endif
    }

    static void Warn(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(GetMutex());
#ifdef _WIN32
        SetColor(Color::YELLOW);
        std::cout << "[WARN] " << GetCurrentTimestamp() << " ";
        SetColor(Color::WHITE);
        std::cout << message << std::endl;
#else
        std::cout << "[WARN] " << GetCurrentTimestamp() << " " << message << std::endl;
#endif
    }

    static void Error(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(GetMutex());
#ifdef _WIN32
        SetColor(Color::RED);
        std::cerr << "[ERROR] " << GetCurrentTimestamp() << " ";
        SetColor(Color::WHITE);
        std::cerr << message << std::endl;
#else
        std::cerr << "[ERROR] " << GetCurrentTimestamp() << " " << message << std::endl;
#endif
    }
};
