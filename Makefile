.PHONY: all clean check iwyu test

GPP_WARNS = \
 -Wcast-align \
 -Wconversion \
 -Wctor-dtor-privacy \
 -Wduplicated-branches \
 -Wduplicated-cond \
 -Wfloat-equal \
 -Wlogical-op \
 -Wlong-long \
 -Wno-init-list-lifetime \
 -Wold-style-cast \
 -Woverloaded-virtual \
 -Wredundant-decls \
 -Wshadow \
 -Wsign-promo \
 -Wstrict-null-sentinel \
 -Wtrampolines \
 -Wundef \
 -Wuseless-cast \
 -Wzero-as-null-pointer-constant \
 -Wno-float-conversion

CLANG_WARNS = \
 -Weverything \
 -Wno-c++98-compat \
 -Wno-missing-prototypes \
 -Wno-padded \
 -fno-c++-static-destructors \

CLANG_DIAGS= \
 -fdiagnostics-show-template-tree \

SANI_FLAGS = \
 -O0 \
 -fsanitize=address,pointer-compare,pointer-subtract,undefined\
 -fno-omit-frame-pointer


MYSANI2 = \
 -fsanitize=address \
 -fsanitize=pointer-compare \
 -fsanitize=pointer-subtract \
 -fno-omit-frame-pointer

CPP_FLAGS = \
 -Wall \
 -Wextra \
 -Werror \
 -pedantic \
 -std=c++14 \
 -ggdb3 \
 $(MYSANI)

GPP_FLAGS = \
  $(CPP_FLAGS) \
  $(GPP_WARNS) \
  $(SANI_FLAGS)

CLANG_FLAGS = \
	$(CPP_FLAGS) \
	$(CLANG_WARNS) \
	$(CLANG_DIAGS) \
	$(SANI_FLAGS)

all: scan-std

scan-std: scan-std.cpp

clang: scan-stdc

scan-stdc: scan-std.cpp
	clang++ $(CLANG_FLAGS) -o $@ $<

clean:
	-rm scan-std scan-stdc

check: scan-std.cpp
	cppcheck --enable=all --suppress=missingIncludeSystem $<

iwyu: scan-std.cpp
	iwyu $<

test: scan-std stl.cfg
	./scan-std -c stl.cfg scan-std.cpp

testc: scan-stdc stl.cfg
	./scan-stdc -c stl.cfg scan-std.cpp

%: %.cpp
	   g++ $(GPP_FLAGS) -o $@ $<
