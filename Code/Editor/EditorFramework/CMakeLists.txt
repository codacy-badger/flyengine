pl_cmake_init()

pl_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(LIBRARY ${PROJECT_NAME})

pl_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Svg)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ToolsFoundation
  GuiFoundation
  EditorEngineProcessFramework
)
