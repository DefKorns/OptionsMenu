/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
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

#include "close_on_exec.h"

#include <cstdlib>
#include <memory>
#include <dirent.h>
#include <fcntl.h>

void MarkFdsCloseOnExec()
{
    std::unique_ptr<DIR, decltype(&closedir)> dir(opendir("/proc/self/fd"), &closedir);
    if(!dir)
        return;
    const int dirFd = dirfd(dir.get());
    while(const dirent * entry = readdir(dir.get()))
    {
        char * end = nullptr;
        const long fd = std::strtol(entry->d_name, &end, 10);
        if(end == entry->d_name || *end != '\0' || fd < 3 || fd == dirFd)
            continue;
        const int flags = fcntl(static_cast<int>(fd), F_GETFD);
        if(flags >= 0 && !(flags & FD_CLOEXEC))
            fcntl(static_cast<int>(fd), F_SETFD, flags | FD_CLOEXEC);
    }
}
