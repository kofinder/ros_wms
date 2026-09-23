#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"


class HelloWorldSubscriberNode: public rclcpp::Node {

    public:

        HelloWorldSubscriberNode() : Node("hello_world_subscriber_node") {
            subscriber = this->create_subscription<std_msgs::msg::String>(
                "hello_world", 10, 
                std::bind(&HelloWorldSubscriberNode::message_callback, this, std::placeholders::_1)
            );
        }

    private:

        rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscriber;

        void message_callback(std_msgs::msg::String& message) {
            std::cout << "subscribe message ===>" << message.data << std::endl;
        }
};


int main(int argc, char const *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<HelloWorldSubscriberNode>());
    rclcpp::shutdown();

    return 0;
}
