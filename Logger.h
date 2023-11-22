#pragma once

#include <iostream>

#ifdef _DEBUG
#define LOG(x) std::cout << x << std::endl;
#define LOG_NO_NL(x) std::cout << x;
#define LOG_ERR(x) std::cout << x << std::endl;
#else
#define LOG(x)
#define LOG_NO_NL(x) 
#define LOG_ERR(x) 
#endif // DEBUG
