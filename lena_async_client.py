#! /usr/bin/python

import grpc
import rgbd_pb2
import rgbd_pb2_grpc
import cv2
import cvui
import pdb
import numpy as np
import time

WINDOW_NAME = "gRPC Test"
IMAGE_WIDTH = 640
IMAGE_HEIGHT = 480

options = [('grpc.max_send_message_length', 30 * 1024 * 1024),
           ('grpc.max_receive_message_length', 30 * 1024 * 1024)]


cvui.init(WINDOW_NAME)
key = cv2.waitKey(10)
count = 0

with grpc.insecure_channel('localhost:50051', options=options) as channel:
    while key & 0xFF != ord('q') or key & 0xFF != 27:
        stub = rgbd_pb2_grpc.RGBDServiceStub(channel)
        _response = stub.RGBDSend.future(rgbd_pb2.RGBDRequest())
        response = _response.result()

        count += 1
        print(count)
        w = response.rgb_image.width
        h = response.rgb_image.height
        c = response.rgb_image.channel
        if w == 0:
            print("No Image")
            continue
        img_np = np.frombuffer(response.rgb_image.data, np.uint8)
        img_rgb = img_np.reshape(h, w, c)
        time.sleep(1)
        cv2.imshow("Lena on gRPC", img_rgb)
        key = cv2.waitKey(10)
cv2.destroyAllWindows()
