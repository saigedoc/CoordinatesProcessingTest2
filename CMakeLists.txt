cmake_minimum_required(VERSION 3.30)
project(http_servis LANGUAGES CXX)

add_executable(app src/main.cpp)

find_package(boost REQUIRED)

# Связываем серверное приложение с библиотекой
#target_link_libraries(server_app PRIVATE server_lib)
target_link_libraries(app PRIVATE cppzmq cppzmq-static)
# Установка имени выходного файла (опционально)
#set_target_properties(server_app PROPERTIES OUTPUT_NAME "server")
# Добавляем путь к заголовкам сервера
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)