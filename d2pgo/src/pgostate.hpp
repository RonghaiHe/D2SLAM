#include <d2common/d2state.hpp>
#include <swarm_msgs/drone_trajectory.hpp>

using namespace D2Common;

namespace D2PGO {
class PGOState : public D2State {
protected:
    std::map<int, std::vector<VINSFrame*>> drone_frames;
    std::map<int, Swarm::DroneTrajectory> ego_drone_trajs;

public:
    PGOState(int _self_id, bool _is_4dof = false) :
        D2State(_self_id, _is_4dof) {
        if (is_4dof) {
            printf("[D2PGO] PGOState: is 4dof\n");
        } else {
            printf("[D2PGO] PGOState: is 6dof\n");
        }
        drone_frames[self_id] = std::vector<VINSFrame*>();
    }

    void addFrame(const VINSFrame & _frame) {
        printf("[D2PGO@%d] PGOState: add frame %ld for drone %d\n", self_id, _frame.frame_id, _frame.drone_id);
        auto frame = addVINSFrame(_frame);
        if (drone_frames.find(_frame.drone_id) == drone_frames.end()) {
            drone_frames[_frame.drone_id] = std::vector<VINSFrame*>();
            ego_drone_trajs[_frame.drone_id] = Swarm::DroneTrajectory();
        }
        drone_frames.at(_frame.drone_id).push_back(frame);
        ego_drone_trajs[_frame.drone_id].push(frame->stamp, frame->initial_ego_pose, frame->frame_id);
    }

    int size(int drone_id) {
        if (drone_frames.find(drone_id) == drone_frames.end()) {
            return 0;
        }
        return drone_frames.at(drone_id).size();
    }

    std::vector<VINSFrame*> & getFrames(int drone_id) {
        return drone_frames.at(drone_id);
    }

    Swarm::DroneTrajectory & getTraj(int drone_id) {
        return ego_drone_trajs.at(drone_id);
    }

    FrameIdType headId(int drone_id) {
        if (drone_frames.find(drone_id) == drone_frames.end() || 
            drone_frames.at(drone_id).size() == 0) {
            return -1;
        }
        return drone_frames.at(drone_id)[0]->frame_id;
    }

    void syncFromState() {
        const Guard lock(state_lock);
        for (auto it : _frame_pose_state) {
            auto frame_id = it.first;
            if (frame_db.find(frame_id) == frame_db.end()) {
                printf("[D2VINS::D2EstimatorState] Cannot find frame %ld\033[0m\n", frame_id);
            }
            auto frame = frame_db.at(frame_id);
            if (is_4dof) {
                frame->odom.pose().from_vector(it.second, true);
            } else {
                frame->odom.pose().from_vector(it.second);
            }
        }
    }
};
}