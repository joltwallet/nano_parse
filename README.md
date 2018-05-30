# Nano Parse
Library for crafting `rai_node` rpc calls and parsing their responses to be compatible with [`nano_lib`](https://github.com/joltwallet/nano_lib).

# Design
This library was split into two main categories:
1) Parsing json strings using cJSON into a more usable format
2) Helper websocket calls that wrap the json parsers

If you are developing a standalone device/application, the websocket calls will probably be more friendly to quickly get your project running. The project can be built without any libwebsocket dependencies by disabling it in the Kconfig.

# Unit Tests
Unit tests can be used by selecting this library with a target using the [ESP32 Unit Tester](https://github.com/BrianPugh/esp32_unit_tester).

```
make flash TEST_COMPONENTS='nano_parse' monitor
```

The unit tests (in the `test` folder) is a good source of examples on how to use this library. Be careful about how strings are escaped.
