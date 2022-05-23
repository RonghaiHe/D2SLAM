#include "visualization.hpp"
#include <nav_msgs/Odometry.h>
#include <sensor_msgs/PointCloud.h>
#include "../estimator/d2estimator.hpp"
#include "CameraPoseVisualization.h"

namespace D2VINS {
sensor_msgs::PointCloud toPointCloud(const std::vector<D2Common::LandmarkPerId> landmarks, bool use_raw_color = false);

D2Visualization::D2Visualization():
    drone_colors{
        Vector3d(1, 1, 0), //drone 0 yellow
        Vector3d(1, 0, 0), //drone 1 red
        Vector3d(0, 1, 0), //drone 2 green
        Vector3d(0, 0, 1), //drone 3 blue
        Vector3d(0, 1, 1), //drone 4 cyan
        Vector3d(1, 0, 1), //drone 5 magenta
        Vector3d(1, 1, 1), //drone 6 white
        Vector3d(0, 0, 0), //drone 7 black
        Vector3d(0.5, 0.5, 0.5), //drone 8 gray
        Vector3d(0.5, 0, 0), //drone 9 orange
        Vector3d(0, 0.5, 0), //drone 10 green
        Vector3d(0, 0, 0.5), //drone 11 blue
        Vector3d(0.5, 0, 0.5), //drone 12 purple
        Vector3d(0.5, 0.5, 0), //drone 13 orange
        Vector3d(0, 0.5, 0.5), //drone 14 cyan
        Vector3d(0.5, 0.5, 0.5) //drone 15 white
    }
    {}

void D2Visualization::init(ros::NodeHandle & nh, D2Estimator * estimator) {
    pcl_pub = nh.advertise<sensor_msgs::PointCloud>("point_cloud", 1000);
    margined_pcl = nh.advertise<sensor_msgs::PointCloud>("margined_cloud", 1000);
    odom_pub = nh.advertise<nav_msgs::Odometry>("odometry", 1000);
    imu_prop_pub = nh.advertise<nav_msgs::Odometry>("imu_propagation", 1000);
    path_pub = nh.advertise<nav_msgs::Path>("path", 1000);
    sld_win_pub = nh.advertise<visualization_msgs::MarkerArray>("slding_window", 1000);
    _estimator = estimator;
}

void D2Visualization::postSolve() {
    auto odom = _estimator->getOdometry().toRos();
    odom_pub.publish(odom);
    auto imu_prop = _estimator->getImuPropagation().toRos();
    imu_prop_pub.publish(imu_prop);
    auto pcl = _estimator->getState().getInitializedLandmarks();
    pcl_pub.publish(toPointCloud(pcl));
    margined_pcl.publish(toPointCloud(_estimator->getMarginedLandmarks(), true));

    geometry_msgs::PoseStamped pose_stamped;
    pose_stamped.header = odom.header;
    pose_stamped.header.frame_id = "world";
    pose_stamped.pose = odom.pose.pose;
    path.header = odom.header;
    path.header.frame_id = "world";
    path.poses.push_back(pose_stamped);
    path_pub.publish(path);

    //For self_drone
    auto  & state = _estimator->getState();
    CameraPoseVisualization cam_visual;
    for (auto drone_id: state.availableDrones()) {
        for (int i = 0; i < state.sizeRemote(drone_id); i ++) {
            auto & frame = state.getRemoteFrame(drone_id, i);
            CamIdType camera_id = *state.getAvailableCameraIds().begin();
            auto cam_pose = frame.odom.pose()*state.getExtrinsic(camera_id);
            cam_visual.addPose(cam_pose.pos(), cam_pose.att(), drone_colors[drone_id], display_alpha);
        }
    }
    cam_visual.publishBy(sld_win_pub, odom.header);
}

sensor_msgs::PointCloud toPointCloud(const std::vector<D2Common::LandmarkPerId> landmarks, bool use_raw_color) {
    sensor_msgs::PointCloud pcl;
    pcl.header.frame_id = "world";
    pcl.points.resize(landmarks.size());
    pcl.channels.resize(3);
    pcl.channels[0].name = "rgb";
    pcl.channels[0].values.resize(landmarks.size());
    for (int i = 0; i < landmarks.size(); i++) {
        pcl.points[i].x = landmarks[i].position.x();
        pcl.points[i].y = landmarks[i].position.y();
        pcl.points[i].z = landmarks[i].position.z();
        Vector3i color(255, 255, 0.);
        if (use_raw_color) {
            color = Vector3i(landmarks[i].color[2], landmarks[i].color[1], landmarks[i].color[0]);
        } else {
            if (landmarks[i].flag == D2Common::LandmarkFlag::ESTIMATED) {
                //set color to green
                color = Vector3i(0, 255, 0.);
            } else if (landmarks[i].flag == D2Common::LandmarkFlag::OUTLIER) {
                //set color to gray
                color = Vector3i(200, 200, 200.);
            }
        }
        uint32_t hex_r = (0xff & color(0)) << 16;
        uint32_t hex_g = (0xff & color(1)) << 8;
        uint32_t hex_b = (0xff & color(2));
        uint32_t hex_rgb = hex_r | hex_g | hex_b;
        memcpy(pcl.channels[0].values.data() + i, &hex_rgb, sizeof(float));
    }
    return pcl;
}
}