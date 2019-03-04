macro(stringify outfiles)
	foreach(it ${ARGN})
		get_filename_component(outfile ${it} NAME)
		get_filename_component(infile ${it} ABSOLUTE)
		set(outfile "${CMAKE_CURRENT_BINARY_DIR}/${outfile}.i")
		set(${outfiles} ${${outfiles}} ${outfile})
		add_custom_command(OUTPUT ${outfile}
			COMMAND cat ${infile} | $<TARGET_FILE:bin2c> > ${outfile}
			MAIN_DEPENDENCY ${infile} VERBATIM)
	endforeach()
endmacro()
