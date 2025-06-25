cmake_minimum_required(VERSION 3.11)

project(LoadRunner)

add_executable(${PROJECT_NAME} context.c)


target_link_libraries(${PROJECT_NAME} PRIVATE
    pspdebug
    pspdisplay
    pspge
    pspgu
    pspgum
)

# Create an EBOOT.PBP file
create_pbp_file(
    TARGET ${PROJECT_NAME}
    ICON_PATH NULL
    BACKGROUND_PATH NULL
    PREVIEW_PATH NULL
    TITLE ${PROJECT_NAME}
    VERSION 01.00
)