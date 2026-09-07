//
// Created by maxi on 9/7/26.
//

#ifndef MYAPP_OVERLOAD_H
#define MYAPP_OVERLOAD_H

template<class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

#endif // MYAPP_OVERLOAD_H
