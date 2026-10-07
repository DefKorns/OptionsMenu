# FIX_LIBSTDCXX=1 builds the _compat variant: it ships a private libstdc++.so.6
# via rpath, immune to other hmods replacing the system copy (~225KB more).
ifdef FIX_LIBSTDCXX
MOD_ID   := options_menu_compat
MOD_NAME := Options Menu (compat)
VARIANT  := compat
else
MOD_ID   := options_menu
MOD_NAME := Options Menu
VARIANT  := plain
endif
MOD_CREATOR  := CompCom, DefKorns
MOD_CATEGORY := System
BINARIES     := mod/etc/options_menu/options mod/etc/options_menu/optiond mod/etc/options_menu/mod_uninstall/mod_uninstall mod/bin/standby_watchdog mod/etc/options_menu/scripts/gen_splash mod/etc/options_menu/scripts/ChangeCombo

CXX = g++
STRIP = strip
ifdef CROSS_PREFIX
PKG_CONFIG_LIBDIR = /usr/lib/arm-linux-gnueabihf/pkgconfig
SDL_CFLAGS = -I/usr/include/arm-linux-gnueabihf $(shell PKG_CONFIG_LIBDIR=$(PKG_CONFIG_LIBDIR) pkg-config --cflags sdl2 SDL2_ttf libpng)
SDL_LIBS = $(shell PKG_CONFIG_LIBDIR=$(PKG_CONFIG_LIBDIR) pkg-config --libs sdl2 SDL2_ttf libpng)
PNG_LIBS = $(shell PKG_CONFIG_LIBDIR=$(PKG_CONFIG_LIBDIR) pkg-config --libs libpng)
LDFLAGS = -Wl,--allow-shlib-undefined
VENDORED_LIBS = mod/etc/options_menu/lib/libSDL2_ttf-2.0.so.0 mod/etc/options_menu/lib/libfreetype.so.2.8
ifdef FIX_LIBSTDCXX
VENDORED_LIBS += mod/etc/options_menu/lib/libstdc++.so.6
STDCXX_RPATH = -Wl,-rpath,/etc/options_menu/lib
STDCXX_RPATH_ORIGIN = -Wl,-rpath,'$$ORIGIN/lib'
endif
else
SDL_CFLAGS = $(shell sdl2-config --cflags) $(shell pkg-config --cflags SDL2_ttf)
SDL_LIBS = $(shell sdl2-config --libs) $(shell pkg-config --libs SDL2_ttf) -lpng
PNG_LIBS = -lpng
LDFLAGS =
VENDORED_LIBS =
endif
CXXFLAGS = -std=c++11 -Os $(SDL_CFLAGS) -DMOD_VERSION=\"v$(MOD_VER)\"
LDLIBS = $(SDL_LIBS)
SOURCES = src/main.cpp src/command.cpp src/localization.cpp src/framework/sdl_context.cpp src/framework/texture.cpp src/framework/controller.cpp src/framework/powerwatch.cpp src/framework/draw_helpers.cpp src/framework/utf8.cpp src/framework/font8x8_lookup.cpp src/framework/uitheme.cpp
OBJECTS = $(SOURCES:.cpp=.o)
ALL_OBJECTS = $(OBJECTS) src/daemon.o src/mod_uninstall.o src/standby_watchdog.o src/gen_splash.o src/change_combo.o

MOD_DEPS := $(BINARIES) $(VENDORED_LIBS)
ifndef FIX_LIBSTDCXX
# a prior compat build leaves this behind in mod/
MOD_DEPS += no-private-libstdcxx
endif

all: hmod

compile: $(BINARIES) $(VENDORED_LIBS)

# relink everything when switching variant (different rpaths) or version (MOD_VERSION)
.build-flags: FORCE
	@[ "$$(cat $@ 2>/dev/null)" = "$(VARIANT) $(MOD_VER)" ] || echo "$(VARIANT) $(MOD_VER)" > $@

$(ALL_OBJECTS): .build-flags

no-private-libstdcxx:
	rm -f mod/etc/options_menu/lib/libstdc++.so.6

# SDL2_ttf/freetype aren't on the console's own rootfs; shipped as
# mod/etc/options_menu/lib/*.so and found via rpath instead.
mod/etc/options_menu/options: $(OBJECTS)
	$(CROSS_PREFIX)$(CXX) $(OBJECTS) $(LDLIBS) $(LDFLAGS) -Wl,-rpath,'$$ORIGIN/lib' -o $@
	$(CROSS_PREFIX)$(STRIP) $@

mod/etc/options_menu/lib:
	mkdir -p $@

# not ours to redistribute, pulled from the toolchain image each build
mod/etc/options_menu/lib/libSDL2_ttf-2.0.so.0: | mod/etc/options_menu/lib
	cp /usr/lib/arm-linux-gnueabihf/libSDL2_ttf.so $@

mod/etc/options_menu/lib/libfreetype.so.2.8: | mod/etc/options_menu/lib
	cp /usr/lib/arm-linux-gnueabihf/libfreetype.so.2.8.1 $@

mod/etc/options_menu/lib/libstdc++.so.6: | mod/etc/options_menu/lib
	cp $$($(CROSS_PREFIX)$(CXX) -print-file-name=libstdc++.so.6) $@
	$(CROSS_PREFIX)$(STRIP) --strip-unneeded $@

mod/etc/options_menu/optiond: src/daemon.o src/framework/controller.o
	$(CROSS_PREFIX)$(CXX) $(LDFLAGS) $^ $(STDCXX_RPATH_ORIGIN) -o $@
	$(CROSS_PREFIX)$(STRIP) $@

mod/etc/options_menu/mod_uninstall/mod_uninstall: src/mod_uninstall.o src/localization.o $(filter src/framework/%,$(OBJECTS))
	$(CROSS_PREFIX)$(CXX) $^ $(LDLIBS) $(LDFLAGS) -Wl,-rpath,/etc/options_menu/lib -o $@
	$(CROSS_PREFIX)$(STRIP) $@
	upx --lzma $@

mod/bin/standby_watchdog: src/standby_watchdog.o src/framework/controller.o src/framework/powerwatch.o
	$(CROSS_PREFIX)$(CXX) $(LDFLAGS) $^ $(STDCXX_RPATH) -o $@
	$(CROSS_PREFIX)$(STRIP) $@
	upx --lzma $@

mod/etc/options_menu/scripts/gen_splash: src/gen_splash.o src/localization.o src/framework/utf8.o src/framework/font8x8_lookup.o
	$(CROSS_PREFIX)$(CXX) $^ $(PNG_LIBS) $(LDFLAGS) $(STDCXX_RPATH) -o $@
	$(CROSS_PREFIX)$(STRIP) $@

mod/etc/options_menu/scripts/ChangeCombo: src/change_combo.o src/framework/controller.o src/localization.o
	$(CROSS_PREFIX)$(CXX) $^ $(LDFLAGS) $(STDCXX_RPATH) -o $@
	$(CROSS_PREFIX)$(STRIP) $@
	upx --lzma $@

%.o: %.cpp
	$(CROSS_PREFIX)$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	find . -name "*.o" -type f -not -path "./toolchain/*" -delete
	rm -f $(BINARIES) .build-flags
	rm -rf mod/etc/options_menu/lib out/

include hmod-build/hmod.mk

.PHONY: all compile clean no-private-libstdcxx
