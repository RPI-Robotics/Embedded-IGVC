#include <FastLED.h>
#include <micro_ros_arduino.h>

#include <stdio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/Bool.h>

rcl_subscription_t subscriber;
std_msgs__msg__Bool msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rcl_timer_t timer;

#define ONBOARD_LED 2
#define LED_PIN 3
bool blinking=false; //could be an enum
bool blink_on=false;
#define NUM_LEDS 60
CRGB leds[NUM_LEDS];

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}

void error_loop(){
  while(1){
    digitalWrite(ONBOARD_LED, HIGH);  
    delay(200);                  
    digitalWrite(ONBOARD_LED, LOW);  
    delay(200);                  
  }
}

void change_leds(bool state){
  if(state){
    for(int i=0;i<NUM_LEDS;i++){
      leds[i]=CRGB::Green;
    }
  } else {
    FastLED.clear();
  }
  FastLED.show();
}

//use blink boolean to tell if we should blink or not in the timer callback
void subscription_callback(const void * msgin){  
  blinking = static_cast<const std_msgs__msg__Bool*>(msgin)->data; 
}
void timer_callback(rcl_timer_t * timer, int64_t last_call_time){
  if (blinking){
    change_leds(blink_on);
    blink_on = !blink_on;
  } else {
    change_leds(true);
  }
}

void setup() {
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, LOW);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);

  //start yellow, make them green on first write to subscription
  for (int i =0; i<NUM_LEDS;i++){
    leds[i]=CRGB::Yellow;
  }
  FastLED.show();

  set_microros_transports();
   
  delay(2000);

  allocator = rcl_get_default_allocator();

  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
  RCCHECK(rclc_node_init_default(&node, "micro_ros_led", "", &support));
  
  RCCHECK(rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool),
    "/enabled"));
  
  const unsigned int timer_timeout = 250;
  RCCHECK(rclc_timer_init_default(
    &timer,
    &support,
    RCL_MS_TO_NS(timer_timeout),
    timer_callback));

  RCCHECK(rclc_executor_init(&executor, &support.context, 2, &allocator));
  RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &msg, &subscription_callback, ON_NEW_DATA));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));
}

void loop() {
  RCCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100))); 
}
