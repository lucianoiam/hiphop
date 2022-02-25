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

#include <string.h>

#include "extra/UIEx.hpp"

UIEx::UIEx(uint width, uint height, bool automaticallyScaleAndSetAsMinimumSize)
    : UI(width, height, automaticallyScaleAndSetAsMinimumSize)
{
#if HIPHOP_ENABLE_SHARED_MEMORY
    // Asynchronous state updates are only possible from the UI to the Plugin
    // instance and not the other way around. So the UI creates the shared
    // memory and lets the Plugin know how to locate it with filenames and when
    // to destroy it. DPF state updates flow: UI--state-->Host--state-->Plugin.

    if (fMemory.create()) {
        String val;
        val = "init_p2ui:";
        val += fMemory.in.getDataFilename();
        setState("_shmem", val);
        val = "init_ui2p:";
        val += fMemory.out.getDataFilename();
        setState("_shmem", val);
    } else {
        d_stderr2("Could not create shared memory");
    }
#endif
}

UIEx::~UIEx()
{
#if HIPHOP_ENABLE_SHARED_MEMORY
    fMemory.close();
    setState("_shmem", "deinit");
#endif
}

#if HIPHOP_ENABLE_SHARED_MEMORY
size_t UIEx::getSharedMemorySize() const noexcept
{
    return fMemory.out.getSizeBytes();
}

bool UIEx::writeSharedMemory(const char* metadata, const unsigned char* data, size_t size)
{
    if (fMemory.out.write(metadata, data, size)) {
        // Notify Plugin instance there is new data available for reading
        setState("_shmem", "data_ui2p");
        return true;
    } else {
        d_stderr2("Could not write shared memory (ui->plugin)");
        return false;
    }
}

#if HIPHOP_ENABLE_WASM_PLUGIN
void UIEx::sideloadWasmBinary(const unsigned char* data, size_t size)
{
    // Send binary to the Plugin instance. This could be also achieved using the
    // state interface by first encoding data into something like Base64.
    
    writeSharedMemory("_wasm_bin", data, size);
}
#endif // HIPHOP_ENABLE_WASM_PLUGIN
#endif // HIPHOP_ENABLE_SHARED_MEMORY

#if HIPHOP_ENABLE_SHARED_MEMORY
void UIEx::uiIdle()
{
    // ExternalWindow does not implement the IdleCallback methods. If uiIdle()
    // is not fast enough for visualizations a custom timer solution needs to be
    // implemented, or DPF modified so the uiIdle() frequency can be configured.

    if (! fMemory.in.isRead()) {
        sharedMemoryChanged(fMemory.in.getMetadata(), fMemory.in.getDataPointer(),
                            fMemory.in.getDataSize());
        fMemory.in.setRead();
    }
}
#endif
