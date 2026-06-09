# HW16 — STM32 电流控制 main.c 代码块

把下面每段贴进 `main.c` 里对应的 `/* USER CODE BEGIN xxx */ ... /* USER CODE END xxx */` 区域内。
句柄假设：`hadc1`、`hi2c1`、`htim1`(PA8=CH1, PA1=CH2)、`htim2`(1kHz 中断)、串口 `hcom_uart[COM1]`。
ADC 12 位 (0~4095)。

---

## 1) USER CODE BEGIN Includes
```c
#include <stdio.h>
#include <string.h>
```

## 2) USER CODE BEGIN PD  (宏定义)
```c
#define INA219_ADDR             0x40
#define INA219_REG_CONFIG       0x00
#define INA219_REG_CURRENT      0x04
#define INA219_REG_CALIBRATION  0x05

#define NSAMP   400          // 采集点数
#define PWM_MAX 2400         // = TIM1 Counter Period (100% 占空比)
```

## 3) USER CODE BEGIN PV  (全局变量)
```c
// 控制状态
volatile int   state   = 0;      // 0=空闲, 1=运行
volatile int   counter = 0;      // 中断里的循环计数
volatile float desired = 200.0f; // 目标电流 mA (会来回翻转)
float eint = 0.0f;               // 误差积分

// PI 增益 —— 先随便给，靠 python 画图来调
float kp = 2.0f;
float ki = 0.1f;

// 数据记录 (主循环里打印出来)
volatile float desired_log[NSAMP];
volatile float actual_log[NSAMP];
```

## 4) USER CODE BEGIN 0  (函数：放在 main 之前)
```c
// ---------- INA219 底层读写 ----------
void writeINA219(int reg, int value){
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = value >> 8;
    buf[2] = value & 0xff;
    HAL_I2C_Master_Transmit(&hi2c1, INA219_ADDR << 1, buf, 3, 10);
}

signed short readINA219(unsigned char reg){
    HAL_I2C_Master_Transmit(&hi2c1, INA219_ADDR << 1, &reg, 1, 10);
    uint8_t buffer[2];
    HAL_I2C_Master_Receive(&hi2c1, INA219_ADDR << 1, buffer, 2, 10);
    signed short value = (buffer[0] << 8) | buffer[1];
    return value;
}

void init_ina219(void){
    unsigned short cal    = 1024;
    unsigned short config = 0b0011000010001111;
    writeINA219(INA219_REG_CALIBRATION, cal);
    writeINA219(INA219_REG_CONFIG, config);
}

// 返回电流, 单位 mA
float read_ina219(void){
    signed short value = readINA219(INA219_REG_CURRENT);
    return value / 3.0f;
}

// ---------- ADC (电位器位置) ----------
uint32_t read_adc(void){
    uint32_t raw = 0;
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK){
        raw = HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);
    return raw;
}

// ---------- 电机控制 ----------
void motor_off(void){
    // 两个输入都拉高 = 电机关
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_MAX);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, PWM_MAX);
}

// u: 控制量, 正负代表方向; 绝对值越大转越快
void set_motor(float u){
    if (u >  PWM_MAX) u =  PWM_MAX;
    if (u < -PWM_MAX) u = -PWM_MAX;
    if (u >= 0){
        // 一路 100%, 另一路 = MAX - 速度 (数越小越快)
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, PWM_MAX);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_MAX - (uint32_t)u);
    } else {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_MAX);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, PWM_MAX - (uint32_t)(-u));
    }
}
```

## 5) USER CODE BEGIN 4  (中断回调：放在文件末尾的 user 区)
```c
// 1kHz 定时器中断
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    if (htim == &htim2){
        uint32_t pos = read_adc();

        // ---- 安全急停: 电位器超范围就关电机 ----
        if (pos < 250 || pos > (4095 - 250)){
            motor_off();
            return;
        }

        if (state == 1){
            float actual = read_ina219();
            float error  = desired - actual;
            eint += error;
            float u = kp * error + ki * eint;
            set_motor(u);

            // 记录数据
            if (counter < NSAMP){
                desired_log[counter] = desired;
                actual_log[counter]  = actual;
            }
            counter++;

            // 每 100 个周期翻转目标电流 -> 方波
            if ((counter % 100) == 0){
                desired = -desired;
            }

            // 400 个周期后停止
            if (counter >= NSAMP){
                motor_off();
                state   = 0;
                counter = 0;
                eint    = 0.0f;
            }
        }
    }
}
```

## 6) USER CODE BEGIN 2  (main 里, 进 while(1) 之前)
```c
init_ina219();

// 启动 PWM (先关电机)
motor_off();
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

// 启动 1kHz 中断
HAL_TIM_Base_Start_IT(&htim2);

printf("HW16 ready. Send 'a' to run.\r\n");
```

## 7) USER CODE BEGIN 3  (while(1) 循环体内)
```c
char c = 0;
if (HAL_UART_Receive(&hcom_uart[COM1], (uint8_t *)&c, 1, 10) == HAL_OK){
    if (c == 'a'){
        eint    = 0.0f;
        counter = 0;
        desired = 200.0f;
        state   = 1;

        while (state == 1){ }      // 等中断跑完 400 周期

        // 打印: index  desired(mA)  actual(mA)   (printf 用 int 避免 float 问题)
        for (int i = 0; i < NSAMP; i++){
            printf("%d %d %d\r\n", i, (int)desired_log[i], (int)actual_log[i]);
        }
        printf("done\r\n");
    }
}
```

---

## 调参说明
- 先编译烧录，串口监视器手动发 `a`，看是否打印 400 行数据。
- 用 `plot_current.py` 自动发 `a`、收数据、画图。
- 看图调 `kp`、`ki`：
  - 实际电流跟不上 / 太慢 → 加大 `kp`
  - 有稳态误差(跟不到目标线) → 加大 `ki`
  - 抖动 / 超调震荡 → 减小 `kp` 或 `ki`
- 改完增益重新编译烧录再测。调到方波跟得又快又稳即可截图。

## 注意
- 目标电流 200mA、安全阈值 250 都可按你的电机实际情况改。
- 若电机一发 `a` 就乱冲，先把 `kp`、`ki` 调小，并注意夹手。
