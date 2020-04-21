//
// Created by Hython on 2020/4/17.
//

#ifndef NS3_BASE64_H
#define NS3_BASE64_H
//
//  base64 encoding and decoding with C++.
//  Version: 1.01.00
//

#include <string>

std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);

#endif //NS3_BASE64_H
