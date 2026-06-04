#pragma once
#include <functional>

using JobFunc = std::function<void()>;

class IExecutionService
{
public:
    virtual ~IExecutionService() = default;
    virtual void Execute(JobFunc job) = 0;
};
