# install first
source /opt/ros/jazzy/setup.bash
source install/setup.bash

# Terminal 1 — Gazebo + Robot
ros2 launch wms_simulation simulation.launch.py


# Terminal 2 — SLAM + RViz
ros2 launch wms_navigation mapping.launch.py


# Terminal 3 — Manual Control
ros2 run teleop_twist_keyboard teleop_twist_keyboard