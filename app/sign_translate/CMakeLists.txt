# ##############################################################################
# apps/examples/sign_translate/CMakeLists.txt
#
# Sign Language Translate Terminal (OpenVela / NuttX apps 规范)
#
# 依赖：
#   - LVGL：          CONFIG_GRAPHICS_LVGL + CONFIG_LV_USE_NUTTX
#   - TFLite Micro：  CONFIG_TFLITEMICRO (+ SYSTEM_FLATBUFFERS, MATH_GEMMLOWP,
#                     MATH_KISSFFT, MATH_RUY)
# 部署：拷贝本目录到 apps/examples/sign_translate/ 后由构建系统自动发现
# ##############################################################################

if(CONFIG_EXAMPLES_SIGN_TRANSLATE)

  set(SRCS
    sign_translate_main.cxx
    ui.cxx
    recognition.cxx
    camera.cxx
    audio.cxx
    upload.cxx
    model_data.cc)

  nuttx_add_application(
    NAME
    sign_translate
    PRIORITY
    ${CONFIG_EXAMPLES_SIGN_TRANSLATE_PRIORITY}
    STACKSIZE
    ${CONFIG_EXAMPLES_SIGN_TRANSLATE_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_SIGN_TRANSLATE}
    DEPENDS
    lvgl
    tflite_micro
    DEFINITIONS
    TFLITE_WITH_STABLE_ABI=0
    TFLITE_USE_OPAQUE_DELEGATE=0
    TFLITE_SINGLE_ROUNDING=0
    TF_LITE_STRIP_ERROR_STRINGS
    TF_LITE_STATIC_MEMORY
    SRCS
    ${SRCS})

endif()
