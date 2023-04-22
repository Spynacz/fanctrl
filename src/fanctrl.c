#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define max(a, b) \
    ({ typeof (a) _a = (a); \
        typeof (b) _b = (b); \
     _a > _b ? a : b; })

// TODO replace defines with something more elegant

#define HWMON_BASE "/sys/class/hwmon"
#define CHIPSET_HWMON "hwmon3"

#define CPU_FAN_PWM "pwm2"
#define CHA1_FAN_PWM "pwm1"
#define CHA2_FAN_PWM "pwm4"
#define CHA3_FAN_PWM "pwm5"
#define CHA4_FAN_PWM "pwm3"

#define CPU_HWMON "hwmon2"
#define CPU_TEMP_TEMP "temp1"

const char* cpu_temp_file = HWMON_BASE "/" CPU_HWMON "/" CPU_TEMP_TEMP "_input";
const char* gpu_temp_file = "/tmp/temp-gpu";

const char* cpu_fan_pwm_file = HWMON_BASE "/" CHIPSET_HWMON "/" CPU_FAN_PWM;
const char* cha1_fan_pwm_file = HWMON_BASE "/" CHIPSET_HWMON "/" CHA1_FAN_PWM;
const char* cha2_fan_pwm_file = HWMON_BASE "/" CHIPSET_HWMON "/" CHA2_FAN_PWM;
const char* cha3_fan_pwm_file = HWMON_BASE "/" CHIPSET_HWMON "/" CHA3_FAN_PWM;
const char* cha4_fan_pwm_file = HWMON_BASE "/" CHIPSET_HWMON "/" CHA4_FAN_PWM;

int gpu_temp = -1;
int cpu_temp = -1;

int gpu_temp_hyst = 0;
int cpu_temp_hyst = 0;
int max_temp_hyst = 0;
int pwm_val = 150;

int cpu_pwm_old = -1;
int cha1_pwm_old = -1;
int cha2_pwm_old = -1;
int cha3_pwm_old = -1;
int cha4_pwm_old = -1;

int cpu_pwm = -1;
int cha1_pwm = -1;
int cha2_pwm = -1;
int cha3_pwm = -1;
int cha4_pwm = -1;

int read_number(const char* filename) {
    FILE* file = fopen(filename, "r");

    int num = -1;
    if (file) {
        fscanf(file, "%d", &num);
        fclose(file);
    }
    return num;
}

void write_number(const char* filename, int num) {
    char buf[10];
    snprintf(buf, sizeof buf, "%d", num);

    FILE* file = fopen(filename, "w");

    if (file) {
        fputs(buf, file);
        fclose(file);
    }
}

void enable_control() {
    const char* cpu_fan_pwm_enable_file = HWMON_BASE "/" CHIPSET_HWMON "/" CPU_FAN_PWM "_enable";
    const char* cha1_fan_pwm_enable_file = HWMON_BASE "/" CHIPSET_HWMON "/" CHA1_FAN_PWM "_enable";
    const char* cha2_fan_pwm_enable_file = HWMON_BASE "/" CHIPSET_HWMON "/" CHA2_FAN_PWM "_enable";
    const char* cha3_fan_pwm_enable_file = HWMON_BASE "/" CHIPSET_HWMON "/" CHA3_FAN_PWM "_enable";
    const char* cha4_fan_pwm_enable_file = HWMON_BASE "/" CHIPSET_HWMON "/" CHA4_FAN_PWM "_enable";

    write_number(cpu_fan_pwm_enable_file, 1);
    write_number(cha1_fan_pwm_enable_file, 1);
    write_number(cha2_fan_pwm_enable_file, 1);
    write_number(cha3_fan_pwm_enable_file, 1);
    write_number(cha4_fan_pwm_enable_file, 1);
}

void gather_inputs() {
    char* nvidia_temp = "nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader --id=0";

    FILE* fp;
    if ((fp = popen(nvidia_temp, "r"))) {
        fscanf(fp, "%d", &gpu_temp);
    }
    pclose(fp);

    cpu_temp = read_number(cpu_temp_file) / 1000;
}

void add_hysteresis(const int* new_temp, int* curr_temp, int hyst) {
    if (*new_temp >= *curr_temp + hyst)
        *curr_temp = *new_temp;
    else if ((*new_temp < *curr_temp - hyst) && (*new_temp > 0))
        *curr_temp = *new_temp;
}

// TODO Replace defines

#define MIN_TEMP 60
#define MAX_TEMP 85
#define ZERO_RPM 0
#define MIN_PWM 85
#define MAX_PWM 255

void temp_rule() {
    add_hysteresis(&gpu_temp, &gpu_temp_hyst, 2);
    add_hysteresis(&cpu_temp, &cpu_temp_hyst, 2);

    max_temp_hyst = max(cpu_temp_hyst, gpu_temp_hyst);

    if (max_temp_hyst <= MIN_TEMP)
        pwm_val = MIN_PWM;
    else if (max_temp_hyst >= MAX_TEMP)
        pwm_val = MAX_PWM;
    else {
        pwm_val = (int)(MIN_PWM + ((max_temp_hyst - MIN_TEMP) / (MAX_TEMP - MIN_TEMP) * (MAX_PWM - MIN_PWM)));
    }

    cha1_pwm = pwm_val;
    cha2_pwm = pwm_val;
    cha3_pwm = pwm_val;
    cha4_pwm = pwm_val;
    cpu_pwm = pwm_val;
}

void apply_pwm() {
    if (cpu_pwm != cpu_pwm_old) {
        write_number(cpu_fan_pwm_file, cpu_pwm);
        cpu_pwm_old = cpu_pwm;
    }

    if (cha1_pwm != cha1_pwm_old) {
        write_number(cha1_fan_pwm_file, cha1_pwm);
        cha1_pwm_old = cha1_pwm;
    }

    if (cha2_pwm != cha2_pwm_old) {
        write_number(cha2_fan_pwm_file, cha2_pwm);
        cha2_pwm_old = cha2_pwm;
    }

    if (cha3_pwm != cha3_pwm_old) {
        write_number(cha3_fan_pwm_file, cha3_pwm);
        cha3_pwm_old = cha3_pwm;
    }

    if (cha4_pwm != cha4_pwm_old) {
        write_number(cha4_fan_pwm_file, cha4_pwm);
        cha4_pwm_old = cha4_pwm;
    }
}

int main() {
    enable_control();
    while (1) {
        gather_inputs();
        temp_rule();
        apply_pwm();

        sleep(2);
    }
    return 0;
}
