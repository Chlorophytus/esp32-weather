idf_component_register(
    SRCS "weather_trace.cpp" "weather_logging.cpp" "weather_gps.cpp" "weather.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_uart
)