#pragma once

#include <iostream>
#include <thread>


#define LOG(x)
//#define LOG(x) std::cerr << __FUNCTION__ << "(" << __LINE__ << "): " << x << "(" << std::this_thread::get_id() << ")" << std::endl;
