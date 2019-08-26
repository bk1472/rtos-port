SET ( CMAKE_BUILD_TYPE Rlease )
SET ( OUTPUT_ELF "../../ccdv" )

SET (CMAKE_C_COMPILER "gcc" )
SET ( SRC_FILES
		ccdv.c
		sift-warn.c
   	)

ADD_COMPILE_OPTIONS ( -Wno-format-security -Wno-format )
SET ( CMAKE_EXE_LINKER_FLAGS "-static" )

ADD_EXECUTABLE( ${OUTPUT_ELF} ${SRC_FILES} )

ADD_CUSTOM_COMMAND (
		TARGET ${OUTPUT_ELF}
		POST_BUILD
		COMMENT "Strip Target"
		COMMAND strip ${OUTPUT_ELF}
		#		COMMAND mv ${OUTPUT_ELF} ../..
	)
