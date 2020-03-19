idf_component_register(
SRCS  "main.cpp"
    "src/network/https_requester.cpp" 
    "src/network/http_types.cpp" "src/spotify/spotify_manager.cpp" 
    "src/io/io_types.cpp" "src/io/io_manager.cpp"
    "src/graphics/graphics_manager.cpp" "src/graphics/graphics_types.cpp"
EMBED_TXTFILES
    server_root_cert.pem)

spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)