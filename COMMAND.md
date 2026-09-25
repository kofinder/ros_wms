### ROS Client Library C++ API
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

## Inheriting RCL Node Functionality
    class ClassName: public rclcpp::Node {
        public:
            ClassName() : Node("name_of_node") {....}
        private:
            
    }


Creating A Pulisiher Instance
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
    publisher_ = this->create_publisher<std_msgs::msg::String>(
        topic_name", 10,
    )

Publising A Message
    auto message = std_msgs::msg::String();
    message.data = "My message";
    publisher_->publish(message);

Creating A Timer Instance
    #include <chrono>
    #include <functional>
    using namespace std::chrono_literals;
    timer_ = this->create_wall_timer(1w, std::bind(&ClassName::our_callback_function, this))

Running Our Node
    int main(int argc, char* argv[]) {

        rclcpp::init(argc, argv);
        rclcpp::spin(std::make_shared<ClassName>());
        rclcpp::shutdown();

        return 0;
    }


## Recap: Compling ROS2 Nodes
* Add depencies Top Package.xml
<buildtool_depend>ament_cmake</buildtool_depend>
<depend>rclcpp</depend>
<depend>std_msgs</depend>
* Add depencies to CMakeLists.txt
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

Add Node Executables to CMakeLists.txt
 add_executable(executable_name src/file.cpp)
 ament_target_dependencies(executable_name rclcpp std_msgs)

Install Targets in CMakeLists.txt
install(TARGETS
executable_name
DESTINATION lib/{PROJECT_NAME})

## Recap: Creating Subscriber
rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
subscription_ = this->create_subscription<std_msgs::msg::String>(
    "topic_name", 10,
    std::bind(&NodeClassName::sub_callback, this, std::placeholder::_1)
)

....
void sub_callback (const std_msgs::msg::String& msg) const {
    std::cout << msg.data >> std::endl;
}

### Recap: Creating Parameter 
* Terminal Command
ros2 param list
ros2 param get <node_name> <param_name>
ros2 param set <node_name> <param_name> <value>

* using Parameter in c++
    this->declare_parameter<float>(<param_name> <value>)
    float param_var;
    this->get_parameter(<param_name> <value>)

## Recap: Launch Files
    Teirmail Command
        ros2 launch <pkg_name> <lunch_file_name>

    Creating Laucnh file in Python
        from launch import LunchDescription
        form luanc_os.action import Node
        form launct.action import ExecuteProcess

        def generate_launch_description():
            retunr LunchDescription();

    Configure Node In LucnthDescription()
        Node(
            package="your_pkg_name",
            executable="node_executable_name",
            name="name_of_node_when_launched",
            parameters=[{"param_name": 7.0}]
        )
    Configure Execute Process Luanch Description
        ExecuteProcess(
            cmd=['ros2', 'topic', 'list'],
            output='screen'
        )

Recap: Creating Serives
    Create Custom SRV File
        int64 request_attr_name #comment
        stirng response_attr_name

    Configrer package.xml
        <build_depend>rosidl_defauat_generators</>
        <exec_dpend>rosldel_defualt_runtie</>
        <memror_of_group> rosdil_interface_pakaes</>

    Configure CmakeList.txt
        find_package(rosidl_default_generator REQUIRED)
        rosidel_generate_interface(${PROJECT_NAME}
        "src/yoursrname.srv")
        rosidle_get_typesupport_targe(cpp_typesupport_target "$Project_nae}" "rolsidel_typesupport_cpp")
        add_excutatole(my_srv_exuat src/serive_server.cpp)
        ament_target_depencies(my_srv_execuatable rclcpp std_msgs)
        target_link_libriates(my_srv_executalbe "${cpp_typesuport_target}")
    Termial Commands
        ros2 interface list
        ros2 services list
        ros2 serccies call <service_name> <srv_type> <req>

    Import Customer Serive in C++
    #include "your_pkg_name/srv/your_srv_name.hpp"

    Create Server Node In c++
        typedef your_pkg_name::srv::YourSrvName name;
        rclcpp::Serive<name>::SharedPtr serive_server_;

        serive_server_ = this->create_service<name>(
            "service_name",
            std::bind(&yournodeclass::service_callback_func, this, std::placeholder::_1, std::placeholder::_2)
        )
    Serives Call back function
        vvoid serive_calback_func(
            const yourSrvName::Request::SharedPtr request,
            yourSrvName::Response::Shartr reponse
        ) {
            do_something(request->request_attr_name)
            ...
            reponse->response_attr_name = "your result to return to client"
        }

    Service Cline Node In c++