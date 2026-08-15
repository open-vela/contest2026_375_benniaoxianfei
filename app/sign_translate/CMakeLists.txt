# sign_translate 应用构建（OpenVela / NuttX apps 规范）
# 部署：拷贝到 apps/examples/sign_translate/ 后由构建系统自动发现

if(CONFIG_EXAMPLES_SIGN_TRANSLATE)

  # TFLite Micro 需要 C++17
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  # 应用源文件
  set(SRCS
    sign_translate_main.cxx
    ui.cxx
    recognition.cxx
    camera.cxx
    audio.cxx
    upload.cxx
    model_data.cc
  )

  # 板端 LVGL 头文件（由 CONFIG_GRAPHICS_LVGL 提供）
  # 按需调整路径：OpenVela 的 lvgl 在 apps/graphics/lvgl
  set(INCS
    ${CMAKE_CURRENT_LIST_DIR}
  )

  # TFLite Micro 库（按 OpenVela edge_ai 文档接入后放开）
  # set(TFLM_INC_DIR ...)
  # set(TFLM_LIB ...)

  if(CONFIG_EXAMPLES_SIGN_TRANSLATE_PROGNAME)
    set(PROGNAME ${CONFIG_EXAMPLES_SIGN_TRANSLATE_PROGNAME})
  else()
    set(PROGNAME sign_translate)
  endif()

  if(CONFIG_EXAMPLES_SIGN_TRANSLATE_PRIORITY)
    set(PRIORITY ${CONFIG_EXAMPLES_SIGN_TRANSLATE_PRIORITY})
  else()
    set(PRIORITY SCHED_PRIORITY_DEFAULT)
  endif()

  if(CONFIG_EXAMPLES_SIGN_TRANSLATE_STACKSIZE)
    set(STACKSIZE ${CONFIG_EXAMPLES_SIGN_TRANSLATE_STACKSIZE})
  else()
    set(STACKSIZE 16384)
  endif()

  nuttx_add_application(
    NAME          ${PROGNAME}
    SRCS          ${SRCS}
    INCS          ${INCS}
    STACKSIZE     ${STACKSIZE}
    PRIORITY      ${PRIORITY}
    MODULE        ${CONFIG_EXAMPLES_SIGN_TRANSLATE}
  )

  # 链接 TFLite Micro（接入后放开）
  # target_link_libraries(sign_translate PRIVATE tflm)

endif()
