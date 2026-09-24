import os

from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():

    navigation_dir = get_package_share_directory("wms_navigation")

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

    # ---------------------------------------------------------
    # Map Server
    # ---------------------------------------------------------
    map_server = Node(
        package="nav2_map_server",
        executable="map_server",
        name="map_server",
        output="screen",
        parameters=[
            {
                "yaml_filename": map_file,
                "use_sim_time": True,
            }
        ],
    )

    # ---------------------------------------------------------
    # AMCL
    # ---------------------------------------------------------
    amcl = Node(
        package="nav2_amcl",
        executable="amcl",
        name="amcl",
        output="screen",
        parameters=[
            nav2_config,
            {
                "use_sim_time": True,
            },
        ],
    )

    # ---------------------------------------------------------
    # Lifecycle Manager
    # Automatically configures + activates map_server and AMCL
    # ---------------------------------------------------------
    lifecycle_manager = Node(
        package="nav2_lifecycle_manager",
        executable="lifecycle_manager",
        name="lifecycle_manager_localization",
        output="screen",
        parameters=[
            {
                "use_sim_time": True,
                "autostart": True,
                "node_names": [
                    "map_server",
                    "amcl",
                ],
            }
        ],
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
        map_server,
        amcl,
        lifecycle_manager,
        rviz,
    ])