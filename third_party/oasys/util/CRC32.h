/*
 *    Copyright 2004-2006 Intel Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */



#ifndef _OASYS_CRC32_H_
#define _OASYS_CRC32_H_

#include <sys/types.h>
#include "../compat/inttypes.h"

namespace oasys {
class CRC32 {
public:
    CRC32();

    typedef u_int32_t CRC_t;

    /** @{
     * Update the crc with the data in the buf 
     */
    void update(const u_char* buf, size_t length);
    void update(const char*   buf, size_t length) {
        update((const u_char*)buf, length);
    }
    /** @} */

    const CRC_t& value();
    void reset();

    static CRC_t from_bytes(u_char* buf);

private:
    CRC_t crc_;
    CRC_t crc_finished_;
};
}; // namespace oasys

#endif //_OASYS_CRC32_H_
