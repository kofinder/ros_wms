#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include <chrono>

const float WHEEL_REDIUS = 12.5 / 100;
class SpeedCalNode: public rclcpp::Node {

    public:
        SpeedCalNode(): Node("speed_calc_node") {
            
            rpm_subscription_ = this->create_subscription<std_msgs::msg::Float64>(
                "rpm", 
                10,
                std::bind(&SpeedCalNode::calculate__and_pub_speed, this, std::placeholders::_1)
            ); 

            speed_publisher = this->create_publisher<std_msgs::msg::Float64>(
                "spped",
                10
            );

            std::cout << "Speed Cal Node is Running..." << std::endl;
        }

    private:

        rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr rpm_subscription_;
        rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr speed_publisher;


        void calculate__and_pub_speed(const std_msgs::msg::Float64& rpm_msg) {
            auto speed = std_msgs::msg::Float64();
            speed.data = rpm_msg.data * (2 * WHEEL_REDIUS * M_PI) / 60;
            speed_publisher->publish(speed);
        }

};

int main(int argc, char* argv[]) {

    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<SpeedCalNode>());
    rclcpp::shutdown();

    return 0;
}