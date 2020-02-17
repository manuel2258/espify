idf_component_register(SRCS "main.cpp" "src/network/https_requester.cpp" "src/network/http_types.cpp"
                INCLUDE_DIRS ""
                EMBED_TXTFILES server_root_cert.pem)