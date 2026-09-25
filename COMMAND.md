                 MOTOR
                   ↓
             Wheel rotates
                   ↓
               Encoder
                   ↓
             KINEMATICS
                   ↓
              ODOMETRY
                   ↓
          odom → base_link
                   ↓
                  TF2
                   ↓
       Robot coordinate frames
                   ↓
      ┌────────────┼────────────┐
      ↓            ↓            ↓
    LiDAR         IMU         Camera



           Map
            ↓
           AMCL
            ↓
      Nav2 Planner
            ↓
      Nav2 Controller
            ↓
      /cmd_vel
            ↓
      wms_drive_controller  [C++]
            ↓
      front steering + drive wheel

      

Kinematics = “Given wheel/joint motion, how should the robot move?”
Odometry = “Based on measured motion, where does the robot estimate it has moved to?”
TF(Transform) - Where is coordinate frame A relative to coordinate frame B?
URDF(Unified Robot Description Format) = URDF is the standard robot-description format you'll encounter constantly in ROS 2.
SDF(Simulation Description Format) = SDF is much more focused on simulation and is widely used with Gazebo.
Xacro(XML Macros) = it is a convenient way to generate URDF files without writing huge amounts of repetitive XML.