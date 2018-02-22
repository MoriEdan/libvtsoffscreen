#ifndef vtsoffscreen_snapper_hpp_included_
#define vtsoffscreen_snapper_hpp_included_

#include <atomic>
#include <future>
#include <mutex>
#include <thread>
#include <queue>
#include <memory>

#include <opencv2/core/core.hpp>

#include "optics/camera.hpp"
#include "geo/srsdef.hpp"
#include "glsupport/eglfwd.hpp"

namespace vts::offscreen {

using Image = cv::Mat_<cv::Vec3b>;

struct Config {
    std::string mapConfigUrl;

    std::string authUrl;

    /** Custom SRS #1, passed to createf VTS map.
     */
    geo::SrsDefinition customSrs1;

    /** Custom SRS #1, passed to createf VTS map.
     */
    geo::SrsDefinition customSrs2;
};

/** View definition.
 */
struct View {
    /** Intrinsic parameters.
     */
    optics::Camera::Parameters camera;

    /** Extrinsic parameters.
     */
    optics::Camera::Position position;

    /** Viewport definition
     */
    optics::Camera::Viewport viewport;

    /** Keypoints to sample in the scene.
     */
    math::Points2 keypoints;

    View() {}
};

struct Point {
    math::Point2 image;
    math::Point3 world;

    typedef std::vector<Point> list;

    Point(const math::Point2 &image, const math::Point3 &world)
        : image(image), world(world)
    {}
};

/** Photographed snapshot.
 */
struct Snapshot {
    /** Take photograph.
     */
    Image image;

    /** Samples keypoints.
     */
    Point::list keypoints;

    Snapshot(const math::Size2 &size);
};

class Snapper : public boost::noncopyable {
public:
    /** Run snapper on default native display.
     */
    Snapper(const Config &config);

    /** Run snapper on provided EGL device.
     */
    Snapper(const Config &config, const glsupport::egl::Device &device);

    ~Snapper();

    Snapshot snap(const View &view);

    struct Detail;

private:
    std::unique_ptr<Detail> detail_;
};

/** Asynchronous version.
 */
class AsyncSnapper : public boost::noncopyable {
public:
    AsyncSnapper(const Config &config);

    ~AsyncSnapper();

    Snapshot operator()(const View &view);

private:
    struct Request {
        View view;

        std::promise<Snapshot> promise;

        Request(const View &view)
            : view(view)
        {}
    };

    void stop();

    void worker(int threadId, const Config &config
                , const glsupport::egl::Device &device);

    std::vector<std::thread> threads_;
    std::atomic<bool> running_;

    std::queue<Request> requests_;
    std::condition_variable requestsCond_;
    std::mutex requestsMutex_;
};

} // namespace vts::offscreen

#endif // vtsoffscreen_snapper_hpp_included_
