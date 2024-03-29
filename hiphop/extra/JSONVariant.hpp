/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2023 Luciano Iam <oss@lucianoiam.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef JSON_VARIANT_HPP
#define JSON_VARIANT_HPP

#include <initializer_list>
#include <utility>

#include "distrho/extra/String.hpp"
#include "thirdparty/cJSON.h"

#include "VariantUtil.hpp"

START_NAMESPACE_DISTRHO

class JSONVariant
{
public:
    JSONVariant() noexcept;
    JSONVariant(bool b) noexcept;
    JSONVariant(double d) noexcept;
    JSONVariant(String s) noexcept;
    JSONVariant(const BinaryData& data) noexcept;

    // Convenience constructors for plugin code
    JSONVariant(int32_t i) noexcept;
    JSONVariant(uint32_t i) noexcept;
    JSONVariant(float f) noexcept;
    JSONVariant(const char* s) noexcept;

    typedef std::pair<const char*,JSONVariant> KeyValue;
    JSONVariant(std::initializer_list<KeyValue> items) noexcept;
    JSONVariant(std::initializer_list<JSONVariant> items) noexcept;

    ~JSONVariant();

    JSONVariant(const JSONVariant& var) noexcept;
    JSONVariant& operator=(const JSONVariant& var) noexcept;
    JSONVariant(JSONVariant&& var) noexcept;
    JSONVariant& operator=(JSONVariant&& var) noexcept;

    static JSONVariant createObject(std::initializer_list<KeyValue> items = {}) noexcept;
    static JSONVariant createArray(std::initializer_list<JSONVariant> items = {}) noexcept;

    bool isNull() const noexcept;
    bool isBoolean() const noexcept;
    bool isNumber() const noexcept;
    bool isString() const noexcept;
    bool isBinaryData() const noexcept;
    bool isArray() const noexcept;
    bool isObject() const noexcept;

    String      asString() const noexcept;
    bool        getBoolean() const noexcept;
    double      getNumber() const noexcept;
    String      getString() const noexcept;
    BinaryData  getBinaryData() const noexcept;
    int         getArraySize() const noexcept;
    JSONVariant getArrayItem(int idx) const noexcept;
    JSONVariant getObjectItem(const char* key) const noexcept;
    JSONVariant operator[](int idx) const noexcept;
    JSONVariant operator[](const char* key) const noexcept;

    void pushArrayItem(const JSONVariant& var) noexcept;
    void setArrayItem(int idx, const JSONVariant& var) noexcept;
    void insertArrayItem(int idx, const JSONVariant& var) noexcept;
    void setObjectItem(const char* key, const JSONVariant& var) noexcept;

    JSONVariant sliceArray(int start, int end = -1) const noexcept
    {
        return ::sliceVariantArray(*this, start, end);
    }

    JSONVariant& operator+=(const JSONVariant& other) noexcept
    {
        return ::joinVariantArrays(*this, other);
    }

    friend JSONVariant operator+(JSONVariant lhs, const JSONVariant& rhs) noexcept
    {
        lhs += rhs;
        return lhs;
    }

    operator bool()   const noexcept { return getBoolean(); }
    operator double() const noexcept { return getNumber(); }
    operator String() const noexcept { return getString(); }

    String toJSON(bool format = false) const noexcept;
    static JSONVariant fromJSON(const char* jsonText) noexcept;

private:
    JSONVariant(cJSON* impl) noexcept;

    cJSON* fImpl;

};

END_NAMESPACE_DISTRHO

#endif // JSON_VARIANT_HPP
