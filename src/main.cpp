#include <Arduino.h>
#include <micro_ros_platformio.h>
#include <WiFi.h> // 加入 WiFi 庫
#include <ESP32Ping.h>  // ESP32 需要這個庫來執行 Ping

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>

// ROS 相關變數
rcl_publisher_t publisher;
std_msgs__msg__Int32 msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rcl_timer_t timer;

// Micro-ROS Agent 設定
IPAddress agent_ip(192,168,0,116);
size_t agent_port = 8090;

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}

// Error handle loop
void error_loop() {
  while(1) {
    delay(100);
  }
}

// WiFi 連線
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin("screamlab", "s741852scream");

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("Connected to: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Ping Micro-ROS Agent
void pingMicroRosAgent() {
  Serial.print("Pinging Micro-ROS Agent at ");
  Serial.println(agent_ip);

  if (Ping.ping(agent_ip)) {
    Serial.println("✅ Ping successful! Micro-ROS Agent is reachable.");
  } else {
    Serial.println("❌ Ping failed! Check network connection.");
  }
}

// ROS Timer Callback
void timer_callback(rcl_timer_t * timer, int64_t last_call_time) {
  RCLC_UNUSED(last_call_time);
  if (timer != NULL) {
    RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL));
    msg.data++;
  }
}

void setup() {
  // 設定 Serial 輸出
  Serial.begin(115200);

  // 連接 WiFi
  connectToWiFi();

  // 設定 Micro-ROS WiFi 傳輸
  set_microros_wifi_transports("screamlab", "s741852scream", agent_ip, agent_port);
  delay(2000);

  // 執行 Ping 確認 Agent 是否可達
  pingMicroRosAgent();

  // 初始化 Micro-ROS
  allocator = rcl_get_default_allocator();

  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
  RCCHECK(rclc_node_init_default(&node, "micro_ros_platformio_node", "", &support));
  RCCHECK(rclc_publisher_init_default(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "micro_ros_platformio_node_publisher"));

  const unsigned int timer_timeout = 1000;
  RCCHECK(rclc_timer_init_default(
    &timer,
    &support,
    RCL_MS_TO_NS(timer_timeout),
    timer_callback));

  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));

  msg.data = 0;
}

void loop() {
  delay(100);
  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));

  Serial.println("hello");

  // 每 5 秒 Ping 一次 Micro-ROS Agent
  static unsigned long lastPingTime = 0;
  if (millis() - lastPingTime > 5000) {
    pingMicroRosAgent();
    lastPingTime = millis();
  }
}
