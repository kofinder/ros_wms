import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration

from launch_ros.actions import Node


def generate_launch_description():

    navigation_dir = get_package_share_directory("wms_navigation")
    nav2_bringup_dir = get_package_share_directory("nav2_bringup")

    map_file = os.path.join(
        navigation_dir,
        "maps",
        "factory_map.yaml",
    )

    nav2_config = os.path.join(
        navigation_dir,
        "config",
        "nav2.yaml",
    )

    rviz_config = os.path.join(
        navigation_dir,
        "rviz",
        "navigation.rviz",
    )

    use_sim_time = LaunchConfiguration("use_sim_time")
    autostart = LaunchConfiguration("autostart")

    declare_use_sim_time = DeclareLaunchArgument(
        "use_sim_time",
        default_value="true",
        description="Use Gazebo simulation clock",
    )

    declare_autostart = DeclareLaunchArgument(
        "autostart",
        default_value="true",
        description="Automatically activate Nav2 lifecycle nodes",
    )

    # ---------------------------------------------------------
    # Full Nav2 bringup
    #
    # Includes:
    #   map_server
    #   AMCL
    #   planner_server
    #   controller_server
    #   smoother_server
    #   behavior_server
    #   bt_navigator
    #   waypoint_follower
    #   lifecycle managers
    # ---------------------------------------------------------

    nav2_bringup = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                nav2_bringup_dir,
                "launch",
                "bringup_launch.py",
            )
        ),
        launch_arguments={
            "slam": "False",
            "map": map_file,
            "params_file": nav2_config,
            "use_sim_time": use_sim_time,
            "autostart": autostart,
            "use_composition": "False",
        }.items(),
    )

    # ---------------------------------------------------------
    # RViz
    # ---------------------------------------------------------

    rviz = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        output="screen",
        arguments=[
            "-d",
            rviz_config,
        ],
        parameters=[
            {
                "use_sim_time": use_sim_time,
            }
        ],
    )

    return LaunchDescription([
        declare_use_sim_time,
        declare_autostart,

        nav2_bringup,
        rviz,
    ])