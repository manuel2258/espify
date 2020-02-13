idf_component_register(SRCS "main.cpp" "src/network/HttpsRequester.cpp" "src/network/HttpData.cpp"
                    INCLUDE_DIRS ""
                    EMBED_TXTFILES server_root_cert.pem)