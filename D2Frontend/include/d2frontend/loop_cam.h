#pragma once

#include <ros/ros.h>
#include <cv_bridge/cv_bridge.h>
#include <swarm_msgs/ImageDescriptor.h>
#include <swarm_msgs/LoopEdge.h>
#include <camodocal/camera_models/Camera.h>
#include <camodocal/camera_models/PinholeCamera.h>
#include <functional>
// #include <vins/VIOKeyframe.h>
#include <swarm_msgs/ImageDescriptor_t.hpp>
#include <swarm_msgs/FisheyeFrameDescriptor_t.hpp>
#include "d2frontend_params.h"
#include "superpoint_tensorrt.h"
#include "mobilenetvlad_tensorrt.h"
#include <message_filters/subscriber.h>
#include <message_filters/time_synchronizer.h>
#include <d2frontend/utils.h>

//#include <swarm_loop/HFNetSrv.h>

using namespace swarm_msgs;
using namespace camodocal;
using namespace Eigen;

namespace D2Frontend {


void match_local_features(std::vector<cv::Point2f> & pts_up, std::vector<cv::Point2f> & pts_down, 
    std::vector<float> & _desc_up, std::vector<float> & _desc_down, 
    std::vector<int> & ids_up, std::vector<int> & ids_down);

    
struct StereoFrame{
    ros::Time stamp;
    int keyframe_id;
    std::vector<cv::Mat> left_images, right_images, depth_images;
    Swarm::Pose pose_drone;
    std::vector<Swarm::Pose> left_extrisincs, right_extrisincs;

    StereoFrame():stamp(0) {

    }

    StereoFrame(ros::Time _stamp, cv::Mat _left_image, cv::Mat _right_image, 
        Swarm::Pose _left_extrinsic, Swarm::Pose _right_extrinsic, int self_id):
        stamp(_stamp)
    {
        left_images.push_back(_left_image);
        right_images.push_back(_right_image);
        left_extrisincs.push_back(_left_extrinsic);
        right_extrisincs.push_back(_right_extrinsic);
        keyframe_id = generate_keyframe_id(_stamp, self_id);

    }

    StereoFrame(ros::Time _stamp, cv::Mat _left_image, cv::Mat _dep_image, 
        Swarm::Pose _left_extrinsic, int self_id):
        stamp(_stamp)
    {
        left_images.push_back(_left_image);
        depth_images.push_back(_dep_image);
        left_extrisincs.push_back(_left_extrinsic);
        keyframe_id = generate_keyframe_id(_stamp, self_id);
    }

    // StereoFrame(vins::FlattenImages vins_flatten, int self_id):
    //     stamp(vins_flatten.header.stamp) {
    //     for (int i = 1; i < vins_flatten.up_cams.size(); i++) {
    //         left_extrisincs.push_back(vins_flatten.extrinsic_up_cams[i]);
    //         right_extrisincs.push_back(vins_flatten.extrinsic_down_cams[i]);
            
    //         auto _l = getImageFromMsg(vins_flatten.up_cams[i]);
    //         auto _r = getImageFromMsg(vins_flatten.down_cams[i]);

    //         left_images.push_back(_l->image);
    //         right_images.push_back(_r->image);
    //     }

    //     keyframe_id = generate_keyframe_id(stamp, self_id);
    // }
};

struct LoopCamConfig
{
    /* data */
    CameraConfig camera_configuration;
    std::string camera_config_path;
    std::string superpoint_model;
    std::string pca_comp;
    std::string pca_mean;
    double superpoint_thres;
    int superpoint_max_num;
    std::string netvlad_model;
    int width;
    int height; 
    int self_id = 0;
    bool OUTPUT_RAW_SUPERPOINT_DESC;
    bool LOWER_CAM_AS_MAIN;
    double DEPTH_NEAR_THRES;
    double TRIANGLE_THRES;
    int ACCEPT_MIN_3D_PTS;
    double DEPTH_FAR_THRES;
};

struct VisualImageDesc {
    //This stands for single image
    ros::Time timestamp;
    StereoFrame * stereo_frame = nullptr;
    int drone_id = 0;
    uint64_t frame_id = 0; 
    int camera_id = 0; //camera id in stereo_frame
    Swarm::Pose extrinsic;
    Swarm::Pose pose_drone; //IMU propagated pose
    std::vector<Vector3d> landmarks_3d;
    std::vector<Vector2d> landmarks_2d_norm; //normalized 2d 
    std::vector<cv::Point2f> landmarks_2d; //normalized 2d 
    std::vector<uint8_t> landmarks_flag; //0 no 3d, 1 has 3d

    std::vector<float> image_desc;
    std::vector<float> feature_descriptor;
    bool prevent_adding_db = false;

    std::vector<uint8_t> image; //Buffer to store compressed image.

    int landmark_num() const {
        return landmarks_2d.size();
    }
};

struct VisualImageDescArray {
    int drone_id = 0;
    uint64_t frame_id;
    ros::Time stamp;
    std::vector<VisualImageDesc> images;
    Swarm::Pose pose_drone;
    int landmark_num;
    bool prevent_adding_db;
};

class LoopCam {
    LoopCamConfig _config;
    int cam_count = 0;
    int loop_duration = 10;
    int self_id = 0;
    int kf_count = 0;
    ros::ServiceClient hfnet_client;
    ros::ServiceClient superpoint_client;
    CameraConfig camera_configuration;
    std::fstream fsp;
#ifdef USE_TENSORRT
    Swarm::SuperPointTensorRT superpoint_net;
    Swarm::MobileNetVLADTensorRT netvlad_net;
#endif

    bool send_img;
public:

    bool show = false;

    // LoopDetector * loop_detector = nullptr;
    LoopCam(LoopCamConfig config, ros::NodeHandle & nh);
    
    VisualImageDesc extractor_img_desc_deepnet(ros::Time stamp, cv::Mat img, bool superpoint_mode=false);
    VisualImageDesc generate_stereo_image_descriptor(const StereoFrame & msg, cv::Mat & img, const int & vcam_id, cv::Mat &_show);
    VisualImageDesc generate_gray_depth_image_descriptor(const StereoFrame & msg, cv::Mat & img, const int & vcam_id, cv::Mat &_show);
    VisualImageDescArray * process_stereoframe(const StereoFrame & msg, std::vector<cv::Mat> & imgs);

    void encode_image(const cv::Mat & _img, VisualImageDesc & _img_desc);
    
    CameraPtr cam;
    cv::Mat cameraMatrix;

    CameraConfig get_camera_configuration() const {
        return camera_configuration;
    }
};
}
