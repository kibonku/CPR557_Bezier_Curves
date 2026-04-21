#ifndef __MY_APPLICATION_H__
#define __MY_APPLICATION_H__

#include "my_descriptors.h"
#include "my_window.h"
#include "my_device.h"
#include "my_renderer.h"
#include "my_game_object.h"
#include "my_camera.h"
#include "my_bezier_curve_surface.h"

#include <memory>
#include <vector>

class MyApplication 
{
public:
	static constexpr int WIDTH  = 1000;
	static constexpr int HEIGHT = 1000;

	MyApplication();

	void run();

	// Phase 1-4
	void switchProjectionMatrix();
	void mouseButtonEvent(bool bMouseDown, float posx, float posy);
	void mouseMotionEvent(float posx, float posy);
	void setCameraNavigationMode(MyCamera::MyCameraMode mode);
	void switchEditMode();
	void resetSurface();
	void showHideNormalVectors();
	void createBezierRevolutionSurface();

	// Phase 5: creativity controls
	void toggleTwist();         // key X: flip twist on/off
	void toggleColorGradient(); // key G: flip rainbow coloring on/off

private:
	void _loadGameObjects();
	void _rebuildSurface();     // internal helper: rebuild after param change

	MyWindow                          m_myWindow{ WIDTH, HEIGHT, "Assignment 5" };
	MyDevice                          m_myDevice{ m_myWindow };
	MyRenderer                        m_myRenderer{ m_myWindow, m_myDevice };

	std::unique_ptr<MyDescriptorPool> m_pMyGlobalPool{};
	std::vector<MyGameObject>         m_vMyGameObjects;
	MyCamera                          m_myCamera{};
	bool                              m_bPerspectiveProjection;
	bool                              m_bMouseButtonPress = false;
	bool                              m_bCreateModel      = false;
	bool                              m_bShowNormals      = false;
	bool                              m_bSurfaceExists    = false; // track if surface was built

	// Resolution used when building the surface (Phase 5: could be exposed interactively)
	int m_iXResolution = 50;
	int m_iRResolution = 50;

	std::vector<MyModel::PointCurve> m_vControlPointVertices;
	std::vector<MyModel::PointCurve> m_vNormalVectors;
	std::shared_ptr<MyBezier>        m_pMyBezier;
};

#endif
