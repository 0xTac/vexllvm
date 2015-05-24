#ifndef __SUGAR__
#define __SUGAR__

#include <cstring>

// Lame C++ sugar.  I'm pretty sure there better definitions.

#define let(_lhs, _rhs)  auto _lhs = _rhs

#define foreach(_i, _b, _e) \
	  for(auto _i = _b, _i ## end = _e; _i != _i ## end;  ++ _i)


struct ltstr
{
    bool operator()(const char* str1, const char* str2) const
    {
        return std::strcmp(str1, str2) < 0;
    }
};

#include <vector>
#include <list>
#include <memory>
template<typename T> using ptr_vec_t = std::vector<std::unique_ptr<T>>;
template<typename T> using ptr_list_t = std::list<std::unique_ptr<T>>;

#endif /* __SUGAR__ */

