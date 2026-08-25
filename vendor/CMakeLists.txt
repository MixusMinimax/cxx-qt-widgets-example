add_library(qcustomplot qcustomplot.cpp)

target_include_directories(qcustomplot PUBLIC .)

target_link_libraries(qcustomplot Qt6::Core Qt6::Widgets)