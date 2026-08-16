# ##############################################################################
# apps/examples/sign_translate/CMakeLists.txt
#
# Sign Language Translate Terminal (OpenVela / NuttX apps 规范)
#
# 两种构建模式：
#   - 板端（默认）：CONFIG_GRAPHICS_LVGL + CONFIG_TFLITEMICRO
#     (依赖 SYSTEM_FLATBUFFERS, MATH_GEMMLOWP, MATH_KISSFFT, MATH_RUY)
#   - 模拟器 SIM：CONFIG_EXAMPLES_SIGN_TRANSLATE_SIM=y（无 TFLM，识别为假数据）
#
# 部署：拷贝本目录到 apps/examples/sign_translate/ 后由构建系统自动发现
# ##############################################################################

if(CONFIG_EXAMPLES_SIGN_TRANSLATE)

  set(SRCS
    sign_translate_main.cxx
    ui.cxx
    recognition.cxx
    camera.cxx
    audio.cxx
    upload.cxx)

  set(APP_DEPENDS lvgl)

  if(CONFIG_EXAMPLES_SIGN_TRANSLATE_SIM)
    # 模拟模式：不依赖 TFLM，不需要模型数组
  else()
    list(APPEND SRCS model_data.cc)
    list(APPEND APP_DEPENDS tflite_micro)
  endif()

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
    ${APP_DEPENDS}
    DEFINITIONS
    TFLITE_WITH_STABLE_ABI=0
    TFLITE_USE_OPAQUE_DELEGATE=0
    TFLITE_SINGLE_ROUNDING=0
    TF_LITE_STRIP_ERROR_STRINGS
    TF_LITE_STATIC_MEMORY
    SRCS
    ${SRCS})

endif()
