import os

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():

    navigation_dir = get_package_share_directory("wms_navigation")
    slam_toolbox_dir = get_package_share_directory("slam_toolbox")

    slam_config = os.path.join(
        navigation_dir,
        "config",
        "slam.yaml",
    )

    rviz_config = os.path.join(
        navigation_dir,
        "rviz",
        "mapping.rviz",
    )

    # ---------------------------------------------------------
    # SLAM Toolbox
    # ---------------------------------------------------------
    slam = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                slam_toolbox_dir,
                "launch",
                "online_async_launch.py",
            )
        ),
        launch_arguments={
            "use_sim_time": "true",
            "slam_params_file": slam_config,
            "autostart": "true",
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
                "use_sim_time": True,
            }
        ],
    )

    return LaunchDescription([
        slam,
        rviz,
    ])