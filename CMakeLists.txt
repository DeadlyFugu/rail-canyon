cmake_minimum_required( VERSION 3.0 )
project( railcanyon )

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -std=c++11")
endif()

set(BIGG_EXAMPLES OFF CACHE BOOL "Build examples.")
add_subdirectory( extern/bigg )
add_subdirectory( extern/rwstreamlib )

set(RAILCANYON_CC
		# util
		src/util/buffer.cc
		src/util/whereami.c
		src/util/fsfile.cc
		src/util/fspath.cc
		src/util/log.cc
		src/util/prs.cc
		src/util/config.cc

		# io
		src/io/ONEArchive.cc

		# render
		src/render/BSPModel.cc
		src/render/Camera.cc
		src/render/TexDictionary.cc
		src/render/TXCAnimation.cc

		# main
		src/railcanyon.cc
		src/stage.cc
)
set(RAILCANYON_HH
		# util
		src/util/buffer.hh
		src/util/whereami.h
		src/util/fsfile.hh
		src/util/fspath.hh
		src/util/log.hh
		src/util/prs.hh
		src/util/types.hh
		src/util/config.hh

		# io
		src/io/ONEArchive.hh

		# render
		src/render/BSPModel.hh
		src/render/Camera.hh
		src/render/TexDictionary.hh
		src/render/TXCAnimation.hh

		# main
		src/common.hh
		src/stage.hh
)
set(RAILCANYON_SC
		# bspmesh
		src/shaders/vs_bspmesh.sc
		src/shaders/fs_bspmesh.sc
)

add_executable( railcanyon ${RAILCANYON_CC} ${RAILCANYON_HH} ${RAILCANYON_SC} )
target_link_libraries( railcanyon PUBLIC bigg rwstream )
target_include_directories( railcanyon PUBLIC extern/rwstreamlib/include src )

add_shader( src/shaders/vs_bspmesh.sc VERTEX   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders DX11_MODEL 5_0 GLSL 130 )
add_shader( src/shaders/fs_bspmesh.sc FRAGMENT OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders DX11_MODEL 5_0 GLSL 130 )

configure_debugging( railcanyon WORKING_DIR ${CMAKE_CURRENT_BINARY_DIR} )
