#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "drawable_cloud.h"
#include "drawable_line.h"
#include "drawable_selectable_cloud.h"

#include <QColor>
#include <QDebug>
#include <QFileDialog>
#include <QImage>
#include <QPixmap>
#include <QUuid>
#include <QRadioButton>
#include <QLabel>
// save to txt file
#include <fstream>
#include <ostream>

MainWindow::MainWindow(QWidget *parent):
    // QWidget(parent),
    BaseViewerWidget(parent),
    ui(new Ui::MainWindow),
    isFullScreen(false)
    // ui(new Ui::OpenGlFolderPlayer)
{
    angle_threshold = 10;
    depthImagefilter = false;
    girdImageResize = 1;
    lShpaeHorizonResolution = 0.08;
    infoTextEdit = new QTextEdit;
    this->playCloud = false;
    // 设置开始位置
    // this->curr_data_idx = 0;
    curr_data_idx = 0;
    ui->setupUi(this);

    horizontalLayout_tracking = new QHBoxLayout();
    horizontalLayout_tracking->setSpacing(6);
    horizontalLayout_tracking->setObjectName(QStringLiteral("horizontalLayout_tracking"));
    trackIDSB = new QSpinBox(ui->layoutWidget_3);
    // trackIDSB = new QSpinBox();
    trackIDSB->setObjectName(QStringLiteral("trackIDSB"));
    horizontalLayout_tracking->addWidget(trackIDSB);
    trackIDSB->setRange(0, 10000);
    trackIDSB->setValue(9999);

    // trackLB = new QLabel(ui->layoutWidget_3);
    // trackLB->setObjectName(QStringLiteral("trackId"));
    // horizontalLayout_tracking->addWidget(trackLB);
    ui->verticalLayout_3->addLayout(horizontalLayout_tracking);
    //
    connect(ui->openFolderBT, SIGNAL(released()), this, SLOT(onOpenFolderToRead()));
    connect(ui->DataIdxVSlider, SIGNAL(valueChanged(int)), this, SLOT(onSliderMovedTo(int)));
    connect(ui->playBT, SIGNAL(released()), this, SLOT(onPlayClouds()));
    connect(ui->resetBT, SIGNAL(released()), this, SLOT(onReset()));

    // auto it = ui->groundRB;
    // 定义的控件， 全部都在 ui 中直接找到
    connect(ui->cloudCB, SIGNAL(toggled(bool)), this, SLOT(onUpdateShow()));
    connect(ui->groundCB, SIGNAL(toggled(bool)), this, SLOT(onUpdateShow()));
    connect(ui->obstacleCB, SIGNAL(toggled(bool)), this, SLOT(onUpdateShow()));

    connect(ui->updatePB, SIGNAL(released()), this, SLOT(onUpdate()));

    connect(ui->paramDSB, SIGNAL(valueChanged(double)), this, SLOT(onParamSet()));
    connect(ui->clearSelectionPB, SIGNAL(released()), this, SLOT(onClearSelection()));
    connect(ui->girdNumSB, SIGNAL(valueChanged(int)), this, SLOT(onUpdateShow(int)));

    connect(this, SIGNAL(fullScreenSignals()), this, SLOT(onUpdateShow()));
    // connect(ui->infoTab->curr
    // auto it = ui->infoTab->widget(0)->findChild<QRadioButton>("groundRB");

    // connect()
    // 读取数据
    infoTextEdit->clear();
    infoTextEdit->setPlaceholderText("data log");
    infoTextEdit->setReadOnly(true);
    infoTextEdit->document()->setMaximumBlockCount(200);

    
    // ui->CloudViewer->setBackgroundColor(QColor(1.0f, 1.0f, 1.0f));    
    _viewer = ui->CloudViewer;
    // connect(ui->CloudViewer, SIGNAL(fullScreen(bool)), this, SLOT(onUpdateShow(bool)));
    // fprintf(stderr, "_viewer = ui->CloudViewer;\n");
    _viewer->installEventFilter(this);
    _viewer->setAutoFillBackground(true);

    ui->resetBT->setEnabled(false);
    ui->DataIdxSBox->setEnabled(false);
    ui->DataIdxVSlider->setEnabled(false);
    ui->playBT->setEnabled(false);

    // 设置尺寸百分比
    // infoshow 的
    // qDebug() << ui->width();
    // ui->infoTab->setTabText(0, "infoShow");
    // ui->infoTab->clear();
    ui->infoTab->insertTab(0, infoTextEdit, tr("infoShow"));
    ui->infoTab->setTabText(1, tr("choose"));    
    ui->infoTab->setTabText(2, tr("params"));
    ui->infoTab->setCurrentIndex(0);
    // 
    ui->cloudCB->setChecked(false);
    ui->groundCB->setChecked(true);
    ui->obstacleCB->setChecked(false);
    ui->insertCB->setChecked(false);
    ui->lineCB->setChecked(false); 
    ui->updatePB->setEnabled(false);

    ui->clusterCB->setChecked(true);
    ui->voxelCB->setChecked(false);
    ui->bboxCB->setChecked(false);
    // param adjust
    ui->paramDSB->setRange(-1000, 1000);
    ui->paramDSB->setSingleStep(0.01);
    ui->paramDSB->setDecimals(4);
    ui->clearSelectionPB->setEnabled(false);

    // 数据 序列显示
    ui->dataSeqSB->setValue(20);
    ui->dataSeqSB->setRange(0, 22);

    // 创建图像显示窗口
    ui->girdNumSB->setSingleStep(60);
    ui->girdNumSB->setRange(120, 1200);
    ui->girdNumSB->setValue(500);
    // 
    ui->bboxCB->setChecked(true);

    // trackId = trackIDSB->value();
    // int showImageGV_x = (ui->CloudViewer->width() - ui->showImageGV->width()) / 2;
    // ui->showImageGV->move(showImageGV_x, 0);
    // _graphView.reset(new QGraphicsView);
    //     //
    // _graphView->setStyleSheet("padding:0px;border:0px");
    dock_Image = new QDockWidget(tr("Image"), this);
    addDockWidget(Qt::TopDockWidgetArea,dock_Image);
    dock_Image->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    dock_Image->setAllowedAreas(Qt::AllDockWidgetAreas);
    imgLabel = new QLabel(dock_Image);
    imgLabel->setScaledContents(true);
    dock_Image->setFloating(true);

    // ----------------------  update matrix --------------------
      // 读取参数信息
    std::ifstream timeStream;
	std::ifstream poseStream;
	string lineStr;
	char ch;
	string timeFileDir = _params.kitti_base_velo_dir + "20/info/timestamp.txt";
	std::cout << timeFileDir <<std::endl;
	timeStream.open(timeFileDir, std::ios::in);
	if (timeStream.fail())
	{
		fprintf(stderr, "open timeStream error\n");
        exit(0);
	}
	// while(timeStream >> ch)
	while(timeStream >> lineStr)
	{
		// std::cout << ch << std::endl;
		// std::cout << lineStr << std::endl;
		timestampVec.emplace_back(std::stof(lineStr));
	}
	// for (int idx = 0; idx < timestamp.size(); ++idx)
	// {
	// 	fprintf(stderr, "%f\n", timestamp[idx]);
	// }
	// fprintf(stderr, "num of timestamp : %d\n", timestamp.size());
	timeStream.close();
	// -------------------------------------------------------------
	string poseFileDir = _params.kitti_base_velo_dir + "20/info/pose.txt";
	poseStream.open(poseFileDir, std::ios::in);
	if (poseStream.fail())
	{
		fprintf(stderr, "open poseStream error\n");
		exit(1);
	}
	int count = 0;
	while (getline(poseStream, lineStr, '\n'))
	{
		string str;
		int strIdx = 0;
		std::array<float, 3> ps;
		stringstream ss(lineStr);
		// while(getline(ss, str, ' '))
		// {
		// 	ps[strIdx] = std::stof(str);
		// 	strIdx++;
		// }
		ss >> ps[0] >> ps[1] >> ps[2];
		selfCarPose.emplace_back(ps);
	}
    
    hisRefPoints.reset(new Cloud);
    // ----------------------------------------------------------
    // dock_Image->setWindowFlags(Qt::WindowStaysOnTopHint);

    // dock_cluster_image = new QDockWidget(tr("Imagecluster"), this);
    // addDockWidget(Qt::TopDockWidgetArea,dock_cluster_image);
    // dock_cluster_image->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    // dock_cluster_image->setAllowedAreas(Qt::AllDockWidgetAreas);
    // cluster_image = new QLabel(dock_cluster_image);
    // cluster_image->setScaledContents(true);
    // dock_cluster_image->setFloating(true);

    // dockshow_depth_image = new QDockWidget(tr("ImageDepth"), this);
    // addDockWidget(Qt::TopDockWidgetArea, dockshow_depth_image);
    // dockshow_depth_image->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    // dockshow_depth_image->setAllowedAreas(Qt::AllDockWidgetAreas);
    // depth_image = new QLabel(dockshow_depth_image);
    // depth_image->setScaledContents(true);
    // dockshow_depth_image->setFloating(true);

    // dockshow_depth_image2 = new QDockWidget(tr("ImageDepth2"), this);
    // addDockWidget(Qt::TopDockWidgetArea, dockshow_depth_image2);
    // dockshow_depth_image2->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    // dockshow_depth_image2->setAllowedAreas(Qt::AllDockWidgetAreas);
    // depth_image2 = new QLabel(dockshow_depth_image2);
    // depth_image2->setScaledContents(true);
    // dockshow_depth_image2->setFloating(true); 

    // dockshow_depth_image->close();
    // dockshow_depth_image2->close();

}

MainWindow::~MainWindow()
{
    // delete ui;
    delete infoTextEdit;
    delete dock_Image;
    delete imgLabel;
    delete ObjSelectCB;
    // delete dockshow_depth_image;
    // delete dock_cluster_image;
    delete infoTextEdit;
    delete cluster_image;
    delete depth_image;
    delete depth_image2;
}

void MainWindow::keyPressEvent(QKeyEvent *event) 
{
    fprintf(stderr, "key value [%d]\n", event->key());
    switch (event->key())
    {
        case Qt::Key_Right:
            ui->DataIdxSBox->setValue(ui->DataIdxSBox->value() + 1);
            fprintf(stderr, "Key_Right pressed \n");
            break;
        case Qt::Key_Left:
            ui->DataIdxSBox->setValue(ui->DataIdxSBox->value() - 1);
            fprintf(stderr, "Key_Left pressed \n");
            break;

        case Qt::Key_1:
            fprintf(stderr, "key 1 pressed\n");
            isFullScreen = (isFullScreen == true) ? false : true;
            onUpdate();
            break;

        case Qt::Key_2:
            fprintf(stderr, "key 2 pressed, onUpdate\n");
            onSliderMovedTo(curr_data_idx);
            break;

        case Qt::Key_3:
            fprintf(stderr, "key 3 pressed, clear selsection\n");
            onClearSelection();
            onSliderMovedTo(curr_data_idx);
            break;

        case Qt::Key_Space:
            onPlayClouds();
            break;
    }
}

void MainWindow::onOpenFolderToRead()
{
    int dataSeq = ui->dataSeqSB->value();
    std::string kitti_img_dir = 
        "/home/yyg/code/lidarCode/lidarVisualization/data_odometry_color/dataset/sequences/" +
            std::to_string(dataSeq) + "/image_2";

    std::string kitti_velo_dir = 
        "/home/yyg/code/lidarCode/lidarVisualization/data_odometry_velodyne/dataset/sequences/" +
            std::to_string(dataSeq) + "/velodyne";

    // QString folder_name = QFileDialog::getExistingDirectory(this, "打开数据路径",
    // QString::fromStdString(_params.kitti_velo_dir));
    // QString folder_name = QString::fromStdString(_params.kitti_velo_dir);
    // if (folder_name.size() == 0) return; // no dir choosed
    //ui->infoTextEdit->append(QString::fromStdString("Picked path: " + folder_name.toStdString()));
    moveCursorToEnd();

    // qDebug() << "Picked path:" << folder_name;

    _file_names_velo.clear();
    _file_names_img.clear();

    // utils::ReadKittiFileByDir(folder_name.toStdString(), _file_names_velo);
    utils::ReadKittiFileByDir(kitti_velo_dir, _file_names_velo);
    utils::ReadKittiFileByDir(kitti_img_dir, _file_names_img);
    // fprintf(stderr, "_file_names_velo.size %d, _file_names_imgs %d\n",
    //         _file_names_velo.size(), _file_names_img.size());
    // utils::ReadKittiFileByDir(_params.kitti_img_dir, _file_names_img);
    std::sort(_file_names_velo.begin(), _file_names_velo.end()); // 对文件夹排序
    std::sort(_file_names_img.begin(), _file_names_img.end());
    numData = _file_names_velo.size();
    if (numData > 0)
    {
        infoTextEdit->append(QString::fromStdString(_file_names_velo[0]));
        infoTextEdit->append(QString::fromStdString(_file_names_img[0]));
    }
    moveCursorToEnd();
    ui->DataIdxVSlider->setMaximum(numData - 1);
    ui->DataIdxSBox->setMaximum(numData - 1);
    _viewer->update();
    ui->DataIdxSBox->setEnabled(true);
    ui->DataIdxVSlider->setEnabled(true);
    ui->playBT->setEnabled(true);
    ui->updatePB->setEnabled(true);
    ui->clearSelectionPB->setEnabled(true);
    ui->resetBT->setEnabled(true);
}

// onPlayClouds 函数会改变 VSlider 的值， 所以会触发控件对应的槽函数 onSliderMovedTo 
// 所以在这个函数里面读取点云 并且显示即可

void MainWindow::onPlayClouds()
{
    if (this->playCloud == false)
    {
        this->playCloud = true;
        ui->playBT->setText("stop");
    }
    else
    {
        ui->playBT->setText("start");
        this->playCloud = false;
    }

    for (int i = curr_data_idx; i < ui->DataIdxVSlider->maximum(); ++i)
    {
        if (!playCloud) return;
        curr_data_idx = i; // recselectObjectIDsord current data Idx
        ui->DataIdxVSlider->setValue(i);
        ui->CloudViewer->update();
        QApplication::processEvents();
    }
}

void MainWindow::onReset()
{
    // ui->resetBT->setEnabled(false);
    curr_data_idx = 0;
    ui->DataIdxVSlider->setValue(curr_data_idx);
    ui->DataIdxSBox->setValue(curr_data_idx);
}

void MainWindow::onSliderMovedTo(int cloud_number)
{
    if(_file_names_velo.empty())
        return;
    this->curr_data_idx = cloud_number;
    ui->DataIdxSBox->setValue(curr_data_idx);
    ui->DataIdxVSlider->setValue(curr_data_idx);
    
    const auto &file_name = _file_names_velo[cloud_number];
    _cloud = utils::ReadKittiBinCloudByPath(file_name);
    // fprintf(stderr, "num Point %d\n", _cloud->size());
    assert(_cloud->size() != 0);
    fprintf(stderr, "\n\n------------------------------>\n");
    infoTextEdit->append("read bin file from: " + QString::fromStdString(file_name));
    infoTextEdit->append("current frame is: " + QString::number(cloud_number));
    fprintf(stderr, "current frame is %d\n", cloud_number);
    moveCursorToEnd();   

    // 添加点云显示
    // _viewer->Clear();
    // _viewer->AddDrawable(DrawableCloud::FromCloud(_cloud));
    // _viewer->update();

    // if (this->curr_data_idx == numData - 1)
    //     ui->resetBT->setEnabled(true);

    // 去地
    // fprintf(stderr, "<<<<<<<<<<<<<<<-------------------------------\n");
    SegmentaionNode groundRemove(params_groundRemove);

    // fprintf(stderr, "params_groundRemove.line_search_angle : %f\n", params_groundRemove.line_search_angle);
    // fprintf(stderr, "params_groundRemove.max_slope : %f\n", params_groundRemove.max_slope);
    

    // 查看点击的位置 跟新点击的位置
    // double poseX, poseY;
    // _viewer->getClickedPoint(poseX, poseY);
    // fprintf(stderr, "clicked(%f, %f)\n", poseX, poseY);
    // groundRemove.setClickedPoint(poseX, poseY);
    /////
    // 结束
    Cloud::Ptr ground_cloud(new Cloud);
    Cloud::Ptr obstacle_cloud(new Cloud);
    // Cloud ground_cloud, obstacle_cloud;
    Cloud::Ptr cloudTmp(new Cloud);
    groundRemove.scanCallBack(*_cloud, *ground_cloud, *obstacle_cloud);    
    // fprintf(stderr, "------------------1\n");
    // 获取选择的 ID
    // if (_viewer->selection.size())
    if (0)
    {
        fprintf(stderr, "_viewer->selection has size :%d\n", _viewer->selection.size());
        for (const int & elem : _viewer->selection)
        {
            cloudTmp->emplace_back((*obstacle_cloud)[elem]);
        }

        obstacle_cloud->clear();
        obstacle_cloud = cloudTmp;        
        fprintf(stderr, "obstacle_cloud has point size :%d\n", obstacle_cloud->size());
    }
    // if (_viewer->selection.size())
    // {
    //     obstacle_cloud->clear();
    //     groundRemove.setSelectObjectID(_viewer->selection);

    //     // 开启 debug 模式
    //     for (const int & elem : _viewer->selection)
    //     {
    //         obstacle_cloud->emplace_back((*_cloud)[elem]);
    //     }        
    // }
    // fprintf(stderr, "------------------------------->>>>>>>>>>>>>>>\n");
    // depth_clustering -----------------------------------------
    // 开启调试模式
    if (obstacle_cloud->size() && _viewer->selection.size() != 0)
    {
        fprintf(stderr, "has choosed %d debug point\n", obstacle_cloud->size());
    }
    
    // 聚类 ------------------------------------------------------
    GLfloat pointSize(1.8);
    Cloud::Ptr insertCloud(new Cloud);
    cv::Mat visClusterImg;
    int numGrid = ui->girdNumSB->value();
    float roiM = _params.max_dist, numCluster = 0, kernelSize = 3;
    cluster cluster(roiM, numGrid, numCluster, kernelSize, *obstacle_cloud,  _params);
    
    // 添加各种显示前， 清楚 _drawables--> vector
    _viewer->Clear();

    std::vector<Rect2D> rect2DVec;
    std::vector<Cloud::Ptr> clusters; 
    std::vector<Cloud::Ptr> bboxPts;  // minAre + pca
    // std::vector<Cloud::Ptr> bboxPts2; // L_shape
    Cloud::Ptr markPoints(new Cloud); // 标记的点

    
    if (ui->clusterCB->isChecked())
    {
        std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
        cluster.componentClustering();     
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> fp_ms = end - start;
        std::cout << "Cluster took about " << fp_ms.count() << " ms" << std::endl;              
        // cluster.getClusterImg(visClusterImg);
        cluster.makeClusteredCloud(*obstacle_cloud, clusters);

        // 找出最佳的 最表面的 clusters 
        int bboxDebugId = -1;
        if (ui->bboxCB->isChecked())
        {
            if (_viewer->bboxSelection.size() > 0)
            {
                bboxDebugId = bboxToCluster[_viewer->bboxSelection[0]];
            }
        }
        Cloud::Ptr lShapePoints (new Cloud);
        // cluster.getLShapePoints(clusters, lShapePoints, bboxDebugId, lShpaeHorizonResolution);
        
        // fprintf(stderr, "------------------2\n");
        rect2DVec = cluster.getRectVec();
        if (ui->voxelCB->isChecked())
        {
            _viewer->AddDrawable(DrawableRect::FromRectVec(rect2DVec), "DrawAbleRect");
        }

        // bbox 拟合 L-shape fitting
        if (ui->bboxCB->isChecked())
        {
            // getBoundingBox(clusters, bboxPts);
            // getBBox(clusters, bboxPts);
            // fprintf(stderr, "------------------3\n");
            // L_shape bbox
            // 找出 选择的 BBox
            // int bboxDebugId = -1;
            // if (_viewer->bboxSelection.size() > 0)
            // {
            //     bboxDebugId = bboxToCluster[_viewer->bboxSelection[0]];
            // }
            // 将 bboxPts 投影到全局坐标系
            updateTransMatrix(curr_data_idx);
            // fprintf(stderr, "updateTransMatrix(curr_data_idx);--------->\n");
            std::chrono::high_resolution_clock::time_point start_bbox = std::chrono::high_resolution_clock::now();
            getBBox(clusters, bboxPts, markPoints, lShapePoints ,bboxToCluster, lShpaeHorizonResolution ,bboxDebugId);
            Eigen::Vector3f color;
            float pointSize = 3.2f;
            color << 1.0, 1.0, 1.0;
            transCloudL2G(lShapePoints);
            _viewer->AddDrawable(DrawableCloud::FromCloud(lShapePoints, color, pointSize), "lShapePoints");
            // 对比方法
            // getOrientedBBox(clusters, bboxPts2);
            std::chrono::high_resolution_clock::time_point end_bbox = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> fp_ms_bbox = end_bbox - start_bbox;
            std::cout << "boundingbox took about " << fp_ms_bbox.count() << " ms" << std::endl;            
            // fprintf(stderr, "------------------4\n");
            // fprintf(stderr, "drawSelectedBBox objects size : %d\n", _viewer->drawSelectableBBox.objects.size());
            // _viewer->AddDrawable(DrawableBBox::FromCloud(bboxPts, true));
            // 转换到全局坐标系

            transCloudL2G(bboxPts, 8);

            // fprintf(stderr, "transCloudL2G(bboxPts);------------->\n");
            _viewer->drawSelectableBBox = DrawSelectAbleBBox(bboxPts, true);
            // _viewer->setBBoxs(bboxPts);  为了显示 id 设置给全局使用的
            // 检测可选择的框
            Eigen::Vector3f detectBBoxColor(67.0f/255, 141.0f/255, 43.0f/255);
            // Eigen::Vector3f detectBBoxColor(0.0f, 0.0f, 0.0f);
            // fprintf(stderr, "drawSelectedBBox objects size : %d\n", _viewer->drawSelectableBBox.objects.size());
            _viewer->AddDrawable(DrawSelectAbleBBox::FromCloud(bboxPts, false, detectBBoxColor), "DrawSelectAbleBBox");
            // 对比方法 bbox
            // _viewer->AddDrawable(DrawableBBox::FromCloud(bboxPts2, true, 1));
            // 显示三个端点
            transCloudL2G(markPoints);
            _viewer->AddDrawable(DrawableCloud::FromCloud(markPoints, Eigen::Vector3f(0.0f, 1.0f, 0.2f),
                     GLfloat(6)),"L_shape markPoints");
            // 保留历史的 TrackHis point， // 添加历史 ref 点的显示
            
            // fprintf(stderr, "before add history ref point : %d!\n", hisRefPoints->size());
            Cloud::Ptr hisMarkerTmp(new Cloud);
            (*hisMarkerTmp) = (*hisRefPoints);
            _viewer->AddDrawable(DrawableCloud::FromCloud(hisMarkerTmp, Eigen::Vector3f(0.0f, 1.0f, 0.2f),
                     GLfloat(6)), "history ref point");
            hisRefPoints->clear();
            // fprintf(stderr, "hisMarkerTmp has %d points\n", hisMarkerTmp->size());
            // hisRefPoints->swap();
            // fprintf(stderr, "add history ref point!\n");
            for (int markerIdx = 0; markerIdx < markPoints->size(); ++markerIdx)
            {
                auto & cloud_marker = (*markPoints);
                point & currMarkerPt = cloud_marker[markerIdx];
                if (currMarkerPt.ptType == pointType::TRACK)
                {
                    hisRefPoints->emplace_back(point(currMarkerPt.x(), currMarkerPt.y(), currMarkerPt.z(), pointType::TRACKHIS));
                }
            }
            // fprintf(stderr, "add history ref point finished!\n");
            // fprintf(stderr, "after hisRefPoints size : %d\n", hisRefPoints->size());
        }
        
        infoTextEdit->append("number of cluster : " + QString::number(cluster.getNumCluster()));
        infoTextEdit->append("number of non-empty voxels : " + QString::number(rect2DVec.size()));
        moveCursorToEnd();

        /*imm_ukf_pda_tracking part*/   
        std::chrono::high_resolution_clock::time_point start_tracker = std::chrono::high_resolution_clock::now();     
        std::vector<Cloud::Ptr> trackerBBoxPts;  // minAre + pca
        // 为了链接前一状态与预测状态的 虚线的点
        Cloud::Ptr connectPoints(new Cloud);
        std::vector<rotateEllipse> roateEllipseVec;
        Eigen::Vector3f trackerBBoxColor(0.0f, 1.0f, 0.0f);


        // fprintf(stderr, "tracker start\n");
        tracker.callback(CloudToBBoxs(bboxPts), curr_data_idx, 
                trackerBBoxPts, timestampVec[curr_data_idx],trackIDSB->value(), connectPoints, roateEllipseVec, getMatrixL2G());
        // fprintf(stderr, "tracker finished with tracker bbox : %d\n", trackerBBoxPts.size());
        // _viewer->AddDrawable(DrawSelectAbleBBox::FromCloud(trackerBBoxPts, false, trackerBBoxColor), "DrawSelectAbleTrackerBBox");
        // for (int idx = 0; idx < trackerBBoxPts.size(); ++idx)
        // {
        //     auto & bbox = (*trackerBBoxPts[idx]);
        //     fprintf(stderr, "trackerBBoxPts size %d\n", trackerBBoxPts.size());
        //     fprintf(stderr, "(%f, %f)\n", bbox[0].x(), bbox[0].y());
        // }
        Eigen::Vector3f colorConnect;
        colorConnect << 0.0f, 1.0f, 0.0f;
        _viewer->AddDrawable(DrawableTracking::FromCloud(connectPoints, colorConnect, 1), "DrawableTracking");
        _viewer->AddDrawable(DrawableBBox::FromCloud(hisBBoxs, false, 1), "DrawSelectAbleTrackerBBox");

        _viewer->AddDrawable(DrawableEllipse::FromEllipseVec(roateEllipseVec), "DrawableEllipse");
        std::vector<Cloud::Ptr>().swap(hisBBoxs); // 清除历史记录 bbox
        hisBBoxs = trackerBBoxPts;
        // ---------------------------------   显示历史 bbox ----------------------------------------------
        _viewer->AddDrawable(DrawableBBox::FromCloud(trackerBBoxPts, false, 0), "DrawSelectAbleTrackerBBox");
        std::chrono::high_resolution_clock::time_point end_tracker = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> fp_ms_tracker = end_tracker - start_tracker;
        std::cout << "ImmUkfPda tracker took about " << fp_ms_tracker.count() << " ms" << std::endl;  
        // 强制刷新一次， 将当前添加的内容显示完全
        _viewer->setBBoxs(trackerBBoxPts); // 可视化id velocity 等信息 提供给全局使用
        _viewer->update();
        
    }

    cv::Mat visImage, depthImage;
    depth_clustering depthCluster(*obstacle_cloud, depthImagefilter, angle_threshold);
    if (ui->depthClusterCB->isChecked())
    {
        // depth_clustering depthCluster(* _cloud);
        depthCluster.depthCluster();
        depthCluster.LabelCloud(*obstacle_cloud);
        // cv::Mat depthImage = depthCluster.getVisualizeDepthImage();
        visImage = depthCluster.visSegmentImage();
        depthImage = depthCluster.getVisualizeDepthImage();
        // dock_cluster_image->show();
        // dockshow_depth_image2->show();
        // dockshow_depth_image->show();
        // 直接聚类
    }   


    // fprintf(stderr, "numClister :%d\n", cluster.getNumCluster());
    _viewer->drawSelectableCloud = DrawSelectAbleCloud(_cloud);

    if (ui->cloudCB->isChecked())
    {
        Eigen::Vector3f color;
        // color << 0.0, 1.0, 0.0;
        // color << 0.79f, 1.0f, 0.439f;
        color << 1.0f, 1.0f, 1.0f;
        // _viewer->AddDrawable(DrawableCloud::FromCloud(_cloud));
        // _viewer->AddDrawable(DrawSelectAbleCloud::FromCloud(_cloud, color, 2), "DrawSelectAbleCloud");
        transCloudL2G(_cloud);
        _viewer->AddDrawable(DrawableCloud::FromCloud(_cloud, color, 1.8f), "DrawSelectAbleCloud");
        // 为 viewer 的 drawSelectableCloud 赋值
        // _viewer->selection.clear();
    }

    if (ui->groundCB->isChecked())
    {
        Eigen::Vector3f color;
        // color << 0.0, 1.0, 0.0;
        color << 6.0f / 255, 4.0f / 255, 110.0f / 255;  // 蓝色地面
        transCloudL2G(ground_cloud);
        _viewer->AddDrawable(DrawableCloud::FromCloud(ground_cloud, color, pointSize), "DrawableCloud ground");
    }
    
    if (ui->obstacleCB->isChecked())
    {
        Eigen::Vector3f color;
        // color << 1.0, 0.0, 0.0;
        color << 1.0f, 0.0f, 0.0f;
        transCloudL2G(obstacle_cloud);
        _viewer->AddDrawable(DrawableCloud::FromCloud(obstacle_cloud, color, pointSize), "DrawableCloud obstacle");
    }

    if (ui->clusterCB->isChecked())
    {
        ui->depthClusterCB->setChecked(false);
        Eigen::Vector3f color;
        color << 0.5, 0.5, 0.3;
        transCloudL2G(obstacle_cloud);
        _viewer->AddDrawable(DrawableCloud::FromCloud(obstacle_cloud, color, pointSize, cluster.getNumCluster()), "clsuter cloud");
    }
        
    if (ui->depthClusterCB->isChecked())
    {
        ui->clusterCB->setChecked(false);
        Eigen::Vector3f color;
        color << 0.5, 0.5, 0.3;
        if (obstacle_cloud->size() < 200)
        {
            for (int idx = 0; idx < obstacle_cloud->size(); ++idx)
            {
                fprintf(stderr, "point id(%d) ---> (%f, %f, %f)[%d]\n", idx, 
                        (*obstacle_cloud)[idx].x(), (*obstacle_cloud)[idx].y(), (*obstacle_cloud)[idx].z(),
                        (*obstacle_cloud)[idx].classID);
            }
        }
        _viewer->AddDrawable(DrawableCloud::FromCloud(obstacle_cloud, color, pointSize, depthCluster.getNumCluster()));
    }

    // 是否显示插入点
    // 顺带可视化 圆柱体
    if (ui->insertCB->isChecked())
    {
        groundRemove.getInsertedPoint(*_cloud, *insertCloud);
        Eigen::Vector3f color;
        color << 0, 0, 1;
        GLfloat pointSize(3);
        _viewer->AddDrawable(DrawableCloud::FromCloud(insertCloud, color, pointSize), "insert cloud");
    }

    // 可视化线段的点
    Cloud::Ptr lines_cloud(new Cloud);
    if (ui->lineCB->isChecked())
    {
        groundRemove.getLinesPoint(*lines_cloud);
        Eigen::Vector3f color;
        color << 1.0, 1.0, 0.0;
        _viewer->AddDrawable(DrawableLine::FromCloud(lines_cloud, color), "DrawableLine");
    }

    // 显示图像
    cv::Mat imgShowCV;
    utils::ReadKittiImageByPath(_file_names_img[cloud_number], imgShowCV);    
    
    cv::cvtColor(imgShowCV, imgShowCV, cv::COLOR_BGR2RGB);

    cv::resize(imgShowCV, imgShowCV, cv::Size(imgShowCV.cols / 2, imgShowCV. rows / 2));
    // imgShowCV.resize(imgShowCV.rows / 2, imgShowCV.cols / 2);
    QImage qimage = utils::MatToQImage(imgShowCV);
    dock_Image->resize(qimage.width(), qimage.height());
    imgLabel->setPixmap(QPixmap::fromImage(qimage));
    imgLabel->resize(qimage.width(), qimage.height());

    // if (!visClusterImg.empty())
    // {
    //     cv::resize(visClusterImg, visClusterImg, cv::Size(), girdImageResize, girdImageResize);
    //     QImage qimage_cluster = utils::MatToQImage(visClusterImg);
    //     dock_cluster_image->resize(qimage_cluster.width() , qimage_cluster.height());
    //     cluster_image->setPixmap(QPixmap::fromImage(qimage_cluster));
    //     cluster_image->resize(qimage_cluster.width(), qimage_cluster.height());
    // }

    if (ui->depthClusterCB->isChecked())
    {
        // QImage qimage_depth = utils::MatToQImage(visImage);
        // dockshow_depth_image->resize(qimage_depth.width() * 2, qimage_depth.height() * 2);
        // depth_image->setPixmap(QPixmap::fromImage(qimage_depth));
        // depth_image->resize(qimage_depth.width() * 2, qimage_depth.height() * 2);

        // QImage qimage_depth2 = utils::MatToQImage(depthImage);
        // dockshow_depth_image2->resize(qimage_depth2.width() * 2, qimage_depth2.height() * 2);
        // depth_image2->setPixmap(QPixmap::fromImage(qimage_depth2));
        // depth_image2->resize(qimage_depth2.width() * 2, qimage_depth2.height() * 2);
    }
    _viewer->update();
    fprintf(stderr, "<------------------------------\n\n\n");
}

std::vector<BBox> MainWindow::CloudToBBoxs(const std::vector<Cloud::Ptr> & bboxPts)
{
    // 过滤掉一些重合的检测
    int numBBoxs = bboxPts.size();
    std::vector<BBox> res;
    for (int idx = 0; idx < numBBoxs; ++idx)
    {
        auto & cloud = (*bboxPts[idx]);
        // 这一部分是剔除重叠交叉的 bbox
        if (cloud.refIdx == -1) continue;
        auto bbox = BBox(cloud[0], cloud[1], cloud[2], cloud[3]);
        bbox.minZ = cloud[0].z();
        bbox.maxZ = cloud[4].z();
        // 赋值 refIdx
        bbox.refIdx = cloud.refIdx;
        bbox.shape = cloud.shape;
        // 剔除车载激光雷达支架点云的干扰
        // if (std::abs(bbox.pose.position.x) < 3.0f &&
        //     std::abs(bbox.pose.position.y) < 3.0f &&
        //     (bbox.dimensions.x * bbox.dimensions.y) < 0.01f)
        // {
        //     fprintf(stderr, "CloudToBBoxs <=> bbox pose and demension (%f, %f), (%f, %f)\n", 
        //                 bbox.pose.position.x, bbox.pose.position.y, bbox.dimensions.x, bbox.dimensions.y);
        //     continue;
        // }
        res.emplace_back(bbox);
    }
    // 添加自车的跟踪轨迹
    point pt1 = transPointL2G(point(2.5f,   1.3f,  0.0f));
    point pt2 = transPointL2G(point(2.5f,  -1.3f,  0.0f));
    point pt3 = transPointL2G(point(-2.5f, -1.3f,  0.0f));
    point pt4 = transPointL2G(point(-2.5f,  1.3f,  0.0f));
    auto selfBBox = BBox(pt1, pt2, pt3, pt4);
    selfBBox.minZ = -1.73f;
    selfBBox.maxZ = 0.0f;
    selfBBox.refIdx = 20; 
    selfBBox.shape = shapeType::MINAREA;
    res.emplace_back(selfBBox);
    // 跟踪取中心点
    return res;
}

void MainWindow::moveCursorToEnd()
{
    QTextCursor cursor = infoTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    infoTextEdit->setTextCursor(cursor);
    infoTextEdit->append("---------------------------------------------------");
}


void MainWindow::onUpdateShow()
{
    // if (ui->obstacleCB->isChecked() || ui->groundCB->isChecked())
    //     ui->cloudCB->setChecked(false);
    // if (ui->cloudCB->isChecked())
    // {
    //     ui->groundCB->setChecked(false);
    //     ui->obstacleCB->setChecked(false);
    // }
    
    // qDebug() << "some this occur" << endl;
    
    for (int i = curr_data_idx; i < ui->DataIdxVSlider->maximum(); ++i)
    {
        if (!playCloud) return;
        curr_data_idx = i; // record current data Idx
        ui->DataIdxVSlider->setValue(i);
        ui->CloudViewer->update();
        QApplication::processEvents();
    }
}

void MainWindow::onUpdateShow(int num)
{
    onSliderMovedTo(curr_data_idx);
}

void MainWindow::onUpdate()
{
        // 是否全屏显示
    if (isFullScreen)
    {
        // _viewer->setParent(0);
        fprintf(stderr, "full Screen\n");
        subWindSize = _viewer->geometry();
        // QDesktopWidget *system_screen=QApplication::desktop();
        // QRect desktop_screen = system_screen->screenGeometry();
        _viewer->setWindowFlags(Qt::Dialog);
        // _viewer->setFixedSize(desktop_screen.width(),desktop_screen.height());
        // _viewer->showFullScreen();
        _viewer->showMaximized();
    }
    else
    {
        _viewer->setWindowFlags(Qt::SubWindow);
        // _viewer->setGeometry(subWindSize);
        _viewer->showNormal();
    }

    onSliderMovedTo(curr_data_idx);
}

void MainWindow::onUpdateShow(bool isFullScreen)
{
    onSliderMovedTo(curr_data_idx);
}

void MainWindow::onParamSet()
{
    // fprintf(stderr, "current value %f", ui->paramDSB->value());
    // fprintf(stderr, "   curren paramID %d", ui->paramSB->value());
    int paramID = ui->paramSB->value();
    double paramValue = ui->paramDSB->value();
    switch (paramID)
    {
        case (0): params_groundRemove.line_search_angle = paramValue;           break;
        case (1): params_groundRemove.max_slope = paramValue;                   break; 
        case (2): params_groundRemove.tHmin = paramValue;                       break; 
        case (3): params_groundRemove.tHmax = paramValue;                       break; 
        case (4): params_groundRemove.tHDiff = paramValue;                      break; 
        case (5): params_groundRemove.hSensor = paramValue;                     break; 
        case (6): params_groundRemove.r_min_bin = paramValue;                   break; 
        case (7): params_groundRemove.r_max_bin = paramValue;                   break; 
        case (8): params_groundRemove.r_min_square = paramValue;                break; 
        case (9): params_groundRemove.r_max_square = paramValue;                break; 
        case (10): params_groundRemove.n_bins = paramValue;                     break; 
        case (11): params_groundRemove.n_segments = paramValue;                 break; 
        // case (12):
        case (13):params_groundRemove.max_dist_to_line = paramValue;            break; 
        case (14):params_groundRemove.visualize = paramValue;                   break;
        case (15): params_groundRemove.max_error_square = paramValue;           break; 
        case (16): params_groundRemove.long_threshold = paramValue;             break; 
        case (17): params_groundRemove.max_long_height = paramValue;            break; 
        case (18): params_groundRemove.max_start_height = paramValue;           break; 
        case (19): params_groundRemove.sensor_height = paramValue;              break; 
        // case (20): 
        // case (21):
        case (22):params_groundRemove.min_split_dist = paramValue;              break; 
        case (23):params_groundRemove.theta_start = paramValue;                 break; 
        case (24): params_groundRemove.theta_end = paramValue;                  break; 
        case (25): params_groundRemove.angle_resolution = paramValue;           break; 
        case (27):depthImagefilter = static_cast<bool>(paramValue);             break;
        case (28):angle_threshold = paramValue;                                 break; 
        case (29):girdImageResize = static_cast<size_t>(paramValue);            break;
        // 提取 l shape 的时候的分辨率 0.08 默认
        case (30):lShpaeHorizonResolution = paramValue;                         break;
        default:
            break;
    }
    // update show
    onUpdate();
}

void MainWindow::onClearSelection()
{
    // fprintf(stderr, "current elsection id :\n");
    // for (auto & elem : _viewer->selection) 
    //     fprintf(stderr, "\n%d ", elem);
    _viewer->selection.clear();
    _viewer->bboxSelection.clear();
    bboxToCluster.clear();
}


// 为了窗口跟随， 移动到中间， 重写的事件
void MainWindow::resizeEvent(QResizeEvent *event)
{
    dock_Image->resize(imgLabel->width(), imgLabel->height());
    // dock_cluster_image->resize(cluster_image->width(), cluster_image->height());
    // dockshow_depth_image->resize(depth_image->width(), depth_image->height());
    // int showImageGV_x = (ui->CloudViewer->width() - ui->showImageGV->width()) / 2;
    // ui->showImageGV->move(showImageGV_x, 0);
}

void MainWindow::updateTransMatrix(const int & currentFrame)
{
  float theta = (selfCarPose[currentFrame][0]);  
  float detX = selfCarPose[currentFrame][1];
  float detY = selfCarPose[currentFrame][2];

  float cos_theta = cos(theta);
  float sin_theta = sin(theta);
  transL2G_ << cos_theta, sin_theta, 0,
              -sin_theta, cos_theta, 0,
              detX, detY, 1;
  // 求逆矩阵
  transG2L_ = transL2G_.inverse();
}

point MainWindow::transPointG2L(const point & input)
{
  point tmp;
  tmp.x() = input.x() * transG2L_(0, 0) + input.y() * transG2L_(1, 0) + transG2L_(2, 0);
  tmp.y() = input.x() * transG2L_(0, 1) + input.y() * transG2L_(1, 1) + transG2L_(2, 1);
  tmp.z() = input.z();
  return tmp;
}

point MainWindow::transPointL2G(const point & input)
{
  point tmp;
  tmp.x() = input.x() * transL2G_(0, 0) + input.y() * transL2G_(1, 0) + transL2G_(2, 0);
  tmp.y() = input.x() * transL2G_(0, 1) + input.y() * transL2G_(1, 1) + transL2G_(2, 1);
  tmp.z() = input.z();
  tmp.ptType = input.ptType;
  return tmp;
}

void MainWindow::transformPoseToGlobal(const std::vector<BBox>& input,
                                      std::vector<BBox>& transformed_input,
                                      const size_t & currentFrame)
{
  for (int bboxIdx = 0; bboxIdx < input.size(); ++bboxIdx)
  {
    auto & bbox = input[bboxIdx];
    std::vector<point> tmpPt(4);
    for (size_t idx = 0; idx < 4; ++idx)
    {
      tmpPt[idx] = transPointL2G(bbox[idx]);
    }
    BBox dd(tmpPt);
    dd.minZ = bbox.minZ;
    dd.maxZ = bbox.maxZ;
    transformed_input.emplace_back(dd);
  }
}

void MainWindow::transCloudL2G(std::vector<Cloud::Ptr> & input, int pointNum = 4)
{
    for (int bboxIdx = 0; bboxIdx < input.size(); ++bboxIdx)
    {
        auto & bbox = (*input[bboxIdx]);
        for (int idx = 0; idx < pointNum; ++idx)
        {
            bbox[idx] = transPointL2G(bbox[idx]);
        }
    }
}

void MainWindow::transCloudL2G(Cloud::Ptr & input)
{
    auto & cloud = (*input);
    for (int idx = 0; idx < cloud.size(); ++idx)
    {
        cloud[idx] = transPointL2G(cloud[idx]);
    }
}

