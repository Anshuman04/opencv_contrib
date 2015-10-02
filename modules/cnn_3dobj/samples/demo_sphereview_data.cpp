/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**
 * @file demo_sphereview_data.cpp
 * @brief Generating training data for CNN with triplet loss.
 * @author Yida Wang
 */
#include <opencv2/cnn_3dobj.hpp>
#include <opencv2/viz/vizcore.hpp>
#include <iostream>
#include <stdlib.h>
using namespace cv;
using namespace std;
using namespace cv::cnn_3dobj;
int main(int argc, char *argv[])
{
    const String keys = "{help | | demo :$ ./sphereview_test -ite_depth=2 -plymodel=../data/3Dmodel/ape.ply -imagedir=../data/images_all/ -labeldir=../data/label_all.txt -num_class=6 -label_class=0, then press 'q' to run the demo for images generation when you see the gray background and a coordinate.}"
    "{ite_depth | 2 | Iteration of sphere generation.}"
    "{plymodel | ../data/3Dmodel/ape.ply | Path of the '.ply' file for image rendering. }"
    "{imagedir | ../data/images_all/ | Path of the generated images for one particular .ply model. }"
    "{labeldir | ../data/label_all.txt | Path of the generated images for one particular .ply model. }"
    "{cam_head_x | 0 | Head of the camera. }"
    "{cam_head_y | 0 | Head of the camera. }"
    "{cam_head_z | -1 | Head of the camera. }"
    "{image_size | 128 | Size of captured images. }"
    "{label_class | 1 | Class label of current .ply model. }"
    "{label_item | 1 | Item label of current .ply model. }"
    "{rgb_use | 0 | Use RGB image or grayscale. }"
    "{num_class | 6 | Total number of classes of models. }"
    "{binary_out | 0 | Produce binaryfiles for images and label. }";
    /* Get parameters from comand line. */
    cv::CommandLineParser parser(argc, argv, keys);
    parser.about("Generating training data for CNN with triplet loss");
    if (parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }
    int ite_depth = parser.get<int>("ite_depth");
    string plymodel = parser.get<string>("plymodel");
    string imagedir = parser.get<string>("imagedir");
    string labeldir = parser.get<string>("labeldir");
    int label_class = parser.get<int>("label_class");
    int label_item = parser.get<int>("label_item");
    float cam_head_x = parser.get<float>("cam_head_x");
    float cam_head_y = parser.get<float>("cam_head_y");
    float cam_head_z = parser.get<float>("cam_head_z");
    int image_size = parser.get<int>("image_size");
    int rgb_use = parser.get<int>("rgb_use");
    int num_class = parser.get<int>("num_class");
    int binary_out = parser.get<int>("binary_out");
    cv::cnn_3dobj::icoSphere ViewSphere(10,ite_depth);
    std::vector<cv::Point3d> campos = ViewSphere.CameraPos;
    std::fstream imglabel;
    char* p=(char*)labeldir.data();
    imglabel.open(p, fstream::app|fstream::out);
    bool camera_pov = true;
    /* Create a window using viz. */
    viz::Viz3d myWindow("Coordinate Frame");
    /* Set window size as 64*64, we use this scale as default. */
    myWindow.setWindowSize(Size(image_size,image_size));
    /* Set background color. */
    myWindow.setBackgroundColor(viz::Color::gray());
    myWindow.spin();
    /* Add light. */
    myWindow.addLight(Vec3d(0,0,100000), Vec3d(0,0,0), viz::Color::white(), viz::Color::gray(), viz::Color::black(), viz::Color::white());
    /* Create a Mesh widget, loading .ply models. */
    viz::Mesh objmesh = viz::Mesh::load(plymodel);
    /* Get the center of the generated mesh widget, cause some .ply files.  */
    Point3d cam_focal_point = ViewSphere.getCenter(objmesh.cloud);
    float radius = ViewSphere.getRadius(objmesh.cloud, cam_focal_point);
    objmesh.cloud = objmesh.cloud/radius*100;
    cam_focal_point = cam_focal_point/radius*100;
    Point3d cam_y_dir;
    cam_y_dir.x = cam_head_x;
    cam_y_dir.y = cam_head_y;
    cam_y_dir.z = cam_head_z;
    const char* headerPath = "../data/header_for_";
    const char* binaryPath = "../data/binary_";
    if (binary_out)
    {
        ViewSphere.createHeader((int)campos.size(), image_size, image_size, headerPath);
    }
    char* temp = new char;
    /* Images will be saved as .png files. */
    for(int pose = 0; pose < (int)campos.size(); pose++){
        int label_x, label_y, label_z;
        label_x = (int)(campos.at(pose).x*100);
        label_y = (int)(campos.at(pose).y*100);
        label_z = (int)(campos.at(pose).z*100);
        sprintf (temp,"%02d_%02d_%04i_%04i_%04i", label_class, label_item, label_x, label_y, label_z);
        string filename = temp;
        filename += ".png";
        imglabel << filename << ' ' << label_class << endl;
        filename = imagedir + filename;
        /* Get the pose of the camera using makeCameraPoses. */
        Affine3f cam_pose = viz::makeCameraPose(campos.at(pose)*380+cam_focal_point, cam_focal_point, cam_y_dir*380+cam_focal_point);
        /* Get the transformation matrix from camera coordinate system to global. */
        Affine3f transform = viz::makeTransformToGlobal(Vec3f(1.0f,0.0f,0.0f), Vec3f(0.0f,1.0f,0.0f), Vec3f(0.0f,0.0f,1.0f), campos.at(pose));
        viz::WMesh mesh_widget(objmesh);
        /* Pose of the widget in camera frame. */
        Affine3f cloud_pose = Affine3f().translate(Vec3f(1.0f,1.0f,1.0f));
        /* Pose of the widget in global frame. */
        Affine3f cloud_pose_global = transform * cloud_pose;
        /* Visualize camera frame. */
        if (!camera_pov)
        {
            viz::WCameraPosition cpw(1); // Coordinate axes
            viz::WCameraPosition cpw_frustum(Vec2f(0.5, 0.5)); // Camera frustum
            myWindow.showWidget("CPW", cpw, cam_pose);
            myWindow.showWidget("CPW_FRUSTUM", cpw_frustum, cam_pose);
        }

        /* Visualize widget. */
        mesh_widget.setRenderingProperty(viz::LINE_WIDTH, 4.0);
        myWindow.showWidget("ape", mesh_widget, cloud_pose_global);

        /* Set the viewer pose to that of camera. */
        if (camera_pov)
            myWindow.setViewerPose(cam_pose);
        /* Save screen shot as images. */
        myWindow.saveScreenshot(filename);
        if (binary_out)
        {
        /* Write images into binary files for further using in CNN training. */
            ViewSphere.writeBinaryfile(filename, binaryPath, headerPath,(int)campos.size()*num_class, label_class, (int)(campos.at(pose).x*100), (int)(campos.at(pose).y*100), (int)(campos.at(pose).z*100), rgb_use);
        }
    }
    imglabel.close();
    return 1;
};
