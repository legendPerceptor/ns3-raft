//
// Created by Hython on 2020/3/13.
//

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <iostream>

int main() {

    rapidjson::Document d;
    d.SetObject();
    rapidjson::Value value;
    //value = "dsdgsdgad";
    d.AddMember("hello", value, d.GetAllocator());
    d.AddMember("good", value, d.GetAllocator());
    d["hello"].SetUint64(3525);
    d["good"].SetString("abcd",4);


    rapidjson::StringBuffer stringBuffer;
    rapidjson::Writer writer(stringBuffer);
    d.Accept(writer);

    std::cout<< stringBuffer.GetString() <<std::endl;


    return 0;
}