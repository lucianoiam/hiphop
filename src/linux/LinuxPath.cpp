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
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef CEF_HELPER_BINARY
// Including this file when compiling the CEF helper would involve adding lots
// of dependencies from DPF. Such helper only needs calling getCachesPath().
// The GTK-based helper does not need to call functions in LinuxPath.cpp at all.
#include "DistrhoPluginUtils.hpp"
#endif

#include "util/Path.hpp"
#include "util/macro.h"

USE_NAMESPACE_DISTRHO

#ifndef CEF_HELPER_BINARY
String path::getLibraryPath()
{
    String path = String(getBinaryFilename());
    path.truncate(path.rfind('/'));

    const char* format = getPluginFormatName();

    if (strcmp(format, "LV2") == 0) {
        return path + "/" + kBundleLibrarySubdirectory;
    } else if (strcmp(format, "VST2") == 0) {
        return path + "/" + kNoBundleLibrarySubdirectory;
    } else if (strcmp(format, "VST3") == 0) {
        return path.truncate(path.rfind('/')) + "/Resources";
    }

    return path + "/" + kNoBundleLibrarySubdirectory;
}
#endif // CEF_HELPER_BINARY

String path::getCachesPath()
{
    String path;
    struct passwd *pw = getpwuid(getuid());
    path += pw->pw_dir;
    path += "/.config/" XSTR(PLUGIN_BIN_BASENAME);
    mkdir(path, 0777);
    path += "/" + kCacheSubdirectory;
    mkdir(path, 0777);
    return path;
}
