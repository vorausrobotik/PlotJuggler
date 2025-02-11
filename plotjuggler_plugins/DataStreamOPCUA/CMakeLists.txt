if(BUILDING_WITH_VCPKG)
    message(STATUS "DataStreamOPCUA: Finding [open62541] with vcpkg")
elseif(BUILDING_WITH_CONAN)
    message(STATUS "DataStreamOPCUA: Finding [open62541] with conan")
else()
    message(STATUS "DataStreamOPCUA: Finding [open62541] without package managers")
endif()

find_package(open62541 QUIET)

if(open62541_FOUND)
    message(STATUS "DataStreamOPCUA: [open62541] found.")
    add_definitions(${QT_DEFINITIONS})
    add_definitions(-DQT_PLUGIN)

    file(GLOB SRC "*.cpp")

    add_library(DataStreamOPCUA SHARED ${SRC} ${UI_SRC}  )

    target_link_libraries(DataStreamOPCUA 
        ${Qt5Widgets_LIBRARIES} 
        ${Qt5Xml_LIBRARIES}
        plotjuggler_base
        open62541::open62541
        nlohmann_json
        )


    install(TARGETS DataStreamOPCUA DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
else()
    message("DataStreamOPCUA: [open62541] not found. Skipping plugin DataStreamOPCUA.")
endif()
