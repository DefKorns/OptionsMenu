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

#ifndef UTF8_H_
#define UTF8_H_

#include <string>
#include <vector>

// decodes UTF-8 into Unicode codepoints - malformed sequences become U+FFFD
std::vector<unsigned int> Utf8ToCodepoints(const std::string & text);

// number of codepoints, not bytes
int Utf8Length(const std::string & text);

// truncates by codepoint, not byte, so multi-byte glyphs (e.g. Japanese) aren't split
std::string TruncateUtf8(const std::string & text, int maxCodepoints);

#endif
