#include "config.h"
#include <toml.hpp>
#include <iostream>
#include <string>
#include <cstring>

extern "C" {

int load_config(const char *config_path, pList *p)
{
    try {
        // Parse the TOML file
        const auto config = toml::parse(config_path);

        // I2C settings
        if (config.contains("i2c")) {
            const auto& i2c = toml::find(config, "i2c");

            if (i2c.contains("portpath")) {
                std::string path = toml::find<std::string>(i2c, "portpath");
                strncpy(p->portpath, path.c_str(), PATH_MAX - 1);
                p->portpath[PATH_MAX - 1] = '\0';
            }

            if (i2c.contains("bus_number")) {
                p->i2cBusNumber = toml::find<int>(i2c, "bus_number");
            }

            if (i2c.contains("scan_bus")) {
                p->scanI2CBUS = toml::find<bool>(i2c, "scan_bus") ? TRUE : FALSE;
            }
        }

        // Magnetometer settings
        if (config.contains("magnetometer")) {
            const auto& mag = toml::find(config, "magnetometer");

            if (mag.contains("address")) {
                p->magAddr = toml::find<int>(mag, "address");
            }

            if (mag.contains("cc_x")) {
                p->cc_x = toml::find<int>(mag, "cc_x");
            }

            if (mag.contains("cc_y")) {
                p->cc_y = toml::find<int>(mag, "cc_y");
            }

            if (mag.contains("cc_z")) {
                p->cc_z = toml::find<int>(mag, "cc_z");
            }

            if (mag.contains("gain_x")) {
                p->x_gain = toml::find<double>(mag, "gain_x");
            }

            if (mag.contains("gain_y")) {
                p->y_gain = toml::find<double>(mag, "gain_y");
            }

            if (mag.contains("gain_z")) {
                p->z_gain = toml::find<double>(mag, "gain_z");
            }

            if (mag.contains("tmrc_rate")) {
                p->TMRCRate = toml::find<int>(mag, "tmrc_rate");
            }

            if (mag.contains("nos_reg_value")) {
                p->NOSRegValue = toml::find<int>(mag, "nos_reg_value");
            }

            if (mag.contains("drdy_delay")) {
                p->DRDYdelay = toml::find<int>(mag, "drdy_delay");
            }

            if (mag.contains("sampling_mode")) {
                std::string mode = toml::find<std::string>(mag, "sampling_mode");
                if (mode == "POLL") {
                    p->samplingMode = POLL;
                } else if (mode == "CMM") {
                    p->samplingMode = CMM;
                }
            }

            if (mag.contains("cmm_sample_rate")) {
                p->CMMSampleRate = toml::find<int>(mag, "cmm_sample_rate");
            }

            if (mag.contains("readback_cc_regs")) {
                p->readBackCCRegs = toml::find<bool>(mag, "readback_cc_regs") ? TRUE : FALSE;
            }
        }

        // Temperature settings
        if (config.contains("temperature")) {
            const auto& temp = toml::find(config, "temperature");

            if (temp.contains("remote_temp_address")) {
                p->remoteTempAddr = toml::find<int>(temp, "remote_temp_address");
            }
        }

        // Output settings
        if (config.contains("output")) {
            const auto& output = toml::find(config, "output");

            if (output.contains("use_pipes")) {
                p->usePipes = toml::find<bool>(output, "use_pipes") ? TRUE : FALSE;
            }

            if (output.contains("pipe_in_path")) {
                std::string path = toml::find<std::string>(output, "pipe_in_path");
                strncpy((char*)p->pipeInPath, path.c_str(), PATH_MAX - 1);
                ((char*)p->pipeInPath)[PATH_MAX - 1] = '\0';
            }

            if (output.contains("pipe_out_path")) {
                std::string path = toml::find<std::string>(output, "pipe_out_path");
                strncpy((char*)p->pipeOutPath, path.c_str(), PATH_MAX - 1);
                ((char*)p->pipeOutPath)[PATH_MAX - 1] = '\0';
            }
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return -1;
    }
}

} // extern "C"