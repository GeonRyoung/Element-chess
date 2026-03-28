#pragma once
#include <iostream>
#include <format>
#include <chrono>
#include <string>

/*=====================
	Managers
=====================*/
#define GDBManager DBManager::GetInstance()

#define GDataManager DataManager::GetInstance()

/*=====================
	Log
=====================*/
inline std::string GetTimestamp() {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	struct tm buf;
	localtime_s(&buf, &in_time_t);
	char str[10];
	strftime(str, sizeof(str), "%H:%M:%S", &buf);
	return std::string(str);
}

// [옵션] 내용 형태의 매크로
#define GLOG(Category, Format, ...) \
printf("[%s][%s] " Format "\n", GetTimestamp().c_str(), #Category, ##__VA_ARGS__)

// 에러 로그용 (빨간색 강조나 cerr 사용 가능)
#define GLOG_ERROR(Category, Format, ...) \
fprintf(stderr, "[%s][%s][ERROR] " Format "\n", GetTimestamp().c_str(), #Category, ##__VA_ARGS__)