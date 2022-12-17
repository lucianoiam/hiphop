/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2022 Luciano Iam <oss@lucianoiam.com>
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

#ifndef UI_EX_HPP
#define UI_EX_HPP

#include "DistrhoUI.hpp"

#if defined(HIPHOP_SHARED_MEMORY_SIZE)
# if ! DISTRHO_PLUGIN_WANT_STATE
#  error Shared memory support requires DISTRHO_PLUGIN_WANT_STATE
# endif
# include "SharedMemoryImpl.hpp"
#endif 

START_NAMESPACE_DISTRHO

// This class adds some goodies to DISTRHO::UI like shared memory support

class UIEx : public UI
{
public:
    UIEx(uint width = 0, uint height = 0);
    virtual ~UIEx() {}

protected:
#if defined(HIPHOP_SHARED_MEMORY_SIZE)
    SharedMemoryImpl& getSharedMemory() noexcept { return fMemory; }

    bool writeSharedMemory(const unsigned char* data, size_t size, size_t offset = 0,
                           uint32_t hints = 0);

    virtual void sharedMemoryReady() {}

# if HIPHOP_SHARED_MEMORY_WRITE_CALLBACK
    virtual void sharedMemoryChanged(const unsigned char* data, size_t size, uint32_t hints)
    {
        (void)data;
        (void)size;
        (void)hints;
    }

    void uiIdle() override;
# endif
# if defined(HIPHOP_WASM_SUPPORT)
    void sideloadWasmBinary(const unsigned char* data, size_t size);
# endif
#endif // HIPHOP_SHARED_MEMORY_SIZE

#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* key, const char* value) override;
#endif

private:
#if defined(HIPHOP_SHARED_MEMORY_SIZE)
    SharedMemoryImpl fMemory;
#endif

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIEx)

};

END_NAMESPACE_DISTRHO

#endif  // UI_EX_HPP
