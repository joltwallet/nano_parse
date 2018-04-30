rm lws_test
gcc ../src/cJSON.c ../components/nano_lws/src/nano_lws.c ../src/nano_parse.c translation.c lws_test.c -o lws_test -I../test -lwebsockets -lmbedtls -lmbedx509 -lmbedcrypto -lsodium
