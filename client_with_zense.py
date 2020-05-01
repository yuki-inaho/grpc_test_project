#! /usr/bin/python
import grpc
import rgbd_pb2
import rgbd_pb2_grpc
from utils import bytes_to_ndarray
import sys

import cv2
import cvui
import numpy as np

WINDOW_NAME = "gRPC Test"
IMAGE_WIDTH = 640
IMAGE_HEIGHT = 480

options = [('grpc.max_send_message_length', 30 * 1024 * 1024),
           ('grpc.max_receive_message_length', 30 * 1024 * 1024)]

with grpc.insecure_channel('localhost:50051', options=options) as channel:
    cvui.init(WINDOW_NAME)
    key = cv2.waitKey(20)
    while ((key & 0xFF != ord('q')) or (key & 0xFF != 27)):
        stub = rgbd_pb2_grpc.RGBDServiceStub(channel)
        response = stub.RGBDSend(rgbd_pb2.RGBDRequest())
        rgb_img = bytes_to_ndarray(response.rgb_image)
        depth_img = bytes_to_ndarray(response.depth_image)
        if rgb_img.shape[0] == 0:
            print("No Image")
            continue

        rgb_img_resized = cv2.resize(rgb_img, (IMAGE_WIDTH, IMAGE_HEIGHT))

        depth_img_colorized = np.zeros([IMAGE_HEIGHT, IMAGE_WIDTH,
                                        3]).astype(np.uint8)
        depth_img_colorized[:, :, 1] = 255
        depth_img_colorized[:, :, 2] = 255

        _depth_img_zense_hue = depth_img.copy().astype(np.float32)
        _depth_img_zense_hue[np.where(_depth_img_zense_hue > 2000)] = 0
        zero_idx = np.where((_depth_img_zense_hue > 2000)
                            | (_depth_img_zense_hue == 0))
        _depth_img_zense_hue *= 255.0 / 2000.0

        depth_img_colorized[:, :, 0] = _depth_img_zense_hue.astype(np.uint8)
        depth_img_colorized = cv2.cvtColor(depth_img_colorized,
                                           cv2.COLOR_HSV2RGB)
        depth_img_colorized[zero_idx[0], zero_idx[1], :] = 0

        frame = np.zeros((IMAGE_HEIGHT * 2, IMAGE_WIDTH * 2, 3), np.uint8)
        frame[0:IMAGE_HEIGHT, 0:IMAGE_WIDTH, :] = rgb_img_resized
        frame[0:IMAGE_HEIGHT,
              IMAGE_WIDTH:IMAGE_WIDTH * 2, :] = depth_img_colorized

        cvui.update()
        cv2.imshow(WINDOW_NAME, frame)
        key = cv2.waitKey(20)
        if key == 27:
            break

    cv2.destroyAllWindows()
