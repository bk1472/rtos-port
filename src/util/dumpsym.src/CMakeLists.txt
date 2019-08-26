cmake_minimum_required(VERSION 3.1)

set ( DUMP_SYM_SRC_FILES
		./main.c
		./elf.c
		./process.c
		./process32.c
		./process64.c
		./addr2line.c
		./decode_frame.c
		./dwarf.c
		./hist.c
		./util.c
	)

set ( dump_inc_paths
		./include
		.
	)
INCLUDE_DIRECTORIES( ${dump_inc_paths} )

add_definitions(-DUNIX)
add_definitions(-DMKSYM)
add_definitions(-DBIT_WIDTH=64)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wno-format -Wno-unused-result -Wno-incompatible-pointer-types -Wno-pointer-to-int-cast" )
set (CMAKE_LINK_FLAGS "-lz")

add_executable (dumpsym ${DUMP_SYM_SRC_FILES} )
