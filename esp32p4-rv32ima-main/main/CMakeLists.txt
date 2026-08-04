idf_component_register(SRCS "mini-rv32ima.c"
			"cache.c"
			"port-esp.c"
			#"image.S"
		       LDFRAGMENTS "link.lf"
                       INCLUDE_DIRS ".")
