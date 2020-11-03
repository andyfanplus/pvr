CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(ParallelRendering)

FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})

FIND_PACKAGE(IceT REQUIRED)
INCLUDE_DIRECTORIES(${ICET_INCLUDE_DIRS})

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	PVVTKExtensionsRendering
	PVClientServerCoreRendering
)

ADD_SUBDIRECTORY(PVClientServerCoreRendering)
ADD_SUBDIRECTORY(PVVTKExtensionsRendering)

ADD_DEPENDENCIES(PVClientServerCoreRendering PVVTKExtensionsRendering)

FILE(COPY "comb.vtk" DESTINATION ${PROJECT_BINARY_DIR})

ADD_EXECUTABLE(client client.cpp)
TARGET_LINK_LIBRARIES(client 
	${VTK_LIBARIES}
	PVClientServerCoreRendering
	PVVTKExtensionsRendering
	)

ADD_EXECUTABLE(server server.cpp)
TARGET_LINK_LIBRARIES(server 
	${VTK_LIBARIES}
	PVClientServerCoreRendering
	PVVTKExtensionsRendering
	)