add_subdirectory(mediaServer)
set_property(TARGET mediaServer PROPERTY FOLDER "App")

add_subdirectory(vedioServer) 
set_property(TARGET vedioServer PROPERTY FOLDER "App")

add_subdirectory(rtspserver) 
set_property(TARGET rtspserver PROPERTY FOLDER "App")

add_subdirectory(server2) 
set_property(TARGET server2 PROPERTY FOLDER "App")

add_subdirectory(ffmpegstreamer) 
set_property(TARGET ffmpegstreamer PROPERTY FOLDER "App")

add_subdirectory(clience) 
set_property(TARGET clience PROPERTY FOLDER "App")





