#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include <chrono>

class HelloWorldPublisherNode: public rclcpp::Node {

    public:
        HelloWorldPublisherNode(): Node("hello_world_pub_node") {
            publisher = this->create_publisher<std_msgs::msg::String>("hello_world", 10); 
            timer = this->create_wall_timer(
                std::chrono::seconds(1),
                std::bind(&HelloWorldPublisherNode::publish_hello_world, this));
        }

    private:
        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher;
        rclcpp::TimerBase::SharedPtr timer; 
        size_t counter = 0;

        void publish_hello_world() {
            auto message = std_msgs::msg::String();
            message.data = "Hello world 1" + std::to_string(counter);
            publisher->publish(message);
            counter++;
        }

};

int main(int argc, char* argv[]) {

    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<HelloWorldPublisherNode>());
    rclcpp::shutdown();

    return 0;
}