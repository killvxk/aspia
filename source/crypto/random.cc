//
// Aspia Project
// Copyright (C) 2018 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "crypto/random.h"

#include <openssl/opensslv.h>
#include <openssl/rand.h>

namespace aspia {

// static
bool Random::fillBuffer(void* buffer, size_t size)
{
    if (!buffer || !size)
        return false;

    return RAND_bytes(reinterpret_cast<uint8_t*>(buffer), size) > 0;
}

// static
std::string Random::generateBuffer(size_t size)
{
    std::string random_buffer;
    random_buffer.resize(size);

    if (!fillBuffer(random_buffer.data(), random_buffer.size()))
        return std::string();

    return random_buffer;
}

// static
uint32_t Random::generateNumber()
{
    uint32_t result;

    if (!fillBuffer(&result, sizeof(result)))
        return 0;

    return result;
}

} // namespace aspia
