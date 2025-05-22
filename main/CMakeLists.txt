idf_component_register(SRCS "main.cc"
                            "logger.cc"
                            "file_system.cc"
                            "gpio_control.cc"
                            "task.cc"
                            "pwm.cc"
                            "util.cc"
                            "wifi_manager.cc"
                            "mqtt.cc"
                            "mqtt_publish_status_task.cc"
                            "irrigation_controller.cc"
                            "http_request.cc"
                            "httpd_server_task.cc"
                            "management_task.cc"
                            "valve_task.cc"
                            "voltage_check_task.cc"
                            "watering_button_task.cc"
                            "schedule_base.cc"
                            "schedule_manager.cc"
                            "schedule_dummy.cc"
                            "schedule_adjust.cc"
                            "schedule_watering.cc"
                            "schedule_manual.cc"
                            "watering_record.cc"
                            "watering_setting.cc"
                            "weather_forecast.cc"
                            "water_level_check_task.cc"
                            "water_flow_sensor.cc"
                    INCLUDE_DIRS "")


component_compile_options(-Wno-error=format= -Wno-format)




