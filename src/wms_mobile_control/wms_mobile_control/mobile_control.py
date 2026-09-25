#!/usr/bin/env python3

import json
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

import rclpy
from geometry_msgs.msg import Twist
from rclpy.node import Node


HTML_PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta
    name="viewport"
    content="width=device-width, initial-scale=1.0, user-scalable=no">

  <title>WMS AMR Control</title>

  <style>
    * {
      box-sizing: border-box;
      -webkit-user-select: none;
      user-select: none;
      touch-action: manipulation;
    }

    body {
      margin: 0;
      font-family: Arial, sans-serif;
      background: #111827;
      color: white;
      display: flex;
      justify-content: center;
    }

    .container {
      width: 100%;
      max-width: 480px;
      padding: 24px;
      text-align: center;
    }

    h1 {
      margin-bottom: 4px;
    }

    .status {
      margin-bottom: 24px;
      color: #9ca3af;
    }

    .controls {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 14px;
    }

    button {
      height: 90px;
      border: none;
      border-radius: 16px;
      font-size: 28px;
      font-weight: bold;
      background: #374151;
      color: white;
      box-shadow: 0 5px 12px rgba(0,0,0,0.35);
    }

    button:active {
      transform: scale(0.96);
      background: #4b5563;
    }

    .forward {
      background: #2563eb;
    }

    .turn {
      background: #7c3aed;
    }

    .reverse {
      background: #d97706;
    }

    .stop {
      background: #dc2626;
    }

    .empty {
      visibility: hidden;
    }

    .speed {
      margin-top: 25px;
      background: #1f2937;
      padding: 16px;
      border-radius: 12px;
    }

    input[type="range"] {
      width: 100%;
    }

    .value {
      margin-top: 8px;
      color: #d1d5db;
    }
  </style>
</head>

<body>

<div class="container">

  <h1>WMS AMR</h1>

  <div class="status" id="status">
    Ready
  </div>

  <div class="controls">

    <button
      class="turn"
      data-linear="1"
      data-angular="1">
      ↖
    </button>

    <button
      class="forward"
      data-linear="1"
      data-angular="0">
      ↑
    </button>

    <button
      class="turn"
      data-linear="1"
      data-angular="-1">
      ↗
    </button>

    <div class="empty"></div>

    <button
      class="stop"
      id="stopButton">
      STOP
    </button>

    <div class="empty"></div>

    <button
      class="turn"
      data-linear="-1"
      data-angular="-1">
      ↙
    </button>

    <button
      class="reverse"
      data-linear="-1"
      data-angular="0">
      ↓
    </button>

    <button
      class="turn"
      data-linear="-1"
      data-angular="1">
      ↘
    </button>

  </div>


  <div class="speed">

    <label>
      Speed
    </label>

    <input
      id="speed"
      type="range"
      min="0.05"
      max="0.60"
      step="0.05"
      value="0.20">

    <div class="value">
      <span id="speedValue">0.20</span> m/s
    </div>

  </div>

</div>


<script>

const speedSlider =
  document.getElementById("speed");

const speedValue =
  document.getElementById("speedValue");

const statusElement =
  document.getElementById("status");

let commandTimer = null;


speedSlider.addEventListener(
  "input",
  () => {
    speedValue.textContent =
      Number(speedSlider.value).toFixed(2);
  }
);


async function sendCommand(
  linear,
  angular)
{
  try {

    const speed =
      Number(speedSlider.value);

    const body = {
      linear: linear * speed,
      angular: angular * 0.6
    };

    await fetch(
      "/api/cmd",
      {
        method: "POST",

        headers: {
          "Content-Type":
            "application/json"
        },

        body:
          JSON.stringify(body)
      });

    statusElement.textContent =
      "linear: "
      + body.linear.toFixed(2)
      + " m/s   angular: "
      + body.angular.toFixed(2)
      + " rad/s";

  }
  catch (error) {

    statusElement.textContent =
      "Connection error";

  }
}


function startCommand(
  linear,
  angular)
{
  stopCommandTimer();

  sendCommand(
    linear,
    angular);

  commandTimer =
    setInterval(
      () => {
        sendCommand(
          linear,
          angular);
      },
      100);
}


function stopCommandTimer()
{
  if (commandTimer !== null) {

    clearInterval(
      commandTimer);

    commandTimer = null;
  }
}


function stopRobot()
{
  stopCommandTimer();

  sendCommand(
    0,
    0);

  statusElement.textContent =
    "STOPPED";
}


document
  .querySelectorAll(
    "button[data-linear]")
  .forEach(
    button => {

      const linear =
        Number(
          button.dataset.linear);

      const angular =
        Number(
          button.dataset.angular);


      button.addEventListener(
        "pointerdown",
        event => {

          event.preventDefault();

          startCommand(
            linear,
            angular);
        });


      button.addEventListener(
        "pointerup",
        event => {

          event.preventDefault();

          stopRobot();
        });


      button.addEventListener(
        "pointerleave",
        stopRobot);


      button.addEventListener(
        "pointercancel",
        stopRobot);

    });


document
  .getElementById(
    "stopButton")
  .addEventListener(
    "click",
    stopRobot);


window.addEventListener(
  "blur",
  stopRobot);


document.addEventListener(
  "visibilitychange",
  () => {

    if (document.hidden) {
      stopRobot();
    }

  });

</script>

</body>
</html>
"""


class MobileControlNode(Node):

    def __init__(self):

        super().__init__(
            "wms_mobile_control")

        self.publisher_ = \
            self.create_publisher(
                Twist,
                "/cmd_vel",
                10)

        self.declare_parameter(
            "http_port",
            8080)

        self.declare_parameter(
            "max_linear_speed",
            0.60)

        self.declare_parameter(
            "max_angular_speed",
            1.00)

        self.http_port = \
            self.get_parameter(
                "http_port"
            ).value

        self.max_linear_speed = \
            self.get_parameter(
                "max_linear_speed"
            ).value

        self.max_angular_speed = \
            self.get_parameter(
                "max_angular_speed"
            ).value

        self.server = None

        self.start_http_server()


    def publish_cmd(
        self,
        linear: float,
        angular: float):

        linear = max(
            -self.max_linear_speed,
            min(
                self.max_linear_speed,
                linear))

        angular = max(
            -self.max_angular_speed,
            min(
                self.max_angular_speed,
                angular))

        msg = Twist()

        msg.linear.x = \
            float(linear)

        msg.angular.z = \
            float(angular)

        self.publisher_.publish(
            msg)


    def start_http_server(self):

        node = self


        class RequestHandler(
            BaseHTTPRequestHandler):

            def do_GET(self):

                path = \
                    urlparse(
                        self.path
                    ).path

                if path == "/":

                    content = \
                        HTML_PAGE.encode(
                            "utf-8")

                    self.send_response(
                        200)

                    self.send_header(
                        "Content-Type",
                        "text/html; charset=utf-8")

                    self.send_header(
                        "Content-Length",
                        str(len(content)))

                    self.end_headers()

                    self.wfile.write(
                        content)

                    return


                self.send_error(
                    404)


            def do_POST(self):

                path = \
                    urlparse(
                        self.path
                    ).path

                if path != "/api/cmd":

                    self.send_error(
                        404)

                    return


                try:

                    content_length = \
                        int(
                            self.headers.get(
                                "Content-Length",
                                "0"))

                    raw_body = \
                        self.rfile.read(
                            content_length)

                    data = \
                        json.loads(
                            raw_body.decode(
                                "utf-8"))

                    linear = \
                        float(
                            data.get(
                                "linear",
                                0.0))

                    angular = \
                        float(
                            data.get(
                                "angular",
                                0.0))

                    node.publish_cmd(
                        linear,
                        angular)


                    response = \
                        json.dumps({
                            "ok": True
                        }).encode(
                            "utf-8")


                    self.send_response(
                        200)

                    self.send_header(
                        "Content-Type",
                        "application/json")

                    self.send_header(
                        "Content-Length",
                        str(len(response)))

                    self.end_headers()

                    self.wfile.write(
                        response)


                except Exception as exc:

                    node.get_logger().error(
                        f"HTTP command error: {exc}")

                    self.send_error(
                        400)


            def log_message(
                self,
                format,
                *args):

                return


        self.server = \
            ThreadingHTTPServer(
                (
                    "0.0.0.0",
                    self.http_port
                ),
                RequestHandler)


        thread = \
            threading.Thread(
                target=
                    self.server.serve_forever,
                daemon=True)

        thread.start()


        self.get_logger().info(
            f"Mobile controller running on "
            f"http://0.0.0.0:{self.http_port}")


    def destroy_node(self):

        self.publish_cmd(
            0.0,
            0.0)

        if self.server is not None:

            self.server.shutdown()

            self.server.server_close()

        super().destroy_node()


def main(args=None):

    rclpy.init(
        args=args)

    node = \
        MobileControlNode()

    try:

        rclpy.spin(
            node)

    except KeyboardInterrupt:

        pass

    finally:

        node.publish_cmd(
            0.0,
            0.0)

        node.destroy_node()

        rclpy.shutdown()


if __name__ == "__main__":

    main()