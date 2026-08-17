############################################################################
# apps/examples/sign_translate/Makefile
#
# NuttX/OpenVela Make 体系构建（与 CMakeLists.txt 二选一：
# - Make 体系：configure.sh + make（P4 板端当前用这个）
# - CMake 体系：build.sh --cmake（goldfish 模拟器用这个）
############################################################################

include $(APPDIR)/Make.defs

PROGNAME  = sign_translate
PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = 16384
MODULE    = $(CONFIG_EXAMPLES_SIGN_TRANSLATE)

MAINSRC = sign_translate_main.cxx
CXXSRCS = ui.cxx recognition.cxx camera.cxx audio.cxx upload.cxx

# TFLM 模式（板端接入后放开，需要 TFLM 库链接）
# ifeq ($(CONFIG_EXAMPLES_SIGN_TRANSLATE_SIM),)
# CXXSRCS += model_data.cc
# LDLIBS  += $(wildcard $(APPDIR)/staging/*.a)   # 按实际 TFLM 库调整
# endif

include $(APPDIR)/Application.mk
