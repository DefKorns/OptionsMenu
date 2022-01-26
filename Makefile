TARGET_NAME = options_deluxe
MOD_NAME := Options Menu
MOD_CREATOR := CompCom
MOD_BUILDER := DefKorns
MOD_CATEGORY := System
MOD_VER := 1.3.4b
ESC_MOD_VER := $(subst .,_,$(MOD_VER))
CXX = g++
STRIP = strip
CXXFLAGS = -std=c++11 -Os `sdl2-config --cflags`
LDLIBS =  `sdl2-config --libs` -lpng
LDFLAGS = 
SOURCES = src/main.cpp src/command.cpp src/framework/sdl_context.cpp src/framework/texture.cpp src/framework/controller.cpp src/framework/powerwatch.cpp
OBJECTS = $(SOURCES:.cpp=.o)
DEV_DIR = $(shell realpath .)
OUT = $(DEV_DIR)/out
TMP = $(DEV_DIR)/temp

define inject-readme =
	mkdir -p $(OUT)/ $(TMP)/
	cp mod/readme.md temp/readme.md

	printf "%s\n" \
	"---" \
	"Name: $(MOD_NAME)" \
	"Creator: $(MOD_CREATOR)" \
	"Category: $(MOD_CATEGORY)" \
	"Version: $(MOD_VER)" \
	"Built on: $(shell date +"%A, %d %b %Y - %T") by $(MOD_BUILDER)" \
	"Git commit: $(GIT_COMMIT)" \
	"---" > mod/readme.md
	
	sed 1d temp/readme.md >> mod/readme.md
endef

all: $(TARGET_NAME).hmod $(TARGET_NAME)_b_down.hmod

$(TARGET_NAME)_b_down.hmod: $(TARGET_NAME).hmod
	$(inject-readme)
	
	cp button.cfg mod/etc/options_menu/
	cd mod/; tar -czvf "../out/$(TARGET_NAME)_b_down.hmod" *
	cp temp/readme.md mod/readme.md


$(TARGET_NAME).hmod: mod/etc/options_menu/options mod/etc/options_menu/optiond mod/etc/options_menu/mod_uninstall/mod_uninstall mod/bin/standby_watchdog
	$(CROSS_PREFIX)$(STRIP) mod/etc/options_menu/options
	$(CROSS_PREFIX)$(STRIP) mod/etc/options_menu/optiond
	$(CROSS_PREFIX)$(STRIP) mod/etc/options_menu/mod_uninstall/mod_uninstall
	$(CROSS_PREFIX)$(STRIP) mod/bin/standby_watchdog

	$(inject-readme)

	rm -f mod/etc/options_menu/button.cfg
	cd mod/; tar -czvf "../out/$(TARGET_NAME).hmod" *
	
	cp temp/readme.md mod/readme.md
	find -name "*.o" -type f -delete
	rm -rf $(TMP)

mod/etc/options_menu/options: $(OBJECTS)
	$(CROSS_PREFIX)$(CXX) $(OBJECTS) $(LDLIBS) $(LDFLAGS) -o mod/etc/options_menu/options

mod/etc/options_menu/optiond: src/daemon.o src/framework/controller.o
	$(CROSS_PREFIX)$(CXX) $(LDFLAGS) src/daemon.o src/framework/controller.o -o mod/etc/options_menu/optiond

mod/etc/options_menu/mod_uninstall/mod_uninstall: src/mod_uninstall.o
	$(CROSS_PREFIX)$(CXX) src/mod_uninstall.o src/framework/*.o $(LDLIBS) $(LDFLAGS) -o mod/etc/options_menu/mod_uninstall/mod_uninstall

mod/bin/standby_watchdog: src/standby_watchdog.o src/framework/controller.o src/framework/powerwatch.o
	$(CROSS_PREFIX)$(CXX) src/standby_watchdog.o src/framework/controller.o src/framework/powerwatch.o -o mod/bin/standby_watchdog

%.o: %.cpp
	$(CROSS_PREFIX)$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	find -name "*.o" -type f -delete
	rm -f mod/etc/options_menu/options mod/etc/options_menu/optiond mod/etc/options_menu/mod_uninstall/mod_uninstall mod/bin/standby_watchdog mod/etc/options_menu/button.cfg $(TARGET_NAME).hmod $(TARGET_NAME)_b_down.hmod
	rm -rf out/ temp/

.PHONY: clean
