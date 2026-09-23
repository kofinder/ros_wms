#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include <chrono>

using namespace std::chrono_literals;

const double RPM_VALUE = 100.0;

class RpmPubNode: public rclcpp::Node {

    public:
        RpmPubNode(): Node("hello_world_pub_node") {
            publisher = this->create_publisher<std_msgs::msg::Float64>("rpm", 10); 
            timer = this->create_wall_timer(
                std::chrono::seconds(1),
                std::bind(&RpmPubNode::publish_rpm, this));
        }

    private:
        rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr publisher;
        rclcpp::TimerBase::SharedPtr timer; 
        size_t counter = 0;

        void publish_rpm() {
            auto message = std_msgs::msg::Float64();
            message.data = RPM_VALUE;
            publisher->publish(message);
            counter++;
        }

};

int main(int argc, char* argv[]) {

    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<RpmPubNode>());
    rclcpp::shutdown();

    return 0;
}