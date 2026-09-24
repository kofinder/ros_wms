import os

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command

from launch_ros.actions import Node

from ament_index_python.packages import get_package_share_directory


def generate_launch_description():

    description_package = get_package_share_directory(
        "wms_description"
    )

    simulation_package = get_package_share_directory(
        "wms_simulation"
    )

    ros_gz_package = get_package_share_directory(
        "ros_gz_sim"
    )

    xacro_file = os.path.join(
        description_package,
        "urdf",
        "wms_robot.urdf.xacro"
    )

    world_file = os.path.join(
        simulation_package,
        "worlds",
        "factory_test.sdf"
    )

    bridge_config = os.path.join(
        simulation_package,
        "config",
        "bridge.yaml"
    )

    ekf_config = os.path.join(
        simulation_package,
        "config",
        "ekf.yaml"
    )

    robot_description = Command([
        "xacro ",
        xacro_file
    ])

    
    ekf_node = Node(
        package="robot_localization",
        executable="ekf_node",
        name="ekf_filter_node",
        output="screen",
        parameters=[
            ekf_config,
            {
                "use_sim_time": True
            }
        ]
    )


    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        parameters=[
            {
                "robot_description": robot_description,
                "use_sim_time": True
            }
        ],
        output="screen"
    )

    imu_sensor_tf = Node(
        package="tf2_ros",
        executable="static_transform_publisher",
        name="imu_sensor_tf",
        arguments=[
            "--x", "0",
            "--y", "0",
            "--z", "0",
            "--roll", "0",
            "--pitch", "0",
            "--yaw", "0",
            "--frame-id", "imu_link",
            "--child-frame-id", "wms_robot/base_footprint/imu_sensor",
        ],
        output="screen",
    )

    lidar_sensor_tf = Node(
        package="tf2_ros",
        executable="static_transform_publisher",
        name="lidar_sensor_tf",
        arguments=[
            "--x", "0",
            "--y", "0",
            "--z", "0",
            "--roll", "0",
            "--pitch", "0",
            "--yaw", "0",
            "--frame-id", "lidar_link",
            "--child-frame-id", "wms_robot/base_footprint/lidar_sensor",
        ],
        output="screen",
    )

    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                ros_gz_package,
                "launch",
                "gz_sim.launch.py"
            )
        ),
        launch_arguments={
            "gz_args": f"-r {world_file}"
        }.items()
    )

    ros_gz_bridge = Node(
        package="ros_gz_bridge",
        executable="parameter_bridge",
        name="ros_gz_bridge",
        parameters=[
            {
                "config_file": bridge_config,
                "use_sim_time": True
            }
        ],
        output="screen"
    )

    spawn_robot = Node(
        package="ros_gz_sim",
        executable="create",
        arguments=[
            "-name", "wms_robot",
            "-topic", "robot_description",
            "-x", "0",
            "-y", "0",
            "-z", "0.25"
        ],
        output="screen"
    )

    return LaunchDescription([
        robot_state_publisher,
        gazebo,
        spawn_robot,
        ros_gz_bridge,
        imu_sensor_tf,
        lidar_sensor_tf,
        ekf_node
    ])