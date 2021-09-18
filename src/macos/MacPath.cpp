/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021 Luciano Iam <oss@lucianoiam.com>
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

#include <cstring>
#include <dlfcn.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/syslimits.h>

#include "Path.hpp"
#include "macro.h"

USE_NAMESPACE_DISTRHO

String path::getBinaryPath()
{
    Dl_info dl_info;

    if (dladdr((void *)&__PRETTY_FUNCTION__, &dl_info) != 0) {
        return String(dl_info.dli_fname);
    }

    HIPHOP_LOG_STDERR(dlerror());

    return String();
}

String path::getLibraryPath()
{
    char path[PATH_MAX];
    strcpy(path, getBinaryPath());
    void *handle = dlopen(path, RTLD_NOLOAD);

    if (handle != 0) {
        void *addr = dlsym(handle, "VSTPluginMain");
        dlclose(handle);
        if (addr != 0) {
            return String(dirname(path)) + "/../Resources";
        }
    }

    strcpy(path, getBinaryPath().buffer());

    return String(dirname(path)) + "/" + kDefaultLibrarySubdirectory;
}

String path::getTemporaryPath()
{
    return String(); // not implemented
}