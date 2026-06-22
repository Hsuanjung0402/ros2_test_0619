/*
 * uros_init.cpp
 *
 *  Created on: Jun 19, 2026
 *      Author: hsuanjung
 */


#include "uros_init.h"
#include <math.h>
#include <string.h>
#include <rmw_microros/time_sync.h>

rcl_publisher_t           topic_test_pub;
std_msgs__msg__Int32      topic_test_msg;
rcl_timer_t test_pub_timer;

rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rcl_init_options_t init_options;
rclc_executor_t executor;

agent_status_t status = AGENT_WAITING;

int ping_fail_count = 0;
#define MAX_PING_FAIL_COUNT 5

int b = 0;

extern UART_HandleTypeDef USARTx;
extern UART_HandleTypeDef huart2;

void uros_init(void) {
  // Initialize micro-ROS
  rmw_uros_set_custom_transport(
    true,
    (void *) &huart2,
    cubemx_transport_open,
    cubemx_transport_close,
    cubemx_transport_write,
    cubemx_transport_read);

  rcl_allocator_t freeRTOS_allocator = rcutils_get_zero_initialized_allocator();

  freeRTOS_allocator.allocate = microros_allocate;
  freeRTOS_allocator.deallocate = microros_deallocate;
  freeRTOS_allocator.reallocate = microros_reallocate;
  freeRTOS_allocator.zero_allocate =  microros_zero_allocate;

  if (!rcutils_set_default_allocator(&freeRTOS_allocator)) {
  printf("Error on default allocators (line %d)\n", __LINE__);
  }
}

void uros_agent_status_check(void) {
	b = 1;
  switch (status) {
    case AGENT_WAITING:
      handle_state_agent_waiting();
      break;
    case AGENT_AVAILABLE:
      handle_state_agent_available();
      break;
    case AGENT_CONNECTED:
      handle_state_agent_connected();
      break;
    case AGENT_TRYING:
      handle_state_agent_trying();
      break;
    case AGENT_DISCONNECTED:
      handle_state_agent_disconnected();
      break;
    default:
      break;
  }
}

void handle_state_agent_waiting(void) {
  status = (rmw_uros_ping_agent(100, 10) == RMW_RET_OK) ? AGENT_AVAILABLE : AGENT_WAITING;
}

void handle_state_agent_available(void) {
  uros_create_entities();
  status = AGENT_CONNECTED;
}

void handle_state_agent_connected(void) {
  if(rmw_uros_ping_agent(20, 5) == RMW_RET_OK){
    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
    ping_fail_count = 0; // Reset ping fail count
  } else {
    ping_fail_count++;
    if(ping_fail_count >= MAX_PING_FAIL_COUNT){
      status = AGENT_TRYING;
      ping_fail_count = 0;
    }
  }
}

void handle_state_agent_trying(void) {
  if(rmw_uros_ping_agent(50, 10) == RMW_RET_OK){
    status = AGENT_CONNECTED;
    ping_fail_count = 0; // Reset ping fail count
  } else {
    ping_fail_count++;
    if(ping_fail_count >= MAX_PING_FAIL_COUNT){
      status = AGENT_DISCONNECTED;
      ping_fail_count = 0;
    }
  }
}

void handle_state_agent_disconnected(void) {
  uros_destroy_entities();
  status = AGENT_WAITING;
}


void uros_create_entities(void) {
  allocator = rcl_get_default_allocator();






  rmw_uros_set_custom_transport(true, (void *) &huart2, cubemx_transport_open, cubemx_transport_close, cubemx_transport_write, cubemx_transport_read);

  // ----- 加入這段 Ping 迴圈 -----
  // 每 100 毫秒 Ping 一次，總共嘗試直到成功為止
  while(rmw_uros_ping_agent(100, 1) != RMW_RET_OK) {
      // 可以在這裡閃爍一下 LED，代表正在等待連線
      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
      osDelay(100);
  }
  // 走到這裡代表確定與 Docker Agent 連線成功！LED 恆亮
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
  // -----------------------------






  init_options = rcl_get_zero_initialized_init_options();
  rcl_init_options_init(&init_options, allocator);
  rcl_init_options_set_domain_id(&init_options, DOMAIN_ID);

  rclc_support_init_with_options(&support, 0, NULL, &init_options, &allocator); // Initialize support structure

  rcl_init_options_fini(&init_options);

  rclc_node_init_default(&node, NODE_NAME, "", &support);                       // Initialize node

  rclc_publisher_init_default(                                                  // Initialize publisher for pose
    &topic_test_pub,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "robot/topic_test");
  topic_test_msg.data = 0;

  rmw_uros_set_publisher_session_timeout(                                       // Set session timeout for publisher
    rcl_publisher_get_rmw_handle(&topic_test_pub),
    10);

  rclc_timer_init_default(&test_pub_timer, &support, RCL_MS_TO_NS(1000), test_pub_timer_cb);

  rclc_executor_init(&executor, &support.context, 1, &allocator); // Create executor (1 timer + 2 subscriptions)

  rclc_executor_add_timer(&executor, &test_pub_timer); // Add timer to executor
}

void uros_destroy_entities(void) {
  rmw_context_t* rmw_context = rcl_context_get_rmw_context(&support.context);
  (void) rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);

  // Destroy publisher
  rcl_publisher_fini(&topic_test_pub, &node);

  // Destroy subscriber

  rcl_timer_fini(&test_pub_timer);

  // Destroy executor
  rclc_executor_fini(&executor);

  // Destroy node
  rcl_node_fini(&node);
  rclc_support_fini(&support);
}

void test_pub_timer_cb(rcl_timer_t * timer, int64_t last_call_time){
	rcl_ret_t ret = rcl_publish(&topic_test_pub, &topic_test_msg, NULL);
	if (ret != RCL_RET_OK) {
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,GPIO_PIN_SET);
	}else{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,GPIO_PIN_RESET);
	}
	topic_test_msg.data++;
}

