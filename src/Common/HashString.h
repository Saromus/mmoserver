/*
---------------------------------------------------------------------------------------
This source file is part of SWG:ANH (Star Wars Galaxies - A New Hope - Server Emulator)

For more information, visit http://www.swganh.com

Copyright (c) 2006 - 2010 The SWG:ANH Team
---------------------------------------------------------------------------------------
Use of this source code is governed by the GPL v3 license that can be found
in the COPYING file or at http://www.gnu.org/licenses/gpl-3.0.html

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
---------------------------------------------------------------------------------------
*/

#ifndef SRC_COMMON_HASHSTRING_H_
#define SRC_COMMON_HASHSTRING_H_

#include <cstdint>
#include <string>

#include "Common/declspec.h"

/*! \brief Common is a catch-all library containing primarily base classes and
 * classes used for maintaining application lifetimes.
 */
namespace common {

class HashString;

/**
 * Writes a HashString to an output stream in a human readable format.
 *
 * \param message The output stream to write the data to.
 * \param string The HashString to stream data from.
 */
COMMON_API std::ostream& operator<<(std::ostream& message, const HashString& string);

static const char* const kWildCardHashString = "*";

/*! \brief This class provides a utility for generating identifiers that are
 * easy to read and can be used as key values in the standard associative containers.
 */
class COMMON_API HashString {
public:
    /**
     * This explicit constructor takes a human readable string and stores a hash of it.
     */
    explicit HashString(char const * const ident_string);

    /// The default destructor.
    ~HashString();

    // \note: The default copy constructor and assignment operator functions
    // that are generated by the compiler are perfectly fine as is.

    /**
     * Uses a less-than comparison on two HashStrings.
     *
     * This is a requirement for this data type to be used as a key value in
     * containers like std::map.
     *
     * \param other The HashString to compare to the current one.
     * \returns True if the ident is less than that of the other's, false if not.
     */
    bool operator<(const HashString& other) const;

    /**
     * Uses a greater-than comparison on two HashStrings.
     *
     * This is a requirement for this data type to be used as a key value in
     * containers like std::map.
     *
     * \param other The HashString to compare to the current one.
     * \returns True if the ident is greater than that of the other's, false if not.
     */
    bool operator>(const HashString& other) const;

    /**
     * Compares two HashStrings to determine if they are equal.
     *
     * \param other The HashString to compare to the current one.
     * \returns True if the two HashStrings are equal, false if not.
     */
    bool operator==(const HashString& other) const;

    /**
     * Compares two HashStrings to determine if they are not equal.
     *
     * \param other The HashString to compare to the current one.
     * \returns True if the two HashStrings are not equal, false if they are.
     */
    bool operator!=(const HashString& other) const;

    /**
     * \returns A unique identifier for this HashString.
     */
    uint32_t ident() const;

    /**
     * \returns A unique identifier for this HashString.
     */
    const std::string& ident_string() const;

private:
    /// Disable the default constructor. Must use the explicit constructor.
    HashString();

    // \note: ident_ is stored as a void * not an int, so that in
    // the debugger it will show up as hex-values instead of
    // integer values. This is a bit more representative of what
    // we're doing here and makes it easy to allow external code
    // to assign event types as desired.
    void * ident_; ///< This is a 32bit hash of the ident_string.

    // Win32 complains about stl during linkage, disable the warning.
#ifdef _WIN32
#pragma warning (disable : 4251)
#endif
    std::string ident_string_; ///< This is a human readable form of the event type.
    // Re-enable the warning.
#ifdef _WIN32
#pragma warning (default : 4251)
#endif
};

}  // namespace common

#endif  // SRC_COMMON_HASHSTRING_H_
