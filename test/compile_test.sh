rm lws_test
gcc ../src/cJSON.c ../components/nano_lws/src/websocket_cl.c ../src/nano_parse.c lws_test.c -o lws_test -lwebsockets -lmbedtls -lmbedx509 -lmbedcrypto -lsodium
