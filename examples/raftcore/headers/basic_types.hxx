/**
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  The ASF licenses
* this file to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef _BASIC_TYPES_HXX_
#define _BASIC_TYPES_HXX_

typedef uint64_t ulong;
typedef void* any_ptr;
typedef unsigned char byte;
typedef uint16_t ushort;
typedef uint32_t uint;
typedef int32_t int32;
#include "ns3env.h"
#ifdef NS3_ENV
#include "ns3/nstime.h"
#include "ns3/simulator.h"

using time_point = ns3::Time;
using system_clock = ns3::Simulator;
#else

using time_point = std::chrono::high_resolution_clock::time_point;
using system_clock = std::chrono::high_resolution_clock;

#endif

#endif // _BASIC_TYPES_HXX_