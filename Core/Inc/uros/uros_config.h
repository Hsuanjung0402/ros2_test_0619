/*
 * uros_config.h
 *
 *  Created on: Jun 19, 2026
 *      Author: hsuanjung
 */

#ifndef INC_UROS_UROS_CONFIG_H_  //"ifndef": if not defined. If the macro is defined, it will jump to #endif
#define INC_UROS_UROS_CONFIG_H_

// micro-ROS configuration
#define NODE_NAME "stm32_topic_test_node"
#define DOMAIN_ID 0
#define FREQUENCY 100
#define USARTx huart2

// Motor control configuration
#define ENABLE_MOTOR_CONTROL 0  // 設為 0 可禁用 motor 功能進行測試

#endif /* INC_UROS_UROS_CONFIG_H_ */
