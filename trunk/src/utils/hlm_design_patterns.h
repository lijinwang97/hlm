#ifndef DESIGN_PATTERNS_H
#define DESIGN_PATTERNS_H

#include <functional>

#include "crow.h"

// 装饰器模式：包装处理函数，添加日志功能
crow::response LogWrapper(const crow::request& req, std::function<crow::response(const crow::request&)> handler);

#endif  // DESIGN_PATTERNS_H
