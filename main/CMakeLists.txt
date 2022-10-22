idf_component_register(SRCS "main.cpp"
                            "logger.cpp"
                            "file_system.cpp"
                            "gpio_control.cpp"
                            "system_queue.cpp"
                            "task.cpp"
                            "util.cpp"
                            "wifi_manager.cpp"
                            "irrigation_controller.cpp"
                            "http_request.cpp"
                            "httpd_server_task.cpp"
                            "management_task.cpp"
                            "valve_task.cpp"
                            "voltage_check_task.cpp"
                            "watering_button_task.cpp"
                            "schedule_base.cpp"
                            "schedule_manager.cpp"
                            "schedule_dummy.cpp"
                            "schedule_adjust.cpp"
                            "schedule_watering.cpp"
                            "watering_record.cpp"
                            "watering_setting.cpp"
                            "weather_forecast.cpp"
                    INCLUDE_DIRS "")


component_compile_options(-Wno-error=format= -Wno-format)




