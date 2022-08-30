#!/usr/bin/env python3
from test_depth_estimation import *
from quad_cam_split import split_image
from cv_bridge import CvBridge
import cv2 as cv
import rospy
from sensor_msgs.msg import Image, CompressedImage
from sensor_msgs.msg import PointCloud2, PointField
import sensor_msgs.point_cloud2 as pc2

bridge = CvBridge()
FIELDS = [PointField('x', 0, PointField.FLOAT32, 1),
    PointField('y', 4, PointField.FLOAT32, 1),
    PointField('z', 8, PointField.FLOAT32, 1),
    PointField('b', 12, PointField.FLOAT32, 1),
    PointField('g', 16, PointField.FLOAT32, 1),
    PointField('r', 20, PointField.FLOAT32, 1)]

class DepthGenerateNode:
    def __init__(self, fisheye_configs, stereo_paths, photometric_path, fov=190, width=600, height=300):
        self.gens = loadConfig(fisheye_configs, stereo_paths, fov=fov, width=width, height=height)
        self.photometric = cv.imread(photometric_path, cv.IMREAD_GRAYSCALE)/255.0
        self.pcl_pub = rospy.Publisher("/depth_estimation/point_cloud", PointCloud2, queue_size=1)

    def callback(self, img_msg):
        if img_msg._type == "sensor_msgs/Image":
            img = bridge.imgmsg_to_cv2(img_msg, desired_encoding='passthrough')
        else:
            img = bridge.compressed_imgmsg_to_cv2(img_msg, desired_encoding='passthrough')
        imgs = split_image(img)
        photometric_calibed = []
        photometric = self.photometric
        for img in imgs:
            #Apply inverse of photometric calibration
            if photometric is not None:
                #Convert to grayscale
                if len(img.shape) == 3:
                    img = cv.cvtColor(img, cv.COLOR_BGR2GRAY)
                calibed = calib_photometric(img, photometric)
                photometric_calibed.append(calibed)
            else:
                photometric_calibed.append(img)
        pcl, texture = None, None
        for gen in self.gens[1:]:
            _pcl, _texture = test_depth_gen(gen, photometric_calibed, imgs, detailed=args.verbose)
            if pcl is None:
                pcl = _pcl
                texture = _texture
            else:
                pcl = np.concatenate((pcl, _pcl), axis=0)
                texture = np.concatenate((texture, _texture), axis=0)
        header = img_msg.header
        header.frame_id = "world"
        colored_pcl = np.c_[pcl, texture]
        msg = pc2.create_cloud(header, FIELDS, colored_pcl)
        self.pcl_pub.publish(msg)

if __name__ == "__main__":
    rospy.init_node('depth_generate_node')
    #Register node
    parser = argparse.ArgumentParser(description='Fisheye undist')
    parser.add_argument("-f","--fov", type=float, default=190, help="hoizon fov of fisheye")
    parser.add_argument("-c","--config", type=str, default="", help="config file path")
    parser.add_argument("-p","--photometric", type=str, help="photometric calibration image")
    parser.add_argument("-v","--verbose", action='store_true', help="show image")
    args = parser.parse_args()
    stereo_paths = ["/home/xuhao/Dropbox/data/d2slam/quadcam2/stereo_calib_1_0.yaml",
                "/home/xuhao/Dropbox/data/d2slam/quadcam2/stereo_calib_2_1.yaml",
                "/home/xuhao/Dropbox/data/d2slam/quadcam2/stereo_calib_3_2.yaml",
                "/home/xuhao/Dropbox/data/d2slam/quadcam2/stereo_calib_0_3.yaml"]
    node = DepthGenerateNode(args.config, stereo_paths, args.photometric)
    #Subscribe to image using ImageTransport
    sub_comp = rospy.Subscriber("/arducam/image/compressed", CompressedImage, node.callback)
    sub_raw = rospy.Subscriber("/arducam/image/raw", Image, node.callback)
    print("depth_generate_node started")
    rospy.spin()
