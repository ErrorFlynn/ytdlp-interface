#include "subclass.hpp"


std::recursive_mutex subclass::mutex_;
std::map<HWND, subclass*> subclass::table_;
bool subclass::eat_mouse {false};