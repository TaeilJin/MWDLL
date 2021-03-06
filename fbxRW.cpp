
#include <Windows.h>
#include <iostream>
#include <algorithm>

#include <osg/Geode>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <osg/LineWidth>
#include <osgDB/ReadFile>

#include "Visualizer/gOSGShape.h"
#include "Loader/MotionLoader.h"
#include "Loader/mgSkeletonToBCharacter.h"
#include "AvatarWorld.h"

#include "MocapProcessor/mgMBSUtil.h"
#include "MocapProcessor/mgUtility.h"
#include "Visualizer/gEventHandler.h"

#include "Visualizer/gOSGSkin.h"



#include <osgAnimation/AnimationManagerBase>
#include <osgAnimation/BasicAnimationManager>
#include <osgAnimation/Animation>
#include <osgAnimation/Skeleton>
#include <osgAnimation/Bone>
#include <osgAnimation/UpdateBone>
#include <osgAnimation/StackedRotateAxisElement>
#include <osgAnimation/StackedMatrixElement>
#include <osgAnimation/StackedTranslateElement>
#include <osgAnimation/StackedQuaternionElement>
#include <osgAnimation/StackedScaleElement>
#include <osg/TriangleIndexFunctor>
#include <osgDB/Options>

#define NCPtoNSP(n) (n+2) // convert # of control point to # of spline control points, which used in BSpline
#define NCPtoNSE(n) (n-1) // convert # of control point to # of spline segment, which used in BSpline


double DEBUG_DRAW_CONSTRAINT_SIZE = 2;
gVec3 MW_GRAVITY_VECTOR(0, -9.8, 0);
gVec3 MW_GROUND_NORMAL(0, 1, 0);


// test
bCharacter *avatar;
arma::mat refCoord;

osg::ref_ptr<osg::Group> debugGroup = new osg::Group;
osg::ref_ptr<osg::Group> debugGroup2 = new osg::Group;
osg::ref_ptr<osg::Group> debugGroup3 = new osg::Group;

const static std::string kTaeilPath = "D:/Taeil_Jin/Development/feature_MWTaeil/Projects/Env_Retargeting/Data/demo/taeil/";
const static std::string kSourceObjectFileName = kTaeilPath + "vitra_noarm.obj"; //"VItra_noArm_tri.obj";// "source.obj";//"source.obj";
const static std::string kSourceObjectFileName2 = kTaeilPath + "test_desk_tri.obj";// "test_desk_tri.obj";
const static std::string kTargetObjectFileName = kTaeilPath + "vitra_noarm.obj";//"VItra_noArm_tri.obj";// "Long_woodbench.obj";//"Loft_small.obj";// Sofa_tri.obj";// "Sofa_tri.obj";// Vitra_tallChair.obj";
const static std::string kTargetObjectFileName2 = kTaeilPath + "WhiteBoard.obj"; // WhiteBoard;

void initializeVisualization()
{
	osg::ref_ptr<osgDB::Options> options = new osgDB::Options;
	options->setOptionString("noRotation");
	osg::ref_ptr<osg::Node> src_obj_node = osgDB::readNodeFile(kSourceObjectFileName, options.get());
	osg::ref_ptr<osg::Node> tar_obj_node = osgDB::readNodeFile(kTargetObjectFileName, options.get());
	src_obj_node->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
	tar_obj_node->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);

	osg::Vec3 src_p = src_obj_node->getWorldMatrices().begin()->getTrans();
	std::cout << " x " << src_p.x() << " y " << src_p.y() << " z " << src_p.z() << std::endl;


	debugGroup2->addChild(src_obj_node);
	debugGroup3->addChild(tar_obj_node);

	osg::ref_ptr<osg::Node> src_obj_node2 = osgDB::readNodeFile(kSourceObjectFileName2, options.get());
	osg::ref_ptr<osg::Node> tar_obj_node2 = osgDB::readNodeFile(kTargetObjectFileName2, options.get());
	src_obj_node2->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
	tar_obj_node2->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);

	src_p = src_obj_node2->getWorldMatrices().begin()->getTrans();
	std::cout << " x " << src_p.x() << " y " << src_p.y() << " z " << src_p.z() << std::endl;

	debugGroup2->addChild(src_obj_node2);
	debugGroup3->addChild(tar_obj_node2);
}
//

osgAnimation::BasicAnimationManager* findFirstOsgAnimationManagerNode(osg::Node* node)
{
	osgAnimation::BasicAnimationManager* manager = dynamic_cast<osgAnimation::BasicAnimationManager*>(node->getUpdateCallback());
	if (manager)
	{
		//std::cout << "name: " << node->getName() << std::endl;
		return manager;
	}

	//return NULL if not osg::Group
	osg::Group* group = node->asGroup();
	if (!group) return NULL;

	//else, traverse children		
	for (int i = 0; i<group->getNumChildren(); ++i)
	{
		manager = findFirstOsgAnimationManagerNode(group->getChild(i));
		if (manager) return manager;
	}
	return NULL;
}

osgAnimation::Skeleton* findFirstOsgAnimationSkeletonNode(osg::Node* node)
{
	//return NULL if not osg::Group
	osg::Group* group = node->asGroup();
	if (!group) return NULL;

	//see if node is Skeleton
	osgAnimation::Skeleton* re = dynamic_cast<osgAnimation::Skeleton*>(node);
	if (re)  return re;

	//else, traverse children		
	for (int i = 0; i<group->getNumChildren(); ++i)
	{
		re = findFirstOsgAnimationSkeletonNode(group->getChild(i));
		if (re) return re;
	}
	return NULL;
}

osg::Vec3 drawBone(osg::ref_ptr<osg::Node> node, osg::ref_ptr<osg::Group> view_group)
{
	osg::ref_ptr<osgAnimation::Bone> b = dynamic_cast<osgAnimation::Bone*>(node.get());
	if (!b)
	{
		osg::ref_ptr<osg::Group> group = dynamic_cast<osg::Group*>(node.get());
		if (group)
		{
			for (int i = 0; i < group->getNumChildren(); ++i)
			{
				drawBone(group->getChild(i), view_group);
			}
		}
		return osg::Vec3(0, 0, 0);
	}

	osg::Matrix wMat = b->getWorldMatrices()[0];
	//osg::Matrix wMat = b->getMatrixInSkeletonSpace();
	//osg::Vec3 pos = b->getMatrix().getTrans();
	osg::Vec3 pos = wMat.getTrans();

	gOSGShape::setColor(osg::Vec4(1, 0, 0, 1));
	view_group->addChild(gOSGShape::createPoint(pos, 3.));

	for (int i = 0; i<b->getNumChildren(); ++i)
	{
		osg::Vec3 c_pos = drawBone(b->getChild(i), view_group);

		gOSGShape::setColor(osg::Vec4(0, 0, 0, 1));
		view_group->addChild(gOSGShape::createLineShape(pos, c_pos, 1.));

	}

	return pos;
}

osg::Vec3 drawBone(mgBone* bone, double* data, osg::ref_ptr<osg::Group> view_group)
{
	gXMat wMat;
	bone->skeleton->getWMatrixAt(bone->id, data, wMat);

	gVec3 pos_g = wMat.trn();
	osg::Vec3 pos(pos_g.x(), pos_g.y(), pos_g.z());

	gOSGShape::setColor(osg::Vec4(1, 0, 0, 1));
	view_group->addChild(gOSGShape::createPoint(pos, 3.));

	for (int i = 0; i<bone->children.size(); ++i)
	{
		osg::Vec3 c_pos = drawBone(bone->children[i], data, view_group);

		gOSGShape::setColor(osg::Vec4(0, 0, 0, 1));
		view_group->addChild(gOSGShape::createLineShape(pos, c_pos, 1.));
	}

	return pos;
}

#include "Loader/FBXLoader.h"

osg::ref_ptr<osg::Node> getNodeFromName(osg::ref_ptr<osg::Node> node, std::string& name)
{
	osg::Group* group = node->asGroup();
	if (!group) return NULL;

	if (node->getName() == name)
		return node;

	for (int i = 0; i < group->getNumChildren(); ++i)
	{
		osg::ref_ptr<osg::Node> n = getNodeFromName(group->getChild(i), name);
		if (n) return n;
	}
	return NULL;
}

gXMat getNodeTransform(osg::ref_ptr<osg::Node> node, FBXLoader* fbx)
{
	return gXMat();
}

//save motion coordinates
arma::vec coords;
std::ofstream myfile_coordinates;
bool first_bool_coordinates = false;
void writeCSVFile_coords(int n_iter, int n_Motion) {
	using namespace std;

	//write distance of each frame
	if (first_bool_coordinates == false && n_iter < n_Motion) {

		avatar->getCompactCoordArray(coords);

		myfile_coordinates << coords[0] * 180 / M_PI << "," << coords[1] * 180 / M_PI << "," << coords[2] * 180 / M_PI << ",";
		myfile_coordinates << coords[3] << "," << coords[4] << "," << coords[5] << ",";
		//myfile << g_frame << ",";// "," << 3 << "\n";// "a,b,c,\n";
		for (int i = 6; i < coords.n_rows; i++) {
			myfile_coordinates << coords[i] * 180 / M_PI << ",";
		}
		myfile_coordinates << n_iter << "," << "\n";

	}
	else
	{
		std::cout << " write csvFile_ foot position end " << std::endl;
		first_bool_coordinates = true;
		myfile_coordinates.close();
	}

}
//
//save motion curve
std::ofstream myfile_motioncurve;
bool first_bool_motioncurve = false;
gXMat T_pelvis_tr_o; double Dist_MC = 0; gVec3 v_start2end;
void writeCSVFile_motioncurve(int n_iter, int n_Motion) {
	using namespace std;

	//write distance of each frame
	if (first_bool_motioncurve == false && n_iter < n_Motion) {


		gVec3 pelvis_pos = avatar->link(0)->frame().trn(); // pelvis 
		gVec3 rightF_pos = avatar->findLink("r_ankle")->frame().trn(); // rightFoot
		gVec3 leeftF_pos = avatar->findLink("l_ankle")->frame().trn(); // leftFoot

		pelvis_pos = T_pelvis_tr_o.invMultVec4(pelvis_pos);
		rightF_pos = T_pelvis_tr_o.invMultVec4(rightF_pos);
		leeftF_pos = T_pelvis_tr_o.invMultVec4(leeftF_pos);


		myfile_motioncurve << pelvis_pos.x() << "," << pelvis_pos.y() << "," << pelvis_pos.z() << ",";
		myfile_motioncurve << rightF_pos.x() << "," << rightF_pos.y() << "," << rightF_pos.z() << ",";
		myfile_motioncurve << leeftF_pos.x() << "," << leeftF_pos.y() << "," << leeftF_pos.z() << "," << n_iter << "," << "\n";

		Dist_MC = pelvis_pos.z(); 

		v_start2end.set(pelvis_pos.x(), 0 , pelvis_pos.z());
		v_start2end.normalize();
		cout << " D " << Dist_MC << " v" << v_start2end <<endl;
	}
	else
	{
		std::cout << " write csvFile_motioncurve end " << std::endl;
		
		if(first_bool_motioncurve == false)
			myfile_motioncurve << Dist_MC << "," << v_start2end.x() << ","<< v_start2end.z() << "," << "\n";
		
		first_bool_motioncurve = true;
		myfile_motioncurve.close();
		
	}

}

#ifdef  UNITY_MW_DLL_TEST_EXPORTS
#define DLL_TEST_API __declspec(dllexport)
#else
#define DLL_TEST_API __declspec(dllimport)
#endif //  DLL_TEST_EXPORTS

extern "C"
{
	

	DLL_TEST_API bool INIT_MW(double scale) {
		const char* filename = "D:/Taeil_Jin/Development/2019/Github_UnityMW/Unity_MotionWorks/Assets/CRSF_Model.txt";

		AvatarWorld::getInstance()->loadAvatarModelFromFile(filename);
		//scene->addChild(AvatarWorld::getInstance()->visSys.getOSGGroup());
		//avatar = AvatarWorld::getInstance()->avatar;

		//BOOL b_Check = FALSE;
		//parser = new Parser();
		//character = new gMultibodySystem();

		//b_Check = loadModel(filename, scale);
		bool b_Check = true;
		return b_Check;
	}

	//DLL_TEST_API LPCSTR TEST_CHA() {
	//	char* test = const_cast<char*>(character->link(0)->name());

	//	return test;
	//}

}

int main(int argc, char **argv)
{
	// construct the viewer.
	osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;

	osg::ref_ptr<osg::Group> scene = new osg::Group;
	osg::ref_ptr<osg::MatrixTransform> rootTrans = new osg::MatrixTransform;
	osg::ref_ptr<osg::MatrixTransform> refTrans = new osg::MatrixTransform;

	//scene->addChild(rootTrans);
	//scene->addChild(refTrans);

	scene->addChild(debugGroup);
	scene->addChild(debugGroup2);
	scene->addChild(debugGroup3);

	viewer->setSceneData(scene);


	osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
	traits->x = 10;
	traits->y = 10;
	traits->width = 1024;
	traits->height = 768;
	traits->windowDecoration = true;
	traits->supportsResize = false;
	traits->windowName = "test";

	osg::ref_ptr<osg::GraphicsContext> graphicscontext = osg::GraphicsContext::createGraphicsContext(traits);
	graphicscontext->realize();

	viewer->getCamera()->setGraphicsContext(graphicscontext);

	osg::Camera* camera = viewer->getCamera();

	camera->setClearColor(osg::Vec4(1., 1., 1., 1.0));
	camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
	camera->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(traits->width) / static_cast<double>(traits->height), 1.0f, 10000.0f);

	osg::ref_ptr<osgGA::TrackballManipulator> manipulator = new osgGA::TrackballManipulator;
	viewer->setCameraManipulator(manipulator);
	manipulator->setAutoComputeHomePosition(true);
	//manipulator->setHomePosition(osg::Vec3(-100, 100, -100), osg::Vec3(0, 0, 0), osg::Vec3(0, 1, 0), false);
	manipulator->home(0);

	int margin = 300;
	int w = margin * 2;
	int h = margin * 2;
	double size = 10;

	double startPoint_h = 0;
	double startPoint_w = 0;

	float width = 1.0f;

	gOSGShape::setColor(osg::Vec4(0, 0, 0, 1));
	for (int i = 0; i <= (h / size + 1); i++)
		scene->addChild(gOSGShape::createLineShape(osg::Vec3(startPoint_w, 0.0, i*size + startPoint_h), osg::Vec3(1, 0, 0), w + size, width));
	for (int i = 0; i <= (w / size + 1); i++)
		scene->addChild(gOSGShape::createLineShape(osg::Vec3(i*size + startPoint_w, 0.0, startPoint_h), osg::Vec3(0, 0, 1), h + size, width));
	scene->addChild(gOSGShape::createAxis(5.0, 5.0));

	initializeVisualization();

	MotionLoader loader;
	//loader.loadMotionFile(t_filename.c_str());
	//loader.loadMotionFile("blonde_sitting_with_walking.fbx");
	loader.loadMotionFile("FemaleStopWalking_Lfirst.bvh");
	//loader.loadMotionFile("CRSF_FemaleStopWalking.fbx");

	mgData* motion = loader.getMotion();
	mgSkeleton* skeleton = loader.getSkeleton();


	//const double mass = 70.;
	//mgSkeletonToBCharacter::saveToBCharacter(skeleton, "CRSF_Model.txt", mass);

	AvatarWorld::getInstance()->loadAvatarModelFromFile("CRSF_Model.txt");
	scene->addChild(AvatarWorld::getInstance()->visSys.getOSGGroup());
	avatar = AvatarWorld::getInstance()->avatar;

	arma::mat refCoord(avatar->sizeCompactCoordArray() + 1, motion->nMotion, arma::fill::zeros);
	for (int f = 0; f < motion->nMotion; f++)
	{
		arma::vec coord;

		mgMBSUtil::getCoordArrayFromRawData(
			coord,
			avatar,
			skeleton,
			motion->motions[f]
		);

		//refCoord.col(f) = coord;
		refCoord.submat(0, f, arma::SizeMat(coord.n_elem, 1)) = coord;
	}

	// skin
	//gOsgSkin skin(avatar);
	//skin.loadSkin(scene, "skin2/blonde.FBX");
	//skin.loadSkin(scene, "skin2/blonde.fbx");

	//{
	//	//osg::ref_ptr<osg::Node> node = osgDB::readNodeFile("D:/code/FBXMG/msvc2017/skin/blonde.FBX");
	//	ReaderWriterFBX rw;
	//	osgDB::ReaderWriter::ReadResult res = rw.readNode("D:/code/FBXMG/msvc2017/skin2/blonde.fbx", new osgDB::Options());
	//	osg::ref_ptr<osg::Node> node = res.getNode();

	//	if (!node) {
	//		std::cout << "ERROR: Loading failed" << std::endl;
	//		return -1;
	//	}

	//	scene->addChild(node);
	//}

	//arma::mat m_data(motion->pMotions, motion->nChannel, motion->nMotion);
	//m_data.save("motion.csv", arma::csv_ascii);

	int n_rows = avatar->numLinks() * 3 + 3;
	//cout << "rows " << avatar->numLinks() << " " << n_rows << " " << avatar->findLink("vc2")->id() << endl;
	coords.resize(n_rows, 1);
	myfile_coordinates.open("FemaleStopWalking_Lfirst.csv");
	myfile_motioncurve.open("FemaleStopWalking_Lfirst_MC.csv");
	avatar->setFromCompactCoordArray(refCoord.col(0));
	avatar->updateKinematicsUptoPos();
	avatar->updateKinematicBodiesOfCharacterSim();
	T_pelvis_tr_o = avatar->link(0)->frame(); // pelvis world matrix


	int iter = 0; int nFnt = 0;
	double simulationTime = 0;
	while (!viewer->done())
	{
		viewer->frame(simulationTime);

		if (iter < 0)
			iter = 0;
		if (iter >= motion->nMotion)
			iter = 0;



		avatar->setFromCompactCoordArray(refCoord.col(iter));
		avatar->updateKinematicsUptoPos();
		avatar->updateKinematicBodiesOfCharacterSim();
		AvatarWorld::getInstance()->visSys.update();

		writeCSVFile_coords(nFnt, motion->nMotion);
		writeCSVFile_motioncurve(nFnt, motion->nMotion);
		nFnt++;

		//skin.updateSkin();


		debugGroup->removeChildren(0, debugGroup->getNumChildren());

		drawBone(skeleton->boneRoot, motion->motions[iter], debugGroup);

		simulationTime += 1. / 30.;
		iter++;

	}
	return 0;


}