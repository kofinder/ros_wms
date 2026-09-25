#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <memory>
#include <string>

#include "geometry_msgs/msg/twist.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "std_msgs/msg/float64.hpp"

using namespace std::chrono_literals;

class WmsDriveController : public rclcpp::Node
{
public:
  WmsDriveController()
  : Node("wms_drive_controller")
  {
    // Robot geometry.
    wheelbase_ = declare_parameter<double>("wheelbase", 0.865);
    wheel_radius_ = declare_parameter<double>("wheel_radius", 0.090);

    // Motion limits.
    max_steering_angle_ =
      declare_parameter<double>("max_steering_angle", 0.785398);  // 45 deg
    max_wheel_speed_ =
      declare_parameter<double>("max_wheel_speed", 10.0);         // rad/s
    cmd_timeout_ =
      declare_parameter<double>("cmd_timeout", 0.5);              // seconds

    // Direction correction parameters. Keep at +1 unless the real / simulated
    // joint direction is reversed.
    drive_sign_ = declare_parameter<double>("drive_sign", 1.0);
    steering_sign_ = declare_parameter<double>("steering_sign", 1.0);

    odom_frame_ = declare_parameter<std::string>("odom_frame", "odom");
    base_frame_ = declare_parameter<std::string>("base_frame", "base_footprint");

    steering_pub_ =
      create_publisher<std_msgs::msg::Float64>("/front_steering_cmd", 10);

    drive_pub_ =
      create_publisher<std_msgs::msg::Float64>("/front_drive_cmd", 10);

    odom_pub_ =
      create_publisher<nav_msgs::msg::Odometry>("/odom_raw", 20);

    cmd_vel_sub_ =
      create_subscription<geometry_msgs::msg::Twist>(
        "/cmd_vel",
        10,
        std::bind(
          &WmsDriveController::cmdVelCallback,
          this,
          std::placeholders::_1));

    joint_state_sub_ =
      create_subscription<sensor_msgs::msg::JointState>(
        "/joint_states",
        20,
        std::bind(
          &WmsDriveController::jointStateCallback,
          this,
          std::placeholders::_1));

    last_cmd_time_ = now();

    safety_timer_ =
      create_wall_timer(
        100ms,
        std::bind(
          &WmsDriveController::safetyTimerCallback,
          this));

    RCLCPP_INFO(get_logger(), "WMS steer-drive controller started");
    RCLCPP_INFO(
      get_logger(),
      "wheelbase=%.3f m, wheel_radius=%.3f m",
      wheelbase_,
      wheel_radius_);
    RCLCPP_INFO(
      get_logger(),
      "Publishing wheel odometry on /odom_raw (%s -> %s). TF is left to robot_localization.",
      odom_frame_.c_str(),
      base_frame_.c_str());
  }

private:
  static double normalizeAngle(double angle)
  {
    return std::atan2(std::sin(angle), std::cos(angle));
  }

  static int findJointIndex(
    const sensor_msgs::msg::JointState & msg,
    const std::string & joint_name)
  {
    const auto it = std::find(msg.name.begin(), msg.name.end(), joint_name);
    if (it == msg.name.end()) {
      return -1;
    }

    return static_cast<int>(std::distance(msg.name.begin(), it));
  }

  void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    last_cmd_time_ = now();
    stopped_ = false;

    const double linear_velocity = msg->linear.x;
    const double angular_velocity = msg->angular.z;

    double steering_angle = 0.0;
    double wheel_speed = 0.0;

    constexpr double velocity_epsilon = 0.001;

    // Bicycle / tricycle kinematics:
    //   yaw_rate = v / L * tan(delta)
    //   delta    = atan(L * yaw_rate / v)
    //
    // A steer-drive robot cannot rotate in place when v == 0.
    if (std::abs(linear_velocity) > velocity_epsilon) {
      steering_angle =
        std::atan((wheelbase_ * angular_velocity) / linear_velocity);

      steering_angle =
        std::clamp(
          steering_angle,
          -max_steering_angle_,
          max_steering_angle_);

      // The front wheel travels faster than the rear-axle reference by
      // 1/cos(delta).
      const double cos_steering = std::cos(steering_angle);

      if (std::abs(cos_steering) > 0.01) {
        wheel_speed =
          linear_velocity /
          (wheel_radius_ * cos_steering);
      }
    }

    steering_angle *= steering_sign_;
    wheel_speed *= drive_sign_;

    wheel_speed =
      std::clamp(
        wheel_speed,
        -max_wheel_speed_,
        max_wheel_speed_);

    publishCommands(steering_angle, wheel_speed);
  }

  void jointStateCallback(const sensor_msgs::msg::JointState::SharedPtr msg)
  {
    const int steering_index = findJointIndex(*msg, "front_steering_joint");
    const int drive_index = findJointIndex(*msg, "front_drive_wheel_joint");

    if (steering_index < 0 || drive_index < 0) {
      RCLCPP_WARN_THROTTLE(
        get_logger(),
        *get_clock(),
        2000,
        "Required front steering / drive joints are missing from /joint_states");
      return;
    }

    if (
      static_cast<std::size_t>(steering_index) >= msg->position.size() ||
      static_cast<std::size_t>(drive_index) >= msg->velocity.size())
    {
      RCLCPP_WARN_THROTTLE(
        get_logger(),
        *get_clock(),
        2000,
        "/joint_states does not contain the required position / velocity fields");
      return;
    }

    // Convert actual joint signs back into the robot's positive-forward /
    // positive-left convention.
    const double steering_angle =
      normalizeAngle(msg->position[steering_index] * steering_sign_);

    const double wheel_angular_velocity =
      msg->velocity[drive_index] * drive_sign_;

    // Front-wheel tangential speed.
    const double front_wheel_linear_velocity =
      wheel_radius_ * wheel_angular_velocity;

    // For a bicycle / tricycle model:
    //   v_body   = v_front * cos(delta)
    //   yaw_rate = v_front * sin(delta) / L
    const double body_linear_velocity =
      front_wheel_linear_velocity * std::cos(steering_angle);

    const double yaw_rate =
      front_wheel_linear_velocity * std::sin(steering_angle) / wheelbase_;

    rclcpp::Time stamp(msg->header.stamp);

    // Some sources may provide a zero timestamp. Fall back to the node clock.
    if (stamp.nanoseconds() == 0) {
      stamp = now();
    }

    if (!have_last_joint_stamp_) {
      last_joint_stamp_ = stamp;
      have_last_joint_stamp_ = true;
      publishOdometry(stamp, body_linear_velocity, yaw_rate);
      return;
    }

    const double dt = (stamp - last_joint_stamp_).seconds();
    last_joint_stamp_ = stamp;

    // Ignore invalid / discontinuous timestamps. This also prevents a large
    // pose jump if simulation time resets.
    if (dt > 0.0 && dt < 0.5) {
      const double mid_yaw = yaw_ + 0.5 * yaw_rate * dt;

      x_ += body_linear_velocity * std::cos(mid_yaw) * dt;
      y_ += body_linear_velocity * std::sin(mid_yaw) * dt;
      yaw_ = normalizeAngle(yaw_ + yaw_rate * dt);
    }

    publishOdometry(stamp, body_linear_velocity, yaw_rate);
  }

  void publishOdometry(
    const rclcpp::Time & stamp,
    double linear_velocity,
    double yaw_rate)
  {
    nav_msgs::msg::Odometry odom;

    odom.header.stamp = stamp;
    odom.header.frame_id = odom_frame_;
    odom.child_frame_id = base_frame_;

    odom.pose.pose.position.x = x_;
    odom.pose.pose.position.y = y_;
    odom.pose.pose.position.z = 0.0;

    const double half_yaw = 0.5 * yaw_;
    odom.pose.pose.orientation.x = 0.0;
    odom.pose.pose.orientation.y = 0.0;
    odom.pose.pose.orientation.z = std::sin(half_yaw);
    odom.pose.pose.orientation.w = std::cos(half_yaw);

    odom.twist.twist.linear.x = linear_velocity;
    odom.twist.twist.linear.y = 0.0;
    odom.twist.twist.angular.z = yaw_rate;

    // Conservative placeholder covariances for simulated wheel odometry.
    // The EKF currently uses wheel longitudinal velocity and IMU yaw rate.
    odom.pose.covariance[0] = 0.05 * 0.05;   // x
    odom.pose.covariance[7] = 0.05 * 0.05;   // y
    odom.pose.covariance[35] = 0.10 * 0.10;  // yaw

    odom.twist.covariance[0] = 0.02 * 0.02;   // vx
    odom.twist.covariance[35] = 0.05 * 0.05;  // yaw rate

    odom_pub_->publish(odom);
  }

  void safetyTimerCallback()
  {
    const double elapsed = (now() - last_cmd_time_).seconds();

    if (elapsed > cmd_timeout_ && !stopped_) {
      publishCommands(0.0, 0.0);
      stopped_ = true;
    }
  }

  void publishCommands(double steering_angle, double wheel_speed)
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

  std::string odom_frame_;
  std::string base_frame_;

  bool stopped_{false};
  bool have_last_joint_stamp_{false};

  rclcpp::Time last_cmd_time_;
  rclcpp::Time last_joint_stamp_{0, 0, RCL_ROS_TIME};

  double x_{0.0};
  double y_{0.0};
  double yaw_{0.0};

  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr steering_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr drive_pub_;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;

  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_state_sub_;

  rclcpp::TimerBase::SharedPtr safety_timer_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<WmsDriveController>());
  rclcpp::shutdown();
  return 0;
}