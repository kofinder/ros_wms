#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"

using namespace std::chrono_literals;

class WmsDriveController : public rclcpp::Node {
    public:
    WmsDriveController() : Node("wms_drive_controller")
  {
    // Robot geometry
    wheelbase_ = declare_parameter<double>("wheelbase", 0.865);
    wheel_radius_ = declare_parameter<double>("wheel_radius", 0.090);

    // Limits
    max_steering_angle_ = declare_parameter<double>("max_steering_angle", 0.785398);  // 45 deg
    max_wheel_speed_ = declare_parameter<double>("max_wheel_speed", 10.0);  // rad/s

    cmd_timeout_ = declare_parameter<double>("cmd_timeout", 0.5);  // seconds

    // Useful later if physical direction is reversed
    drive_sign_ = declare_parameter<double>("drive_sign", 1.0);
    steering_sign_ = declare_parameter<double>("steering_sign", 1.0);

    steering_pub_ =
      create_publisher<std_msgs::msg::Float64>(
        "/front_steering_cmd", 10);

    drive_pub_ =
      create_publisher<std_msgs::msg::Float64>(
        "/front_drive_cmd", 10);

    cmd_vel_sub_ =
      create_subscription<geometry_msgs::msg::Twist>(
        "/cmd_vel",
        10,
        std::bind(
          &WmsDriveController::cmdVelCallback,
          this,
          std::placeholders::_1));

    last_cmd_time_ = now();

    safety_timer_ =
      create_wall_timer(
        100ms,
        std::bind(
          &WmsDriveController::safetyTimerCallback,
          this));

    RCLCPP_INFO(
      get_logger(),
      "WMS drive controller started");
    RCLCPP_INFO(
      get_logger(),
      "wheelbase=%.3f m, wheel_radius=%.3f m",
      wheelbase_,
      wheel_radius_);
  }

private:
  void cmdVelCallback(
    const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    last_cmd_time_ = now();

    const double linear_velocity = msg->linear.x;
    const double angular_velocity = msg->angular.z;

    double steering_angle = 0.0;
    double wheel_speed = 0.0;

    constexpr double velocity_epsilon = 0.001;

    /*
     * Tricycle / bicycle kinematics
     *
     * yaw_rate = v / L * tan(delta)
     *
     * therefore:
     *
     * delta = atan(L * yaw_rate / v)
     *
     * The robot cannot rotate in place like a differential-drive robot.
     */
    if (std::abs(linear_velocity) > velocity_epsilon)
    {
      steering_angle =
        std::atan(
          (wheelbase_ * angular_velocity) /
          linear_velocity);

      steering_angle =
        std::clamp(
          steering_angle,
          -max_steering_angle_,
          max_steering_angle_);

      /*
       * Front wheel rolling velocity.
       *
       * v_front = v / cos(delta)
       *
       * wheel angular speed:
       *
       * omega_wheel = v_front / wheel_radius
       */
      const double cos_steering = std::cos(steering_angle);

      if (std::abs(cos_steering) > 0.01)
      {
        wheel_speed =
          linear_velocity /
          (wheel_radius_ * cos_steering);
      }
    }
    else
    {
      // Tricycle robot cannot perform an in-place rotation.
      steering_angle = 0.0;
      wheel_speed = 0.0;
    }

    steering_angle *= steering_sign_;
    wheel_speed *= drive_sign_;

    wheel_speed =
      std::clamp(
        wheel_speed,
        -max_wheel_speed_,
        max_wheel_speed_);

    publishCommands(
      steering_angle,
      wheel_speed);

    RCLCPP_DEBUG(
      get_logger(),
      "cmd_vel: v=%.3f m/s w=%.3f rad/s -> steer=%.3f rad wheel=%.3f rad/s",
      linear_velocity,
      angular_velocity,
      steering_angle,
      wheel_speed);
  }

  void safetyTimerCallback()
  {
    const double elapsed =
      (now() - last_cmd_time_).seconds();

    if (elapsed > cmd_timeout_)
    {
      if (!stopped_)
      {
        publishCommands(0.0, 0.0);
        stopped_ = true;
      }

      return;
    }

    stopped_ = false;
  }

  void publishCommands(
    double steering_angle,
    double wheel_speed)
  {
    std_msgs::msg::Float64 steering_msg;
    steering_msg.data = steering_angle;

    std_msgs::msg::Float64 drive_msg;
    drive_msg.data = wheel_speed;

    steering_pub_->publish(steering_msg);
    drive_pub_->publish(drive_msg);
  }

  double wheelbase_;
  double wheel_radius_;
  double max_steering_angle_;
  double max_wheel_speed_;
  double cmd_timeout_;
  double drive_sign_;
  double steering_sign_;

  bool stopped_{false};

  rclcpp::Time last_cmd_time_;

  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr
    steering_pub_;

  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr
    drive_pub_;

  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr
    cmd_vel_sub_;

  rclcpp::TimerBase::SharedPtr safety_timer_;
};


int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  rclcpp::spin(
    std::make_shared<WmsDriveController>());

  rclcpp::shutdown();

  return 0;
}